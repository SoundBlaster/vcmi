/*
 * WrappedSpriteSdlTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "../../client/mapView/MapViewMovement.h"
#include "../../lib/mapping/MapTopology.h"
#include "render/CanvasImage.h"

class SoftwareSpriteImage : public CanvasImage
{
public:
	SoftwareSpriteImage() : CanvasImage(Point(96, 96), CanvasScalingPolicy::IGNORE) {}
	void prepare() { sprite = toSharedImage(); }
	void draw(SDL_Surface * target, const Point & position, const Rect * source, int scalingFactor) const override
	{
		assert(scalingFactor == 1);
		sprite->draw(target, nullptr, position, source, ColorRGBA(255, 255, 255, 255), 255, EImageBlitMode::OPAQUE);
	}
private:
	std::shared_ptr<ISharedImage> sprite;
};

TEST(WrappedSpriteSdl, CompositesPeriodicFragmentsOnSoftwareCanvas)
{
	auto image = std::make_shared<SoftwareSpriteImage>();
	{
		auto canvas = image->getCanvas();
		for(int y = 0; y < 96; ++y)
			for(int x = 0; x < 96; ++x)
				canvas.drawPoint(Point(x, y), ColorRGBA(x, y, 90, 255));
	}
	image->prepare();

	const MapTopology topology(3, 3, 1, true);
	for(int dx : {-1, 1})
	for(int dy : {-1, 1})
	for(int progressPixels = 0; progressPixels <= 32; progressPixels += 8)
	{
		SCOPED_TRACE(::testing::Message() << "direction=" << dx << "," << dy << " progress=" << progressPixels);
		const int3 from(dx < 0 ? 0 : 2, dy < 0 ? 0 : 2, 0);
		const auto destination = *topology.canonicalize(from + int3(dx, dy, 0));
		const auto offset = movingObjectImageOffset(topology, true, from, destination, destination, progressPixels / 32.0);
		CanvasImage tile(Point(32, 32), CanvasScalingPolicy::IGNORE);
		{
			auto canvas = tile.getCanvas();
			forEachWrappedImageFragment(offset, image->dimensions(), Point(96, 96), [&](const Point & target, const Rect & source)
			{
				canvas.draw(image, target, source);
			});
		}

		std::vector<uint8_t> pixels;
		tile.readPixelsRGBA(pixels);
		ASSERT_EQ(pixels.size(), 32u * 32u * 4u);
		for(int y = 0; y < 32; ++y)
			for(int x = 0; x < 32; ++x)
			{
				const auto index = static_cast<size_t>((y * 32 + x) * 4);
				EXPECT_EQ(pixels[index], MapTopology::wrapCoordinate(96 - offset.x - 32 + x, 96));
				EXPECT_EQ(pixels[index + 1], MapTopology::wrapCoordinate(96 - offset.y - 32 + y, 96));
				EXPECT_EQ(pixels[index + 2], 90);
				EXPECT_EQ(pixels[index + 3], 255);
			}
	}
}
