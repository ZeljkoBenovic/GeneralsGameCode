/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "Lib/BaseType.h"

// TheSuperHackers @feature ZeljkoBenovic 17/10/2026
// Collects the timings of every engine update while a game is running and writes a report when done.
// Enabled with the -frameStats <seconds> [startMinute] command line option, so that performance changes can be measured
// in release builds, for example with: -replay MyReplay.rep -frameStats 60 20
// The game logic runs as fast as possible until the start minute, then at its normal rate while the uncapped
// render frames are measured. While enabled, replays keep playing after a CRC mismatch, so that builds which are
// not CRC compatible with a replay can still be benchmarked with it. This also applies to -headless replay simulation.
class FrameStats
{
public:
	static void enable(UnsignedInt seconds, UnsignedInt startFrame); ///< Collect for this many seconds from the start frame, then write the report and quit.
	static Bool isEnabled();
	static UnsignedInt getStartFrame();

	static void beginFrame();
	static void beginLogic();
	static void endLogic();
	static Bool endFrame(Bool inGame, Bool collect); ///< Returns true once the report has been written: when the duration has elapsed, or when the game ended before.
};
