#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstjellyfilter.h"

GST_DEBUG_CATEGORY_STATIC (gst_jelly_filter_debug_category);
#define GST_CAT_DEFAULT gst_jelly_filter_debug_category

/* prototypes */


static void gst_jelly_filter_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_jelly_filter_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_jelly_filter_dispose (GObject * object);
static void gst_jelly_filter_finalize (GObject * object);

static gboolean gst_jelly_filter_start (GstBaseTransform * trans);
static gboolean gst_jelly_filter_stop (GstBaseTransform * trans);
static GstFlowReturn gst_jelly_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);
static gboolean gst_jelly_filter_set_info (GstVideoFilter * filter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
static GstFlowReturn gst_jelly_filter_transform_frame (GstVideoFilter * filter,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
// static GstFlowReturn gst_jelly_filter_transform_frame_ip (GstVideoFilter * filter,
//     GstVideoFrame * frame);

enum
{
  PROP_0
};

/* pad templates */

#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ RGBA }")

#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ RGBA }")


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstJellyFilter, gst_jelly_filter, GST_TYPE_VIDEO_FILTER,
  GST_DEBUG_CATEGORY_INIT (gst_jelly_filter_debug_category, "jellyfilter", 0,
  "debug category for jellyfilter element"));

static void
gst_jelly_filter_class_init (GstJellyFilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
        gst_caps_from_string (VIDEO_SRC_CAPS)));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
        gst_caps_from_string (VIDEO_SINK_CAPS)));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "Jellyfish filter", "Generic", "Artistic transform for jellyfish images",
      "Nathan Olson <email>");

  gobject_class->set_property = gst_jelly_filter_set_property;
  gobject_class->get_property = gst_jelly_filter_get_property;
  gobject_class->dispose = gst_jelly_filter_dispose;
  gobject_class->finalize = gst_jelly_filter_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_jelly_filter_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_jelly_filter_stop);
  video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_jelly_filter_set_info);
  video_filter_class->transform_frame = GST_DEBUG_FUNCPTR (gst_jelly_filter_transform_frame);
  // video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_jelly_filter_transform_frame_ip);

}

static void
gst_jelly_filter_init (GstJellyFilter *jellyfilter)
{
  GstBaseTransform *base_transform = GST_BASE_TRANSFORM (jellyfilter);
  gst_pad_set_chain_function (base_transform->sinkpad,
      GST_DEBUG_FUNCPTR (gst_jelly_chain));
}

void
gst_jelly_filter_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstJellyFilter *jellyfilter = GST_JELLY_FILTER (object);

  GST_DEBUG_OBJECT (jellyfilter, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_jelly_filter_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstJellyFilter *jellyfilter = GST_JELLY_FILTER (object);

  GST_DEBUG_OBJECT (jellyfilter, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_jelly_filter_dispose (GObject * object)
{
  GstJellyFilter *jellyfilter = GST_JELLY_FILTER (object);

  GST_DEBUG_OBJECT (jellyfilter, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_jelly_filter_parent_class)->dispose (object);
}

void
gst_jelly_filter_finalize (GObject * object)
{
  GstJellyFilter *jellyfilter = GST_JELLY_FILTER (object);

  GST_DEBUG_OBJECT (jellyfilter, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_jelly_filter_parent_class)->finalize (object);
}

static gboolean
gst_jelly_filter_start (GstBaseTransform * trans)
{
  GstJellyFilter *jellyfilter = GST_JELLY_FILTER (trans);

  GST_DEBUG_OBJECT (jellyfilter, "start");

  return TRUE;
}

static gboolean
gst_jelly_filter_stop (GstBaseTransform * trans)
{
  GstJellyFilter *jellyfilter = GST_JELLY_FILTER (trans);

  GST_DEBUG_OBJECT (jellyfilter, "stop");

  return TRUE;
}

static gboolean
gst_jelly_filter_set_info (GstVideoFilter * filter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstJellyFilter *jellyfilter = GST_JELLY_FILTER (filter);

  GST_DEBUG_OBJECT (jellyfilter, "set_info");

  return TRUE;
}

static GstFlowReturn
gst_jelly_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (parent);
  GstBaseTransformClass *bclass = GST_BASE_TRANSFORM_GET_CLASS (trans);
  GstBaseTransformPrivate *priv = trans->priv;
  GstFlowReturn ret;
  GstClockTime position = GST_CLOCK_TIME_NONE;
  GstClockTime timestamp, duration;
  GstBuffer *outbuf = NULL;

  timestamp = GST_BUFFER_TIMESTAMP (buffer);
  duration = GST_BUFFER_DURATION (buffer);

  /* calculate end position of the incoming buffer */
  if (timestamp != GST_CLOCK_TIME_NONE) {
    if (duration != GST_CLOCK_TIME_NONE)
      position = timestamp + duration;
    else
      position = timestamp;
  }

  if (bclass->before_transform)
    bclass->before_transform (trans, buffer);

  GST_DEBUG_OBJECT (trans, "calling prepare buffer");
  ret = bclass->prepare_output_buffer (trans, buffer, &outbuf);

  if (ret != GST_FLOW_OK || outbuf == NULL)
  {
    gst_buffer_unref (buffer);
    outbuf = NULL;
    GST_WARNING_OBJECT (trans, "could not get buffer from pool: %s",
        gst_flow_get_name (ret));
    return ret;
  }

  GST_DEBUG_OBJECT (trans, "using allocated buffer in %p, out %p", buffer, outbuf);

  GST_DEBUG_OBJECT (trans, "doing non-inplace transform");

  ret = bclass->transform (trans, buffer, outbuf);
  
  if (outbuf != buffer)
    gst_buffer_unref (buffer);

  ret = gst_pad_push (trans->srcpad, outbuf);

  return ret;
}

/* transform */
static GstFlowReturn
gst_jelly_filter_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe,
    GstVideoFrame * outframe)
{
  GstJellyFilter *jellyfilter = GST_JELLY_FILTER (filter);

  GST_DEBUG_OBJECT (jellyfilter, "transform_frame");
  jellyfilter->jelly_filter.transform(inframe, outframe);

  return GST_FLOW_OK;
}

// static GstFlowReturn
// gst_jelly_filter_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
// {
//   GstJellyFilter *jellyfilter = GST_JELLY_FILTER (filter);

//   GST_DEBUG_OBJECT (jellyfilter, "transform_frame_ip");

//   return GST_FLOW_OK;
// }

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "jellyfilter", GST_RANK_NONE,
      GST_TYPE_JELLY_FILTER);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    jellyfilter,
    "FIXME plugin description",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

