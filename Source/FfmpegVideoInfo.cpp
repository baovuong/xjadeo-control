/*
  ==============================================================================

    FfmpegVideoInfo.cpp

  ==============================================================================
*/

#include "FfmpegVideoInfo.h"

extern "C"
{
    #include <libavformat/avformat.h>
}

FfmpegVideoInfo readFfmpegVideoInfo (const juce::File& videoFile)
{
    FfmpegVideoInfo info;

    AVFormatContext* formatContext = nullptr;

    if (avformat_open_input (&formatContext, videoFile.getFullPathName().toRawUTF8(), nullptr, nullptr) != 0)
        return info;

    if (avformat_find_stream_info (formatContext, nullptr) < 0)
    {
        avformat_close_input (&formatContext);
        return info;
    }

    const auto streamIndex = av_find_best_stream (formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    if (streamIndex < 0)
    {
        avformat_close_input (&formatContext);
        return info;
    }

    const auto* stream = formatContext->streams[(size_t) streamIndex];

    const auto frameRateRational = stream->avg_frame_rate.num != 0 ? stream->avg_frame_rate
                                                                    : stream->r_frame_rate;
    info.frameRate = frameRateRational.den != 0 ? av_q2d (frameRateRational) : 0.0;

    info.width = stream->codecpar->width;
    info.height = stream->codecpar->height;

    if (stream->duration != AV_NOPTS_VALUE)
        info.durationSeconds = stream->duration * av_q2d (stream->time_base);
    else if (formatContext->duration != AV_NOPTS_VALUE)
        info.durationSeconds = (double) formatContext->duration / AV_TIME_BASE;

    // Many containers (e.g. Matroska) don't populate nb_frames, so fall back
    // to deriving a frame count from duration and frame rate.
    if (stream->nb_frames > 0)
        info.frameCount = stream->nb_frames;
    else if (info.frameRate > 0.0 && info.durationSeconds > 0.0)
        info.frameCount = (juce::int64) std::llround (info.frameRate * info.durationSeconds);

    info.isValid = info.frameCount > 0;

    avformat_close_input (&formatContext);
    return info;
}
