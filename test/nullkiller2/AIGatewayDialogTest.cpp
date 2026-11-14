/*
 * AIGatewayDialogTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "NullkillerTest.h"
#include "mock/GameHandlerTestServer.h"

#include "lib/AsyncRunner.h"
#include "lib/callback/IClient.h"
#include "lib/networkPacks/PacksForServer.h"
#include "server/CGameHandler.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);

/// Records answers that AI sends to the server
class AnswerRecordingClient : public IClient
{
public:
	std::vector<QuestionAnswer> answers;

	std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor, const BattleID &, const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor, bool) override
	{
		if(const auto * answer = dynamic_cast<const QuestionAnswer *>(&request))
			answers.push_back(*answer);
		return static_cast<int>(answers.size());
	}
};

class Nullkiller2_AIGatewayDialog : public NullkillerTest
{
};

class ResolutionForwardingServer : public GameHandlerTestServer
{
public:
	using GameHandlerTestServer::GameHandlerTestServer;
	NK2AI::AIGateway * gateway = nullptr;
	int pendingAfterResolution = -1;
	std::optional<bool> lastAnswerResult;

	void applyPack(CPackForClient & pack) override
	{
		GameHandlerTestServer::applyPack(pack);
		if(const auto * resolved = dynamic_cast<const QuestionResolved *>(&pack); resolved && gateway)
		{
			gateway->questionResolved(resolved->questionID);
			pendingAfterResolution = gateway->status.getQuestionsCount();
		}
	}

	void sendPack(CPackForClient & pack, GameConnectionID) override
	{
		if(auto * applied = dynamic_cast<PackageApplied *>(&pack); applied && gateway)
		{
			lastAnswerResult = applied->result;
			gateway->requestRealized(applied);
		}
	}
};
}

/// Dialogs such as commander resurrection are not caused by a visit, so AI has no target tile for them
TEST_F(Nullkiller2_AIGatewayDialog, answersDialogWithoutTargetTile)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER);
	startWithMap(std::move(builder));

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);

	gateway->showBlockingDialog("resurrect commander?", {}, QuestionID(55), 0, false, true, false);
	gateway->asyncTasks->wait();

	ASSERT_EQ(client.answers.size(), 1u);
	EXPECT_EQ(client.answers.front().questionID, QuestionID(55));
	EXPECT_EQ(client.answers.front().reply, 1);

	// server would resolve the question once it applies the answer
	gateway->status.removeQuestion(QuestionID(55));
}

TEST_F(Nullkiller2_AIGatewayDialog, rejectedAnswerKeepsQuestionPendingUntilAnotherAnswerSucceeds)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER);
	startWithMap(std::move(builder));

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);
	const QuestionID question(56);

	gateway->status.addQuestion(question, "test question");
	gateway->status.attemptedAnsweringQuestion(question, 101);
	gateway->status.receivedAnswerConfirmation(101, false);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 1);

	// A rejection does not tell us whether the server still expects a different answer.
	// This scenario clears it with a separately submitted and accepted answer.
	gateway->status.attemptedAnsweringQuestion(question, 102);
	gateway->status.receivedAnswerConfirmation(102, true);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 0);
}

TEST_F(Nullkiller2_AIGatewayDialog, resolvedQuestionClearsRejectedAnswerWithoutAffectingOtherQuestions)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER);
	startWithMap(std::move(builder));

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);
	const QuestionID question(56), other(57);
	gateway->status.addQuestion(question, "resolved elsewhere");
	gateway->status.addQuestion(other, "still open");
	gateway->status.attemptedAnsweringQuestion(question, 101);
	gateway->status.receivedAnswerConfirmation(101, false);
	gateway->questionResolved(question);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 1);
	gateway->questionResolved(QuestionID(999));
	EXPECT_EQ(gateway->status.getQuestionsCount(), 1);
	gateway->status.removeQuestion(other);
}

TEST_F(Nullkiller2_AIGatewayDialog, resolvedQuestionWaitsForOutstandingAnswerAcknowledgement)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER);
	startWithMap(std::move(builder));

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);
	for(const bool accepted : {false, true})
	{
		const QuestionID question(accepted ? 59 : 58);
		gateway->status.addQuestion(question, "answer in flight");
		gateway->status.attemptedAnsweringQuestion(question, 102);
		gateway->questionResolved(question);
		EXPECT_EQ(gateway->status.getQuestionsCount(), 1);
		gateway->status.receivedAnswerConfirmation(102, accepted);
		EXPECT_EQ(gateway->status.getQuestionsCount(), 0);
		gateway->questionResolved(question); // Duplicate broadcast is harmless.
	}
}

TEST_F(Nullkiller2_AIGatewayDialog, lateAnswerToResolvedQuestionDoesNotBlockOrAssert)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER);
	startWithMap(std::move(builder));

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);
	const QuestionID question(60);
	gateway->status.addQuestion(question, "resolved before asynchronous answer");
	gateway->questionResolved(question);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 0);
	gateway->status.attemptedAnsweringQuestion(question, 103);
	gateway->status.receivedAnswerConfirmation(103, false);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 0);
}

TEST_F(Nullkiller2_AIGatewayDialog, rejectedOutstandingAnswersFinishBeforeResolvedQuestionIsReleased)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER);
	startWithMap(std::move(builder));

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);
	const QuestionID question(61);
	gateway->status.addQuestion(question, "two answers in flight");
	gateway->status.attemptedAnsweringQuestion(question, 104);
	gateway->status.attemptedAnsweringQuestion(question, 105);
	gateway->questionResolved(question);
	gateway->status.receivedAnswerConfirmation(104, false);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 1);
	gateway->status.receivedAnswerConfirmation(105, false);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 0);
}

TEST_F(Nullkiller2_AIGatewayDialog, unknownAcknowledgementDoesNotChangePendingQuestion)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER);
	startWithMap(std::move(builder));

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);
	const QuestionID question(62);
	gateway->status.addQuestion(question, "unknown acknowledgement");
	gateway->status.receivedAnswerConfirmation(106, true);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 1);
	gateway->status.attemptedAnsweringQuestion(question, 106);
	gateway->status.receivedAnswerConfirmation(106, true);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 0);
}

TEST_F(Nullkiller2_AIGatewayDialog, actualServerResolutionAndAcknowledgementUnblockQuestion)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER).heroExperience(999);
	startWithMap(std::move(builder));
	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);
	ResolutionForwardingServer server(gameState(), PLAYER);
	server.gateway = gateway.get();
	CGameHandler handler(server, gameState());
	handler.giveExperience(hero, 10);
	ASSERT_EQ(server.levelUpPromptIDs.size(), 1u);
	const QuestionID question = server.levelUpPromptIDs.front();
	gateway->status.addQuestion(question, "server hero level-up");

	QuestionAnswer answer(question, 0);
	answer.player = PLAYER;
	answer.requestID = 107;
	gateway->requestSent(&answer, answer.requestID);
	handler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, answer);
	ASSERT_EQ(server.resolvedQuestionIDs.size(), 1u);
	EXPECT_EQ(server.resolvedQuestionIDs.front(), question);
	EXPECT_EQ(server.pendingAfterResolution, 1); // The acknowledgement arrives after resolution.
	ASSERT_TRUE(server.lastAnswerResult.has_value());
	EXPECT_TRUE(*server.lastAnswerResult);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 0);
}

TEST_F(Nullkiller2_AIGatewayDialog, actualServerResolutionClearsPreviouslyRejectedAnswer)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER).heroExperience(999);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);
	ResolutionForwardingServer server(gameState(), PLAYER);
	server.gateway = gateway.get();
	CGameHandler handler(server, gameState());
	handler.giveExperience(hero, 10);
	ASSERT_EQ(server.levelUpPromptIDs.size(), 1u);
	const QuestionID question = server.levelUpPromptIDs.front();
	gateway->status.addQuestion(question, "server hero level-up");

	QuestionAnswer invalid(question, 999);
	invalid.player = PLAYER;
	invalid.requestID = 108;
	gateway->requestSent(&invalid, invalid.requestID);
	handler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, invalid);
	ASSERT_TRUE(server.lastAnswerResult.has_value());
	EXPECT_FALSE(*server.lastAnswerResult);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 1);

	// A separate actor supplies a valid answer to the still-open server activity.
	ASSERT_TRUE(handler.answerQuestion(question, 0, PLAYER));
	ASSERT_EQ(server.resolvedQuestionIDs.size(), 1u);
	EXPECT_EQ(server.resolvedQuestionIDs.front(), question);
	EXPECT_EQ(server.pendingAfterResolution, 0);
	EXPECT_EQ(gateway->status.getQuestionsCount(), 0);
}
