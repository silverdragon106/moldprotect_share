QT += core network sql concurrent
QT -= gui

QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-sign-compare -Wno-ignored-qualifiers
QMAKE_CXXFLAGS += -Wno-narrowing -Wno-unused-but-set-variable -Wno-unused-variable -Wno-switch -Wno-unused-but-set-parameter
QMAKE_CXXFLAGS += -Wno-unknown-pragmas
CONFIG += c++11

TARGET = imvs2_nano_arm
CONFIG += console
CONFIG -= app_bundle
UNAME = $$system(uname -m)

TEMPLATE = app

SOURCES += \
	../../../PorBase/3rdsource/libserialport/list_ports_linux.cc \
	../../../PorBase/3rdsource/libserialport/list_ports_osx.cc \
    ../../../PorBase/3rdsource/libserialport/list_ports_win.cc \
    ../../../PorBase/3rdsource/libserialport/serial.cc \
    ../../../PorBase/3rdsource/libserialport/unix.cc \
    ../../../PorBase/3rdsource/libserialport/win.cc \
    ../../../PorBase/3rdsource/netlink/core.cpp \
    ../../../PorBase/3rdsource/netlink/smart_buffer.cpp \
    ../../../PorBase/3rdsource/netlink/socket.cpp \
    ../../../PorBase/3rdsource/netlink/socket_group.cpp \
    ../../../PorBase/3rdsource/netlink/util.cpp \
    ../../../PorBase/3rdsource/quazip/JlCompress.cpp \
    ../../../PorBase/3rdsource/quazip/qioapi.cpp \
    ../../../PorBase/3rdsource/quazip/quaadler32.cpp \
    ../../../PorBase/3rdsource/quazip/quacrc32.cpp \
    ../../../PorBase/3rdsource/quazip/quagzipfile.cpp \
    ../../../PorBase/3rdsource/quazip/quaziodevice.cpp \
    ../../../PorBase/3rdsource/quazip/quazip.cpp \
    ../../../PorBase/3rdsource/quazip/quazipdir.cpp \
    ../../../PorBase/3rdsource/quazip/quazipfile.cpp \
    ../../../PorBase/3rdsource/quazip/quazipfileinfo.cpp \
    ../../../PorBase/3rdsource/quazip/quazipnewinfo.cpp \
    ../../../PorBase/base/app/base_app.cpp \
    ../../../PorBase/base/app/base_app_ivs.cpp \
    ../../../PorBase/base/app/base_app_update.cpp \
    ../../../PorBase/base/app/base_disk.cpp \
    ../../../PorBase/base/database/mysql_manager.cpp \
    ../../../PorBase/base/io/modbus/modbus_com.cpp \
    ../../../PorBase/base/io/modbus/modbus_memory.cpp \
    ../../../PorBase/base/io/modbus/modbus_network.cpp \
    ../../../PorBase/base/io/modbus/modbus_packet.cpp \
	../../../PorBase/base/io/modbus/modbus_serial.cpp \
    ../../../PorBase/base/io/modbus/modbus_server.cpp \
    ../../../PorBase/base/io/usb_dongle.cpp \
    ../../../PorBase/base/logger/logger.cpp \
    ../../../PorBase/base/network/cmd_server.cpp \
    ../../../PorBase/base/network/connection.cpp \
    ../../../PorBase/base/network/ivs_server.cpp \
    ../../../PorBase/base/network/packet.cpp \
    ../../../PorBase/base/network/server.cpp \
    ../../../PorBase/base/network/udp_broadcast.cpp \
    ../../../PorBase/base/proc/cv_connected_component.cpp \
    ../../../PorBase/base/os/gamma_ramp.cpp \
    ../../../PorBase/base/os/linux_func.cpp \
    ../../../PorBase/base/os/qt_base.cpp \
    ../../../PorBase/base/os/window_func.cpp \
    ../../../PorBase/base/proc/camera/basler_pylon_camera.cpp \
    ../../../PorBase/base/proc/camera/mind_vision_camera.cpp \
    ../../../PorBase/base/proc/emulator/emu_samples.cpp \
    ../../../PorBase/base/proc/connected_components.cpp \
    ../../../PorBase/base/proc/contour_trace.cpp \
    ../../../PorBase/base/proc/fit_shape.cpp \
    ../../../PorBase/base/proc/histogram.cpp \
    ../../../PorBase/base/proc/image_proc.cpp \
    ../../../PorBase/base/proc/memory_pool.cpp \
    ../../../PorBase/base/streaming/base_encoder.cpp \
    ../../../PorBase/base/streaming/ipc_manager.cpp \
    ../../../PorBase/base/struct/camera_calib.cpp \
    ../../../PorBase/base/struct/camera_setting.cpp \
    ../../../PorBase/base/struct/run_table.cpp \
    ../../../PorBase/base/base.cpp \
    ../../../PorBase/base/struct.cpp \
    ../../../PorBase/base/lock_guide.cpp \
    ../../../PorBase/3rdsource/quazip/unzip.c \
    ../../../PorBase/3rdsource/quazip/zip.c \
    ../source/camera_catpure/camera_capture.cpp \
    ../source/camera_catpure/camera_focus.cpp \
    ../source/camera_catpure/capture_stream.cpp \
    ../source/manager/checked_step.cpp \
    ../source/manager/mv_base.cpp \
    ../source/manager/mv_data.cpp \
    ../source/manager/mv_db_client.cpp \
    ../source/manager/mv_db_manager.cpp \
    ../source/manager/mv_disk.cpp \
    ../source/manager/mv_io_module.cpp \
    ../source/manager/mv_struct_int.cpp \
    ../source/manager/mv_struct_io.cpp \
    ../source/manager/mv_template.cpp \
    ../source/manager/template_manager.cpp \
    ../source/mold_engine/auto_check_region.cpp \
    ../source/mold_engine/mv_engine.cpp \
    ../source/mold_engine/roi_processor.cpp \
    ../source/mold_engine/ssim_processor.cpp \
    ../source/main.cpp \
    ../source/mv_application.cpp \
    ../source/setting_update.cpp \
    ../source/test/test_code.cpp \
     ../../../PorBase/base/proc/cryptograph/md5.cpp \
    ../../../PorBase/base/proc/cryptograph/sha512.cpp \
    ../../../PorBase/base/struct/image.cpp \
    ../../../PorBase/3rdsource/simpleini/convert_utf.cpp \
    ../common/mv_struct.cpp \
    ../common/mv_template_core.cpp \
    ../../../PorBase/base/proc/img_proc/threshold.cpp \
    ../../../PorBase/base/proc/camera/emulator_camera.cpp \
    ../source/openvx/mv_vx_kernels.cpp \
    ../source/openvx/mv_graph_pool.cpp \
    ../source/openvx/kernels/vx_roi_region_kernel.cpp \
    ../source/openvx/kernels/vx_roi_check_kernel.cpp \
    ../source/openvx/kernels/vx_motion_estimation_kernel.cpp \
    ../../../PorBase/base/performance/openvx_pool/ovx_resource_pool.cpp \
    ../../../PorBase/base/performance/openvx_pool/ovx_object.cpp \
    ../../../PorBase/base/performance/openvx_pool/ovx_node.cpp \
    ../../../PorBase/base/performance/openvx_pool/ovx_graph_pool.cpp \
    ../../../PorBase/base/performance/openvx_pool/ovx_graph.cpp \
    ../../../PorBase/base/performance/openvx_pool/ovx_context.cpp \
    ../../../PorBase/base/performance/openvx_pool/ovx_base.cpp \ 
    ../../../PorBase/base/performance/vx_kernels/vx_filter.cpp \
    ../../../PorBase/base/performance/vx_kernels/vx_api_filter.cpp \
    ../../../PorBase/base/performance/vx_kernels/vx_run_table.cpp \
    ../../../PorBase/base/performance/vx_kernels/vx_kernels.cpp \
    ../../../PorBase/base/performance/vx_kernels/vx_arithmetic.cpp \
    ../../../PorBase/base/performance/vx_kernels/vx_api_image_proc.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += ./config.h \
    ../../../PorBase/3rdsource/libserialport/serial.h \
    ../../../PorBase/3rdsource/libserialport/unix.h \
    ../../../PorBase/3rdsource/libserialport/v8stdint.h \
    ../../../PorBase/3rdsource/libserialport/win.h \
    ../../../PorBase/3rdsource/netlink/config.h \
    ../../../PorBase/3rdsource/netlink/core.h \
    ../../../PorBase/3rdsource/netlink/exception.h \
    ../../../PorBase/3rdsource/netlink/netlink.h \
    ../../../PorBase/3rdsource/netlink/release_manager.h \
    ../../../PorBase/3rdsource/netlink/release_manager.inline.h \
    ../../../PorBase/3rdsource/netlink/smart_buffer.h \
    ../../../PorBase/3rdsource/netlink/smart_buffer.inline.h \
    ../../../PorBase/3rdsource/netlink/socket.h \
    ../../../PorBase/3rdsource/netlink/socket.inline.h \
    ../../../PorBase/3rdsource/netlink/socket_group.h \
    ../../../PorBase/3rdsource/netlink/socket_group.inline.h \
    ../../../PorBase/3rdsource/netlink/util.h \
    ../../../PorBase/3rdsource/netlink/util.inline.h \
    ../../../PorBase/3rdsource/quazip/crypt.h \
    ../../../PorBase/3rdsource/quazip/ioapi.h \
    ../../../PorBase/3rdsource/quazip/JlCompress.h \
    ../../../PorBase/3rdsource/quazip/quaadler32.h \
    ../../../PorBase/3rdsource/quazip/quachecksum32.h \
    ../../../PorBase/3rdsource/quazip/quacrc32.h \
    ../../../PorBase/3rdsource/quazip/quagzipfile.h \
    ../../../PorBase/3rdsource/quazip/quaziodevice.h \
    ../../../PorBase/3rdsource/quazip/quazip.h \
    ../../../PorBase/3rdsource/quazip/quazip_global.h \
    ../../../PorBase/3rdsource/quazip/quazipdir.h \
    ../../../PorBase/3rdsource/quazip/quazipfile.h \
    ../../../PorBase/3rdsource/quazip/quazipfileinfo.h \
    ../../../PorBase/3rdsource/quazip/quazipnewinfo.h \
    ../../../PorBase/3rdsource/quazip/unzip.h \
    ../../../PorBase/3rdsource/quazip/zip.h \
    ../../../PorBase/base/app/base_app.h \
    ../../../PorBase/base/app/base_app_ivs.h \
    ../../../PorBase/base/app/base_app_update.h \
    ../../../PorBase/base/app/base_disk.h \
    ../../../PorBase/base/database/mysql_manager.h \
    ../../../PorBase/base/io/modbus/modbus_com.h \
    ../../../PorBase/base/io/modbus/modbus_memory.h \
    ../../../PorBase/base/io/modbus/modbus_network.h \
    ../../../PorBase/base/io/modbus/modbus_packet.h \
	../../../PorBase/base/io/modbus/modbus_serial.h \
    ../../../PorBase/base/io/modbus/modbus_server.h \
    ../../../PorBase/base/io/usb_dongle.h \
    ../../../PorBase/base/logger/logger.h \
    ../../../PorBase/base/math/squareroots.h \
    ../../../PorBase/base/network/cmd_server.h \
    ../../../PorBase/base/network/connection.h \
    ../../../PorBase/base/network/ivs_server.h \
    ../../../PorBase/base/network/packet.h \
    ../../../PorBase/base/network/server.h \
    ../../../PorBase/base/network/udp_broadcast.h \
    ../../../PorBase/base/os/gamma_ramp.h \
    ../../../PorBase/base/os/linux_func.h \
    ../../../PorBase/base/os/qt_base.h \
    ../../../PorBase/base/os/window_func.h \
    ../../../PorBase/base/proc/camera/base_camera.h \
    ../../../PorBase/base/proc/camera/basler_pylon_camera.h \
    ../../../PorBase/base/proc/camera/mind_vision_camera.h \
    ../../../PorBase/base/proc/emulator/emu_samples.h \
    ../../../PorBase/base/proc/connected_components.h \
    ../../../PorBase/base/proc/contour_trace-inl.h \
    ../../../PorBase/base/proc/contour_trace.h \
    ../../../PorBase/base/proc/fit_shape.h \
    ../../../PorBase/base/proc/histogram-inl.h \
    ../../../PorBase/base/proc/histogram.h \
    ../../../PorBase/base/proc/image_proc-inl.h \
    ../../../PorBase/base/proc/image_proc.h \
    ../../../PorBase/base/proc/memory_pool.h \
    ../../../PorBase/base/streaming/base_encoder.h \
    ../../../PorBase/base/streaming/ipc_manager.h \
    ../../../PorBase/base/struct/camera_calib.h \
    ../../../PorBase/base/struct/camera_setting.h \
    ../../../PorBase/base/struct/rect.h \
    ../../../PorBase/base/struct/run_table.h \
    ../../../PorBase/base/struct/vector2d.h \
    ../../../PorBase/base/base.h \
    ../../../PorBase/base/define.h \
    ../../../PorBase/base/po_action.h \
    ../../../PorBase/base/po_error.h \
    ../../../PorBase/base/struct.h \
    ../../../PorBase/base/lock_guide.h \
    ../../../PorBase/base/types.h \
    ../../../PorBase/base/build_defs.h \
    ../source/camera_catpure/camera_capture.h \
    ../source/camera_catpure/camera_focus.h \
    ../source/camera_catpure/capture_stream.h \
    ../source/manager/checked_step.h \
    ../source/manager/mv_action.h \
    ../source/manager/mv_base.h \
    ../source/manager/mv_data.h \
    ../source/manager/mv_db_client.h \
    ../source/manager/mv_db_manager.h \
    ../source/manager/mv_define_int.h \
    ../source/manager/mv_disk.h \
    ../source/manager/mv_error.h \
    ../source/manager/mv_io_module.h \
    ../source/manager/mv_struct_int.h \
    ../source/manager/mv_struct_io.h \
    ../source/manager/mv_template.h \
    ../source/manager/template_manager.h \
    ../source/mold_engine/auto_check_region.h \
    ../source/mold_engine/mv_engine.h \
    ../source/mold_engine/roi_processor.h \
    ../source/mold_engine/ssim_processor.h \
    ../source/mv_application.h \
    ../source/test/test_code.h \
    ../../../PorBase/base/proc/cryptograph/md5.h \
    ../../../PorBase/base/proc/cryptograph/sha512.h \
    ../../../PorBase/base/struct/image.h \
	../../../PorBase/3rdsource/simpleini/simple_ini.h \
	../../MoldDeviceV1.6/embedded/imvs2_middle_layer/imvs2_middle_layer.h \
    ../../../PorBase/3rdsource/simpleini/convert_utf.h \
    ../common/mv_define.h \
    ../common/mv_struct.h \
    ../common/mv_template_core.h \
    ../source/manager/mv_define_int.h \
    log_config.h \
    vx_config.h \
    ../../../PorBase/base/proc/img_proc/threshold.h \
    ../../../PorBase/base/proc/camera/emulator_camera.h \
    ../source/openvx/mv_vx_kernel_types.h \
    ../source/openvx/mv_vx_kernels.h \
    ../source/openvx/mv_graph_pool.h \
    ../source/openvx/cuda/mv_nvx_image_proc.h \
    ../source/openvx/kernels/vx_roi_region_kernel.h \
    ../source/openvx/kernels/vx_roi_check_kernel.h \
    ../source/openvx/kernels/vx_motion_estimation_kernel.h \
    ../../../PorBase/base/performance/openvx_pool/ovx_resource_pool.h \
    ../../../PorBase/base/performance/openvx_pool/ovx_object.h \
    ../../../PorBase/base/performance/openvx_pool/ovx_node.h \
    ../../../PorBase/base/performance/openvx_pool/ovx_graph_pool.h \
    ../../../PorBase/base/performance/openvx_pool/ovx_graph.h \
    ../../../PorBase/base/performance/openvx_pool/ovx_context.h \
    ../../../PorBase/base/performance/openvx_pool/ovx_base.h \ 
    ../../../PorBase/base/performance/vx_kernels/vx_filter.h \
    ../../../PorBase/base/performance/vx_kernels/vx_run_table.h \
    ../../../PorBase/base/performance/vx_kernels/vx_kernel_types.h \
    ../../../PorBase/base/performance/vx_kernels/vx_kernels.h \
    ../../../PorBase/base/performance/vx_kernels/vx_arithmetic.h \
    ../../../PorBase/base/performance/vx_kernels/vx_api_image_proc.h \
    ../../../PorBase/base/performance/vx_kernels/vx_api_filter.h

