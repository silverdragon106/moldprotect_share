#include "camera_capture.h"
#include "mv_base.h"
#include "mv_engine.h"
#include "mv_application.h"
#include "logger/logger.h"
#include "proc/camera/basler_pylon_camera.h"
#include "proc/camera/mind_vision_camera.h"
#include "proc/camera/emulator_camera.h"
#include "proc/emulator/emu_samples.h"
#include "proc/image_proc.h"
#include "log_config.h"

#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

//////////////////////////////////////////////////////////////////////////
struct CapFrame
{
	i32					mode;
	ImageData*			img_data_ptr;
	CCameraCapture*		cam_capture_ptr;

public:
	CapFrame(i32 mode, ImageData* img_data_ptr, CCameraCapture* cam_capture_ptr)
	{
		this->mode = mode;
		this->img_data_ptr = img_data_ptr;
		this->cam_capture_ptr = cam_capture_ptr;
	}
};
typedef std::vector<CapFrame> CapFrameVector;

void capture_frame_process(CapFrame& capture_frame)
{
	CCameraCapture* cam_capture_ptr = capture_frame.cam_capture_ptr;
	if (cam_capture_ptr)
	{
		cam_capture_ptr->processSnapImage(capture_frame.mode, capture_frame.img_data_ptr);
	}
}
//////////////////////////////////////////////////////////////////////////

CCameraManager::CCameraManager()
{
	m_is_inited = false;
	m_is_thread_cancel = false;
	m_cam_set_ptr = NULL;
	m_ipc_stream_ptr = NULL;
	m_captured_size = 0;
}

CCameraManager::~CCameraManager()
{
	exitInstance();
}

bool CCameraManager::initInstance(CameraSet* camera_set_ptr, CIPCManager* ipc_stream_ptr, CMVEngine* mv_engine_ptr)
{
    ERR_PREPARE(0);
    ERR_PREPARE(1);

	if (!m_is_inited)
	{
		singlelog_lv0("The Camera Capture Module InitInstance");

		m_cam_set_ptr = camera_set_ptr;
		m_ipc_stream_ptr = ipc_stream_ptr;
		m_captured_size = 0;

		if (!m_focus_engine.initInstance(mv_engine_ptr))
		{
			printlog_lv1("CameraFocus Module InitInstance is Failed.");
			return false;
		}
	
#ifdef POR_WITH_CAMERA
		i32 avail_cam = camera_set_ptr->getAvailableCamType();

		/* MindVision카메라드라이버를 초기화한다. */
		if (CPOBase::bitCheck(avail_cam, kPOCamMindVision))
		{
#  ifdef POR_SUPPORT_MINDVISION
			if (!checkError(CMindVisionCamera::initSDK()))
			{
				CPOBase::bitRemove(avail_cam, kPOCamMindVision);
				printlog_lv1("MindVision SDKInit is Failed.");
			}
#  else
			CPOBase::bitRemove(avail_cam, kPOCamMindVision);
			printlog_lv1("MindVision SDK Unsupported.");
#  endif
		}

		/* BaslerPylon카메라드라이버를 초기화한다. */
		if (CPOBase::bitCheck(avail_cam, kPOCamBaslerPylon))
		{
#  ifdef POR_SUPPORT_BASLER
			if (!checkError(CBaslerPylonCamera::initSDK()))
			{
                CPOBase::bitRemove(avail_cam, kPOCamBaslerPylon);
				printlog_lv1("BaslerPylon SDKInit is Failed.");
			}
#  else
			CPOBase::bitRemove(avail_cam, kPOCamBaslerPylon);
			printlog_lv1("BaslerPylon SDK Unsupported.");
# endif
		}

		camera_set_ptr->setAvailableCamType(avail_cam);
		if (refreshCameraList() <= 0)
		{
			printlog_lv1("No Camera Connected!....");
		}
		
		m_is_thread_cancel = false;
		QThreadStart();
#else
		buildEmulatorCameras();
#endif
		m_is_inited = true;
	}
	return true;
}

bool CCameraManager::exitInstance()
{
	if (m_is_inited)
	{
		singlelog_lv0("The Camera Capture Module ExitInstance");
		m_is_inited = false;

		for (i32 i = 0; i < PO_CAM_COUNT; i++)
		{
			m_cam_capture[i].exitInstance();
		}
		m_focus_engine.exitInstance();
		m_captured_size = 0;

#ifdef POR_WITH_CAMERA
		m_is_thread_cancel = true;
		QThreadStop();

		i32 avail_camera = m_cam_set_ptr->getAvailableCamType();

		/* MindVision카메라드라이버를 해제한다. */
		if (CPOBase::bitCheck(avail_camera, kPOCamMindVision))
		{
#  ifdef POR_SUPPORT_MINDVISION
			checkError(CMindVisionCamera::exitSDK());
#  endif
		}

		/* BaslerPylon카메라드라이버를 해제한다. */
		if (CPOBase::bitCheck(avail_camera, kPOCamBaslerPylon))
		{
#  ifdef POR_SUPPORT_BASLER
			checkError(CBaslerPylonCamera::exitSDK());
#  endif
		}
#endif
	}
	return true;
}

