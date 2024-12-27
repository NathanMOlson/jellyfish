#include "jelly_filter.h"
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace std;

void JellyFilter::transform(cv::Mat &in, cv::Mat &out, GstClockTime pts, GstClockTime duration)
{
    cv::Mat hpf;
    cv::Mat blur;
    cv::GaussianBlur(in, blur, cv::Size(51, 51), 25);

    cv::subtract(in, blur, hpf, cv::noArray(), CV_16S);

    cv::Mat filtered;

    cv::Mat t = cv::Mat::zeros(3, 3, CV_16S);
    t.at<int16_t>(0,2) = -2;
    t.at<int16_t>(1,1) = -2;
    t.at<int16_t>(0,1) = -0.6;
    t.at<int16_t>(2,1) = 0.6;
    t.at<int16_t>(2,0) = 2;

    cv::transform(hpf, filtered, t);

    filtered.convertTo(out, CV_8UC3);
}

void JellyFilter::transform(GstVideoFrame *inframe, GstVideoFrame *outframe)
{
    cv::Mat in(inframe->info.height, inframe->info.width, CV_8UC3, inframe->data[0]);
    cv::Mat out(outframe->info.height, outframe->info.width, CV_8UC3, outframe->data[0]);
    transform(in, out, inframe->buffer->pts, inframe->buffer->duration);
}