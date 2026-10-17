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
#include "PreRTS.h"

#include "Common/FrameStats.h"
#include "Common/GlobalData.h"

#include <algorithm>
#include <vector>


namespace
{
	UnsignedInt s_durationSeconds = 0;
	UnsignedInt s_startFrame = 0;
	Bool s_sawGame = FALSE;
	Bool s_done = FALSE;

	double s_freq = 0.0;
	Int64 s_frameStart = 0;
	Int64 s_logicStart = 0;
	Int64 s_collectStart = 0;
	double s_frameLogicMs = 0.0;

	std::vector<Real> s_frameMs;      ///< Engine update time of every collected frame
	std::vector<Real> s_logicMs;      ///< Game logic update time of every collected logic frame

	Int64 now()
	{
		LARGE_INTEGER tick;
		QueryPerformanceCounter(&tick);
		return tick.QuadPart;
	}

	double toMs(Int64 ticks)
	{
		return (double)ticks * 1000.0 / s_freq;
	}

	double average(const std::vector<Real>& values)
	{
		if (values.empty())
			return 0.0;
		double sum = 0.0;
		for (size_t i = 0; i < values.size(); ++i)
			sum += values[i];
		return sum / values.size();
	}

	// Returns the value below which the given fraction of the sorted values fall.
	Real percentile(const std::vector<Real>& sorted, double fraction)
	{
		if (sorted.empty())
			return 0.0f;
		size_t index = (size_t)(fraction * (sorted.size() - 1));
		return sorted[index];
	}

	// Returns the average of the slowest 1 percent of the frames, as fps.
	double onePercentLowFps(const std::vector<Real>& sorted)
	{
		if (sorted.empty())
			return 0.0;
		size_t count = max((size_t)1, sorted.size() / 100);
		double sum = 0.0;
		for (size_t i = sorted.size() - count; i < sorted.size(); ++i)
			sum += sorted[i];
		return 1000.0 * count / sum;
	}

	void writeReport(double wallMs, Bool complete)
	{
		std::vector<Real> sortedFrames(s_frameMs);
		std::sort(sortedFrames.begin(), sortedFrames.end());
		std::vector<Real> sortedLogic(s_logicMs);
		std::sort(sortedLogic.begin(), sortedLogic.end());

		const double avgFrameMs = average(s_frameMs);

		AsciiString path = TheGlobalData->getPath_UserData();
		path.concat("FrameStats.txt");

		FILE* file = fopen(path.str(), "w");
		if (file == nullptr)
			return;

		fprintf(file, "complete            %d\n", complete ? 1 : 0);
		fprintf(file, "frames              %u\n", (UnsignedInt)s_frameMs.size());
		fprintf(file, "wall_seconds        %.2f\n", wallMs / 1000.0);
		fprintf(file, "wall_fps            %.1f\n", wallMs > 0.0 ? s_frameMs.size() * 1000.0 / wallMs : 0.0);
		fprintf(file, "frame_ms_avg        %.3f\n", avgFrameMs);
		fprintf(file, "frame_ms_p50        %.3f\n", percentile(sortedFrames, 0.50));
		fprintf(file, "frame_ms_p99        %.3f\n", percentile(sortedFrames, 0.99));
		fprintf(file, "frame_ms_max        %.3f\n", percentile(sortedFrames, 1.0));
		fprintf(file, "fps_avg             %.1f\n", avgFrameMs > 0.0 ? 1000.0 / avgFrameMs : 0.0);
		fprintf(file, "fps_1pct_low        %.1f\n", onePercentLowFps(sortedFrames));
		fprintf(file, "logic_frames        %u\n", (UnsignedInt)s_logicMs.size());
		fprintf(file, "logic_ms_avg        %.3f\n", average(s_logicMs));
		fprintf(file, "logic_ms_p99        %.3f\n", percentile(sortedLogic, 0.99));
		fprintf(file, "logic_ms_max        %.3f\n", percentile(sortedLogic, 1.0));
		fclose(file);
	}
}

void FrameStats::enable(UnsignedInt seconds, UnsignedInt startFrame)
{
	s_durationSeconds = seconds;
	s_startFrame = startFrame;

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	s_freq = (double)freq.QuadPart;
}

Bool FrameStats::isEnabled()
{
	return s_durationSeconds > 0 && !s_done;
}

UnsignedInt FrameStats::getStartFrame()
{
	return s_startFrame;
}

void FrameStats::beginFrame()
{
	if (!isEnabled())
		return;

	s_frameStart = now();
	s_frameLogicMs = 0.0;
}

void FrameStats::beginLogic()
{
	if (!isEnabled())
		return;

	s_logicStart = now();
}

void FrameStats::endLogic()
{
	if (!isEnabled())
		return;

	s_frameLogicMs = toMs(now() - s_logicStart);
}

Bool FrameStats::endFrame(Bool inGame, Bool collect)
{
	if (!isEnabled())
		return FALSE;

	const Int64 end = now();

	if (inGame)
	{
		s_sawGame = TRUE;
	}
	else if (s_sawGame)
	{
		// The game ended before the duration elapsed. Report what was collected.
		writeReport(s_collectStart != 0 ? toMs(end - s_collectStart) : 0.0, FALSE);
		s_done = TRUE;
		return TRUE;
	}

	if (!collect)
		return FALSE;

	if (s_collectStart == 0)
	{
		// Skip the first frame, which still carries the cost of loading the game.
		s_collectStart = end;
		return FALSE;
	}

	s_frameMs.push_back((Real)toMs(end - s_frameStart));
	if (s_frameLogicMs > 0.0)
		s_logicMs.push_back((Real)s_frameLogicMs);

	const double wallMs = toMs(end - s_collectStart);
	if (wallMs < s_durationSeconds * 1000.0)
		return FALSE;

	writeReport(wallMs, TRUE);
	s_done = TRUE;
	return TRUE;
}
