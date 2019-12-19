#pragma once

#include "mv_define_int.h"
#include "streaming/base_encoder.h"

#ifdef POR_WITH_STREAM

class CCaptureStream : public CBaseEncoder
{
	Q_OBJECT

public:
	CCaptureStream();
	~CCaptureStream();
	
	virtual void*		onEncodedFrame(u8* data_ptr, i32 size, i64 pts, ImageData* img_data_ptr);
	virtual void		onSendFrame(void* send_void_ptr);
};

#endif
