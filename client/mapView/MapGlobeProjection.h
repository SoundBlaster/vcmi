/*
 * MapGlobeProjection.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/Point.h"

struct PresentedMesh;

/// Projects the flat map image onto a sphere seen from above, centered on the view.
/// Distance from the view center along the sphere becomes distance on screen: rho = R * sin(r / R).
/// Scale at the center stays 1, so the flat image has to be larger than the view to fill its corners.
class MapGlobeProjection
{
	Point viewDimensions;
	Point flatDimensions;
	double radius = 0; ///< sphere radius, in pixels; 0 when the projection is disabled

public:
	MapGlobeProjection() = default;

	/// cornerAngleDegrees is the angle between the view center and a view corner, seen from the sphere center
	MapGlobeProjection(const Point & viewDimensions, double cornerAngleDegrees);

	bool isEnabled() const;

	/// Size of the flat map image the view is projected from, centered on the view center
	Point getFlatDimensions() const;

	/// Position in the flat map image shown at the given position in the view, in pixels of each
	void toFlat(double viewX, double viewY, double & flatX, double & flatY) const;

	/// Position in the flat map image shown at the given position in the view
	Point toFlat(const Point & viewPosition) const;

	/// Lambert shading of the sphere lit from the viewer, softened so that the view corners stay readable
	double brightness(double viewX, double viewY) const;

	/// Grid mesh covering the whole view, for presenting the flat map image through this projection
	std::shared_ptr<const PresentedMesh> createMesh() const;

	bool operator==(const MapGlobeProjection & other) const;
};