bool CCameraManager::initEvent(CMVApplication* main_app_ptr)
{
	connect(this, SIGNAL(deviceError(i32, i32, i32)), main_app_ptr, SLOT(onDeviceError(i32, i32, i32)));
	connect(this, SIGNAL(devicePluged(i32, i32)), main_app_ptr, SLOT(onDevicePluged(i32, i32)));
	connect(this, SIGNAL(deviceUnPluged(i32, i32)), main_app_ptr, SLOT(onDeviceUnPluged(i32, i32)));
	connect(this, SIGNAL(snapImage(i32, MvControl, ImageGroup*)), main_app_ptr, SLOT(onSnapImage(i32, MvControl, ImageGroup*)));

	CCameraCapture* cam_capture_ptr;
	for (i32 i = 0; i < PO_CAM_COUNT; i++)
	{
		cam_capture_ptr = m_cam_capture + i;
		connect(cam_capture_ptr, SIGNAL(updateCamState(i32)), main_app_ptr, SLOT(onUpdateCamState(i32)));
		connect(cam_capture_ptr, SIGNAL(deviceError(i32, i32, i32)), main_app_ptr, SLOT(onDeviceError(i32, i32, i32)));
	}
	return true;
}

bool CCameraManager::cameraInitInstance(i32 index)
{
	if (!CPOBase::checkIndex(index, PO_CAM_COUNT) || !m_cam_set_ptr)
	{
		return false;
	}

	//camera initialize
	CameraSetting* cam_param_ptr = m_cam_set_ptr->getCamSetting(index);
	if (!m_cam_capture[index].initInstance(cam_param_ptr, &m_focus_engine, m_ipc_stream_ptr))
	{
		printlog_lv1(QString("Camera%1 can't be initialized.").arg(index));
		alarmlog2(kMVErrCamDeviceInit, QString::number(index + 1));
		return false;
	}	
	return true;
}

bool CCameraManager::cameraExitInstance(i32 index, bool is_close)
{
	if (!CPOBase::checkIndex(index, PO_CAM_COUNT))
	{
		return false;
	}

	//camera uninitialize
	return m_cam_capture[index].exitInstance(is_close);
}

bool CCameraManager::onRequireModeSelect(i32 mode, i32 val, i32 cam_set)
{
	if (!m_cam_set_ptr)
	{
		return false;
	}

	for (i32 i = 0; i < PO_CAM_COUNT; i++)
	{
		if (!CMVBase::isCamMask(cam_set, i))
		{
			continue;
		}
		m_cam_capture[i].setCamModeSelect(mode, val);
	}
	return true;
}

bool CCameraManager::onRequireCamSetting(i32 sflag, CameraSetting* cam_setting_ptr)
{
	if (!cam_setting_ptr || !m_cam_set_ptr)
	{
		return false;
	}

	i32 cam_index = cam_setting_ptr->getCamID(); //it is used for camera index for management only
	if (!CPOBase::checkIndex(cam_index, PO_CAM_COUNT) || !m_cam_set_ptr->isReady(cam_index))
	{
		return false;
	}

	return m_cam_capture[cam_index].setCameraSetting(sflag, cam_setting_ptr);
}

void CCameraManager::onRequireImage(i32 mode, MvControl ctrl)
{
	singlelog_lv1("Camera onRequireImage process");

	if (!m_cam_set_ptr || g_main_app_ptr->m_pass_capture_id >= ctrl.m_require_img_id)
	{
		printlog_lvs2("Invalid Require command", LOG_SCOPE_CAM);
		emit snapImage(mode, ctrl, NULL);
		return;
	}

	i32 i, index;
	i32 live_count = 0;
	i32 live_cam[PO_CAM_COUNT];
	i32 stp = ctrl.getSnapStep();
	i32 cam = ctrl.getSnapCam();
	
	if (cam < 0)
	{	
		//all camera capture for snap
		for (i = 0; i < PO_CAM_COUNT; i++)
		{
			if (!m_cam_set_ptr->isReady(i))
			{
				ctrl.m_cam_error = true;
				break;
			}
			live_cam[live_count++] = i;
		}
	}
	else
	{
		//all camera capture for snap
		for (i = 0; i < PO_CAM_COUNT; i++)
		{
			if (!CMVBase::isCamMask(cam, i))
			{
				continue;
			}
			if (!m_cam_set_ptr->isReady(i))
			{
				ctrl.m_cam_error = true;
				break;
			}
			live_cam[live_count++] = i;
		}
	}

	ImageGroup* img_group_ptr = NULL;
	if (live_count <= 0 || ctrl.isCamError())
	{
        //none camera for capture
	}
	else if (live_count == 1)
	{
		img_group_ptr = acquireImageGroup();
		if (img_group_ptr)
		{
			//single capture mode
			index = live_cam[0];
			m_cam_capture[index].processSnapImage(mode, img_group_ptr->getImgData(index));
		}
	}
	else
	{
		img_group_ptr = acquireImageGroup();
		if (img_group_ptr)
		{
			//thread capture mode
			CapFrameVector cap_vector;
			QFutureWatcher<void> cap_watcher;

			for (i = 0; i < live_count; i++)
			{
				index = live_cam[i];
				cap_vector.push_back(CapFrame(mode, img_group_ptr->getImgData(index), m_cam_capture + index));
			}
			cap_watcher.setFuture(QtConcurrent::map(cap_vector, capture_frame_process));
			cap_watcher.waitForFinished();
		}
	}

	emit snapImage(mode, ctrl, img_group_ptr);
}

