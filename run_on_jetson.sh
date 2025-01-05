sudo modprobe nvidia-drm modeset=1
DISPLAY=:0 gst-launch-1.0 filesrc location=jelly.mp4 ! qtdemux ! h264parse ! nvv4l2decoder ! queue ! nvvidconv ! video/x-raw ! videocrop right=70 ! queue ! jellyfilter ! queue ! videoscale ! video/x-raw,width=1920,height=1080 ! nvdrmvideosink --gst-plugin-path=/home/formapath/jellyfish/build
