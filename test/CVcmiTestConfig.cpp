/*
 * CVcmiTestConfig.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CVcmiTestConfig.h"

#include "../lib/GameLibrary.h"
#include "../lib/filesystem/AdapterLoaders.h"
#include "../lib/filesystem/CFilesystemLoader.h"
#include "../lib/filesystem/Filesystem.h"

void CVcmiTestConfig::SetUp()
{
	const char * moddedRoot = std::getenv("VCMI_TEST_MODDED_ROOT");
	const bool useModdedRoot = moddedRoot && *moddedRoot;
	// useTestPreset activates the core+vcmi+vcmi-test preset (config/testModSettings.json),
	// so tests are independent of the developer's active mods and never overwrite the real
	// modSettings.json. The vcmi-test mod supplies all fixtures and flips on the HOTA map
	// format needed by TinyH3MBuilder.
	// The explicit external-mod mode is intended for focused integration tests.
	// Its own config takes precedence over the real user profile and must already
	// contain modSettings.json, so an empty directory cannot silently fall back.
	if(useModdedRoot)
	{
		const boost::filesystem::path root(moddedRoot);
		if(!boost::filesystem::is_regular_file(root / "config" / "modSettings.json") ||
		   !boost::filesystem::is_directory(root / "Mods"))
			throw std::runtime_error("VCMI_TEST_MODDED_ROOT requires config/modSettings.json and Mods/");
	}
	initializeBaseFilesystem(false, /*useTestPreset*/ !useModdedRoot);
	if(useModdedRoot)
	{
		const boost::filesystem::path root(moddedRoot);
		auto * local = dynamic_cast<CFilesystemList *>(CResourceHandler::get("local"));
		auto * initial = dynamic_cast<CFilesystemList *>(CResourceHandler::get("initial"));
		if(!local || !initial)
			throw std::runtime_error("Cannot prepare external-mod test filesystem");
		local->addLoader(std::make_shared<CFilesystemLoader>("CONFIG/", root / "config", 0), true);
		initial->addLoader(std::make_shared<CFilesystemLoader>("MODS/", root / "Mods", 64, true), false);
	}
	auto library = std::make_unique<GameLibrary>();
	library->initializeFilesystem(/*useTestPreset*/ !useModdedRoot);
	library->initializeLibrary();
	setGameLibrary(std::move(library));
}

void CVcmiTestConfig::TearDown()
{
	std::cout << "Ending global test tear-down." << std::endl;
}

