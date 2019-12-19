#pragma once
#include "mv_struct.h"

#if defined(POR_DEVICE)
	#include "mv_struct_int.h"
	#include "mv_data.h"
#endif

class CMVTemplateCore
{
public:
	CMVTemplateCore();
	~CMVTemplateCore();

	void						initBuffer();
	void						freeBuffer();

	i32							memSize();
	bool						memWrite(u8*& buffer_ptr, i32& buffer_size);
	bool						memRead(u8*& buffer_ptr, i32& buffer_size);

	i32							memThumbSize();
	void						memThumbWrite(u8*& buffer_ptr, i32& buffer_size);

public:
	//thumbnail information for model
	i64							m_model_id;
	i32							m_model_rev;
    DateTime                    m_created_time;
    DateTime                    m_updated_time;

	postring					m_dev_version;
	postring					m_filename;
	postring					m_model_name;
	postring					m_class_name;
	postring					m_description;

	//model main information
#if defined(POR_DEVICE)
	CAutoCheckData				m_auto_chk_data[PO_CAM_COUNT];
	CMoldSnapData				m_snap_data[kMVStepCount][PO_CAM_COUNT];
#else
	CAutoCheckBase				m_auto_chk_data[PO_CAM_COUNT];
	CMoldSnapBase				m_snap_data[kMVStepCount][PO_CAM_COUNT];
#endif
};
