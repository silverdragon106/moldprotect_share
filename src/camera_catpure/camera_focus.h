#pragma once

#include <QMutex>
#include <QThread>
#include "mv_base.h"

class CMVEngine;
class CCameraCapture;
class CCameraFocus : public QThread
{
	Q_OBJECT

public:
	CCameraFocus();
	~CCameraFocus();

	bool					initInstance(CMVEngine* mv_engine_ptr);
	bool					exitInstance();

	void					setFocusImage(i32 cam_id, ImageData& img_data);
	f32						getImageFocus(i32 cam_id);

protected:
	void					run() Q_DECL_OVERRIDE;

public:
	bool					m_is_inited;
	std::atomic<bool>		m_is_threadcancel;

	bool					m_req_img_ready;
	i32						m_req_cam_id;
	i32						m_resp_cam_id;
	f32						m_focus_result;

	CMVEngine*				m_mv_engine_ptr;
	ImageData				m_img_data[2];

	POMutex					m_set_mutex;
	POMutex					m_get_mutex;
};

