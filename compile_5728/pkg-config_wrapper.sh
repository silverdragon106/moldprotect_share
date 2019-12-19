#!/bin/sh
PKG_CONFIG_SYSROOT_DIR=/opt/ti-processor-sdk-linux-am57xx-evm-05.00.00.15/linux-devkit/sysroots/armv7ahf-neon-linux-gnueabi
export PKG_CONFIG_SYSROOT_DIR
PKG_CONFIG_LIBDIR=/opt/ti-processor-sdk-linux-am57xx-evm-05.00.00.15/linux-devkit/sysroots/armv7ahf-neon-linux-gnueabi/usr/lib/pkgconfig:/opt/ti-processor-sdk-linux-am57xx-evm-05.00.00.15/linux-devkit/sysroots/armv7ahf-neon-linux-gnueabi/usr/share/pkgconfig:/opt/ti-processor-sdk-linux-am57xx-evm-05.00.00.15/linux-devkit/sysroots/armv7ahf-neon-linux-gnueabi/usr/lib/arm-linux-gnueabihf/pkgconfig
export PKG_CONFIG_LIBDIR
exec pkg-config "$@"