DISTFILES += \
    ../../../PorBase/3rdsource/netlink/exception.code.inc
	
INCLUDEPATH += ../compile_option \
	../../../PorBase/3rdsource \
	../../../PorBase/base \
	../source \
	../common \
	../source/manager \
	../source/mold_engine \
	../source/camera_catpure \
	../source/test \
	../../MoldDeviceV1.6/embedded/imvs2_middle_layer

CONFIG += link_pkgconfig
PKGCONFIG += gstreamer-app-1.0
PKGCONFIG += opencv
LIBS += -lz -ludev #-limvs2_ml #TODO

#support Basler Pylon5 CameraSDK
#INCLUDEPATH += $$(SDK_PATH_TARGET)/opt/pylon5/include
#LIBS += -L$$(SDK_PATH_TARGET)/opt/pylon5/lib -lpylonbase -lpylonutility -lGenApi_gcc_v3_1_Basler_pylon -lGCBase_gcc_v3_1_Basler_pylon \
#        -lLog_gcc_v3_1_Basler_pylon -lMathParser_gcc_v3_1_Basler_pylon -lNodeMapData_gcc_v3_1_Basler_pylon -lXmlParser_gcc_v3_1_Basler_pylon

#support MindVision CameraSDK
INCLUDEPATH += $$(SDK_PATH_TARGET)/opt/mvsdk/include
LIBS += -L$$(SDK_PATH_TARGET)/opt/mvsdk/lib/arm -lMVSDK

