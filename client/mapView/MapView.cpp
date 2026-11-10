/*
 * MapView.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapView.h"

#include "IMapRendererContext.h"
#include "MapViewActions.h"
#include "MapViewCache.h"
#include "MapViewController.h"
#include "MapViewModel.h"
#include "MapViewObservation.h"
#include "mapHandler.h"

#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../GameChatHandler.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../PlayerLocalState.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "render/CAnimation.h"
#include "render/Canvas.h"
#include "render/Colors.h"
#include "render/IImage.h"
#include "render/IScreenHandler.h"
#include "events/InputHandler.h"

#include "../../lib/callback/CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/ResourceSet.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMapHeader.h"

#include "MapOverlayLogVisualizer.h"

#include <vstd/DateUtils.h>
#include <cstdlib>
#include <fstream>

namespace
{
// Disabled unless explicitly requested. Appending avoids overwriting an
// existing file, and the fixed budgets bound work and output per process.
void observeMapView(const MapViewModel & model, const Rect & viewport, const IMapRendererContext & context)
{
	struct Trace
	{
		std::ofstream file;
		std::size_t remaining = 1800;
		std::size_t sequence = 0;
		std::size_t showCount = 0;
		MapObservationSampler sampler;
		MapObservationGate gate;
		const CIntObject * previousWindow = nullptr;
		std::chrono::steady_clock::time_point lastGuiScan{};
		std::string guiData;
		const int64_t session = std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
		Trace()
		{
			if(const char * path = std::getenv("VCMI_MAP_OBSERVATION_PATH"); path && *path)
				file.open(path, std::ios::out | std::ios::app);
		}
	};
	static Trace trace;
	if(!trace.file || trace.remaining == 0)
		return;
	const std::size_t showCount = trace.showCount++;
	const auto now = std::chrono::steady_clock::now();
	if(!trace.sampler.shouldInspect(now))
		return;
	const auto * callback = GAME->interface()->cb.get();
	const auto * header = callback->getMapHeader();
	const auto topology = header->getTopology();
	const auto center = model.getMapViewCenter();
	const auto tileSize = model.getSingleTileSize();
	std::ostringstream out;
	std::ostringstream key;
	key << model.getLevel() << ':' << header->width << ':' << header->height << ':' << header->toroidal
		<< ':' << tileSize.x << ':' << tileSize.y << ':' << viewport.x << ':' << viewport.y << ':' << viewport.w << ':' << viewport.h
		<< ':' << static_cast<int>(std::floor(static_cast<double>(center.x) / tileSize.x))
		<< ':' << static_cast<int>(std::floor(static_cast<double>(center.y) / tileSize.y));
	std::ostringstream playerState;
	const auto player = callback->getPlayerID();
	playerState << ",\"player_state\":";
	if(player && player->isValidPlayer())
	{
		playerState << "{\"player\":" << player->getNum() << ",\"resources\":[";
		bool firstResource = true;
		const auto resources = callback->getResourceAmount();
		for(const auto & [id, amount] : resources.nonZeroEntries())
		{
			if(!firstResource)
				playerState << ',';
			firstResource = false;
			playerState << '[' << id.getNum() << ',' << amount << ']';
		}
		playerState << "],\"selected_hero\":";
		const auto * hero = GAME->interface()->localState->getCurrentHero();
		if(hero && hero->getOwner() == *player)
		{
			const auto position = hero->visitablePos();
			playerState << "{\"id\":" << hero->id.getNum() << ",\"tile\":[" << position.x << ',' << position.y << ',' << position.z
				<< "],\"movement\":" << hero->movementPointsRemaining() << ",\"mana\":" << hero->mana << '}';
		}
		else
			playerState << "null";
		playerState << '}';
	}
	else
		playerState << "null";
	const auto playerData = playerState.str();
	key << playerData;
	out << ",\"level\":" << model.getLevel()
		<< ",\"map_size\":[" << header->width << ',' << header->height << ']'
		<< ",\"toroidal\":" << (header->toroidal ? "true" : "false")
		<< ",\"center\":[" << center.x << ',' << center.y << ']'
		<< ",\"tile_size\":[" << tileSize.x << ',' << tileSize.y << ']'
		<< ",\"viewport\":[" << viewport.x << ',' << viewport.y << ',' << viewport.w << ',' << viewport.h << ']'
		<< ",\"candidates\":[";
	bool first = true;
	bool candidatesComplete = true;
	std::size_t candidateCount = 0;
	std::vector<MapObservationVisibleRun> visibleRuns;
	const bool tilesComplete = forEachObservedMapTile(model, topology, 20000,
		[&](const int3 & displayed)
		{
			const auto canonical = topology.canonicalize(displayed);
			return canonical && context.isVisible(displayed) && callback->isVisible(*canonical);
		},
		[&](const int3 & displayed, const int3 & canonical, const Rect & area)
		{
			appendObservedVisibleTile(visibleRuns, displayed);
			for(const auto & id : context.getObjects(displayed))
			{
				if(!callback->tryGetObj(id))
					continue;
				if(candidateCount == 512)
				{
					candidatesComplete = false;
					break;
				}
				++candidateCount;
				key << ':' << id.getNum() << ',' << displayed.x << ',' << displayed.y << ',' << canonical.x << ',' << canonical.y;
				if(!first)
					out << ',';
				first = false;
				const Point offset = context.objectImageOffset(id, displayed);
				out << "{\"id\":" << id.getNum() << ",\"displayed\":[" << displayed.x << ',' << displayed.y
					<< "],\"canonical\":[" << canonical.x << ',' << canonical.y
					<< "],\"tile_rect\":[" << viewport.x + area.x << ',' << viewport.y + area.y << ',' << area.w << ',' << area.h
					<< "],\"image_offset\":[" << offset.x << ',' << offset.y << "]}";
			}
		});
	const auto visibleData = formatObservedVisibleRuns(visibleRuns);
	out << "],\"visible_runs\":" << visibleData << ",\"complete\":" << (tilesComplete && candidatesComplete ? "true" : "false");
	key << ":v" << visibleData;
	key << ':' << tilesComplete << ':' << candidatesComplete;
	const auto window = ENGINE->windows().topWindow<CIntObject>();
	if(trace.guiData.empty() || trace.previousWindow != window.get() || now - trace.lastGuiScan >= std::chrono::milliseconds(200))
	{
		trace.previousWindow = window.get();
		trace.lastGuiScan = now;
		std::ostringstream gui;
		gui << ",\"gui_available\":" << (window ? "true" : "false") << ",\"gui\":[";
		bool firstWidget = true;
		const bool guiComplete = forEachObservedWidget(window.get(), 4096, 32,
			[&](const CIntObject & widget, int index, int parent)
			{
				if(!firstWidget)
					gui << ',';
				firstWidget = false;
				gui << "{\"index\":" << index << ",\"parent\":" << parent
					<< ",\"kind\":\"" << (dynamic_cast<const ButtonBase *>(&widget) ? "button" : "widget")
					<< "\",\"rect\":[" << widget.pos.x << ',' << widget.pos.y << ',' << widget.pos.w << ',' << widget.pos.h
					<< "],\"active\":" << (widget.isActive() ? "true" : "false")
					<< ",\"disabled\":" << (widget.isDisabled() ? "true" : "false") << '}';
			});
		gui << "],\"gui_complete\":" << (guiComplete ? "true" : "false");
		trace.guiData = gui.str();
	}
	key << trace.guiData;
	if(!trace.gate.shouldRecord(key.str(), center))
		return;
	--trace.remaining;
	const auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	const auto guiTimestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(trace.lastGuiScan.time_since_epoch()).count();
	trace.file << "{\"version\":3,\"session\":" << trace.session << ",\"sequence\":" << trace.sequence++
		<< ",\"show_count\":" << showCount << ",\"monotonic_ns\":" << timestamp << ",\"gui_observed_ns\":" << guiTimestamp
		<< out.str() << playerData << trace.guiData << "}\n";
	trace.file.flush();
}
}

BasicMapView::~BasicMapView() = default;

std::shared_ptr<MapViewModel> BasicMapView::createModel(const Point & dimensions) const
{
	auto result = std::make_shared<MapViewModel>();

	result->setLevel(0);
	result->setTileSize(Point(32, 32));
	result->setViewCenter(Point(0, 0));
	result->setViewDimensions(dimensions);

	return result;
}

BasicMapView::BasicMapView(const Point & offset, const Point & dimensions, bool useGpuLayer)
	: model(createModel(dimensions))
	, tilesCache(std::make_shared<MapViewCache>(model, useGpuLayer))
	, controller(new MapViewController(model, tilesCache))
	, gpuLayerEligible(useGpuLayer)
	, needFullUpdate(false)
{
	OBJECT_CONSTRUCTION;
	pos += offset;
	pos.w = dimensions.x;
	pos.h = dimensions.y;
}

void BasicMapView::render(Canvas & target, bool fullUpdate)
{
	if(gpuLayerEligible && ENGINE->screenHandler().isGpuRenderingEnabled())
	{
		// The software screen keeps a transparent hole over the map's GPU layer. Punched every
		// frame, since any widget redrawing its background would fill it back in. Thread safe.
		target.drawColor(pos, ColorRGBA(0, 0, 0, 0));

		// a redraw arriving from another thread is queued by CIntObject::redraw() and reaches us
		// from the next frame instead, so by here the GL context is ours
		assert(ENGINE->amIGuiThread());

		renderGpu(fullUpdate);
		return;
	}

	Canvas targetClipped(target, pos);
	tilesCache->update(controller->getContext());
	tilesCache->render(controller->getContext(), targetClipped, fullUpdate);

	MapOverlayLogVisualizer r(targetClipped, model);
	logVisual->visualize(r);
}

void BasicMapView::renderGpu(bool fullUpdate)
{
	// tick() normally filled the cache; a scroll, zoom or level change since then invalidates it
	if(!tilesCache->isUpdatedThisFrame())
		tilesCache->update(controller->getContext());

	// Nothing is drawn over the terrain in the usual case, so the cache is composed straight onto
	// the screen. Copying it into a layer here would mean reading a render target in the middle of
	// the frame, which is what stalls the GLES driver on some devices.
	if(!tilesCache->needsOwnLayer(controller->getContext()))
	{
		tilesCache->present(pos * ENGINE->screenHandler().getScalingFactor());
		return;
	}

	Canvas layer = ENGINE->screenHandler().getLayerCanvas(GpuRenderLayer::MAP);
	Canvas targetClipped(layer, pos);

	tilesCache->render(controller->getContext(), targetClipped, fullUpdate);
}

void BasicMapView::tick(uint32_t msPassed)
{
	controller->tick(msPassed);

	// tick() only ever runs from the timer dispatch, which is part of the frame
	assert(ENGINE->amIGuiThread());

	// Draw the tiles into the cache now, so the GPU can work while the rest of the frame is
	// prepared - reading the cache straight after writing it stalls until that drawing is done.
	if(gpuLayerEligible && ENGINE->screenHandler().isGpuRenderingEnabled())
	{
		tilesCache->update(controller->getContext());
#ifndef VCMI_MOBILE
		// a tile-based mobile GPU resolves its render target on every flush, which costs
		// more than the stall this avoids on desktop
		ENGINE->screenHandler().flushRenderCommands();
#endif
	}
}

void BasicMapView::show(Canvas & to)
{
	CanvasClipRectGuard guard(to, pos);
	render(to, needFullUpdate);

	controller->afterRender();
}

void BasicMapView::showAll(Canvas & to)
{
	CanvasClipRectGuard guard(to, pos);
	render(to, true);
}

void MapView::tick(uint32_t msPassed)
{
	if(settings["adventure"]["smoothDragging"].Bool())
		postSwipe(msPassed);

	BasicMapView::tick(msPassed);
}

void MapView::show(Canvas & to)
{
	actions->setContext(controller->getContext());
	BasicMapView::show(to);
	observeMapView(*model, pos, *controller->getContext());
}

MapView::MapView(const Point & offset, const Point & dimensions)
	: BasicMapView(offset, dimensions, true)
{
	OBJECT_CONSTRUCTION;
	actions = std::make_shared<MapViewActions>(*this, model);
	actions->setContext(controller->getContext());

	// catch min 6 frames
	postSwipeCatchIntervalMs = std::max(100, static_cast<int>(6.0 * 1000.0 * (1.0 / settings["video"]["targetfps"].Float())));
}

void MapView::onMapLevelSwitched()
{
	if(GAME->interface()->cb->getMapSize().z > 1)
	{
		int newLevel = model->getLevel() + 1;
		if(newLevel < GAME->interface()->cb->getMapSize().z)
			controller->setViewCenter(model->getMapViewCenter(), newLevel);
		else
			controller->setViewCenter(model->getMapViewCenter(), 0);
	}
}

void MapView::onMapScrolled(const Point & distance)
{
	if(!isGesturing())
	{
		postSwipeSpeed = 0.0;
		controller->setViewCenter(model->getMapViewCenter() + distance, model->getLevel());
	}
}

void MapView::onMapSwiped(const Point & viewPosition)
{
	if(settings["adventure"]["smoothDragging"].Bool())
		swipeHistory.push_back(std::pair<uint32_t, Point>(ENGINE->input().getTicks(), viewPosition));

	controller->setViewCenter(model->getMapViewCenter() + viewPosition, model->getLevel());
}

void MapView::postSwipe(uint32_t msPassed)
{
	if(!actions->dragActive)
	{
		if(swipeHistory.size() > 1)
		{
			Point diff = Point(0, 0);
			std::pair<uint32_t, Point> firstAccepted;
			uint32_t now = ENGINE->input().getTicks();
			for (auto & x : swipeHistory) {
				if(now - x.first < postSwipeCatchIntervalMs) { // only the last x ms are caught
					if(firstAccepted.first == 0)
						firstAccepted = x;
					diff += x.second;
				}
			}

			uint32_t timediff = swipeHistory.back().first - firstAccepted.first;

			if(diff.length() > 0 && timediff > 0)
			{
				postSwipeAngle = diff.angle();
				postSwipeSpeed = static_cast<double>(diff.length()) / static_cast<double>(timediff); // unit: pixel/millisecond
			}	
		}
		swipeHistory.clear();
	} else
		postSwipeSpeed = 0.0;
	if(postSwipeSpeed > postSwipeMinimalSpeed) {
		double len = postSwipeSpeed * static_cast<double>(msPassed);
		Point delta = Point(len * cos(postSwipeAngle), len * sin(postSwipeAngle));

		controller->setViewCenter(model->getMapViewCenter() + delta, model->getLevel());

		postSwipeSpeed /= 1 + msPassed * postSwipeSlowdownSpeed;
	}
}

void MapView::onCenteredTile(const int3 & tile)
{
	controller->setViewCenter(tile);
}

void MapView::onCenteredObject(const CGObjectInstance * target)
{
	controller->setViewCenter(target->getSightCenter());
}

void MapView::onViewSpellActivated(uint32_t tileSize, const std::vector<ObjectPosInfo> & objectPositions, bool showTerrain)
{
	controller->activateSpellViewContext();
	controller->setTileSize(Point(tileSize, tileSize));
	controller->setOverlayVisibility(objectPositions);
	controller->setTerrainVisibility(showTerrain);
}

void MapView::onViewWorldActivated(uint32_t tileSize)
{
	controller->activateWorldViewContext();
	controller->setTileSize(Point(tileSize, tileSize));
}

void MapView::onMapZoomLevelChanged(int stepsChange, bool useDeadZone)
{
	controller->modifyTileSize(stepsChange, useDeadZone);
}

void MapView::onViewMapActivated()
{
	controller->activateAdventureContext();

	int zoom = settings["adventure"]["tileZoom"].Integer();
	if(zoom)
		controller->setTileSize(Point(zoom, zoom));
	else
		controller->setTileSize(Point(32, 32));
}

void MapView::exportScreenshot()
{
	const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / "screenshots";
	boost::filesystem::create_directories(outPath);
	const std::string timestamp = vstd::getDateTimeISO8601Basic(std::time(nullptr));

	const auto context = controller->getContext();

	for(int level = 0; level < context->getMapSize().z; ++level)
	{
		const boost::filesystem::path filePath = outPath / ("screenshot-map-" + timestamp + "-level" + std::to_string(level) + ".png");

		tilesCache->exportMapLevel(context, level, filePath);

		MetaString txt;
		txt.appendTextID("vcmi.client.screenShot");
		txt.replaceRawString(TextOperations::filesystemPathToUtf8(filePath));
		if(GAME->interface())
			GAME->server().getGameChat().sendMessageGameplay(txt.toString(&GAME->translator()));
	}
}

PuzzleMapView::PuzzleMapView(const Point & offset, const Point & dimensions, const int3 & tileToCenter)
	: BasicMapView(offset, dimensions, false)
{
	controller->activatePuzzleMapContext(tileToCenter);
	controller->setViewCenter(tileToCenter);

}
