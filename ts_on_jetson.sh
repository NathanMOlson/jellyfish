sudo modprobe nvidia-drm modeset=1
gst-launch-1.0 filesrc location=jelly_clip.ts ! decodebin ! nvvidconv ! video/x-raw ! videocrop bottom=296 right=576 ! queue ! jellyfilter ! queue ! videocrop bottom=32 left=32 right=32 top=32 ! videoflip method=rotate-180 ! videoscale ! video/x-raw,width=1920,height=1080 ! nvdrmvideosink --gst-plugin-path=/home/formapath/jellyfish/build
