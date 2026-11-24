/*
 * mapcontroller.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "mapcontroller.h"
#include "ObjectViewTopology.h"

#include "../lib/GameConstants.h"
#include "../lib/entities/artifact/CArtifact.h"
#include "../lib/entities/hero/CHeroClass.h"
#include "../lib/entities/hero/CHeroHandler.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjectConstructors/CommonConstructors.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/mapObjects/Quest.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/MapTopology.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/mapping/ObstacleProxy.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModDescription.h"
#include "../lib/TerrainHandler.h"
#include "../lib/CSkillHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/serializer/CMemorySerializer.h"
#include "../lib/spells/CSpellHandler.h"
#include "mapsettings/modsettings.h"
#include "mapview.h"
#include "scenelayer.h"
#include "maphandler.h"
#include "mainwindow.h"
#include "graphics.h"
#include "inspector/inspector.h"
#include "GameLibrary.h"
#include "PlayerSelectionDialog.h"
#include "translator.h"

MapController::MapController(QObject * parent)
	: QObject(parent)
{
}

MapController::MapController(EditorMainWindow * m): main(m)
{
	for(int i = 0; i < MAX_LEVELS; i++)
	{
		_scenes[i].reset(new MapScene(i));
		_miniscenes[i].reset(new MinimapScene(i));
	}
	connectScenes();
	_cb = std::make_unique<EditorCallback>(nullptr);
}

void MapController::connectScenes()
{
	for(int i = 0; i < MAX_LEVELS; i++)
	{
		//selections for both layers will be handled separately
		QObject::connect(_scenes[i].get(), &MapScene::selected, [this, i](bool anythingSelected)
		{
			main->onSelectionMade(i, anythingSelected);
		});
	}
}

MapController::~MapController()
{
	if(_map)
		Translator::instance().uninstall(*_map->texts);

	main = nullptr;
}

void MapController::setCallback(std::unique_ptr<EditorCallback> cb)
{
	_cb = std::move(cb);
}

EditorCallback * MapController::getCallback()
{
	return _cb.get();
}

const std::unique_ptr<CMap> & MapController::getMapUniquePtr() const
{
	return _map;
}

CMap * MapController::map()
{
	return _map.get();
}

MapHandler * MapController::mapHandler()
{
	return _mapHandler.get();
}

MapScene * MapController::scene(int level)
{
	return _scenes[level].get();
}

std::set<MapScene *> MapController::getScenes()
{
	std::set<MapScene *>result;
	if (!map())
		return result;
	for (int i=0; i<map()->levels(); i++)
		result.insert(_scenes[i].get());
	return result;
}

MinimapScene * MapController::miniScene(int level)
{
	return _miniscenes[level].get();
}

void MapController::repairMap()
{
	repairMap(map());
}

void MapController::repairMap(CMap * map)
{
	if(!map)
		return;

	assert(map->cb);
	
	//make sure events/rumors has name to have proper identifiers
	int emptyNameId = 1;
	for(auto & e : map->events)
		if(e.name.empty())
			e.name = "event_" + std::to_string(emptyNameId++);
	emptyNameId = 1;
	for(auto & e : map->rumors)
		if(e.name.empty())
			e.name = "rumor_" + std::to_string(emptyNameId++);
	
	//fix owners for objects
	std::vector<CGObjectInstance*> allImpactedObjects;

	for (const auto & object : map->objects)
		allImpactedObjects.push_back(object.get());

	for (const auto & hero : map->getHeroesInPool())
		allImpactedObjects.push_back(map->tryGetFromHeroPool(hero));

	for(auto obj : allImpactedObjects)
	{
		if(obj == nullptr)
			continue;

		//fix flags
		if(obj->asOwnable() != nullptr && obj->getOwner() == PlayerColor::UNFLAGGABLE)
		{
			obj->tempOwner = PlayerColor::NEUTRAL;
		}
		//fix hero instance
		if(auto * nih = dynamic_cast<CGHeroInstance*>(obj))
		{
			// All heroes present on map or in prisons need to be allowed to rehire them after they are defeated

			// FIXME: How about custom scenarios where defeated hero cannot be hired again?

			map->allowedHeroes.insert(nih->getHeroTypeID());

			auto const & type = (*LIBRARY->heroh)[nih->subID];
			assert(type->getHeroClass());

			if(nih->ID == Obj::HERO) //not prison
				nih->appearance = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, type->getHeroClass()->getIndex()).getTemplates().front();
			//fix spellbook
			if(nih->spellbookContainsSpell(SpellID::SPELLBOOK_PRESET))
			{
				nih->removeSpellFromSpellbook(SpellID::SPELLBOOK_PRESET);
				if(!nih->getArt(ArtifactPosition::SPELLBOOK) && type->haveSpellBook)
					nih->putArtifact(ArtifactPosition::SPELLBOOK, map->createArtifact(ArtifactID::SPELLBOOK));
			}
			
		}
		//fix town instance
		if(auto * tnh = dynamic_cast<CGTownInstance*>(obj))
		{
			if(tnh->getTown())
			{
				for(const auto & building : tnh->getBuildings())
				{
					if(!tnh->getTown()->buildings.count(building))
						tnh->removeBuilding(building);
				}
				vstd::erase_if(tnh->forbiddenBuildings, [tnh](BuildingID bid)
				{
					return !tnh->getTown()->buildings.count(bid);
				});
			}
		}
		//fix spell scrolls
		if(auto * art = dynamic_cast<CGArtifact*>(obj))
		{
			if(art->ID == Obj::SPELL_SCROLL && !art->getArtifactInstance())
			{
				std::vector<SpellID> out;
				for(auto const & spell : LIBRARY->spellh->getObjects()) //spellh size appears to be greater (?)
				{
					//if(map->isAllowedSpell(spell->id))
					{
						out.push_back(spell->id);
					}
				}
				auto a = map->createScroll(*RandomGeneratorUtil::nextItem(out, CRandomGenerator::getDefault()));
				art->setArtifactInstance(a);
			}
		}
		//fix mines 
		if(auto * mine = dynamic_cast<CGMine*>(obj))
		{
			if(!mine->isAbandoned())
			{
				if(mine->getResourceHandler()->getResourceType() == GameResID::NONE) // fallback
					mine->producedResource = GameResID(mine->subID);
				else
					mine->producedResource = mine->getResourceHandler()->getResourceType();
				mine->producedQuantity = mine->defaultResProduction();
			}
		}
	}
}

void MapController::setMap(std::unique_ptr<CMap> cmap)
{
	if(_map)
		Translator::instance().uninstall(*_map->texts);

	cmap->cb = _cb.get();
	_map = std::move(cmap);
	_cb->setMap(_map.get());

	// map texts are inert data - the editor has to install them to render map-defined names
	Translator::instance().install(_map->texts);
	
	repairMap();
	
	for(int i = 0; i < _map->levels(); i++)
	{
		_scenes[i].reset(new MapScene(i));
		_miniscenes[i].reset(new MinimapScene(i));
	}
	resetMapHandler();
	initializeMap();

	connectScenes();

	_map->getEditManager()->getUndoManager().setUndoCallback([this](bool allowUndo, bool allowRedo)
		{
			if(!main)
				return;
			main->enableUndo(allowUndo);
			main->enableRedo(allowRedo);
		}
	);
	_map->getEditManager()->getUndoManager().clearAll();

	initObstaclePainters(_map.get());
}

void MapController::initObstaclePainters(CMap * map)
{
	for (auto const & terrain : LIBRARY->terrainTypeHandler->getObjects())
	{
		auto terrainId = terrain->getId();
		_obstaclePainters[terrainId] = std::make_unique<EditorObstaclePlacer>(map);
		_obstaclePainters[terrainId]->collectPossibleObstacles(terrainId);
	}
}

void MapController::initializeMap()
{
	for(int i = 0; i < _map->levels(); i++)
	{
		_scenes[i]->createMap();
		_miniscenes[i]->createMap();
	}
}

void MapController::addLevel(const MapLayerId & layerType)
{
	if(!canAddLevel())
		return;

	_map->addLevel(layerType);

	resetMapHandler();
	initializeMap();

	main->mapChanged();
}

bool MapController::canAddLevel() const
{
	return _map && _map->levels() < MAX_LEVELS;
}

void MapController::sceneForceUpdate()
{
	for(int i = 0; i < _map->levels(); i++)
	{
		_scenes[i]->updateMap();
		_miniscenes[i]->updateMap();
	}
}

void MapController::resetMapHandler()
{
	if(!_mapHandler)
		_mapHandler.reset(new MapHandler());
	_mapHandler->reset(map());
	for(int i = 0; i < MAX_LEVELS; i++)
	{
		_scenes[i]->initialize(*this);
		_miniscenes[i]->initialize(*this);
	}
}

void MapController::commitTerrainChange(int level, const TerrainId & terrain)
{
	static const int terrainDecorationPercentageLevel = 10;

	std::vector<int3> v(_scenes[level]->selectionTerrainView.selection().begin(),
						_scenes[level]->selectionTerrainView.selection().end());
	if(v.empty())
		return;
	
	_scenes[level]->selectionTerrainView.clear();
	
	_map->getEditManager()->getTerrainSelection().setSelection(v);
	_map->getEditManager()->drawTerrain(terrain, terrainDecorationPercentageLevel, &CRandomGenerator::getDefault());
	
	_scenes[level]->terrainView.redrawTerrain(v);
	
	_miniscenes[level]->updateMap();
	main->mapChanged();
}

void MapController::commitRoadOrRiverChange(int level, ui8 type, bool isRoad)
{
	std::vector<int3> v(_scenes[level]->selectionTerrainView.selection().begin(),
						_scenes[level]->selectionTerrainView.selection().end());
	if(v.empty())
		return;
	
	_scenes[level]->selectionTerrainView.clear();
	
	_map->getEditManager()->getTerrainSelection().setSelection(v);
	if(isRoad)
		_map->getEditManager()->drawRoad(RoadId(type), &CRandomGenerator::getDefault());
	else
		_map->getEditManager()->drawRiver(RiverId(type), &CRandomGenerator::getDefault());

	_scenes[level]->terrainView.redrawTerrain(v);
	
	_miniscenes[level]->updateMap();
	main->mapChanged();
}

void MapController::commitObjectErase(int level)
{
	auto selectedObjects = _scenes[level]->selectionObjectsView.getSelection();
	if (selectedObjects.size() > 1)
	{
		//mass erase => undo in one operation
		_map->getEditManager()->removeObjects(selectedObjects);
	}
	else if (selectedObjects.size() == 1)
	{
		_map->getEditManager()->removeObject(*selectedObjects.begin());
	}
	else //nothing to erase - shouldn't be here
	{
		return;
	}

	for (auto & obj : selectedObjects)
	{
		//invalidate tiles under objects
		_mapHandler->removeObject(obj);
	}
	_scenes[level]->objectsView.redrawObjects(selectedObjects);

	_scenes[level]->selectionObjectsView.clear();
	_scenes[level]->passabilityView.redraw();
	
	_miniscenes[level]->updateMap();
	main->mapChanged();
}

void MapController::copyToClipboard(int level)
{
	_clipboard.clear();
	_clipboardShiftIndex = 0;
	auto selectedObjects = _scenes[level]->selectionObjectsView.getSelection();
	for(auto * obj : selectedObjects)
	{
		assert(obj->pos.z == level);
		_clipboard.push_back(CMemorySerializer::deepCopy(*obj, _cb.get()));
	}
}

void MapController::pasteFromClipboard(int level)
{
	_scenes[level]->selectionObjectsView.clear();
	
	auto shift = int3::getDirs()[_clipboardShiftIndex++];
	if(_clipboardShiftIndex == int3::getDirs().size())
		_clipboardShiftIndex = 0;
	
	QStringList errors;
	for(auto & objUniquePtr : _clipboard)
	{
		auto obj = CMemorySerializer::deepCopyShared(*objUniquePtr, _cb.get());
		int3 original = objUniquePtr->pos;
		original.z = level;
		obj->pos = ObjectViewTopology::clipboardPosition(_map->getTopology(), original, shift);
		QString errorMsg;
		if(!canPlaceObject(obj.get(), errorMsg))
		{
			errors.push_back(std::move(errorMsg));
			continue;
		}
		obj->id = {};
		Initializer init(*this, obj.get(), defaultPlayer);
		_map->getEditManager()->insertObject(obj);
		_scenes[level]->selectionObjectsView.selectObject(obj.get());
		_mapHandler->invalidate(obj.get());
	}
	if(!errors.isEmpty())
		QMessageBox::warning(main, QObject::tr("Can't place object"), errors.join('\n'));
	
	_scenes[level]->objectsView.redraw();
	_scenes[level]->passabilityView.redraw();
	_scenes[level]->selectionObjectsView.redraw();
	
	_miniscenes[level]->updateMap();
	main->mapChanged();
}

bool MapController::discardObject(int level) const
{
	_scenes[level]->selectionObjectsView.clear();
	if(_scenes[level]->selectionObjectsView.newObject)
	{
		_scenes[level]->selectionObjectsView.newObject.reset();
		_scenes[level]->selectionObjectsView.setShift(0, 0);
		_scenes[level]->selectionObjectsView.selectionMode = SelectionObjectsLayer::NOTHING;
		return true;
	}
	return false;
}

void MapController::createObject(int level, std::shared_ptr<CGObjectInstance> obj) const
{
	//A freshly created quest source owns no quest yet - map loaders add one while reading
	//the object. The inspector edits the active quest in place, so give a newly placed
	//seer hut or quest guard a quest to edit instead of letting getQuest() throw.
	if(auto * questSource = dynamic_cast<QuestSource *>(obj.get()))
	{
		if(questSource->allQuests().empty())
			questSource->addQuest();
	}

	_scenes[level]->selectionObjectsView.newObject = obj;
	_scenes[level]->selectionObjectsView.selectionMode = SelectionObjectsLayer::MOVEMENT;
	_scenes[level]->selectionObjectsView.redraw();
}

void MapController::commitObstacleFill(int level)
{
	auto selection = _scenes[level]->selectionTerrainView.selection();
	if(selection.empty())
		return;
	
	//split by zones
	for (auto & painter : _obstaclePainters)
	{
		painter.second->clearBlockedArea();
	}
	
	for(auto & t : selection)
	{
		auto tl = _map->getTile(t);
		if(tl.blocked() || tl.visitable())
			continue;
		
		auto terrain = tl.getTerrainID();
		_obstaclePainters[terrain]->addBlockedTile(t);
	}
	
	for(auto & sel : _obstaclePainters)
	{
		for(auto o : sel.second->placeObstacles(CRandomGenerator::getDefault()))
		{
			_mapHandler->invalidate(o.get());
			_scenes[level]->objectsView.redrawObjects({o.get()});
		}
	}
	
	_scenes[level]->selectionTerrainView.clear();
	_scenes[level]->objectsView.update();
	_scenes[level]->passabilityView.update();
	
	_miniscenes[level]->updateMap();
	main->mapChanged();
}

namespace
{
bool isObstacle(const CGObjectInstance & object)
{
	return !object.isVisitable() && object.getObjectHandler().isStaticObject();
}

QRgb toRgb(const ColorRGBA & color)
{
	return qRgb(color.r, color.g, color.b);
}

/// Whether a terrain picture can mark obstacles on this terrain with a separate colour
bool showsObstacles(const TerrainId & id)
{
	const auto * terrain = id.toEntity(LIBRARY);
	return terrain->isPassable() && !terrain->isWater() && toRgb(terrain->minimapBlocked) != toRgb(terrain->minimapUnblocked);
}
}

QImage MapController::terrainImage(int level) const
{
	QImage image(_map->width, _map->height, QImage::Format_RGB32);
	for(int y = 0; y < _map->height; ++y)
	{
		for(int x = 0; x < _map->width; ++x)
		{
			const auto & tile = _map->getTile(int3(x, y, level));
			const bool covered = std::any_of(tile.blockingObjects.begin(), tile.blockingObjects.end(), [&](const ObjectInstanceID & id)
			{
				return isObstacle(*_map->getObject(id));
			});
			const auto * terrain = tile.getTerrain();
			image.setPixel(x, y, toRgb(covered ? terrain->minimapBlocked : terrain->minimapUnblocked));
		}
	}
	return image;
}

QImage MapController::objectsImage(int level) const
{
	QImage image(_map->width, _map->height, QImage::Format_ARGB32);
	image.fill(Qt::transparent);
	for(const auto & object : _map->objects)
	{
		if(!object || object->anchorPos().z != level || isObstacle(*object))
			continue;
		QColor color(Qt::white);
		if(object->getOwner().isValidPlayer())
			color = QColor::fromRgb(graphics->playerColors.at(object->getOwner().getNum()));
		color.setAlpha(150);
		for(const auto & pos : object->getBlockedPosOnMap(_map->getTopology()))
			image.setPixelColor(pos.x, pos.y, color);
		if(object->isVisitable())
		{
			if(const auto visitable = _map->getTopology().canonicalize(object->visitablePos()))
			{
				color.setAlpha(255);
				image.setPixelColor(visitable->x, visitable->y, color);
			}
		}
	}
	return image;
}

TerrainImportResult MapController::importTerrainImage(int level, const QImage & image)
{
	if(image.width() != _map->width || image.height() != _map->height)
		throw std::invalid_argument("Terrain image size differs from the map size");

	struct PaletteEntry
	{
		QRgb color;
		TerrainId terrain;
		bool obstacle;
	};
	std::vector<PaletteEntry> palette;
	for(const auto & terrain : LIBRARY->terrainTypeHandler->getObjects())
		palette.push_back({toRgb(terrain->minimapUnblocked), terrain->getId(), false});
	// unblocked colours come first and win when a terrain uses one colour for both
	for(const auto & terrain : LIBRARY->terrainTypeHandler->getObjects())
		palette.push_back({toRgb(terrain->minimapBlocked), terrain->getId(), true});

	TerrainImportResult result;
	const int tileCount = _map->width * _map->height;
	std::vector<TerrainId> target(tileCount);
	std::vector<bool> marked(tileCount);
	for(int y = 0; y < _map->height; ++y)
	{
		for(int x = 0; x < _map->width; ++x)
		{
			const QRgb pixel = image.pixel(x, y);
			const PaletteEntry * best = nullptr;
			int bestDistance = std::numeric_limits<int>::max();
			for(const auto & entry : palette)
			{
				const int distance = (qRed(pixel) - qRed(entry.color)) * (qRed(pixel) - qRed(entry.color))
					+ (qGreen(pixel) - qGreen(entry.color)) * (qGreen(pixel) - qGreen(entry.color))
					+ (qBlue(pixel) - qBlue(entry.color)) * (qBlue(pixel) - qBlue(entry.color));
				if(distance < bestDistance)
				{
					bestDistance = distance;
					best = &entry;
				}
			}
			if(bestDistance > 0)
				++result.inexactPixels;
			target[y * _map->width + x] = best->terrain;
			marked[y * _map->width + x] = best->obstacle && showsObstacles(best->terrain);
		}
	}

	const auto index = [&](const int3 & pos) { return pos.y * _map->width + pos.x; };
	std::map<TerrainId, std::vector<int3>> changed;
	std::vector<bool> changedTile(tileCount);
	for(int y = 0; y < _map->height; ++y)
	{
		for(int x = 0; x < _map->width; ++x)
		{
			const int3 pos(x, y, level);
			if(_map->getTile(pos).getTerrainID() == target[index(pos)])
				continue;
			changed[target[index(pos)]].push_back(pos);
			changedTile[index(pos)] = true;
			++result.changedTiles;
		}
	}

	auto * editManager = _map->getEditManager();
	editManager->beginUndoGroup();

	std::set<CGObjectInstance *> staleObstacles;
	for(const auto & object : _map->objects)
	{
		if(!object || object->anchorPos().z != level || !isObstacle(*object))
			continue;
		for(const auto & pos : object->getBlockedPosOnMap(_map->getTopology()))
		{
			// obstacles on water and rock cannot be drawn in the picture, so they stay unless the terrain changes
			if(changedTile[index(pos)] || (showsObstacles(target[index(pos)]) && !marked[index(pos)]))
			{
				staleObstacles.insert(object.get());
				break;
			}
		}
	}
	if(!staleObstacles.empty())
		editManager->removeObjects(staleObstacles);
	result.removedObstacles = static_cast<int>(staleObstacles.size());

	// Base land first, so that later terrains draw their transitions over it; water and rock last restore their borders
	const auto rank = [](const TerrainId & id)
	{
		const auto * terrain = id.toEntity(LIBRARY);
		if(id == ETerrainId::DIRT)
			return 0;
		if(id == ETerrainId::SAND)
			return 1;
		if(terrain->isWater())
			return 3;
		if(terrain->isRock())
			return 4;
		return 2;
	};
	std::vector<TerrainId> order;
	for(const auto & entry : changed)
		order.push_back(entry.first);
	std::stable_sort(order.begin(), order.end(), [&](const TerrainId & a, const TerrainId & b) { return rank(a) < rank(b); });
	static const int terrainDecorationPercentageLevel = 10;
	for(const auto & terrain : order)
	{
		editManager->getTerrainSelection().setSelection(changed.at(terrain));
		editManager->drawTerrain(terrain, terrainDecorationPercentageLevel, &CRandomGenerator::getDefault());
	}

	for(auto & painter : _obstaclePainters)
		painter.second->clearBlockedArea();
	for(int y = 0; y < _map->height; ++y)
	{
		for(int x = 0; x < _map->width; ++x)
		{
			const int3 pos(x, y, level);
			const auto & tile = _map->getTile(pos);
			if(marked[index(pos)] && !tile.blocked() && !tile.visitable() && tile.getTerrainID() == target[index(pos)])
				_obstaclePainters.at(tile.getTerrainID())->addBlockedTile(pos);
		}
	}
	for(auto & painter : _obstaclePainters)
		result.placedObstacles += static_cast<int>(painter.second->placeObstacles(CRandomGenerator::getDefault()).size());

	editManager->endUndoGroup();

	for(const auto & object : _map->objects)
	{
		if(!object || object->anchorPos().z != level || isObstacle(*object) || object->appearance->canBePlacedAtAnyTerrain())
			continue;
		const auto pos = _map->getTopology().canonicalize(object->isVisitable() ? object->visitablePos() : object->anchorPos());
		if(pos && object->appearance->canBePlacedAt(ETerrainId::WATER) != _map->getTile(*pos).isWater())
			result.misplacedObjects.push_back(object.get());
	}

	_scenes[level]->selectionTerrainView.clear();
	_scenes[level]->selectionObjectsView.clear();
	resetMapHandler();
	sceneForceUpdate();
	main->mapChanged();
	return result;
}

void MapController::commitObjectChange(int level)
{	
	_scenes[level]->objectsView.redrawObjects(_scenes[level]->selectionObjectsView.getSelection());
	
	_scenes[level]->selectionObjectsView.redraw();
	_scenes[level]->passabilityView.redraw();
	
	_miniscenes[level]->updateMap();
	main->mapChanged();
}


void MapController::commitChangeWithoutRedraw()
{
	//DO NOT REDRAW
	main->mapChanged();
}

void MapController::commitObjectShift(int level)
{
	auto shift = _scenes[level]->selectionObjectsView.shift;
	bool makeShift = !shift.isNull();
	if(makeShift)
	{
		std::set<CGObjectInstance*> movedObjects = _scenes[level]->selectionObjectsView.getSelection();
		for(auto * obj : movedObjects)
		{
			int3 pos = obj->pos;
			pos.z = level;
			pos.x += shift.x(); pos.y += shift.y();
			if(_map->toroidal)
				if(const auto canonical = _map->getTopology().canonicalize(pos))
					pos = *canonical;
			
			_map->getEditManager()->moveObject(obj, pos);
			_mapHandler->invalidate(obj);
		}
		_scenes[level]->objectsView.redrawObjects(movedObjects);
	}
	
	_scenes[level]->selectionObjectsView.newObject = nullptr;
	_scenes[level]->selectionObjectsView.setShift(0, 0);
	_scenes[level]->selectionObjectsView.selectionMode = SelectionObjectsLayer::NOTHING;
	
	if(makeShift)
	{
		_scenes[level]->objectsView.redraw();
		_scenes[level]->passabilityView.redraw();
		_scenes[level]->selectionObjectsView.redraw();
		
		_miniscenes[level]->updateMap();
		main->mapChanged();
	}
}

void MapController::commitObjectCreate(int level)
{
	auto newObj = _scenes[level]->selectionObjectsView.newObject;
	if(!newObj)
		return;
	
	auto shift = _scenes[level]->selectionObjectsView.shift;
	
	int3 pos = newObj->pos;
	pos.z = level;
	pos.x += shift.x(); pos.y += shift.y();
	if(_map->toroidal)
		if(const auto canonical = _map->getTopology().canonicalize(pos))
			pos = *canonical;
	
	newObj->pos = pos;
	
	Initializer init(*this, newObj.get(), defaultPlayer);
	
	_map->getEditManager()->insertObject(newObj);
	_mapHandler->invalidate(newObj.get());
	_scenes[level]->objectsView.redrawObjects({newObj.get()});
	
	_scenes[level]->selectionObjectsView.newObject = nullptr;
	_scenes[level]->selectionObjectsView.setShift(0, 0);
	_scenes[level]->selectionObjectsView.selectionMode = SelectionObjectsLayer::NOTHING;
	_scenes[level]->passabilityView.redraw();
	
	_miniscenes[level]->updateMap();
	main->mapChanged();
}

bool MapController::canPlaceObject(const CGObjectInstance * newObj, QString & error, std::optional<int3> destination) const
{	
	if(newObj->ID == Obj::GRAIL) //special case for grail
		return canPlaceGrail(newObj, error);

	if((newObj->ID == Obj::HERO || newObj->ID == Obj::RANDOM_HERO) && !canPlaceHeroAtPosition(newObj, error, destination))
		return false;
	
	if(defaultPlayer == PlayerColor::NEUTRAL && (newObj->ID == Obj::HERO || newObj->ID == Obj::RANDOM_HERO))
		return canPlaceHero(newObj, error);
	
	return checkRequiredMods(newObj, error);
}

bool MapController::canPlaceGrail(const CGObjectInstance * grailObj, QString & error) const
{
	assert(grailObj->ID == Obj::GRAIL);

	//find all objects of such type
	int objCounter = 0;
	for(auto o : _map->objects)
	{
		if(o->ID == grailObj->ID && o->subID == grailObj->subID)
		{
			++objCounter;
		}
	}

	if(objCounter >= 1)
	{
		error = QObject::tr("There can only be one grail object on the map.");
		return false; //maplimit reached
	}
	
	return true;
}

bool MapController::canPlaceHero(const CGObjectInstance * heroObj, QString & error) const
{
	assert(heroObj->ID == Obj::HERO || heroObj->ID == Obj::RANDOM_HERO);

	PlayerSelectionDialog dialog(main);
	if(dialog.exec() == QDialog::Accepted)
	{
		main->switchDefaultPlayer(dialog.getSelectedPlayer());
		return true;
	}
	
	error = tr("Hero %1 cannot be created as NEUTRAL.").arg(QString::fromStdString(heroObj->instanceName));
	return false;
}

bool MapController::canPlaceHeroAtPosition(const CGObjectInstance * heroObj, QString & error, std::optional<int3> destination) const
{
	const auto rawVisitable = destination ? *destination - heroObj->getVisitableOffset() : heroObj->visitablePos();
	const auto heroVisitable = _map->getTopology().canonicalize(rawVisitable).value_or(rawVisitable);
	for(const auto & townID : _map->getAllTowns())
	{
		const auto * town = _map->getObject(townID);
		if(town->visitablePos() != heroVisitable || !town->getOwner().isValidPlayer())
			continue;

		const auto & playerInfo = _map->players.at(town->getOwner().getNum());
		if(playerInfo.hasMainTown && playerInfo.generateHeroAtMainTown && playerInfo.posOfMainTown == town->pos)
		{
			error = tr("Hero cannot be placed at the entrance of this town, because it is the main town of its owner and a hero is generated there on game start. Disable \"Generate hero at main\" in player settings first.");
			return false;
		}
	}
	return true;
}

bool MapController::checkRequiredMods(const CGObjectInstance * obj, QString & error) const
{
	ModCompatibilityInfo modsInfo;
	modAssessmentObject(obj, modsInfo);

	for(auto & mod : modsInfo)
	{
		if(!_map->mods.count(mod.first))
		{
			auto reply = QMessageBox::question(main,
				tr("Missing Required Mod"), modMissingMessage(mod.second) + tr("\n\nDo you want to do that now ?"),
				QMessageBox::Yes | QMessageBox::No);

			if(reply == QMessageBox::Yes)
			{
				_map->mods[mod.first] = LIBRARY->modh->getModInfo(mod.first).getVerificationInfo();
				Q_EMIT requestModsUpdate(modsInfo, true); // signal for MapSettings
			}
			else
			{
				error = tr("This object's mod is mandatory for map to remain valid.");
				return false;
			}
		}
	}
	return true;
}

QString MapController::modMissingMessage(const ModVerificationInfo & info)
{
	QString modName = QString::fromStdString(info.name);
	QString submod;
	if(!info.parent.empty())
		submod = QObject::tr(" (submod of %1)").arg(QString::fromStdString(info.parent));

	return QObject::tr("The mod '%1'%2, is required by an object on the map.\n"
		"Add it to the map's required mods in Map->General settings.",
		"should be consistent with Map->General menu entry translation")
		.arg(modName, submod);
}

void MapController::undo()
{
	_map->getEditManager()->getUndoManager().undo();
	resetMapHandler();
	sceneForceUpdate(); //TODO: use smart invalidation (setDirty)
	main->mapChanged();
}

void MapController::redo()
{
	_map->getEditManager()->getUndoManager().redo();
	resetMapHandler();
	sceneForceUpdate(); //TODO: use smart invalidation (setDirty)
	main->mapChanged();
}

ModCompatibilityInfo MapController::modAssessmentAll()
{
	ModCompatibilityInfo result;
	for(auto primaryID : LIBRARY->objtypeh->knownObjects())
	{
		for(auto secondaryID : LIBRARY->objtypeh->knownSubObjects(primaryID))
		{
			auto & handler = LIBRARY->objtypeh->getHandlerFor(primaryID, secondaryID);
			auto modScope = handler.getModScope();
			if(modScope != "core")
				result[modScope] = LIBRARY->modh->getModInfo(modScope).getVerificationInfo();
		}
	}
	return result;
}

void MapController::modAssessmentObject(const CGObjectInstance * obj, ModCompatibilityInfo & result)
{
	auto extractEntityMod = [&result](const auto & entity)
	{
		auto modScope = entity->getModScope();
		if(modScope != "core")
			result[modScope] = LIBRARY->modh->getModInfo(modScope).getVerificationInfo();
	};

	auto & handler = obj->getObjectHandler();
	auto modScope = handler.getModScope();
	if(modScope != "core")
		result[modScope] = LIBRARY->modh->getModInfo(modScope).getVerificationInfo();

	if(obj->ID == Obj::TOWN || obj->ID == Obj::RANDOM_TOWN)
	{
		auto town = dynamic_cast<const CGTownInstance *>(obj);
		for(const auto & spellID : town->possibleSpells)
		{
			if(spellID == SpellID::PRESET)
				continue;
			extractEntityMod(spellID.toEntity(LIBRARY));
		}

		for(const auto & spellID : town->obligatorySpells)
		{
			extractEntityMod(spellID.toEntity(LIBRARY));
		}
	}

	if(obj->ID == Obj::HERO || obj->ID == Obj::RANDOM_HERO)
	{
		auto hero = dynamic_cast<const CGHeroInstance *>(obj);
		for(const auto & spellID : hero->getSpellsInSpellbook())
		{
			if(spellID == SpellID::PRESET || spellID == SpellID::SPELLBOOK_PRESET)
				continue;
			extractEntityMod(spellID.toEntity(LIBRARY));
		}

		for(const auto & [_, slotInfo] : hero->artifactsWorn)
		{
			extractEntityMod(slotInfo.getArt()->getTypeId().toEntity(LIBRARY));
		}

		for(const auto & art : hero->artifactsInBackpack)
		{
			extractEntityMod(art.getArt()->getTypeId().toEntity(LIBRARY));
		}
	}

//TODO: terrains?
}

ModCompatibilityInfo MapController::modAssessmentMap(const CMap & map)
{
	ModCompatibilityInfo result;

	for(auto obj : map.objects)
	{
		if(!obj)
			continue;
		modAssessmentObject(obj.get(), result);
	}
	return result;
}