message(Current Project Option is $$CONFIG)
CONFIG(debug, debug|release) {
        message(debug compile...)
        DEFINES += DEBUG
        DEFINES += POR_DEBUG
        DESTDIR = ../bin
        OBJECTS_DIR = ../bin/compile/tmp_desk_5728/debug_5728/obj
        MOC_DIR = ../bin/compile/tmp_desk_5728/debug_5728/moc
        RCC_DIR = ../bin/compile/tmp_desk_5728/debug_5728/rcc
        UI_DIR = ../bin/compile/tmp_desk_5728/debug_5728/ui
}

CONFIG(release, debug|release) {
        message(release compile...)
        DEFINES += RELEASE
        DEFINES += POR_RELEASE
        DESTDIR = ../bin
        OBJECTS_DIR = ../bin/compile/tmp_desk_5728/release_5728/obj
        MOC_DIR = ../bin/compile/tmp_desk_5728/release_5728/moc
        RCC_DIR = ../bin/compile/tmp_desk_5728/release_5728/rcc
        UI_DIR = ../bin/compile/tmp_desk_5728/release_5728/ui
}

RESOURCES += \
    ../source/MoldVisionCAM.qrc

target.path += /opt/nano/imvs2
INSTALLS += target

###################################### CUDA
contains(UNAME, x86_64) { #cross compiler
	CUDA_INCLUDE_PATH += /opt/nano/rootfs/usr/include
	CUDA_INCLUDE_PATH += /opt/nano/rootfs/usr/include/aarch64-linux-gnu/
	CUDA_LIBS = -L/usr/local/cuda-10.0/targets/aarch64-linux/lib/ -lcudart \
	-L/opt/nano/rootfs/usr/lib/gcc/aarch64-linux-gnu/7 -lstdc++a #must be add this line in ubuntu18.04
	CUDA_GCC_SPEC = aarch64-linux-gnu-g++
}
contains(UNAME, aarch64) { #host compiler
	CUDA_INCLUDE_PATH += /usr/include
	CUDA_INCLUDE_PATH += /usr/include/aarch64-linux-gnu/
	CUDA_PKGCONFIG = cudart-10.0
	CUDA_GCC_SPEC = g++
}

