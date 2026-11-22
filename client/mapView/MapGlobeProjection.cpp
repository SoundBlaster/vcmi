/*
 * MapGlobeProjection.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapGlobeProjection.h"

#include "render/IScreenHandler.h"

namespace
{
/// Share of the Lambert shading that is applied
constexpr double shadingStrength = 0.7;

/// View pixels per mesh cell. The mesh only has to bend the image smoothly; it does not follow map tiles.
constexpr int meshCellSize = 16;
}

MapGlobeProjection::MapGlobeProjection(const Point & viewDimensions, double cornerAngleDegrees)
	: viewDimensions(viewDimensions)
	, flatDimensions(viewDimensions)
{
	const double cornerAngle = std::clamp(cornerAngleDegrees, 0.0, 80.0) * M_PI / 180.0;
	const double cornerDistance = std::hypot(viewDimensions.x / 2.0, viewDimensions.y / 2.0);

	if(cornerAngle <= 0 || cornerDistance <= 0)
		return;

	radius = cornerDistance / std::sin(cornerAngle);

	// The flat offset grows faster than the view offset, so the view corners sample farthest out on both axes.
	// Rounded up to even sizes, so that the view center stays on a whole pixel of the flat image.
	const double growth = cornerAngle / std::sin(cornerAngle);
	flatDimensions.x = 2 * static_cast<int>(std::ceil(viewDimensions.x * growth / 2));
	flatDimensions.y = 2 * static_cast<int>(std::ceil(viewDimensions.y * growth / 2));
}

bool MapGlobeProjection::isEnabled() const
{
	return radius > 0;
}

Point MapGlobeProjection::getFlatDimensions() const
{
	return flatDimensions;
}

void MapGlobeProjection::toFlat(double viewX, double viewY, double & flatX, double & flatY) const
{
	const double offsetX = viewX - viewDimensions.x / 2.0;
	const double offsetY = viewY - viewDimensions.y / 2.0;
	const double distance = std::hypot(offsetX, offsetY);

	// asin(x) / x tends to 1 at the center
	double factor = 1;
	if(isEnabled() && distance > 0)
		factor = radius * std::asin(std::min(distance / radius, 1.0)) / distance;

	flatX = flatDimensions.x / 2.0 + offsetX * factor;
	flatY = flatDimensions.y / 2.0 + offsetY * factor;
}

Point MapGlobeProjection::toFlat(const Point & viewPosition) const
{
	double flatX;
	double flatY;
	toFlat(viewPosition.x, viewPosition.y, flatX, flatY);
	return Point(static_cast<int>(std::floor(flatX)), static_cast<int>(std::floor(flatY)));
}

double MapGlobeProjection::brightness(double viewX, double viewY) const
{
	if(!isEnabled())
		return 1;

	const double distance = std::hypot(viewX - viewDimensions.x / 2.0, viewY - viewDimensions.y / 2.0);
	const double sine = std::min(distance / radius, 1.0);
	const double cosine = std::sqrt(1 - sine * sine);
	return 1 - shadingStrength * (1 - cosine);
}

std::shared_ptr<const PresentedMesh> MapGlobeProjection::createMesh() const
{
	auto mesh = std::make_shared<PresentedMesh>();

	const int columns = std::max(1, viewDimensions.x / meshCellSize);
	const int rows = std::max(1, viewDimensions.y / meshCellSize);

	mesh->vertices.reserve((columns + 1) * (rows + 1));
	for(int row = 0; row <= rows; ++row)
	{
		for(int column = 0; column <= columns; ++column)
		{
			const double viewX = viewDimensions.x * column / static_cast<double>(columns);
			const double viewY = viewDimensions.y * row / static_cast<double>(rows);
			double flatX;
			double flatY;
			toFlat(viewX, viewY, flatX, flatY);

			mesh->vertices.push_back({
				static_cast<float>(column) / columns,
				static_cast<float>(row) / rows,
				static_cast<float>(flatX / flatDimensions.x),
				static_cast<float>(flatY / flatDimensions.y),
				static_cast<float>(brightness(viewX, viewY))
			});
		}
	}

	mesh->indices.reserve(columns * rows * 6);
	for(int row = 0; row < rows; ++row)
	{
		for(int column = 0; column < columns; ++column)
		{
			const int topLeft = row * (columns + 1) + column;
			const int bottomLeft = topLeft + columns + 1;
			mesh->indices.insert(mesh->indices.end(), {topLeft, topLeft + 1, bottomLeft, topLeft + 1, bottomLeft + 1, bottomLeft});
		}
	}

	return mesh;
}

bool MapGlobeProjection::operator==(const MapGlobeProjection & other) const
{
	return viewDimensions == other.viewDimensions && radius == other.radius;
}
