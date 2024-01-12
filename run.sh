#!/bin/bash
export MOD=mmiyoo
export CROSS=/opt/mmiyoo/bin/arm-linux-gnueabihf-
export CC=${CROSS}gcc
export AR=${CROSS}ar
export AS=${CROSS}as
export LD=${CROSS}ld
export CXX=${CROSS}g++
export HOST=arm-linux
export CFLAGS="-g -Wno-undef -marm -mtune=cortex-a7 -mfpu=neon-vfpv4  -march=armv7ve+simd -mfloat-abi=hard -ffunction-sections -fdata-sections"
export CXXFLAGS="-s -O3 -fPIC -pthread"

echo "Building SDL2 for Miyoo Mini / Miyoo Mini Plus"
if [ ! -z "$1" ] && [ "$1" == "config" ]; then
    ./autogen.sh
    ./configure --disable-joystick-virtual --disable-power --disable-alsa --disable-diskaudio --disable-video-x11 --disable-video-wayland --disable-video-kmsdrm --disable-video-vulkan --disable-dbus --disable-ime --disable-fcitx --disable-hidapi --disable-pulseaudio --disable-sndio --disable-libudev --disable-jack --disable-video-opengl --disable-video-opengles --disable-video-opengles2 --disable-oss --disable-dummyaudio --disable-video-dummy --host=${HOST}
fi

make -j4 V=99