#include "gstjellyfilter.h"
#include <gst/gstelementfactory.h>
#include <iostream>
#include "multithreaded_video_filter.h"

using namespace std;

int main(int argc, char *argv[])
{
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    cv::setNumThreads(0);

    /* Build the pipeline */
    GstElement *source = gst_element_factory_make("videotestsrc", "source");
    //gst_element_register(NULL, "jellyfilter", GST_RANK_NONE, GST_TYPE_JELLY_FILTER);
    MultiThreadedVideoFilter filter("jellyfilter", VideoFormat{"RGBA", 1920, 1280}, VideoFormat{"RGBA", 1920, 1280});
    GstElement *convert = gst_element_factory_make("videoconvert", "convert");
    GstElement *display = gst_element_factory_make("ximagesink", "display");
    GstElement *pipeline = gst_pipeline_new("jelly-pipeline");
    GstElement *pipeline2 = gst_pipeline_new("jelly-pipeline2");
    if (!source)
    {
        cout<<"Source could be created, exiting"<<endl;
        return -1;
    }
    if (!convert)
    {
        cout<<"Convert could be created, exiting"<<endl;
        return -1;
    }
    if (!display)
    {
        cout<<"Display could be created, exiting"<<endl;
        return -1;
    }
    if (!pipeline)
    {
        cout<<"Pipeline could be created, exiting"<<endl;
        return -1;
    }

    gst_bin_add_many(GST_BIN(pipeline), source, filter.sink(), NULL);
    gst_bin_add_many(GST_BIN(pipeline2), filter.src(), convert, display, NULL);
    if (gst_element_link(source, filter.sink()) != TRUE)
    {
        cout<<"Source and filter elements could not be linked, exiting"<<endl;
        gst_object_unref(pipeline);
        return -1;
    }
    if (gst_element_link(filter.src(), convert) != TRUE)
    {
        cout<<"Filter and convert elements could not be linked, exiting"<<endl;
        gst_object_unref(pipeline2);
        return -1;
    }
    if (gst_element_link(convert, display) != TRUE)
    {
        cout<<"Convert and display elements could not be linked, exiting"<<endl;
        gst_object_unref(pipeline2);
        return -1;
    }

    /* Start playing */
    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        cout<<"Unable to set the pipeline to the playing state, exiting"<<endl;
        gst_object_unref(pipeline);
        return -1;
    }
    if (gst_element_set_state(pipeline2, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        cout<<"Unable to set the pipeline2 to the playing state, exiting"<<endl;
        gst_object_unref(pipeline);
        return -1;
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
    cout<<"Playing"<<endl;

    /* Wait until error or EOS */
    GstBus *bus = gst_element_get_bus(pipeline2);
    GstMessage *msg =
        gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                   (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR)
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        cout<<"A gstreamer error occurred in "<< GST_MESSAGE_SRC_NAME(msg)<<endl;

        GError *err;
        gchar *debug_info;
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_clear_error(&err);
        g_free(debug_info);
    }

    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_object_unref(pipeline);

    cout<<"Finished!"<<endl;

    return 0;
}