i32 CCameraManager::refreshCameraList()
{
#ifdef POR_WITH_CAMERA
	if (!m_cam_set_ptr)
	{
		return 0;
	}

    singlelog_lvs3("CameraDevice scanning...", LOG_SCOPE_CAM);

	//get all connected cameras
	i32 cam_count = 0;
	i32 avail_cam_type = m_cam_set_ptr->getAvailableCamType();
    CameraDevice cam_dev_list[PO_MAX_CAM_COUNT];

	/* MindVision카메라들을 검색한다. */
#ifdef POR_SUPPORT_MINDVISION
	if (CPOBase::bitCheck(avail_cam_type, kPOCamMindVision))
	{
		checkError(CMindVisionCamera::getAvailableCamera(cam_dev_list, cam_count));
        if (cam_count != 0)
        {
            ERR_UNOCCUR(0);
        }
        else
        {
            ERR_OCCUR(0, printlog_lvs3(QString("MindVision: No camera present.[%1]").arg(_err_rep0), LOG_SCOPE_CAM));
        }
	}
#endif

	/* BaslerPylon카메라들을 검색한다. */
#ifdef POR_SUPPORT_BASLER
	if (CPOBase::bitCheck(avail_cam, kPOCamBaslerPylon))
	{
        checkError(CBaslerPylonCamera::getAvailableCamera(cam_dev_list, cam_count));
        if (cam_count != 0)
        {
            ERR_UNOCCUR(1);
        }
        else
        {
            ERR_OCCUR(1, printlog_lvs3(QString("BalserPylon5: No camera present. [%1]").arg(_err_rep1)));
        }
	}
#endif

	//add new camera
	i32 i, index, connect_cam_count = 0;
	postring cam_device_name;
	bool is_used[PO_MAX_CAM_COUNT];
	memset(is_used, 0, PO_MAX_CAM_COUNT);

	for (i = 0; i < cam_count; i++)
	{
        cam_device_name = cam_dev_list[i].m_cam_name;
		index = m_cam_set_ptr->findCamIndex(cam_device_name);
		if (!CPOBase::checkIndex(index, PO_CAM_COUNT))
		{
            printlog_lvs3(QString("Unused Camera is plugged. [%1]").arg(cam_device_name.c_str()), LOG_SCOPE_APP);
			continue;
		}

		connect_cam_count++;
		is_used[index] = true;
		if (m_cam_set_ptr->isReady(index))
		{
			continue;
		}

		CameraSetting* cam_param_ptr = m_cam_set_ptr->getCamSetting(index);
		CameraInfo* cam_info_ptr = cam_param_ptr->getCameraInfo();

		/* index번째 카메라를 인식하여 상위기로 통보한다. */
        CameraDevice* cam_device_ptr = cam_dev_list + i;
		cam_param_ptr->m_cam_ready = true;
		cam_param_ptr->m_cam_used = false;
		cam_param_ptr->m_cam_id = index;
		cam_param_ptr->m_cam_type = cam_device_ptr->m_cam_type;
		{
			anlock_guard_ptr(cam_info_ptr);
			cam_info_ptr->m_cam_blob.clone(cam_device_ptr->m_cam_blob);
			cam_info_ptr->m_cam_reserved[0] = cam_device_ptr->m_cam_reserved[0];
			cam_info_ptr->m_cam_reserved[1] = cam_device_ptr->m_cam_reserved[1];
		}

		emit devicePluged(kPOSubDevCamera, index);
		actionlog(POAction(kPOActionDevPlugCamera, index + 1));
        printlog_lv1(QString("Camera%1 is pluged:%2").arg(index).arg(cam_device_name.c_str()));
	}

	//remove unplug camera
	for (i = 0; i < PO_CAM_COUNT; i++)
	{
		if (!m_cam_set_ptr->isReady(i) || is_used[i])
		{
			continue;
		}

		CameraSetting* cam_param_ptr = m_cam_set_ptr->getCamSetting(i);
		cam_param_ptr->m_cam_ready = false;
		cam_param_ptr->m_cam_used = false;

		emit deviceUnPluged(kPOSubDevCamera, i);
		actionlog(POAction(kPOActionDevUnplugCamera, i + 1));
		printlog_lv1(QString("Camera%1 is unpluged").arg(i));
	}
	return connect_cam_count;
#else
	return 0;
#endif
}

void CCameraManager::buildEmulatorCameras()
{
#ifdef POR_EMULATOR
	CEmuSamples* emulator_ptr = g_main_app_ptr->getEmuSamples();
	i32 i, count = po::_min(PO_CAM_COUNT, emulator_ptr->getEmuSampleCams());

	for (i = 0; i < count; i++)
	{
		CameraSetting* cam_param_ptr = m_cam_set_ptr->getCamSetting(i);
		CameraInfo* cam_info_ptr = cam_param_ptr->getCameraInfo();

		cam_param_ptr->m_cam_id = i;
		cam_param_ptr->m_cam_type = kPOCamEmulator;
		cam_param_ptr->m_cam_ready = true;
		
		//camera initialize
		if (!m_cam_capture[i].initInstance(cam_param_ptr, &m_focus_engine, m_ipc_stream_ptr))
		{
			printlog_lv1(QString("Camera%1 can't be initialized.").arg(i));
			alarmlog2(kMVErrCamDeviceInit, QString::number(i + 1));
			break;
		}

		emit devicePluged(kPOSubDevCamera, i);
		printlog_lv1(QString("Emulator camera%1 is pluged.").arg(i));
	}
#endif
}

