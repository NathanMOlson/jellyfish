#ifndef _JELLY_FILTER_H_
#define _JELLY_FILTER_H_

#include <gst/video/video.h>
#include <opencv2/core.hpp>
#include "mpmc_queue.h"
#include "turn_queue.h"

struct InOutFrames
{
    GstVideoFrame *in;
    GstVideoFrame *out;
};

class JellyFilter
{
public:
    JellyFilter(GstPad *srcpad);
    ~JellyFilter();
    void transform_async(GstVideoFrame *inframe, GstVideoFrame *outframe);

private:
    void transform(GstVideoFrame *inframe, GstVideoFrame *outframe);
    void transform(cv::Mat &in, cv::Mat &out, GstClockTime pts, GstClockTime duration);
    void work();
    std::vector<std::thread> threads_;
    GstPad* srcpad_;
    MpmcQueue<InOutFrames> q_;
    static TurnQueue turn_q_;
    int n_ = 0;
};

#endif
