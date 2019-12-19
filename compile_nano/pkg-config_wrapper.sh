#!/bin/sh
PKG_CONFIG_SYSROOT_DIR=/opt/nano/rootfs
export PKG_CONFIG_SYSROOT_DIR
PKG_CONFIG_LIBDIR=/opt/nano/rootfs/usr/lib/pkgconfig:/opt/nano/rootfs/usr/share/pkgconfig:/opt/nano/rootfs/usr/lib/aarch64-linux-gnu/pkgconfig
export PKG_CONFIG_LIBDIR
exec pkg-config "$@"
