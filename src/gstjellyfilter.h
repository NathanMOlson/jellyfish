#ifndef _GST_JELLY_FILTER_H_
#define _GST_JELLY_FILTER_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "jelly_filter.h"

G_BEGIN_DECLS

#define GST_TYPE_JELLY_FILTER   (gst_jelly_filter_get_type())
#define GST_JELLY_FILTER(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_JELLY_FILTER,GstJellyFilter))
#define GST_JELLY_FILTER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_JELLY_FILTER,GstJellyFilterClass))
#define GST_IS_JELLY_FILTER(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_JELLY_FILTER))
#define GST_IS_JELLY_FILTER_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_JELLY_FILTER))

typedef struct _GstJellyFilter GstJellyFilter;
typedef struct _GstJellyFilterClass GstJellyFilterClass;

struct _GstJellyFilter
{
  GstVideoFilter base_jellyfilter;
  JellyFilter jelly_filter;

};

struct _GstJellyFilterClass
{
  GstVideoFilterClass base_jellyfilter_class;
};

GType gst_jelly_filter_get_type (void);

G_END_DECLS

#endif
