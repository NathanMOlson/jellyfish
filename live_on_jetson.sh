sudo modprobe nvidia-drm modeset=1
URI=$(./youtube-dl --format "best" --get-url https://www.youtube.com/watch?v=OMlf71t2oV0)
gst-launch-1.0 uridecodebin uri=$URI ! nvvidconv ! video/x-raw ! videocrop bottom=296 right=576 ! queue ! jellyfilter ! queue ! videocrop bottom=32 left=32 right=32 top=32 ! videoscale ! video/x-raw,width=1920,height=1080 ! nvdrmvideosink --gst-plugin-path=/home/formapath/jellyfish/build
