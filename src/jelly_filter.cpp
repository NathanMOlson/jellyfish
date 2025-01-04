#include "jelly_filter.h"
#include <opencv2/imgproc.hpp>

using namespace std;

PtsQueue JellyFilter::pts_q_;

JellyFilter::JellyFilter(GstPad *srcpad) : srcpad_(srcpad)
{
    cv::setNumThreads(0);
}

JellyFilter::~JellyFilter()
{
    pts_q_.disable();
}

void JellyFilter::transform(cv::Mat &in, cv::Mat &out, GstClockTime pts, GstClockTime duration)
{
    cv::Mat hpf;
    cv::Mat blur;
    cv::GaussianBlur(in, blur, cv::Size(51, 51), 25);

    cv::subtract(in, blur, hpf, cv::noArray(), CV_16S);

    cv::Mat filtered;

    cv::Mat t = cv::Mat::zeros(4, 4, CV_16S);
    t.at<int16_t>(0, 2) = -2;
    t.at<int16_t>(1, 1) = -2;
    t.at<int16_t>(0, 1) = -0.6;
    t.at<int16_t>(2, 1) = 0.6;
    t.at<int16_t>(2, 0) = 2;
    t.at<int16_t>(3, 3) = 1;

    cv::transform(hpf, filtered, t);

    filtered.convertTo(out, CV_8UC4);
}

void JellyFilter::transform(GstVideoFrame *inframe, GstVideoFrame *outframe)
{
    GstBuffer *inbuf = inframe->buffer;
    GstBuffer *outbuf = outframe->buffer;
    cv::Mat in(inframe->info.height, inframe->info.width, CV_8UC4, inframe->data[0]);
    cv::Mat out(outframe->info.height, outframe->info.width, CV_8UC4, outframe->data[0]);
    transform(in, out, inbuf->pts, inbuf->duration);

    gst_video_frame_unmap(outframe);
    gst_video_frame_unmap(inframe);

    if (outbuf != inbuf)
        gst_buffer_unref(inbuf);

    pts_q_.WaitForTurn(outbuf->pts);
    gst_pad_push(srcpad_, outbuf);
    pts_q_.MarkTurnComplete(outbuf->pts);
}