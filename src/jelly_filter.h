#ifndef _JELLY_FILTER_H_
#define _JELLY_FILTER_H_

#include <gst/video/video.h>
#include <opencv2/core.hpp>

class JellyFilter
{
public:
    JellyFilter();
    ~JellyFilter();
    void transform(GstVideoFrame *inframe, GstVideoFrame *outframe);
    void transform(cv::Mat &in, cv::Mat &out, GstClockTime pts, GstClockTime duration);
};

#endif
