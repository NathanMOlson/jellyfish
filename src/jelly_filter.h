#ifndef _JELLY_FILTER_H_
#define _JELLY_FILTER_H_

#include <gst/video/video.h>
#include <opencv2/core.hpp>
#include "pts_queue.h"

class JellyFilter
{
public:
    JellyFilter(GstPad *srcpad);
    ~JellyFilter();
    void transform(GstVideoFrame *inframe, GstVideoFrame *outframe);

private:
    void transform(cv::Mat &in, cv::Mat &out, GstClockTime pts, GstClockTime duration);
    GstPad* srcpad_;
    static PtsQueue pts_q_;
    int n_ = 0;
};

#endif
