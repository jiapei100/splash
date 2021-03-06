#include "image_opencv.h"

#include <opencv2/opencv.hpp>

#include <chrono>
#include <hap.h>

#include "cgUtils.h"
#include "log.h"
#include "threadpool.h"
#include "timer.h"

using namespace std;

namespace Splash
{

/*************/
Image_OpenCV::Image_OpenCV(weak_ptr<RootObject> root) : Image(root)
{
    init();
}

/*************/
Image_OpenCV::~Image_OpenCV()
{
    _continueReading = false;
    if (_readLoopThread.joinable())
        _readLoopThread.join();
}

/*************/
bool Image_OpenCV::read(const string& filename)
{
    try
    {
        _inputIndex = stoi(filename);
    }
    catch (...)
    {
        _inputIndex = -1;
    }

    if (_inputIndex == -1)
        _filepath = filename;
    else
        _filepath = to_string(_inputIndex);

    // This releases any previous input
    _continueReading = false;
    if (_readLoopThread.joinable())
        _readLoopThread.join();

    _continueReading = true;
    _readLoopThread = thread([&]() { readLoop(); });

    return true;
}

/*************/
void Image_OpenCV::init()
{
    _type = "image_opencv";
    registerAttributes();

    // If the root object weak_ptr is expired, this means that
    // this object has been created outside of a World or Scene.
    // This is used for getting documentation "offline"
    if (_root.expired())
        return;

    _videoCapture = unique_ptr<cv::VideoCapture>(new cv::VideoCapture());
}

/*************/
void Image_OpenCV::readLoop()
{
    if (!_videoCapture)
    {
        Log::get() << Log::WARNING << "Image_OpenCV::" << __FUNCTION__ << " - Unable to create the VideoCapture" << Log::endl;
        return;
    }

    if (!_videoCapture->isOpened())
    {
        bool status;
        if (_inputIndex >= 0)
            status = _videoCapture->open(_inputIndex);
        else
            status = _videoCapture->open(_filepath);

        if (!status)
        {
            Log::get() << Log::WARNING << "Image_OpenCV::" << __FUNCTION__ << " - Unable to open video capture input " << _filepath << Log::endl;
            return;
        }

        _videoCapture->set(CV_CAP_PROP_FRAME_WIDTH, _width);
        _videoCapture->set(CV_CAP_PROP_FRAME_HEIGHT, _height);
        _videoCapture->set(CV_CAP_PROP_FPS, _framerate);

        Log::get() << Log::MESSAGE << "Image_OpenCV::" << __FUNCTION__ << " - Successfully initialized VideoCapture " << _filepath << Log::endl;
    }

    while (_continueReading)
    {
        if (Timer::get().isDebug())
            Timer::get() << "read " + _name;

        auto capture = cv::Mat();
        if (!_videoCapture->read(capture))
        {
            Log::get() << Log::WARNING << "Image_OpenCV::" << __FUNCTION__ << " - An error occurred while reading the VideoCapture" << Log::endl;
            return;
        }

        auto spec = _readBuffer.getSpec();
        if (spec.width != capture.rows || spec.height != capture.cols || spec.channels != capture.channels())
        {
            ImageBufferSpec newSpec(capture.cols, capture.rows, capture.channels(), ImageBufferSpec::Type::UINT8);
            newSpec.format = vector<string>({"B", "G", "R"});
            _readBuffer = ImageBuffer(newSpec);
        }
        unsigned char* pixels = reinterpret_cast<unsigned char*>(_readBuffer.data());

        unsigned int imageSize = capture.rows * capture.cols * capture.channels();
        copy(capture.data, capture.data + imageSize, pixels);

        lock_guard<mutex> lockWrite(_writeMutex);
        if (!_bufferImage)
            _bufferImage = unique_ptr<ImageBuffer>(new ImageBuffer());
        std::swap(*_bufferImage, _readBuffer);
        _imageUpdated = true;
        updateTimestamp();

        if (Timer::get().isDebug())
            Timer::get() >> "read " + _name;
    }
}

/*************/
void Image_OpenCV::registerAttributes()
{
    addAttribute("size",
        [&](const Values& args) {
            _width = args[0].as<int>();
            _height = args[1].as<int>();

            _width = (_width == 0) ? 640 : _width;
            _height = (_height == 0) ? 640 : _height;

            return true;
        },
        [&]() -> Values {
            return {(int)_width, (int)_height};
        },
        {'n', 'n'});
    setAttributeDescription("size", "Set the desired capture resolution");

    addAttribute("framerate",
        [&](const Values& args) {
            _framerate = (args[0].as<float>() == 0) ? 60 : args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_framerate}; },
        {'n'});
    setAttributeDescription("framerate", "Set the desired capture framerate");
}

} // end of namespace
