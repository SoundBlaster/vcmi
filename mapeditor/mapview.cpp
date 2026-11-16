/*
 * mapview.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "mapview.h"
#include "mainwindow.h"
#include <QGraphicsSceneMouseEvent>
#include "mapcontroller.h"
#include "TerrainViewTopology.h"
#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapping/CMap.h"
#include "../lib/GameLibrary.h"
#include <QScopedValueRollback>

namespace
{
int sceneTileCoordinate(qreal coordinate, bool toroidal)
{
	return TerrainViewTopology::sceneTileCoordinate(coordinate, toroidal, 32);
}
}


MinimapView::MinimapView(QWidget * parent):
	QGraphicsView(parent)
{
	// Disable scrollbars
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void MinimapView::dimensions()
{
	fitInView(0, 0, controller->map()->width, controller->map()->height, Qt::KeepAspectRatio);
}

void MinimapView::setController(MapController * ctrl)
{
	controller = ctrl;
}

void MinimapView::mouseMoveEvent(QMouseEvent *mouseEvent)
{
	this->update();
	
	auto * sc = static_cast<MinimapScene*>(scene());
	if(!sc)
		return;
	
	auto pos = mapToScene(mouseEvent->pos());
	pos *= 32;
	cameraPositionChanged(pos);
}

void MinimapView::mousePressEvent(QMouseEvent* event)
{
	mouseMoveEvent(event);
}

void MinimapView::drawForeground(QPainter * painter, const QRectF & rect)
{
	QGraphicsView::drawForeground(painter, rect);
	if(!controller || !controller->map() || !controller->map()->toroidal)
		return;
	painter->save();
	QPen referencePen(QColor(40, 210, 225), 0, Qt::DashLine);
	referencePen.setCosmetic(true);
	painter->setPen(referencePen);
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(QRectF(0, 0, controller->map()->width, controller->map()->height));
	painter->restore();
}

#ifdef ENABLE_SINGLE_APP_BUILD
namespace MapEditor {
#endif

MapView::MapView(QWidget * parent):
	QGraphicsView(parent),
	selectionTool(MapView::SelectionTool::None),
	tileStart(int3{}),
	tilePrev(int3{}),
	pressedOnSelected(false)
{
	connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &MapView::setViewports);
	connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &MapView::setViewports);
}

void MapView::cameraChanged(const QPointF & pos)
{
	centerOn(pos);
}

void MapView::setController(MapController * ctrl)
{
	controller = ctrl;
}

void MapView::resetInteractionState()
{
	if(rubberBand)
		rubberBand->hide();

	tileStart = int3{};
	tilePrev = int3{};
	pressedOnSelected = false;
	temporaryTiles.clear();
	lassoOutline.clear();
}

void MapView::resizeEvent(QResizeEvent * event)
{
	QGraphicsView::resizeEvent(event);
	setViewports();
}

void MapView::mouseMoveEvent(QMouseEvent *mouseEvent)
{
	this->update();

	auto * sc = static_cast<MapScene*>(scene());
	if(!sc || !controller->map())
		return;

	auto pos = mapToScene(mouseEvent->pos()); //TODO: do we need to check size?
	int3 tile(sceneTileCoordinate(pos.x(), controller->map()->toroidal), sceneTileCoordinate(pos.y(), controller->map()->toroidal), sc->level);
	
	//if scene will be scrolled without mouse movement, selection, object moving and rubber band will not be updated
	//to change this behavior, all this logic should be placed in viewportEvent
	if(rubberBand)
		rubberBand->setGeometry(QRect(mapFromScene(mouseStart), mouseEvent->pos()).normalized());
	
	if(tile == tilePrev) //do not redraw
		return;

	const auto canonical = controller->map()->getTopology().canonicalize(tile);
	if(canonical)
		currentCoordinates(canonical->x, canonical->y);
	else
		currentCoordinates(tile.x, tile.y);

	switch(selectionTool)
	{
	case MapView::SelectionTool::Brush:
		if(mouseEvent->buttons() & Qt::RightButton)
			sc->selectionTerrainView.erase({tile});
		else if(mouseEvent->buttons() == Qt::LeftButton)
			sc->selectionTerrainView.select({tile});
		break;

	case MapView::SelectionTool::Brush2:
		{
			const auto tiles = TerrainViewTopology::brushSelection(controller->map()->getTopology(), tile, 0, 2, controller->map()->toroidal);
			if(mouseEvent->buttons() & Qt::RightButton)
				sc->selectionTerrainView.erase(tiles);
			else if(mouseEvent->buttons() == Qt::LeftButton)
				sc->selectionTerrainView.select(tiles);
		}
		break;

	case MapView::SelectionTool::Brush4:
	{
		const auto tiles = TerrainViewTopology::brushSelection(controller->map()->getTopology(), tile, -1, 4, controller->map()->toroidal);
		if(mouseEvent->buttons() & Qt::RightButton)
			sc->selectionTerrainView.erase(tiles);
		else if(mouseEvent->buttons() == Qt::LeftButton)
			sc->selectionTerrainView.select(tiles);
	}
		break;

	case MapView::SelectionTool::Area:
	{
		if(mouseEvent->buttons() & Qt::RightButton || !(mouseEvent->buttons() & Qt::LeftButton))
			break;

		sc->selectionTerrainView.clear();
		std::vector<int3> selectedTiles;
		for(int j = std::min(tile.y, tileStart.y); j < std::max(tile.y, tileStart.y); ++j)
		{
			for(int i = std::min(tile.x, tileStart.x); i < std::max(tile.x, tileStart.x); ++i)
			{
				selectedTiles.emplace_back(i, j, sc->level);
			}
		}
		sc->selectionTerrainView.select(selectedTiles);
		break;
	}
	case MapView::SelectionTool::Line:
	{
		{
			if(!(mouseEvent->buttons() & (Qt::LeftButton | Qt::RightButton)))
				break;

			if(tile.z != tileStart.z)
			{
				temporaryTiles.clear();
				tileStart = tilePrev = tile;
				break;
			}

			const auto diff = tile - tileStart;
			if(diff == int3{})
				break;
			
			const int edge = std::max(abs(diff.x), abs(diff.y));
			int distMin = std::numeric_limits<int>::max();
			int3 dir;
			
			for(auto & d : int3::getDirs())
			{
				if(tile.dist2d(d * edge + tileStart) < distMin)
				{
					distMin = tile.dist2d(d * edge + tileStart);
					dir = d;
				}
			}
			
			assert(dir != int3{});
			
			if(mouseEvent->buttons() == Qt::LeftButton)
			{
				std::vector<int3>erasedTiles(temporaryTiles.begin(), temporaryTiles.end());
				sc->selectionTerrainView.erase(erasedTiles);

				std::vector<int3>selectedTiles;
				for(auto ts = tileStart; ts.dist2d(tileStart) < edge; ts += dir)
				{
					const auto position = controller->map()->getTopology().canonicalize(ts);
					if(!position)
						break;
					if(!sc->selectionTerrainView.selection().count(*position))
						temporaryTiles.insert(*position);
					selectedTiles.push_back(*position);
				}
				sc->selectionTerrainView.select(selectedTiles);
			}
			if(mouseEvent->buttons() == Qt::RightButton)
			{
				std::vector<int3>selectedTiles(temporaryTiles.begin(), temporaryTiles.end());
				sc->selectionTerrainView.select(selectedTiles);

				std::vector<int3>erasedTiles;
				for(auto ts = tileStart; ts.dist2d(tileStart) < edge; ts += dir)
				{
					const auto position = controller->map()->getTopology().canonicalize(ts);
					if(!position)
						break;
					if(sc->selectionTerrainView.selection().count(*position))
						temporaryTiles.insert(*position);
					erasedTiles.push_back(*position);
				}
				sc->selectionTerrainView.erase(erasedTiles);
			}
			break;
		}
	}

	case MapView::SelectionTool::Lasso:
	{
		if(mouseEvent->buttons() == Qt::LeftButton)
		{
			std::vector<int3>tiles;
			for(auto i = tilePrev; i != tile;)
			{
				int length = std::numeric_limits<int>::max();
				int3 dir;
				for(auto & d : int3::getDirs())
				{
					if(tile.dist2dSQ(i + d) < length)
					{
						dir = d;
						length = tile.dist2dSQ(i + d);
					}
				}
				i += dir;
				tiles.push_back(i);
				lassoOutline.insert(i);
			}
			sc->selectionTerrainView.select(tiles);
		}
		break;
	}

	case MapView::SelectionTool::None:
		if(mouseEvent->buttons() & Qt::RightButton)
			break;

		auto sh = tile - tileStart;
		sc->selectionObjectsView.setShift(sh.x, sh.y);

		if(sh.x || sh.y)
		{
			if(!sc->selectionObjectsView.newObject && (mouseEvent->buttons() & Qt::LeftButton))
			{
				if(sc->selectionObjectsView.selectionMode == SelectionObjectsLayer::SELECTION)
				{
					sc->selectionObjectsView.clear();
					sc->selectionObjectsView.selectObjects(tileStart.x, tileStart.y, tile.x, tile.y);
				}
			}
		}
		break;
	}
	
	tilePrev = tile;
}

void MapView::mousePressEvent(QMouseEvent *event)
{
	this->update();

	auto * sc = static_cast<MapScene*>(scene());
	if(!sc || !controller->map())
		return;
	
	if(sc->objectPickerView.isVisible())
		return;

	mouseStart = mapToScene(event->pos());
	tileStart = tilePrev = int3(sceneTileCoordinate(mouseStart.x(), controller->map()->toroidal), sceneTileCoordinate(mouseStart.y(), controller->map()->toroidal), sc->level);
	const auto canonicalStart = controller->map()->getTopology().canonicalize(tileStart);

	if(canonicalStart && sc->selectionTerrainView.selection().count(*canonicalStart))
		pressedOnSelected = true;
	else
		pressedOnSelected = false;

	switch(selectionTool)
	{
	case MapView::SelectionTool::Brush:
	case MapView::SelectionTool::Line:
		sc->selectionObjectsView.clear();

		if(event->button() == Qt::RightButton)
			sc->selectionTerrainView.erase({tileStart});
		else if(event->button() == Qt::LeftButton)
			sc->selectionTerrainView.select({tileStart});
		break;

	case MapView::SelectionTool::Brush2:
		sc->selectionObjectsView.clear();
	{
		const auto tiles = TerrainViewTopology::brushSelection(controller->map()->getTopology(), tileStart, 0, 2, controller->map()->toroidal);
		if(event->buttons() & Qt::RightButton)
			sc->selectionTerrainView.erase(tiles);
		else if(event->buttons() == Qt::LeftButton)
			sc->selectionTerrainView.select(tiles);
	}
		break;

	case MapView::SelectionTool::Brush4:
		sc->selectionObjectsView.clear();
	{
		const auto tiles = TerrainViewTopology::brushSelection(controller->map()->getTopology(), tileStart, -1, 4, controller->map()->toroidal);
		if(event->buttons() & Qt::RightButton)
			sc->selectionTerrainView.erase(tiles);
		else if(event->buttons() == Qt::LeftButton)
			sc->selectionTerrainView.select(tiles);
	}
		break;

	case MapView::SelectionTool::Area:
	case MapView::SelectionTool::Lasso:
		if(event->button() == Qt::RightButton)
			break;

		sc->selectionTerrainView.clear();
		sc->selectionObjectsView.clear();
		if(selectionTool == MapView::SelectionTool::Lasso)
		{
			lassoOutline.clear();
			lassoOutline.insert(tileStart);
		}
		break;

	case MapView::SelectionTool::Fill:
	{
		if(event->button() != Qt::RightButton && event->button() != Qt::LeftButton)
			break;

		if(!canonicalStart)
			break;
		const auto tilesToFill = TerrainViewTopology::floodSelection(controller->map()->getTopology(), *canonicalStart,
			[&](const int3 & tile, const int3 & tilen)
			{
				if(event->button() == Qt::LeftButton)
				{
					if(controller->map()->getTile(tile).roadType
							&& controller->map()->getTile(tile).roadType != controller->map()->getTile(tilen).roadType)
						return false;
					else if(controller->map()->getTile(tile).riverType
							&& controller->map()->getTile(tile).riverType != controller->map()->getTile(tilen).riverType)
						return false;
					else if(controller->map()->getTile(tile).terrainType != controller->map()->getTile(tilen).terrainType)
						return false;
				}
				if(event->button() == Qt::LeftButton && sc->selectionTerrainView.selection().count(tilen))
					return false;
				if(event->button() == Qt::RightButton && !sc->selectionTerrainView.selection().count(tilen))
					return false;
				return true;
			});
		std::vector<int3> result(tilesToFill.begin(), tilesToFill.end());

		if(event->button() == Qt::LeftButton)
			sc->selectionTerrainView.select(result);
		else
			sc->selectionTerrainView.erase(result);

		sc->selectionObjectsView.clear();
		break;
	}

	case MapView::SelectionTool::None:
		sc->selectionTerrainView.clear();

		if(sc->selectionObjectsView.newObject && sc->selectionObjectsView.isSelected(sc->selectionObjectsView.newObject.get()))
		{
			if(event->button() == Qt::RightButton)
				controller->discardObject(sc->level);
		}
		else
		{
			if(event->button() == Qt::LeftButton)
			{
				//when paste, new object could be beyond initial object so we need to test two objects in order to select new one
				//if object is pasted at place where is multiple objects then proper selection is not guaranteed
				auto * firstSelectedObject = sc->selectionObjectsView.selectObjectAt(tileStart.x, tileStart.y);
				auto * secondSelectedObject = sc->selectionObjectsView.selectObjectAt(tileStart.x, tileStart.y, firstSelectedObject);
				if(firstSelectedObject)
				{
					if(sc->selectionObjectsView.isSelected(firstSelectedObject))
					{
						if(qApp->keyboardModifiers() & Qt::ControlModifier)
						{
							sc->selectionObjectsView.deselectObject(firstSelectedObject);
							sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::SELECTION;
						}
						else
							sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::MOVEMENT;
					}
					else
					{
						if(!secondSelectedObject || !sc->selectionObjectsView.isSelected(secondSelectedObject))
						{
							if(!(qApp->keyboardModifiers() & Qt::ControlModifier))
								sc->selectionObjectsView.clear();
							sc->selectionObjectsView.selectObject(firstSelectedObject);
						}
						sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::MOVEMENT;
					}
				}
				else
				{
					sc->selectionObjectsView.clear();
					sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::SELECTION;

					if(!rubberBand)
						rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
					rubberBand->setGeometry(QRect(mapFromScene(mouseStart), QSize()));
					rubberBand->show();
				}
			}
			sc->selectionObjectsView.setShift(0, 0);
		}
		break;
	}

	//main->setStatusMessage(QString("x: %1 y: %2").arg(QString::number(event->pos().x()), QString::number(event->pos().y())));
}

void MapView::mouseReleaseEvent(QMouseEvent *event)
{
	this->update();

	auto * sc = static_cast<MapScene*>(scene());
	if(!sc || !controller->map())
		return;
	
	if(rubberBand)
		rubberBand->hide();
	
	if(sc->objectPickerView.isVisible())
	{
		if(event->button() == Qt::RightButton)
			sc->objectPickerView.discard();
		
		if(event->button() == Qt::LeftButton)
		{
			mouseStart = mapToScene(event->pos());
			tileStart = tilePrev = int3(sceneTileCoordinate(mouseStart.x(), controller->map()->toroidal), sceneTileCoordinate(mouseStart.y(), controller->map()->toroidal), sc->level);
			if(auto * pickedObject = sc->selectionObjectsView.selectObjectAt(tileStart.x, tileStart.y))
				sc->objectPickerView.select(pickedObject);
		}
		
		return;
	}

	switch(selectionTool)
	{
	case MapView::SelectionTool::Lasso: {
		if(event->button() == Qt::RightButton)
			break;
		std::vector<int3>initialTiles;
		//connect with initial tile
		for(auto i = tilePrev; i != tileStart;)
		{
			int length = std::numeric_limits<int>::max();
			int3 dir;
			for(auto & d : int3::getDirs())
			{
				if(tileStart.dist2dSQ(i + d) < length)
				{
					dir = d;
					length = tileStart.dist2dSQ(i + d);
				}
			}
			i += dir;
			initialTiles.push_back(i);
			lassoOutline.insert(i);
		}
		sc->selectionTerrainView.select(initialTiles);

		//key: y position of tile
		//value.first: x position of left tile
		//value.second: x postiion of right tile
		std::map<int, std::pair<int, int>> selectionRangeMapX;
		std::map<int, std::pair<int, int>> selectionRangeMapY;
		const auto & outline = controller->map()->toroidal ? lassoOutline : sc->selectionTerrainView.selection();
		for(auto & t : outline)
		{
			auto pairIter = selectionRangeMapX.find(t.y);
			if(pairIter == selectionRangeMapX.end())
				selectionRangeMapX[t.y] = std::make_pair(t.x, t.x);
			else
			{
				pairIter->second.first = std::min(pairIter->second.first, t.x);
				pairIter->second.second = std::max(pairIter->second.second, t.x);
			}
			
			pairIter = selectionRangeMapY.find(t.x);
			if(pairIter == selectionRangeMapY.end())
				selectionRangeMapY[t.x] = std::make_pair(t.y, t.y);
			else
			{
				pairIter->second.first = std::min(pairIter->second.first, t.y);
				pairIter->second.second = std::max(pairIter->second.second, t.y);
			}
		}
		
		std::set<int3> selectionByX;
		std::set<int3> selectionByY;
		std::vector<int3> finalSelection;
		for(auto & selectionRange : selectionRangeMapX)
		{
			for(int i = selectionRange.second.first; i < selectionRange.second.second; ++i)
				selectionByX.insert(int3(i, selectionRange.first, sc->level));
		}
		for(auto & selectionRange : selectionRangeMapY)
		{
			for(int i = selectionRange.second.first; i < selectionRange.second.second; ++i)
				selectionByY.insert(int3(selectionRange.first, i, sc->level));
		}
		std::set_intersection(selectionByX.begin(), selectionByX.end(), selectionByY.begin(), selectionByY.end(), std::back_inserter(finalSelection));
		sc->selectionTerrainView.select(finalSelection);
		lassoOutline.clear();

		break;
	}
			
	case MapView::SelectionTool::Line:
		temporaryTiles.clear();
		break;
			
	case MapView::SelectionTool::None:
		if(event->button() == Qt::RightButton)
			break;
		//switch position
		bool tab = false;
		if(sc->selectionObjectsView.selectionMode == SelectionObjectsLayer::MOVEMENT)
		{
			tab = sc->selectionObjectsView.shift.isNull();
			controller->commitObjectShift(sc->level);
		}
		else
		{
			sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::NOTHING;
			sc->selectionObjectsView.setShift(0, 0);
			tab = true;
		}
		auto selection = sc->selectionObjectsView.getSelection();
		if(selection.size() == 1)
		{
			openObjectProperties(*selection.begin(), tab);
		}
		break;
	}
}

void MapView::dragEnterEvent(QDragEnterEvent * event)
{
	if(!controller || !controller->map())
		return;
	
	auto * sc = static_cast<MapScene*>(scene());
	if(!sc)
		return;
	
	if(event->mimeData()->hasImage())
	{
		logGlobal->info("Drag'n'drop: dispatching object");
		QVariant vdata = event->mimeData()->imageData();
		auto data = vdata.toJsonObject();
		if(!data.empty())
		{
			auto preview = data["preview"];
			if(preview != QJsonValue::Undefined)
			{
				auto objId = data["id"].toInt();
				auto objSubId = data["subid"].toInt();
				auto templateId = data["template"].toInt();
				auto factory = LIBRARY->objtypeh->getHandlerFor(objId, objSubId);
				auto templ = factory->getTemplates()[templateId];
				controller->discardObject(sc->level);
				controller->createObject(sc->level, factory->create(controller->getCallback(), templ));
			}
		}
		
		event->acceptProposedAction();
	}
}

void MapView::dropEvent(QDropEvent * event)
{
	if(!controller || !controller->map())
		return;
	
	auto * sc = static_cast<MapScene*>(scene());
	if(!sc)
		return;
	
	if(sc->selectionObjectsView.newObject)
	{
		//the drop position is authoritative: the last dragMoveEvent may lag far behind it
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		moveNewObjectTo(event->position().toPoint());
#else
		moveNewObjectTo(event->pos());
#endif

		QString errorMsg;
		if(controller->canPlaceObject(sc->selectionObjectsView.newObject.get(), errorMsg))
		{
			auto obj = sc->selectionObjectsView.newObject;
			controller->commitObjectCreate(sc->level);
			openObjectProperties(obj.get(), false);
		}
		else
		{
			controller->discardObject(sc->level);
			QMessageBox::information(this, tr("Can't place object"), errorMsg);
		}
	}

	event->acceptProposedAction();
}

void MapView::moveNewObjectTo(const QPoint & viewportPos)
{
	auto * sc = static_cast<MapScene*>(scene());
	if(!sc || !sc->selectionObjectsView.newObject)
		return;

	auto pos = mapToScene(viewportPos);
	int3 tile(sceneTileCoordinate(pos.x(), controller->map()->toroidal) + 1, sceneTileCoordinate(pos.y(), controller->map()->toroidal) + 1, sc->level);
	if(controller->map()->toroidal)
		if(const auto canonical = controller->map()->getTopology().canonicalize(tile))
			tile = *canonical;

	sc->selectionObjectsView.selectionMode = SelectionObjectsLayer::MOVEMENT;
	sc->selectionObjectsView.selectObject(sc->selectionObjectsView.newObject.get());
	sc->selectionObjectsView.setShift(tile.x, tile.y);
}

void MapView::dragMoveEvent(QDragMoveEvent * event)
{
	if(!scene())
		return;

	moveNewObjectTo(event->answerRect().bottomRight()); //TODO: do we need to check size?

	event->acceptProposedAction();
}

void MapView::dragLeaveEvent(QDragLeaveEvent * event)
{
	if(!controller || !controller->map())
		return;

	auto * sc = static_cast<MapScene*>(scene());
	if(!sc)
		return;

	controller->discardObject(sc->level);
}

void MapView::setViewports()
{
	if(updatingViewport || !controller || !controller->map())
		return;
	QScopedValueRollback guard(updatingViewport, true);
	if(auto * sc = dynamic_cast<MapScene*>(scene()))
	{
		const auto * map = controller->map();
		const int width = map->width * 32;
		const int height = map->height * 32;
		if(map->toroidal)
		{
			const auto visibleScene = mapToScene(viewport()->rect()).boundingRect();
			const int paddingX = static_cast<int>(std::ceil(visibleScene.width())) + width;
			const int paddingY = static_cast<int>(std::ceil(visibleScene.height())) + height;
			const QRectF desired(-paddingX, -paddingY, width + 2 * paddingX, height + 2 * paddingY);
			if(sceneRect() != desired)
				setSceneRect(desired);
			const QPointF center = mapToScene(viewport()->rect().center());
			const QPointF canonical(TerrainViewTopology::wrapSceneCoordinate(center.x(), width),
			                        TerrainViewTopology::wrapSceneCoordinate(center.y(), height));
			const QPointF shift = canonical - center;
			if(!shift.isNull())
			{
				centerOn(canonical);
				mouseStart += shift;
				const int dx = static_cast<int>(std::round(shift.x() / 32));
				const int dy = static_cast<int>(std::round(shift.y() / 32));
				tileStart.x += dx;
				tileStart.y += dy;
				tilePrev.x += dx;
				tilePrev.y += dy;
				std::set<int3> shiftedOutline;
				for(const auto & tile : lassoOutline)
					shiftedOutline.insert(tile + int3(dx, dy, 0));
				lassoOutline.swap(shiftedOutline);
			}
		}
		else
		{
			const QRectF desired(0, 0, width, height);
			if(sceneRect() != desired)
				setSceneRect(desired);
		}
		auto rect = mapToScene(viewport()->rect()).boundingRect();
		if(map->toroidal)
		{
			const int left = static_cast<int>(std::floor(rect.left() / 32));
			const int top = static_cast<int>(std::floor(rect.top() / 32));
			const int right = static_cast<int>(std::ceil(rect.right() / 32));
			const int bottom = static_cast<int>(std::ceil(rect.bottom() / 32));
			controller->miniScene(sc->level)->viewport.setViewport(left, top, right - left, bottom - top);
		}
		else
			controller->miniScene(sc->level)->viewport.setViewport(rect.x() / 32, rect.y() / 32, rect.width() / 32, rect.height() / 32);
		for (auto * layer : sc->getDynamicLayers())
		{
			layer->setViewport(rect);
		}
	}
}

#ifdef ENABLE_SINGLE_APP_BUILD
} // namespace MapEditor
#endif

MapSceneBase::MapSceneBase(int lvl):
	QGraphicsScene(nullptr),
	level(lvl)
{
}

void MapSceneBase::initialize(MapController & controller)
{
	for(auto * layer : getStaticLayers())
		layer->initialize(controller);
	for(auto * layer : getDynamicLayers())
		layer->initialize(controller);
}

void MapSceneBase::createMap()
{
	for(auto * layer : getStaticLayers())
		layer->update();
	for(auto * layer : getDynamicLayers())
		layer->createLayer();
}

void MapSceneBase::updateMap()
{
	for(auto * layer : getStaticLayers())
		layer->update();
	for(auto * layer : getDynamicLayers())
		layer->redraw();
}

MapScene::MapScene(int lvl):
	MapSceneBase(lvl),
	emptyLayer(this),
	gridView(this),
	passabilityView(this),
	selectionTerrainView(this),
	terrainView(this),
	objectsView(this),
	selectionObjectsView(this),
	objectPickerView(this),
	isTerrainSelected(false),
	isObjectSelected(false)
{
	connect(&selectionTerrainView, &SelectionTerrainLayer::selectionMade, this, &MapScene::terrainSelected);
	connect(&selectionObjectsView, &SelectionObjectsLayer::selectionMade, this, &MapScene::objectSelected);
}

std::list<AbstractFixedLayer *> MapScene::getStaticLayers()
{
	return {
		&emptyLayer
	};
}

std::list<AbstractViewportLayer *> MapScene::getDynamicLayers()
{
	//sequence is important because it defines rendering order
	return {
		&terrainView,
		&objectsView,
		&gridView,
		&passabilityView,
		&objectPickerView,
		&selectionTerrainView,
		&selectionObjectsView
	};
}

void MapScene::createMap()
{
	MapSceneBase::createMap();

	terrainView.show(true);
	objectsView.show(true);
	selectionTerrainView.show(true);
	selectionObjectsView.show(true);
	objectPickerView.show(true);
}

void MapScene::terrainSelected(bool anythingSelected)
{
	isTerrainSelected = anythingSelected;
	selected(isTerrainSelected || isObjectSelected);
}

void MapScene::objectSelected(bool anythingSelected)
{
	isObjectSelected = anythingSelected;
	selected(isTerrainSelected || isObjectSelected);
}

MinimapScene::MinimapScene(int lvl):
	MapSceneBase(lvl),
	minimapView(this),
	viewport(this)
{
}

std::list<AbstractFixedLayer *> MinimapScene::getStaticLayers()
{
	//sequence is important because it defines rendering order
	return {
		&minimapView,
		&viewport
	};
}

std::list<AbstractViewportLayer *> MinimapScene::getDynamicLayers()
{
	//Nothing here
	return {};
}

void MinimapScene::createMap()
{
	MapSceneBase::createMap();

	minimapView.show(true);
	viewport.show(true);
}