bool CCameraManager::checkError(i32 error_code)
{
	if (error_code == kPODevErrNone)
	{
		return true;
	}

	emit deviceError(kPOSubDevCamera, -1, error_code);
	return false;
}

void CCameraManager::run()
{
	while (!m_is_thread_cancel)
	{
		refreshCameraList();
		QThread::msleep(kCamMgrInterval);
	}
}

ImageGroup* CCameraManager::acquireImageGroup()
{
	if (m_captured_size >= kCamSnapCount)
	{
		return NULL;
	}

	m_captured_size++;
	ImageGroup* img_group_ptr = new ImageGroup();
	img_group_ptr->setTimeStamp(sys_cur_time);
	return img_group_ptr;
}

void CCameraManager::releaseImageGroup()
{
	m_captured_size = po::_max((i32)m_captured_size - 1, 0);
}

//////////////////////////////////////////////////////////////////////////
CCameraCapture::CCameraCapture()
{
	m_buffer_size = 0;
	m_rgb_buffer_ptr[0] = NULL;
	m_rgb_buffer_ptr[1] = NULL;

	m_is_inited = false;
	m_is_thread_cancel = false;
	m_cam_mode = kCamModeNone;
	m_encoder_mode = kPOEncoderNone;
	m_cam_device_ptr = NULL;

	m_focus_engine_ptr = NULL;
	m_cam_param_ptr = NULL;
	m_ipc_stream_ptr = NULL;
	m_cur_encoder_ptr = NULL;
}

CCameraCapture::~CCameraCapture()
{
	freeBuffer();
}

bool CCameraCapture::initInstance(CameraSetting* cam_param_ptr, CCameraFocus* focus_engine_ptr, CIPCManager* ipc_stream_ptr)
{
	if (!cam_param_ptr || !focus_engine_ptr || !ipc_stream_ptr)
	{
		return false;
	}

	if (m_is_inited)
	{
		return true;
	}
	exitInstance(true);

	i32 max_width = 0, max_height = 0;
	if (cam_param_ptr)
	{
		singlelog_lv0(QString("Camera%1 InitInstance").arg(cam_param_ptr->getCamID()));
		
		m_cam_param_ptr = cam_param_ptr;
		m_focus_engine_ptr = focus_engine_ptr;
		m_ipc_stream_ptr = ipc_stream_ptr;
		m_cur_encoder_ptr = NULL;

#ifdef POR_WITH_CAMERA
		/* 카메라의 종류에 따라 실제 카메라장치를 창조한다. */
		switch (m_cam_param_ptr->getCamType())
		{
#  ifdef POR_SUPPORT_MINDVISION
			case kPOCamMindVision:
			{
				m_cam_device_ptr = new CMindVisionCamera(m_cam_param_ptr);
				break;
			}
#  endif
#  ifdef POR_SUPPORT_BASLER
			case kPOCamBaslerPylon:
			{
				m_cam_device_ptr = new CBaslerPylonCamera(m_cam_param_ptr);
				break;
			}
#  endif
			default:
			{
				m_cam_param_ptr = NULL;
				printlog_lv1(QString("Unknown camera type[%1] for camera initialize.").arg(m_cam_param_ptr->getCamType()));
				return false;
			}
		}

		/* 카메라로부터 카메라파라메터들을 (실제한 카메라조종파라메터는 제외, 그값들의 최대최소값) 읽어낸다.
		실례로: 카메라가 지원하는 기능목록, 폭광시간의 최대, 최소값, 등등등.... */
		if (!checkError(m_cam_device_ptr->initCamera()))
		{
            printlog_lv1("InitCamera failed");
			POSAFE_DELETE(m_cam_device_ptr);
			m_cam_param_ptr = NULL;
			return false;
		}

		CameraInfo* cam_info_ptr = m_cam_param_ptr->getCameraInfo();
		max_width = cam_info_ptr->m_max_width; //thread_safe
		max_height = cam_info_ptr->m_max_height; //thread_safe

		//카메라에 대한 표준설정부분이다.(프로그람의 특성에 따라서...)
		{
			i32 gap_x = po::_max(0, (max_width - PO_CAM_WIDTH) / 2);
			i32 gap_y = po::_max(0, (max_height - PO_CAM_HEIGHT) / 2);

			m_cam_param_ptr->m_cam_color.m_color_mode = kCamColorGray;
			m_cam_param_ptr->m_cam_exposure.m_autoexp_mode = kCamAEModeOff;

			m_cam_param_ptr->m_cam_range.m_range.x1 = gap_x;	//thread_safe
			m_cam_param_ptr->m_cam_range.m_range.y1 = gap_y;	//thread_safe
			m_cam_param_ptr->m_cam_range.m_range.x2 = gap_x + PO_CAM_WIDTH;	//thread_safe
			m_cam_param_ptr->m_cam_range.m_range.y2 = gap_y + PO_CAM_HEIGHT;	//thread_safe
			m_cam_param_ptr->m_cam_trigger.m_trigger_interval = 60; //thread_safe
		}

		m_cam_param_ptr->m_cam_ready = true;
		m_cam_param_ptr->m_cam_used = false;
		m_cam_param_ptr->updateValidation();

		//카메라에 대한 표준설정(updateValidation후에...)
		{
			max_width = m_cam_param_ptr->m_cam_range.m_range.getWidth();
			max_height = m_cam_param_ptr->m_cam_range.m_range.getHeight();
		}
		initBuffer(cam_info_ptr);

		/* 카메라설정정보의 내용에 따라 카메라정보를 설정한다. */
		setCameraSetting(kPOSubFlagCamCtrl | kPOSubFlagCamInitFirst, m_cam_param_ptr);

		/* 카메라설정정보의 내용에 따라 트리거를 설정한다. */
		setCameraTriggerMode(m_cam_param_ptr->m_cam_trigger);

		/* 카메라를 활성화시킨다. */
		if (!checkError(m_cam_device_ptr->play()))
		{
            printlog_lv1("InitCamera fail, Camera doesn't play()");
			POSAFE_DELETE(m_cam_device_ptr);
			m_cam_param_ptr = NULL;
			return false;
		}
#else
		CEmuSamples* emu_sample_ptr = g_main_app_ptr->getEmuSamples();
		if (!emu_sample_ptr)
		{
			return false;
		}
		emu_sample_ptr->getEmuMaxSampleSize(max_width, max_height);
		if (max_width <= 0 || max_height <= 0)
		{
			return false;
		}

		CameraInfo* cam_info_ptr = cam_param_ptr->getCameraInfo();
		{
			anlock_guard_ptr(cam_info_ptr);
			cam_info_ptr->m_cam_name = "EmulatorCamera";
			cam_info_ptr->m_cam_capability |= kCamSupportColor;
			cam_info_ptr->m_max_width = max_width;
			cam_info_ptr->m_max_height = max_height;
		}
		m_cam_device_ptr = new CEmulatorCamera(m_cam_param_ptr);
		m_cam_device_ptr->setEmuSampler(emu_sample_ptr);

		initBuffer(cam_info_ptr);
		setCameraTriggerMode(m_cam_param_ptr->m_cam_trigger.getValue());
#endif
	}

	m_is_inited = true;
	m_is_thread_cancel = false;
#ifdef POR_WITH_STREAM
	m_video_streamer.initInstance(max_width, max_height, kPOGrayChannels);
	updateEncoder();
#endif

	QThreadStart();
	return true;
}

