#include "capture_stream.h"
#include "base.h"
#include "proc/image_proc.h"
#include "mv_application.h"

#ifdef POR_WITH_STREAM

CCaptureStream::CCaptureStream()
{

}

CCaptureStream::~CCaptureStream()
{

}

void* CCaptureStream::onEncodedFrame(u8* data_ptr, i32 size, i64 pts, ImageData* img_data_ptr)
{
	if (size <= 0 || !data_ptr || !img_data_ptr)
	{
		return NULL;
	}

	i32 len = 0;
	len += sizeof(img_data_ptr->reserved); //camera channel
	len += sizeof(img_data_ptr->w);
	len += sizeof(img_data_ptr->h);
	len += sizeof(img_data_ptr->channel);
	len += sizeof(img_data_ptr->is_snaped);
	len += sizeof(img_data_ptr->time_stamp);
	len += sizeof(pts);
	len += sizeof(size);
	len += size;

	u8* buffer_ptr;
	i32 buffer_size = len;
	Packet* paket_ptr = new Packet(kMVCmdCamImage, kPOPacketRespOK);
	paket_ptr->setSubCmd(kMVSubTypeImageStream);
	paket_ptr->allocateBuffer(len, buffer_ptr);

	CPOBase::memWrite(img_data_ptr->reserved, buffer_ptr, buffer_size); //camera channel
	CPOBase::memWrite(img_data_ptr->w, buffer_ptr, buffer_size);
	CPOBase::memWrite(img_data_ptr->h, buffer_ptr, buffer_size);
	CPOBase::memWrite(img_data_ptr->channel, buffer_ptr, buffer_size);
	CPOBase::memWrite(img_data_ptr->is_snaped, buffer_ptr, buffer_size);
	CPOBase::memWrite(img_data_ptr->time_stamp, buffer_ptr, buffer_size);
	CPOBase::memWrite(pts, buffer_ptr, buffer_size);
	CPOBase::memWrite(size, buffer_ptr, buffer_size);
	CPOBase::memWrite(data_ptr, size, buffer_ptr, buffer_size);

	if (!g_main_app_ptr->checkPacket(paket_ptr, buffer_ptr))
	{
		printlog_lv0("[OVERWRITE]---------- is detected in Encode Frame.");
		POSAFE_DELETE(paket_ptr);
		return NULL;
	}
	return paket_ptr;
}

void CCaptureStream::onSendFrame(void* packet_ptr)
{
	if (packet_ptr)
	{
		g_main_app_ptr->sendPacketToNet((Packet*)packet_ptr);
	}
}

#endif