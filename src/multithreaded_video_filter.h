#ifndef _MULTITHREADED_VIDEO_FILTER_H_
#define _MULTITHREADED_VIDEO_FILTER_H_

#include <gst/gst.h>
#include <opencv2/core.hpp>
#include <string>
#include "mpmc_queue.h"
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

struct VideoFormat
{
    std::string format;
    size_t width;
    size_t height;
};

class MultiThreadedVideoFilter
{
public:
    MultiThreadedVideoFilter(const std::string& name, const VideoFormat& in, const VideoFormat& out);
    ~MultiThreadedVideoFilter();
    GstElement* src();
    GstElement* sink();

private:
    void rx();
    void work();
    void transform(cv::Mat &in, cv::Mat &out);
    void transform(GstSample *inframe);

    GstAppSink* sink_;
    GstAppSrc* src_;
    MpmcQueue<GstSample*> in_queue_;
    std::vector<std::thread> threads_;
    std::thread rx_thread_;
    VideoFormat in_format_;
    VideoFormat out_format_;
};

#endif