bool CCameraCapture::exitInstance(bool cam_closed)
{
	if (m_is_inited && m_cam_param_ptr)
	{
		singlelog_lv0(QString("Camera%1 ExitInstance").arg(m_cam_param_ptr->getCamID()));

		m_is_inited = false;
		m_is_thread_cancel = true;
		QThreadStop();

#ifdef POR_WITH_CAMERA
		if (m_cam_device_ptr)
		{
			checkError(m_cam_device_ptr->exitCamera());
		}
#endif
#ifdef POR_WITH_STREAM
		m_video_streamer.exitInstance();
#endif
		freeBuffer();

		if (cam_closed)
		{
			m_cam_mode = kCamModeNone;
			m_encoder_mode = kPOEncoderNone;
		}

		m_cam_param_ptr->m_cam_ready = false;
		m_cam_param_ptr->m_cam_used = false;
		m_cam_param_ptr = NULL;
		m_focus_engine_ptr = NULL;
		m_ipc_stream_ptr = NULL;
		m_cur_encoder_ptr = NULL;
	}
	return true;
}

bool CCameraCapture::initBuffer(CameraInfo* cam_info_ptr)
{
	if (!cam_info_ptr)
	{
		return false;
	}

	i32 max_w = cam_info_ptr->getMaxWidth();
	i32 max_h = cam_info_ptr->getMaxHeight();
	i32 channels = cam_info_ptr->getMaxChannel();
	if (channels <= 0)
	{
		return false;
	}

	if (m_buffer_size < max_w*max_h*channels)
	{
		freeBuffer();

		m_buffer_size = channels * max_w*max_h;
#if defined(POR_WITH_CAMERA)
		m_rgb_buffer_ptr[0] = new u8[m_buffer_size];
		m_rgb_buffer_ptr[1] = new u8[m_buffer_size];
#endif
	}
	return true;
}

void CCameraCapture::freeBuffer()
{
	m_buffer_size = 0; 
	POSAFE_DELETE_ARRAY(m_rgb_buffer_ptr[0]);
	POSAFE_DELETE_ARRAY(m_rgb_buffer_ptr[1]);
}

