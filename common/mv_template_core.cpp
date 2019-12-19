#include "mv_template_core.h"
#include "base.h"

CMVTemplateCore::CMVTemplateCore()
{
	initBuffer();
}

CMVTemplateCore::~CMVTemplateCore()
{
	freeBuffer();
}

void CMVTemplateCore::initBuffer()
{
	m_model_id = -1;
	m_model_rev = 0;
	memset(&m_created_time, 0, sizeof(DateTime));
	memset(&m_updated_time, 0, sizeof(DateTime));

	m_dev_version = "";
	m_filename = "";
	m_model_name = "";
	m_class_name = "";
	m_description = "";
		
	i32 i, j;
	for (i = 0; i < kMVStepCount; i++)
	{
		for (j = 0; j < PO_CAM_COUNT; j++)
		{
			m_snap_data[i][j].initBuffer();
		}
	}

	for (i = 0; i < PO_CAM_COUNT; i++)
	{
		m_auto_chk_data[i].initBuffer();
	}
}

void CMVTemplateCore::freeBuffer()
{
	i32 i, j;
	for (i = 0; i < kMVStepCount; i++)
	{
		for (j = 0; j < PO_CAM_COUNT; j++)
		{
			m_snap_data[i][j].freeBuffer();
		}
	}

	for (i = 0; i < PO_CAM_COUNT; i++)
	{
		m_auto_chk_data[i].freeBuffer();
	}
}

i32	CMVTemplateCore::memSize()
{
	i32 len = 0;
	len += sizeof(m_model_id);
	len += sizeof(m_model_rev);
	len += sizeof(m_created_time);
	len += sizeof(m_updated_time);
	len += CPOBase::getStringMemSize(m_dev_version);
	len += CPOBase::getStringMemSize(m_model_name);
	len += CPOBase::getStringMemSize(m_class_name);
	len += CPOBase::getStringMemSize(m_description);

	//model information
	i32 i, j;
	for (i = 0; i < kMVStepCount; i++)
	{
		for (j = 0; j < PO_CAM_COUNT; j++)
		{
			len += m_snap_data[i][j].memSize();
		}
	}

	for (i = 0; i < PO_CAM_COUNT; i++)
	{
		len += m_auto_chk_data[i].memSize();
	}
	return len;
}

bool CMVTemplateCore::memWrite(u8*& buffer_ptr, i32& buffer_size)
{
	//model thumbnail information
	CPOBase::memWrite(m_model_id, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_model_rev, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_created_time, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_updated_time, buffer_ptr, buffer_size);
	CPOBase::memWrite(buffer_ptr, buffer_size, m_dev_version);
	CPOBase::memWrite(buffer_ptr, buffer_size, m_model_name);
	CPOBase::memWrite(buffer_ptr, buffer_size, m_class_name);
	CPOBase::memWrite(buffer_ptr, buffer_size, m_description);

	//model information
	i32 i, j;
	for (i = 0; i < kMVStepCount; i++)
	{
		for (j = 0; j < PO_CAM_COUNT; j++)
		{
			m_snap_data[i][j].memWrite(buffer_ptr, buffer_size);
		}
	}

	for (i = 0; i < PO_CAM_COUNT; i++)
	{
		m_auto_chk_data[i].memWrite(buffer_ptr, buffer_size);
	}
	return true;
}

bool CMVTemplateCore::memRead(u8*& buffer_ptr, i32& buffer_size)
{
	//model thumbnail information
	CPOBase::memRead(buffer_ptr, buffer_size, m_model_name);
	CPOBase::memRead(buffer_ptr, buffer_size, m_class_name);
	CPOBase::memRead(buffer_ptr, buffer_size, m_description);

	//model information
	i32 i, j;
	for (i = 0; i < kMVStepCount; i++)
	{
		for (j = 0; j < PO_CAM_COUNT; j++)
		{
			m_snap_data[i][j].memRead(buffer_ptr, buffer_size);
		}
	}

	for (i = 0; i < PO_CAM_COUNT; i++)
	{
		m_auto_chk_data[i].memRead(buffer_ptr, buffer_size);
	}
	return true;
}

i32 CMVTemplateCore::memThumbSize()
{
	i32 len = 0;
	len += sizeof(m_model_id);
	len += sizeof(m_model_rev);
	len += sizeof(m_created_time);
	len += sizeof(m_updated_time);
	len += CPOBase::getStringMemSize(m_dev_version);
	len += CPOBase::getStringMemSize(m_model_name);
	len += CPOBase::getStringMemSize(m_class_name);
	len += CPOBase::getStringMemSize(m_description);
	return len;
}

void CMVTemplateCore::memThumbWrite(u8*& buffer_ptr, i32& buffer_size)
{
	CPOBase::memWrite(m_model_id, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_model_rev, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_created_time, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_updated_time, buffer_ptr, buffer_size);
	CPOBase::memWrite(buffer_ptr, buffer_size, m_dev_version);
	CPOBase::memWrite(buffer_ptr, buffer_size, m_model_name);
	CPOBase::memWrite(buffer_ptr, buffer_size, m_class_name);
	CPOBase::memWrite(buffer_ptr, buffer_size, m_description);
}
