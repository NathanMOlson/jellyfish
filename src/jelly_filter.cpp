#include "jelly_filter.h"
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace std;

JellyFilter::JellyFilter(GstPad *srcpad) : srcpad_(srcpad), q_("JellyFilter", 1)
{
    static int n = 0;
    n_ = n++;
    cv::setNumThreads(0);
    for (int i = 0; i < 4; i++)
    {
        threads_.push_back(thread(&JellyFilter::work, this));
    }
}

JellyFilter::~JellyFilter()
{
    q_.disable();
    pts_q_.disable();
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
        transform(&frames.in, &frames.out);
    }
}

void JellyFilter::transform(cv::Mat &in, cv::Mat &out, GstClockTime pts, GstClockTime duration)
{
    cv::Mat hpf;
    cv::Mat blur;
    
    // auto t1 = chrono::steady_clock::now().time_since_epoch().count();
    // cv::GaussianBlur(in, blur, cv::Size(51, 51), 25);
    cv::boxFilter(in, blur, -1, cv::Size(43, 43));
    for (int i = 0; i < 3; i++)
    {
        cv::boxFilter(blur, blur, -1, cv::Size(43, 43));
    }
    // auto t2 = chrono::steady_clock::now().time_since_epoch().count();

    cv::subtract(in, blur, hpf, cv::noArray(), CV_16S);
    // auto t3 = chrono::steady_clock::now().time_since_epoch().count();

    cv::Mat filtered;

    cv::Mat t = cv::Mat::zeros(4, 4, CV_16S);
    t.at<int16_t>(0, 2) = -2;
    t.at<int16_t>(1, 1) = -2;
    t.at<int16_t>(0, 1) = -0.6;
    t.at<int16_t>(2, 1) = 0.6;
    t.at<int16_t>(2, 0) = 2;
    t.at<int16_t>(3, 3) = 1;

    cv::transform(hpf, filtered, t);
    // auto t4 = chrono::steady_clock::now().time_since_epoch().count();

    filtered.convertTo(out, CV_8UC4);
    // auto t5 = chrono::steady_clock::now().time_since_epoch().count();

    // cout<<t2-t1<<" "<<t3-t2<<" "<<t4-t3<<" "<<t5-t4<<"\n"<<flush;
}

void JellyFilter::transform(GstVideoFrame *inframe, GstVideoFrame *outframe)
{
    // cout<<"3 "<<chrono::steady_clock::now().time_since_epoch().count()<<" "<<inframe->buffer->pts<<"\n"<<flush;
    GstBuffer *inbuf = inframe->buffer;
    GstBuffer *outbuf = outframe->buffer;
    cv::Mat in(inframe->info.height, inframe->info.width, CV_8UC4, inframe->data[0]);
    cv::Mat out(outframe->info.height, outframe->info.width, CV_8UC4, outframe->data[0]);
    transform(in, out, inbuf->pts, inbuf->duration);

    int pts = inbuf->pts;

    if (outbuf != inbuf)
        gst_buffer_unref(inbuf);

    gst_video_frame_unmap(outframe);
    gst_video_frame_unmap(inframe);

    if (pts >= 0)
    {
        pts_q_.WaitForTurn(pts);
    }
    // cout<<"4 "<<chrono::steady_clock::now().time_since_epoch().count()<<" "<<inframe->buffer->pts<<"\n"<<flush;
    gst_pad_push(srcpad_, outbuf);
    // cout<<"5 "<<chrono::steady_clock::now().time_since_epoch().count()<<" "<<inframe->buffer->pts<<"\n"<<flush;

    gst_buffer_unref(inbuf);
    gst_buffer_unref(outbuf);
    pts_q_.MarkTurnComplete(pts);
}

void JellyFilter::transform_async(GstVideoFrame *inframe, GstVideoFrame *outframe)
{
    InOutFrames frames;
    frames.in = *inframe;
    // cout<<"1 "<<chrono::steady_clock::now().time_since_epoch().count()<<" "<<inframe->buffer->pts<<"\n"<<flush;
    frames.out = *outframe;
    gst_buffer_ref(inframe->buffer);
    gst_buffer_ref(outframe->buffer);
    q_.push(frames, MpmcFullBehavior::BLOCK);
    // cout<<"2 "<<chrono::steady_clock::now().time_since_epoch().count()<<" "<<inframe->buffer->pts<<"\n"<<flush;
    // transform(frames.in, frames.out);
}