void CCameraCapture::run()
{
	singlelog_lv0("The CameraCapture thread is");

	bool has_frame = false;
	bool is_cam_sync = false;
	BaseFrameInfo frame_info;

	i32 cam_id = m_cam_param_ptr->getCamID();
	CameraRange* cam_range_ptr = m_cam_param_ptr->getCameraRange();
	CameraState* cam_state_ptr = m_cam_param_ptr->getCameraState();

	ImageData img_data;
	i64 wait_time = sys_cur_time;
	i64 elasped_time = wait_time;
	i32 cur_sync_time = 0, cam_sync_time = 0;

	while (!m_is_thread_cancel)
	{

		wait_time += m_cam_device_ptr->getTriggerInterval();
		wait_time = po::_max(wait_time, elasped_time - 10000);
		wait_time = po::_min(wait_time, elasped_time + 10000);

		has_frame = false;
		is_cam_sync = false;
		cur_sync_time = sys_cur_time;

		if (m_cam_param_ptr->isReady())
		{
			if (cam_sync_time < cur_sync_time)
			{
				cam_sync_time = cur_sync_time + 400;
				is_cam_sync = CPOBase::bitCheck(m_cam_mode, kCamModeSync);
			}
			//snap
			if (CPOBase::bitCheck(m_cam_mode, kCamModePreview))
			{
				checkError(m_cam_device_ptr->snapToBuffer(img_data, m_rgb_buffer_ptr[1], frame_info));
				has_frame = frame_info.has_frame;
			}
		}

		//when camera capture module has frame...
		if (has_frame)
		{
			//f32 fps = get_fps1(15000);
			//if (fps > 0)
			//{
			//	debug_log(QString("Camera%1, snap fps:%2").arg(cam_id).arg(fps));
			//}

			CImageProc::applyTransform(img_data, frame_info.reg_rotation,
							frame_info.is_flip_x, frame_info.is_flip_y, frame_info.is_invert);
			img_data.setTimeStamp(elasped_time);

			if (is_cam_sync)
			{
				updateSyncCamSetting(img_data.w, img_data.h);
				emit updateCamState(cam_id);

				m_focus_engine_ptr->setFocusImage(cam_id, img_data);
				cam_state_ptr->setCameraFocus(m_focus_engine_ptr->getImageFocus(cam_id));
			}
            sendCameraImage(&img_data, cam_id);
		}

		//sleep camera capture module with specified frame rate
		if (m_cam_device_ptr->needTriggerScan())
		{
			i32 sleep_time_ms;
			elasped_time = sys_cur_time;
			do
			{
				sleep_time_ms = po::_min((i64)40, po::_max((i64)1, wait_time - elasped_time));
				QThread::msleep(sleep_time_ms);
				elasped_time = sys_cur_time;
			}
			while (wait_time > elasped_time);
			printlog_lvs4(QString("SleepTime:%2").arg(elasped_time - wait_time), LOG_SCOPE_CAM);
		}
		else
		{
			QThread::msleep(1);
		}
	}
}

void CCameraCapture::sendCameraImage(ImageData* img_data_ptr, i32 cam_id)
{
	//send image to encoder
	switch (m_encoder_mode)
	{
        case kPOEncoderIPCRaw:
		{
			//send capture frame to raw viedo sender
			if (m_ipc_stream_ptr)
			{
				i32 len = 0; 
				len += sizeof(cam_id);
				len += sizeof(img_data_ptr->time_stamp);
				len += img_data_ptr->memImgSize();

				u8* buffer_ptr;
				i32 buffer_size = len;
				Packet* packet_ptr = new Packet(kMVCmdCamImage, kPOPacketRespOK);
				packet_ptr->setSubCmd(kMVSubTypeImageRaw);
				packet_ptr->allocateBuffer(len, buffer_ptr);
				{
					CPOBase::memWrite(cam_id, buffer_ptr, buffer_size);
					CPOBase::memWrite(img_data_ptr->time_stamp, buffer_ptr, buffer_size);
					img_data_ptr->memImgWrite(buffer_ptr, buffer_size);
				}
				m_ipc_stream_ptr->send(PacketRef(packet_ptr));
			}
			break;
		}
		case kPOEncoderFFMpegMJpeg:
		case kPOEncoderFFMpegH264:
		case kPOEncoderGStreamerMJpeg:
		case kPOEncoderGStreamerH264:
		case kPOEncoderNetworkRaw:
		{
#ifdef POR_WITH_STREAM
			//send capture frame to video stream encoder
			POMutexLocker l(m_encoder_mutex);
			if (m_cur_encoder_ptr)
			{
				m_cur_encoder_ptr->setImageToEncoder(img_data_ptr);
			}
#endif
			break;
		}
		default:
		{
			break;
		}
	}
}

