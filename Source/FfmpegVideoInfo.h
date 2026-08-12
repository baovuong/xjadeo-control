/*
  ==============================================================================

    FfmpegVideoInfo.h

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

struct FfmpegVideoInfo
{
    bool isValid = false;
    juce::int64 frameCount = 0;
    double frameRate = 0.0;
    double durationSeconds = 0.0;
    int width = 0;
    int height = 0;
};

// Reads video stream metadata via libavformat/libavcodec. Safe to call on a
// background thread; opens and closes the file each time it's called.
FfmpegVideoInfo readFfmpegVideoInfo (const juce::File& videoFile);
