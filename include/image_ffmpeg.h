/*
 * Copyright (C) 2015 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @image_ffmpeg.h
 * The Image_FFmpeg class
 */

#ifndef SPLASH_IMAGE_FFMPEG_H
#define SPLASH_IMAGE_FFMPEG_H

#include "config.h"

#include <atomic>
#include <deque>
#include <mutex>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#include "coretypes.h"
#include "basetypes.h"
#include "image.h"
#if HAVE_PORTAUDIO
    #include "speaker.h"
#endif

namespace oiio = OIIO_NAMESPACE;

namespace Splash {

class Image_FFmpeg : public Image
{
    public:
        /**
         * Constructor
         */
        Image_FFmpeg();

        /**
         * Destructor
         */
        ~Image_FFmpeg();

        /**
         * No copy, but some move constructors
         */
        Image_FFmpeg(const Image_FFmpeg&) = delete;
        Image_FFmpeg& operator=(const Image_FFmpeg&) = delete;

        /**
         * Set the path to read from
         */
        bool read(const std::string& filename);

    private:
        void* _avFormatContext {nullptr};
        std::thread _readLoopThread;
        std::atomic_bool _continueRead;
        std::atomic_bool _loopOnVideo {true};

        int64_t _elapsedTime {0};
        int64_t _seekFrame {-1};

#if HAVE_PORTAUDIO
        std::unique_ptr<Speaker> _speaker;
        AVCodecContext* _audioCodecContext {nullptr};
#endif

        /**
         * Free everything related to FFmpeg
         */
        void freeFFmpegObjects();

        /**
         * File read loop
         */
        void readLoop();

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Image_FFmpeg> Image_FFmpegPtr;

} // end of namespace

#endif // SPLASH_IMAGE_FFMPEG_H
