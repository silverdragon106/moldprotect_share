#pragma once

#include "mv_struct_int.h"
#include "struct/camera_setting.h"
#include "capture_stream.h"
#include "camera_focus.h"

#include <QThread>
#include <QMutex>
#include <QTimer>

enum CamModeTypes
{
	kCamModeNone	= 0x00,
	kCamModePreview = 0x01,
	kCamModeSync	= 0x02
};

enum CamControlTypes
{
	kCamCtrlNone,
	kCamCtrlPreview,
	kCamCtrlNoPreview,
	kCamCtrlEncoderMode,
	kCamCtrlSettingSync,

	kCamCtrlCount,

	kCamSnapCheck,
	kCamSnapRecheck,
	kCamSnapTest,
	kCamSnapModel
};

const i32 kCamSnapCount = 2;
const i32 kCamMgrInterval = 3000;

class CMVEngine;
class CCameraManager;
class CIPCManager;
class CBaseCamera;
class CCaptureStream;

class CCameraCapture : public QThread
{
	Q_OBJECT

public:
	CCameraCapture();
	~CCameraCapture();

	bool						initInstance(CameraSetting* cam_param_ptr, CCameraFocus* focus_engine_ptr, CIPCManager* m_ipc_stream_ptr);
	bool						exitInstance(bool cam_closed = true);

	bool						initBuffer(CameraInfo* cam_info_ptr);
	void						freeBuffer();

	bool						setCameraSetting(i32 flags, CameraSetting* pCameraSetting);
	void						setCamModeSelect(i32 mode, i32 val = 0);
	void						setCameraTriggerMode(CameraTrigger cam_trigger);
	void						processSnapImage(i32 mode, ImageData* img_data_ptr);

	bool						readEmuSamples(const i32 cam_id, i32 snap_mode, ImageData* img_data_ptr);
	void						sendCameraImage(ImageData* img_data_ptr, i32 cam_id);

	void						updateSyncCamSetting(i32 w, i32 h);
	void						updateEncoder();

	i32							getCamID();
	const bool					isReady() const { return m_is_inited; }

protected:
	void						run() Q_DECL_OVERRIDE;

private:
	bool						checkError(i32 error_code);

signals:
	void						updateCamState(i32 cam_index);
	void						deviceError(i32 sub_dev, i32 cam_index, i32 errtype);

public:
	std::atomic<i32>			m_cam_mode;
	i32							m_encoder_mode;
    CCaptureStream*				m_cur_encoder_ptr;
#ifdef POR_WITH_STREAM
	CCaptureStream				m_video_streamer;
#endif

	CameraSetting*				m_cam_param_ptr;
	CCameraFocus*				m_focus_engine_ptr;
	CIPCManager*				m_ipc_stream_ptr;
	CBaseCamera*				m_cam_device_ptr;

	i32							m_buffer_size;
    u8*							m_rgb_buffer_ptr[2];

	bool						m_is_inited;
	std::atomic<bool>			m_is_thread_cancel;
	POMutex						m_encoder_mutex;
};

class CMVApplication;
class CCameraManager : public QThread
{
	Q_OBJECT
    ERR_DEFINE(0)
    ERR_DEFINE(1)

public:
	CCameraManager();
	~CCameraManager();

	bool						initInstance(CameraSet* camera_set_ptr, CIPCManager* ipc_stream_ptr, CMVEngine* mv_engine_ptr);
	bool						exitInstance();

	bool						initEvent(CMVApplication* main_app_ptr);
	bool						cameraInitInstance(i32 index);
	bool						cameraExitInstance(i32 index, bool is_close);

	bool						onRequireModeSelect(i32 mode, i32 val = 0, i32 cam = 0xFF);
	bool						onRequireCamSetting(i32 sflag, CameraSetting* cam_setting_ptr);

	ImageGroup*					acquireImageGroup();
	void						releaseImageGroup();

private:
	i32							refreshCameraList();
	void						buildEmulatorCameras();
	bool						checkError(i32 code);

protected:
	void						run() Q_DECL_OVERRIDE;

signals:
	void						snapImage(i32 mode, MvControl ctrl, ImageGroup* img_group_ptr);
	void						applySetting(bool bsuccess, i32 index, i32 flag);
	void						deviceError(i32 devpart, i32 errtype, i32 value);
	void						devicePluged(i32 subdev, i32 index);
	void						deviceUnPluged(i32 subdev, i32 index);

public slots:
	void						onRequireImage(i32 mode, MvControl ctrl);

public:
	std::atomic<i32>			m_captured_size;

	bool						m_is_inited;
	std::atomic<bool>			m_is_thread_cancel;
	CameraSet*					m_cam_set_ptr;
	CIPCManager*				m_ipc_stream_ptr;

	CCameraFocus				m_focus_engine;
	CCameraCapture				m_cam_capture[PO_CAM_COUNT];
};
