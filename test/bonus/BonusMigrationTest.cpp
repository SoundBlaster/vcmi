/*
 * BonusMigrationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../../lib/bonuses/BonusMigration.h"
#include "../../lib/bonuses/BonusList.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/json/JsonBonus.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/json/JsonValidator.h"
#include "../../lib/modding/IdentifierResolver.h"

static JsonNode legacyEnchanted(int value)
{
	JsonNode ability;
	ability["type"].String() = "ENCHANTED";
	ability["subtype"].String() = "shield";
	ability["val"].Integer() = value;
	return ability;
}

TEST(BonusMigrationTest, EnchantedKeepsMasteryInValue)
{
	JsonNode migrated;
	ASSERT_TRUE(BonusMigration::migrateBonus(legacyEnchanted(2), migrated));

	EXPECT_EQ(migrated["type"].String(), "COMBAT_EVENT_TRIGGER");
	EXPECT_EQ(migrated["subtype"].String(), "enchanted");
	EXPECT_EQ(migrated["val"].Integer(), 2);
	EXPECT_EQ(migrated["addInfo"]["spell"].String(), "shield");
	EXPECT_FALSE(migrated["addInfo"]["massive"].Bool());
}

TEST(BonusMigrationTest, EnchantedUnpacksMassEffect)
{
	JsonNode migrated;
	ASSERT_TRUE(BonusMigration::migrateBonus(legacyEnchanted(4), migrated));

	EXPECT_EQ(migrated["val"].Integer(), 1);
	EXPECT_TRUE(migrated["addInfo"]["massive"].Bool());
}

TEST(BonusMigrationTest, NamedValueBoundsParseAndKeepLegacySerialization)
{
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	for(const auto & [name, expectedType, legacyName] : {
		std::tuple{"AT_MOST", BonusValueType::INDEPENDENT_MIN, "INDEPENDENT_MIN"},
		std::tuple{"AT_LEAST", BonusValueType::INDEPENDENT_MAX, "INDEPENDENT_MAX"}})
	{
		JsonNode input;
		input["type"].String() = "MORALE";
		input["val"].Integer() = 4;
		input["valueType"].String() = name;
		input.setModScope("vcmi-test");
		auto bonus = JsonUtils::parseBonus(input, resolver);
		ASSERT_NE(bonus, nullptr);
		EXPECT_EQ(bonus->valType, expectedType);
		auto serialized = bonus->toJsonNode();
		EXPECT_EQ(serialized["valueType"].String(), legacyName);
		auto reloaded = JsonUtils::parseBonus(serialized, resolver);
		ASSERT_NE(reloaded, nullptr);
		EXPECT_EQ(reloaded->valType, expectedType);
		BonusList bonuses;
		bonuses.push_back(bonus);
		// A lone independent bound has legacy standalone-value semantics.
		EXPECT_EQ(bonuses.totalValue(), 4);
		auto additive = std::make_shared<Bonus>();
		additive->val = 10;
		bonuses.push_back(additive);
		if(expectedType == BonusValueType::INDEPENDENT_MIN)
		{
			EXPECT_EQ(bonuses.totalValue(), 4);
			additive->val = 2;
			EXPECT_EQ(bonuses.totalValue(), 2);
		}
		else
		{
			EXPECT_EQ(bonuses.totalValue(), 10);
			additive->val = 2;
			EXPECT_EQ(bonuses.totalValue(), 4);
		}
	}
}

TEST(BonusMigrationTest, SchemaAcceptsNamedAndLegacyValueBounds)
{
	for(const char * name : {"AT_MOST", "AT_LEAST", "INDEPENDENT_MIN", "INDEPENDENT_MAX"})
	{
		JsonNode input;
		input["type"].String() = "MORALE";
		input["val"].Integer() = 4;
		input["valueType"].String() = name;
		input.setModScope("vcmi-test");
		JsonValidator validator;
		EXPECT_TRUE(validator.validate(input, "vcmi:bonusInstance", "BonusBounds")) << name;
	}
}