CONFIG += link_pkgconfig
PKGCONFIG += visionworks
PKGCONFIG += $$CUDA_PKGCONFIG
LIBS += $$CUDA_LIBS

#set cuda sources
CUDA_SOURCES_RM += ../source/openvx/cuda/*.cu \
			../../../PorBase/base/performance/cuda_kernels/*.cu

## Path to cuda SDK install
CUDA_DIR_RM = /usr/local/cuda-10.0

## join the includes in a line
INCLUDEPATH += . \
INCLUDEPATH += $$CUDA_DIR_RM/targets/aarch64-linux/include
INCLUDEPATH += $$CUDA_INCLUDE_PATH
CUDA_INC_RM = $$join(INCLUDEPATH,' -I','-I',' ')

## nvcc flags (ptxas option verbose is always useful)
NVCCFLAGS = -std=c++11 --compiler-options '-fPIC' #-fno-strict-aliasing -use_fast_math --ptxas-options=-v

##prepare intermediat cuda compiler
cudaIntr_RM.input = CUDA_SOURCES_RM
cudaIntr_RM.output = ${OBJECTS_DIR}${QMAKE_FILE_BASE}.o
cudaIntr_RM.commands = $$CUDA_DIR_RM/bin/nvcc -ccbin $$CUDA_GCC_SPEC -m64 -g -gencode arch=compute_62,code=sm_62 \
-gencode arch=compute_62,code=compute_62 -dc $$NVCCFLAGS $$CUDA_INC_RM $$LIBS ${QMAKE_FILE_NAME} -o ${QMAKE_FILE_OUT}

#Set our variable out. These obj files need to be used to create the link obj file
#and used in our final gcc compilation
cudaIntr_RM.variable_out = CUDA_OBJ
cudaIntr_RM.variable_out += OBJECTS
cudaIntr_RM.clean = cudaIntr_RMObj/*.o

# Prepare the linking compiler step
QMAKE_EXTRA_UNIX_COMPILERS += cudaIntr_RM
Cuda_RM.input = CUDA_OBJ
Cuda_RM.output = ${QMAKE_FILE_BASE}_link.o
Cuda_RM.commands = $$CUDA_DIR_RM/bin/nvcc -ccbin $$CUDA_GCC_SPEC -m64 -g -G -gencode arch=compute_62,code=sm_62 \
-gencode arch=compute_62,code=compute_62 -dlink ${QMAKE_FILE_NAME} -o ${QMAKE_FILE_OUT}
Cuda_RM.dependency_type = TYPE_C
Cuda_RM.depend_command = $$CUDA_DIR_RM/bin/nvcc -ccbin $$CUDA_GCC_SPEC -g -G -M $$CUDA_INC_RM $$NVCCFLAGS ${QMAKE_FILE_NAME}

# Tell Qt that we want add more stuff to the Makefile
QMAKE_EXTRA_UNIX_COMPILERS += Cuda_RM