bool CCameraCapture::setCameraSetting(i32 sflag, CameraSetting* tmp_cam_param_ptr)
{
	bool is_success = true;

#ifdef POR_WITH_CAMERA
	if (!m_cam_param_ptr || !m_cam_param_ptr->isReady())
	{
		return false;
	}
	
	CameraState* cam_state_ptr = m_cam_param_ptr->getCameraState();
	CameraColor* cur_cam_color_ptr = m_cam_param_ptr->getCameraColor();
	CameraRange* cur_cam_range_ptr = m_cam_param_ptr->getCameraRange();
	CameraExposure* cur_cam_exposure_ptr = m_cam_param_ptr->getCameraExposure();

	CameraColor* tmp_cam_color_ptr = tmp_cam_param_ptr->getCameraColor();
	CameraRange* tmp_cam_range_ptr = tmp_cam_param_ptr->getCameraRange();
	CameraExposure* tmp_cam_exposure_ptr = tmp_cam_param_ptr->getCameraExposure();

	{
		anlock_guard_ptr(cur_cam_range_ptr);
		anlock_guard_ptr(tmp_cam_range_ptr);

		//set geometric mode
		if (CPOBase::bitCheck(sflag, kPOSubFlagCamGeoInvert))
		{
			cur_cam_range_ptr->m_is_invert = tmp_cam_range_ptr->m_is_invert;
		}
		if (CPOBase::bitCheck(sflag, kPOSubFlagCamGeoRange))
		{
			cur_cam_range_ptr->m_range = tmp_cam_range_ptr->m_range;
			if (!checkError(m_cam_device_ptr->setCaptureRange()))
			{
				is_success = false;
			}
		}
	}
	{
		anlock_guard_ptr(cur_cam_color_ptr);
		anlock_guard_ptr(tmp_cam_color_ptr);

		//set color mode
		if (CPOBase::bitCheck(sflag, kPOSubFlagCamColorMode))
		{
			cur_cam_color_ptr->m_color_mode = tmp_cam_color_ptr->m_color_mode;
			if (!checkError(m_cam_device_ptr->setColorMode()))
			{
				is_success = false;
			}
		}
	}
	{
		anlock_guard_ptr(cur_cam_exposure_ptr);
		anlock_guard_ptr(tmp_cam_exposure_ptr);

		//set auto-exposure windows and brightness, min and max gain and exposure value
		if (CPOBase::bitCheck(sflag, kPOSubFlagCamAEWindow))
		{
			cur_cam_exposure_ptr->m_autoexp_window = tmp_cam_exposure_ptr->m_autoexp_window;
			if (!checkError(m_cam_device_ptr->setAeWindow()))
			{
				is_success = false;
			}
		}
		if (CPOBase::bitCheck(sflag, kPOSubFlagCamAEBrightness))
		{
			cur_cam_exposure_ptr->m_auto_brightness = tmp_cam_exposure_ptr->m_auto_brightness;
			if (!checkError(m_cam_device_ptr->setAeBrightness()))
			{
				is_success = false;
			}
		}
		if (CPOBase::bitCheck(sflag, kPOSubFlagCamAEGain))
		{
			cur_cam_exposure_ptr->m_autogain_min = tmp_cam_exposure_ptr->m_autogain_min;
			cur_cam_exposure_ptr->m_autogain_max = tmp_cam_exposure_ptr->m_autogain_max;
			if (!checkError(m_cam_device_ptr->setAeGain()))
			{
				is_success = false;
			}
		}
		if (CPOBase::bitCheck(sflag, kPOSubFlagCamAEExposure))
		{
			cur_cam_exposure_ptr->m_autoexp_min = tmp_cam_exposure_ptr->m_autoexp_min;
			cur_cam_exposure_ptr->m_autoexp_max = tmp_cam_exposure_ptr->m_autoexp_max;
			if (!checkError(m_cam_device_ptr->setAeExposureTimeMs()))
			{
				is_success = false;
			}
		}

		//set camera auto exposure mode
		if (CPOBase::bitCheck(sflag, kPOSubFlagCamAEMode))
		{
			if (checkError(m_cam_device_ptr->setAeState(tmp_cam_exposure_ptr->m_autoexp_mode)))
			{
				cur_cam_exposure_ptr->m_autoexp_mode = tmp_cam_exposure_ptr->m_autoexp_mode;
			}
			else
			{
				is_success = false;
			}
		}

		//set gain and exposure when auto mode is off
		if (cur_cam_exposure_ptr->m_autoexp_mode == kCamAEModeOff)
		{
			if (CPOBase::bitCheck(sflag, kPOSubFlagCamGain))
			{
				cur_cam_exposure_ptr->m_gain = tmp_cam_exposure_ptr->m_gain;
				if (!m_cam_param_ptr->supportFunc(kCamSupportManualExp) || !checkError(m_cam_device_ptr->setGain()))
				{
					is_success = false;
				}
			}
			if (CPOBase::bitCheck(sflag, kPOSubFlagCamExposure))
			{
				cur_cam_exposure_ptr->m_exposure = tmp_cam_exposure_ptr->m_exposure;
				if (!m_cam_param_ptr->supportFunc(kCamSupportManualExp) || !checkError(m_cam_device_ptr->setExposureTimeMs()))
				{
					is_success = false;
				}
			}
		}
	}
	
	//clear camera focus index
	if (CPOBase::bitCheck(sflag, kPOSubFlagCamClearFocus))
	{
		cam_state_ptr->clearFocusHistory();
	}

	//update camera state, when init
	if (CPOBase::bitCheck(sflag, kPOSubFlagCamInitFirst))
	{
		Recti range = cur_cam_range_ptr->getRange();
		updateSyncCamSetting(range.getWidth(), range.getHeight());
	}
#endif
	return is_success;
}

void CCameraCapture::processSnapImage(i32 snap_mode, ImageData* img_data_ptr)
{
	//check camera ready flag
	if (!m_cam_param_ptr->isReady() || !img_data_ptr)
	{
		return;
	}

	bool has_frame = false;
	BaseFrameInfo frame_info;
	i32 cam_id = m_cam_param_ptr->getCamID();

	/* 카메라로부터 실시간화상자료를 입력받는다. */
	checkError(m_cam_device_ptr->snapManualToBuffer(img_data_ptr[0], m_rgb_buffer_ptr[0], frame_info));
	has_frame = frame_info.has_frame;

	//check captured image validation
	if (has_frame)
	{
		CImageProc::applyTransform(img_data_ptr[0], frame_info.reg_rotation,
						frame_info.is_flip_x, frame_info.is_flip_y, frame_info.is_invert);
		img_data_ptr->setSnapImage(true);
		sendCameraImage(img_data_ptr, cam_id);
	}
}

