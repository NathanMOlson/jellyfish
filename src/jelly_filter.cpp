#include "jelly_filter.h"
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace std;

TurnQueue JellyFilter::turn_q_;

JellyFilter::JellyFilter(GstPad *srcpad) : srcpad_(srcpad), q_("JellyFilter", 4)
{
    static int n = 0;
    n_ = n++;
    cv::setNumThreads(0);
    for (int i = 0; i < 1; i++)
    {
        threads_.push_back(thread(&JellyFilter::work, this));
    }
}

JellyFilter::~JellyFilter()
{
    q_.disable();
    turn_q_.disable();
    for (auto &thread : threads_)
    {
        thread.join();
    }
}

void JellyFilter::work()
{
    while (true)
    {
        InOutFrames frames;
        MpmcResult result = q_.pop(frames);
        if (result != MpmcResult::SUCCESS)
        {
            break;
        }
        transform(frames.in, frames.out);
    }
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

    int turn = inbuf->offset;

    gst_video_frame_unmap(outframe);
    gst_video_frame_unmap(inframe);

    if (outbuf != inbuf)
        gst_buffer_unref(inbuf);

    if (turn >= 0)
    {
        turn_q_.WaitForTurn(turn);
    }
    gst_pad_push(srcpad_, outbuf);
    turn_q_.MarkTurnComplete(turn);
}

void JellyFilter::transform_async(GstVideoFrame *inframe, GstVideoFrame *outframe)
{
    InOutFrames frames;
    frames.in = inframe;
    frames.out = outframe;
    // q_.push(frames, MpmcFullBehavior::DISCARD_OLDEST);
    transform(frames.in, frames.out);
}