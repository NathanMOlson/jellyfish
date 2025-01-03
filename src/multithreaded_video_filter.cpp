#include "multithreaded_video_filter.h"
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace std;

MultiThreadedVideoFilter::MultiThreadedVideoFilter(const std::string &name, const VideoFormat &in, const VideoFormat &out) : in_queue_("VideoFilterQueue", 8), in_format_(in), out_format_(out)
{
    src_ = GST_APP_SRC(gst_element_factory_make("appsrc", (name + "_src").c_str()));
    g_object_set(G_OBJECT(src_), "caps",
                 gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, in.format.c_str(),
                                     "width", G_TYPE_INT, in.width,
                                     "height", G_TYPE_INT, in.height,
                                     "framerate", GST_TYPE_FRACTION, 30, 1,
                                     NULL),
                 NULL);

    sink_ = GST_APP_SINK(gst_element_factory_make("appsink", (name + "_sink").c_str()));
    g_object_set(G_OBJECT(sink_), "caps",
                 gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, out.format.c_str(),
                                     "width", G_TYPE_INT, out.width,
                                     "height", G_TYPE_INT, out.height,
                                     "framerate", GST_TYPE_FRACTION, 30, 1,
                                     NULL),
                 "emit-signals", TRUE,
                 "sync", TRUE,
                 NULL);

    rx_thread_ = thread(&MultiThreadedVideoFilter::rx, this);
    for (int i = 0; i < 8; i++)
    {
        threads_.push_back(thread(&MultiThreadedVideoFilter::work, this));
    }
}

MultiThreadedVideoFilter::~MultiThreadedVideoFilter()
{
    in_queue_.disable();
    for (auto &thread : threads_)
    {
        thread.join();
    }
    rx_thread_.join();
    if (GST_IS_OBJECT(sink_))
    {
        gst_object_unref(GST_OBJECT(sink_));
    }
    if (GST_IS_OBJECT(src_))
    {
        gst_object_unref(GST_OBJECT(src_));
    }
}

GstElement *MultiThreadedVideoFilter::src()
{
    return GST_ELEMENT(src_);
}

GstElement *MultiThreadedVideoFilter::sink()
{
    return GST_ELEMENT(sink_);
}

void MultiThreadedVideoFilter::rx()
{
    while (true)
    {
        GstSample *sample = gst_app_sink_pull_sample(sink_);
        if (sample == NULL)
        {
            this_thread::sleep_for(0.1s);
            if (!in_queue_.enabled())
            {
                break;
            }
            continue;
        }
        MpmcResult result = in_queue_.push(sample, MpmcFullBehavior::DISCARD_OLDEST, 1.0s);
        if (result != MpmcResult::SUCCESS)
        {
            break;
        }
    }
}

void MultiThreadedVideoFilter::work()
{
    while (true)
    {
        GstSample *inframe;
        MpmcResult result = in_queue_.pop(inframe);
        if (result != MpmcResult::SUCCESS)
        {
            break;
        }
        transform(inframe);
    }
}

void MultiThreadedVideoFilter::transform(cv::Mat &in, cv::Mat &out)
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

void MultiThreadedVideoFilter::transform(GstSample *inframe)
{
    GstBuffer *inbuf = gst_sample_get_buffer(inframe);
    GstMapInfo in_info;
    gst_buffer_map(inbuf, &in_info, GST_MAP_READ);

    size_t out_size = 4 * out_format_.width * out_format_.height; // TODO
    GstBuffer *outbuf = gst_buffer_new_allocate(NULL, out_size, NULL);
    GstMapInfo out_info;
    gst_buffer_map(outbuf, &out_info, GST_MAP_WRITE);
    gst_buffer_copy_into(outbuf, inbuf, GST_BUFFER_COPY_METADATA, 0, 0);

    cv::Mat in(in_format_.height, in_format_.width, CV_8UC4, in_info.data);
    cv::Mat out(out_format_.height, out_format_.width, CV_8UC4, out_info.data);

    transform(in, out);
    gst_buffer_unmap(outbuf, &out_info);
    gst_buffer_unmap(inbuf, &in_info);
    gst_sample_unref(inframe);
    gst_app_src_push_buffer(src_, outbuf);
}
