#include "camera_focus.h"
#include "mv_engine.h"

CCameraFocus::CCameraFocus()
{
	m_is_inited = false;
	m_is_threadcancel = false;

	m_req_img_ready = false;
	m_req_cam_id = -1;
	m_resp_cam_id = -1;
	m_focus_result = 0;

	m_mv_engine_ptr = NULL;
}

CCameraFocus::~CCameraFocus()
{
	exitInstance();
}

bool CCameraFocus::initInstance(CMVEngine* mv_engine_ptr)
{
	if (!mv_engine_ptr)
	{
		return false;
	}

	exitInstance();
	if (!m_is_inited)
	{
		singlelog_lv0("CameraFocus Module InitInstance");

		m_is_inited = true;
		m_is_threadcancel = false;
		m_req_img_ready = false;
		m_resp_cam_id = -1;
		m_mv_engine_ptr = mv_engine_ptr;

		QThreadStart();
	}
	return true;
}

bool CCameraFocus::exitInstance()
{
	if (m_is_inited)
	{
		singlelog_lv0("CameraFocus Module ExitInstance");

		m_is_threadcancel = true;
		QThreadStop();

		m_resp_cam_id = -1;
		m_req_img_ready = false;
		m_is_inited = false;

		m_img_data[0].freeBuffer();
		m_img_data[1].freeBuffer();
	}
	return true;
}

void CCameraFocus::run()
{
	singlelog_lv0("The CameraFocus thread is");

	i32 cam_id;
	bool is_img_ready = false;

	while (!m_is_threadcancel)
	{
		is_img_ready = false;
		{
			exlock_guard(m_set_mutex);

			if (m_req_img_ready)
			{
				is_img_ready = true;
				cam_id = m_req_cam_id;
				m_img_data[1].copyImage(m_img_data);
				m_req_img_ready = false;
			}
		}

		if (is_img_ready)
		{
			f32 focus = m_mv_engine_ptr->getMeasuredFocus(m_img_data[1].img_ptr, m_img_data[1].w, m_img_data[1].h);
			{
				exlock_guard(m_get_mutex);
				m_focus_result = focus;
				m_resp_cam_id = cam_id;
			}
		}
		QThread::msleep(80);
	}
}

f32 CCameraFocus::getImageFocus(i32 cam_id)
{
	exlock_guard(m_get_mutex);
 	return (cam_id == m_resp_cam_id) ? m_focus_result : -1.0f;
}

void CCameraFocus::setFocusImage(i32 cam_id, ImageData& img_data)
{
	exlock_guard(m_set_mutex);
	if (!m_req_img_ready)
	{
		m_req_img_ready = true;
		m_req_cam_id = cam_id;
		m_img_data[0].copyImage(img_data);
	}
}