void CCameraCapture::setCamModeSelect(i32 mode, i32 value)
{
	i32 cam_id = -1;
	if (m_cam_param_ptr)
	{
		cam_id = m_cam_param_ptr->getCamID();
	}
		
	switch (mode)
	{
        case kCamCtrlPreview:
        {
            if (!CPOBase::bitCheck(m_cam_mode, kCamModePreview))
            {
                m_cam_mode |= kCamModePreview;
                printlog_lvs2(QString("Cam%1 is Preview").arg(cam_id), LOG_SCOPE_CAM);
            }
            break;
        }
        case kCamCtrlNoPreview:
        {
            if (CPOBase::bitCheck(m_cam_mode, kCamModePreview))
            {
                m_cam_mode &= ~kCamModePreview;
                printlog_lvs2(QString("Cam%1 is NonPreview").arg(cam_id), LOG_SCOPE_CAM);
            }
            break;
        }
        case kCamCtrlSettingSync:
        {
            if (value)
            {
                m_cam_mode |= kCamModeSync;
            }
            else
            {
                m_cam_mode &= ~kCamModeSync;
            }
            break;
        }
        case kCamCtrlEncoderMode:
        {
            m_encoder_mode = value;
            updateEncoder();
            break;
        }
	}
}

void CCameraCapture::setCameraTriggerMode(CameraTrigger cam_trigger)
{
	if (!m_cam_param_ptr->isReady())
	{
		return;
	}

#ifdef POR_WITH_CAMERA
	checkError(m_cam_device_ptr->setTriggerMode(cam_trigger));
#endif
}

bool CCameraCapture::readEmuSamples(const i32 cam_id, i32 snap_mode, ImageData* img_data_ptr)
{
	return true;
}

void CCameraCapture::updateSyncCamSetting(i32 w, i32 h)
{
	i32 cam_handle = m_cam_param_ptr->getCamHandle();
	f32 gain = 0;
	f32 exposure_time_ms = 0;
	bool autoexp_mode = false;

#ifdef POR_WITH_CAMERA
	if (!checkError(m_cam_device_ptr->getCameraState(autoexp_mode, gain, exposure_time_ms)))
	{
		return;
	}
#endif

	CameraState* cam_state_ptr = m_cam_param_ptr->getCameraState();
	{
		anlock_guard_ptr(cam_state_ptr);
		cam_state_ptr->m_is_autoexp_mode = autoexp_mode;
		cam_state_ptr->m_gain = gain;
		cam_state_ptr->m_exposure = exposure_time_ms;
		cam_state_ptr->m_capture_width = w;
		cam_state_ptr->m_capture_height = h;
	}
}

bool CCameraCapture::checkError(i32 error_code)
{
	if (error_code == kPODevErrNone)
	{
		return true;
	}

	if (m_cam_param_ptr)
	{
		emit deviceError(kPOSubDevCamera, m_cam_param_ptr->getCamID(), error_code);
	}
	return false;
}

i32 CCameraCapture::getCamID()
{
	if (!m_cam_param_ptr)
	{
		return -1;
	}
	return m_cam_param_ptr->getCamID();
}

void CCameraCapture::updateEncoder()
{
	if (!m_cam_param_ptr)
	{
		return;
	}

	i32 w, h, channel;
	i32 cam_index = m_cam_param_ptr->m_cam_id;
	m_cam_param_ptr->getCameraResolution(w, h, channel);

#ifdef POR_WITH_STREAM
	switch (m_encoder_mode)
	{
		case kPOEncoderFFMpegMJpeg:
		{
			if (m_video_streamer.acquireEncoder(kPOEncoderFFMpegMJpeg, w, h, channel,
										STREAM_FPS_NORMAL, STREAM_BITRATE_HIGH, cam_index))
			{
				POMutexLocker l(m_encoder_mutex);
				m_cur_encoder_ptr = &m_video_streamer;
			}
            printlog_lvs2("Init FFMpegEncoder", LOG_SCOPE_ENCODE);
			break;
		}
		case kPOEncoderGStreamerMJpeg:
		{
			i32 cam_index = m_cam_param_ptr->getCamID();
			if (m_video_streamer.acquireEncoder(kPOEncoderGStreamerMJpeg, w, h, channel,
										STREAM_FPS_NORMAL, STREAM_QUALITY_MEDIUM, cam_index))
			{
				POMutexLocker l(m_encoder_mutex);
				m_cur_encoder_ptr = &m_video_streamer;
			}
            printlog_lvs2("Init GStreamerEncoder", LOG_SCOPE_ENCODE);
			break;
		}
		case kPOEncoderNetworkRaw:
		{
			i32 cam_index = m_cam_param_ptr->getCamID();
			if (m_video_streamer.acquireEncoder(kPOEncoderNetworkRaw, w, h, channel, 0, 0, cam_index))
			{
				POMutexLocker l(m_encoder_mutex);
				m_cur_encoder_ptr = &m_video_streamer;
			}
			printlog_lvs2("Init RawEncoder", LOG_SCOPE_ENCODE);
			break;
		}
		default:
		{
			m_video_streamer.releaseEncoder();
			{
				POMutexLocker l(m_encoder_mutex);
				m_cur_encoder_ptr = NULL;
			}
            printlog_lvs2("Close Encoder", LOG_SCOPE_ENCODE);
            break;
		}
	}
#endif
}
