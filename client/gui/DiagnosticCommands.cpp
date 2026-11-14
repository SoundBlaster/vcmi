/*
 * DiagnosticCommands.cpp, part of VCMI engine
 * License: GNU General Public License v2.0 or later
 */
#include "../StdInc.h"
#include "DiagnosticCommands.h"

#if defined(__unix__) || defined(__APPLE__)
#include "DiagnosticCommandChannel.h"
#include "../GameEngine.h"
#include "EventDispatcher.h"
#include "Shortcut.h"
#include "events/InputHandler.h"
#include "../../lib/Point.h"

namespace
{
bool dispatchDiagnosticCommand(const std::string & command)
{
	std::istringstream stream(command);
	std::string kind, value, extra;
	stream >> kind;
	if(kind == "click" || kind == "hover")
	{
		int x, y;
		if(!(stream >> x >> y) || (stream >> extra))
			return false;
		const auto size = ENGINE->screenDimensions();
		if(x < 0 || y < 0 || x >= size.x || y >= size.y)
			return false;
		const Point p(x, y);
		ENGINE->input().setCursorPosition(p);
		if(kind == "click")
		{
			ENGINE->events().dispatchMouseLeftButtonPressed(p, 0);
			ENGINE->events().dispatchMouseLeftButtonReleased(p, 0);
		}
		return true;
	}
	if(kind != "shortcut" || !(stream >> value) || (stream >> extra))
		return false;
	static const std::map<std::string, EShortcut> shortcuts = {
		{"cast", EShortcut::ADVENTURE_CAST_SPELL}, {"return", EShortcut::GLOBAL_RETURN},
		{"accept", EShortcut::GLOBAL_ACCEPT}, {"north", EShortcut::ADVENTURE_MOVE_HERO_NN},
		{"south", EShortcut::ADVENTURE_MOVE_HERO_SS}, {"east", EShortcut::ADVENTURE_MOVE_HERO_EE},
		{"west", EShortcut::ADVENTURE_MOVE_HERO_WW}, {"northeast", EShortcut::ADVENTURE_MOVE_HERO_NE},
		{"northwest", EShortcut::ADVENTURE_MOVE_HERO_NW}, {"southeast", EShortcut::ADVENTURE_MOVE_HERO_SE},
		{"southwest", EShortcut::ADVENTURE_MOVE_HERO_SW}
	};
	const auto it = shortcuts.find(value);
	if(it == shortcuts.end())
		return false;
	ENGINE->events().dispatchShortcutPressed({it->second});
	ENGINE->events().dispatchShortcutReleased({it->second});
	return true;
}
}

void pollDiagnosticCommands()
{
	static DiagnosticChannel channel;
	channel.poll(dispatchDiagnosticCommand);
}
#else
void pollDiagnosticCommands() {}
#endif
