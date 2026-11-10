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
#include "render/GpuResources.h"

#include <SDL3/SDL.h>

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
	bool drawTexture(SDL_Renderer * renderer, const Point & position, const Rect * source, int scalingFactor) const override
	{
		assert(scalingFactor == 1);
		return sprite->drawTexture(renderer, nullptr, position, source, ColorRGBA(255, 255, 255, 255), 255, EImageBlitMode::OPAQUE, ImageFlip{});
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

TEST(WrappedSpriteSdl, CompositesPeriodicFragmentsThroughTexturePath)
{
	SDL_Surface * screen = SDL_CreateSurface(32, 32, SDL_PIXELFORMAT_RGBA32);
	ASSERT_NE(screen, nullptr);
	SDL_Renderer * renderer = SDL_CreateSoftwareRenderer(screen);
	if(!renderer)
	{
		SDL_DestroySurface(screen);
		GTEST_SKIP() << "SDL software renderer unavailable";
	}
	GpuResources::get().setRenderer(renderer);
	struct RendererCleanup
	{
		SDL_Surface * screen;
		~RendererCleanup()
		{
			GpuResources::get().destroyRenderer();
			SDL_DestroySurface(screen);
		}
	} cleanup{screen};
	using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
	Texture target(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 32, 32), SDL_DestroyTexture);
	ASSERT_NE(target, nullptr);
	auto image = std::make_shared<SoftwareSpriteImage>();
	{
		auto canvas = image->getCanvas();
		for(int y = 0; y < 96; ++y)
			for(int x = 0; x < 96; ++x)
				canvas.drawPoint(Point(x, y), ColorRGBA(x, y, 90, 255));
	}
	image->prepare();
	using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
	const MapTopology topology(3, 3, 1, true);
	for(int dx : {-1, 0, 1})
	for(int dy : {-1, 0, 1})
	{
		if(dx == 0 && dy == 0)
			continue;
		for(int progressPixels = 0; progressPixels <= 32; ++progressPixels)
		{
			SCOPED_TRACE(::testing::Message() << "direction=" << dx << "," << dy << " progress=" << progressPixels);
			const int3 from(dx < 0 ? 0 : 2, dy < 0 ? 0 : 2, 0);
			const auto destination = *topology.canonicalize(from + int3(dx, dy, 0));
			const auto offset = movingObjectImageOffset(topology, true, from, destination, destination, progressPixels / 32.0);
			ASSERT_TRUE(SDL_SetRenderTarget(renderer, target.get()));
			ASSERT_TRUE(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
			ASSERT_TRUE(SDL_RenderClear(renderer));
			{
				auto canvas = Canvas::createFromRenderTarget(target.get(), Point(32, 32), CanvasScalingPolicy::IGNORE);
				forEachWrappedImageFragment(offset, image->dimensions(), Point(96, 96), [&](const Point & position, const Rect & source)
				{
					canvas.draw(image, position, source);
				});
			}
			Surface pixels(SDL_RenderReadPixels(renderer, nullptr), SDL_DestroySurface);
			ASSERT_NE(pixels, nullptr);
			std::vector<uint8_t> rgba(32 * 32 * 4);
			ASSERT_TRUE(SDL_ConvertPixels(32, 32, pixels->format, pixels->pixels, pixels->pitch, SDL_PIXELFORMAT_RGBA32, rgba.data(), 32 * 4));
			for(int y = 0; y < 32; ++y)
				for(int x = 0; x < 32; ++x)
				{
					const auto index = static_cast<size_t>((y * 32 + x) * 4);
					EXPECT_EQ(rgba[index], MapTopology::wrapCoordinate(96 - offset.x - 32 + x, 96));
					EXPECT_EQ(rgba[index + 1], MapTopology::wrapCoordinate(96 - offset.y - 32 + y, 96));
					EXPECT_EQ(rgba[index + 2], 90);
					EXPECT_EQ(rgba[index + 3], 255);
				}
		}
	}
	SDL_SetRenderTarget(renderer, nullptr);
}
