#pragma once

#include "mv_define.h"
#include "struct.h"
#include "lock_guide.h"
#include "struct/camera_setting.h"

#pragma pack(push, 4)

struct ModelSnapMode
{
	i32					snap_mode;
	i32					auto_snap_step;

public:
	ModelSnapMode();

	void				init();
	bool				isDupAutoSnapStep(i32 stp);
};

class MvControl
{
public:
	MvControl();

	void				initControl(i32 state);
	void				initModelControl();

	bool				isModelAutoSnapMode();
	bool				isModelTestMode();
	bool				isDupAutoSnapStep(i32 stp);
	i32					getSnapCamIndex();
	void				setModelEditMode(i32 mode);
	void				setModelSnapMode(i32 mode, i32 stp = kMVStepNone);
	bool				setModelROITest(i32 stp, i32 cam);

	inline i32			getState() { return m_app_state; };
	inline i32			getSnapCam() { return m_snap_cam; };
	inline i32			getSnapStep() { return m_snap_step; };
	inline i32			getSnapTime() { return m_snap_time; };
	inline i32			getStepTime() { return m_step_time; };
	inline i32			getModelEditMode() { return m_model_edit_mode; };
	inline i32			getModelSnapMode() { return m_model_snap_mode.snap_mode; };
	inline i32			getModelAutoSnapStep() { return m_model_snap_mode.auto_snap_step; };
	inline bool			isCamError() { return m_cam_error; };

	i32					memSize();
	i32					memWrite(u8*& buffer_ptr, i32& buffer_size);
	i32					memRestore(u8*& buffer_ptr, i32& buffer_size);

	bool				fileRead(FILE* fp);
	bool				fileWrite(FILE* fp);

public:
	i32					m_app_state;
	i32					m_snap_step;
	i32					m_snap_cam;
	i64					m_snap_time;
	i32					m_model_edit_mode;
	ModelSnapMode		m_model_snap_mode;
	i32					m_require_img_id;
	i32					m_step_time;
	bool				m_cam_error;
};

#define MVPARAM_BLACK_SENSITIVE		85
#define MVPARAM_WHITE_SENSITIVE		70
#define MVPARAM_HSUBDIFF_OFFSET		10.0f
#define MVPARAM_HSUBDIFF_RATE		0.2f
#define MVPARAM_SUBDIFF_RATE		0.8f
#define MVPARAM_DIFF_MAXMEAN		50.0f
#define MVPARAM_DIFF_MAXDEV			40.0f
#define MVPARAM_GRAY_BALCK			0
#define MVPARAM_GRAY_WHITE			255
#define MVPARAM_GAL_DIFF			15.0f
#define MVPARAM_GAH_DIFF			45.0f

#define MVPARAM_STRUCT_THRESHOLD	0.75f

#define MVPARAM_DEFECT_THIN			5
#define MVPARAM_DEFECT_AREA			100

struct DefectParam
{
	//extract sub difference between model and current frame by these parameters
	i32					th_b_sensitive;	//0 ~ 100: 흑색민감도
	i32					th_w_sensitive;	//0 ~ 100: 백색민감도
	f32					th_max_mean;	//0 ~ 255: 밝기평균의 최대편차
	f32					th_max_st_dev;	//0 ~ 255: 표준편차의 최대편차

	//defect area extract parameters
	i32					th_defect_area;	//1 ~ 9000: 오유구역의 최소면적
	i32					th_defect_thin;	//0 ~ 30: 오유구역의 최소두께

public:
	DefectParam();

	void				init();
	i32					getThGBDiff();
	i32					getThGWDiff();
	i32 				getThGHSubDiff();
	void				getThGDiffs(i32* sub_histo_ptr);

	f32					getThMaxMean();
	f32					getThMaxStdDev();

	i32					memSize();
	i32					memRead(u8*& buffer_ptr, i32& buffer_size);
	i32					memWrite(u8*& buffer_ptr, i32& buffer_size);
};

struct CheckedResult
{
	i32					chk_ok_count;
	i32					chk_ng_count;
	i32					chk_fail_count;
	i32					chk_unk_count;

public:
	CheckedResult();

	void				init();
	void				updateCheckResult(i32 result);
	CheckedResult& operator+=(const CheckedResult& other);
};

struct ChkTimerResult
{
	i32					chk_count;
	f32					cur_proc_time;	//s
	f32					avg_proc_time;	//s
	f32					min_proc_time;	//s
	f32					max_proc_time;	//s

public:
	ChkTimerResult();

	void				init();
	void				updateChkTimeResult(f32 ptime);
	ChkTimerResult& operator+=(const ChkTimerResult& other);
};

struct RuntimeHistory
{
	//accmulation history information
	i32					snap_total_count;
	i32					boot_count;
	f32					acc_up_time;		//s
	f32					backup_acc_up_time;	//s, but it's used to backup acctime in file

	//runtime history
	i32					snap_count;
	i32					snap_ok_count;
	i32					snap_ng_count;
	f32					cur_up_time;		//s

	CheckedResult		checked_result[kMVStepCount][PO_CAM_COUNT];
	ChkTimerResult		checked_proc_time[kMVStepCount][PO_CAM_COUNT];

	//tempoary result
	i32					acc_result;

public:
	RuntimeHistory();

	void				init();
	void				initRuntime();
	void				setAccResult(i32 result);

	i32					memSize(i32 stp = -1);
	void				memWrite(u8*& buffer_ptr, i32& buffer_size, i32 stp = -1);

	bool				fileRead(FILE* fp);
	bool				fileWrite(FILE* fp);
};

class CTransform
{
public:
	CTransform();
	CTransform(f32* T);
	CTransform(f32* T, i32 count);

	void				init(f32* T = 0);
	void				initEx(f32* T, i32 count);
	void				initLP2WP(f32 angle, f32 cx = 0, f32 cy = 0);
	void				initLP2WP(f32 costh, f32 sinth, f32 cx = 0, f32 cy = 0);
	void				initWP2LP(f32 ax, f32 ay, f32 cx = 0, f32 cy = 0);
	void				update();

	CTransform			invert();

	f32*				getMatrix();
	f32*				getInvMatrix();
	void				getTransform(f32& tx, f32& ty, f32& angle);
	void				getInvTransform(f32& tx, f32& ty, f32& angle);
	void				transform(vector2df& vec2d);
	void				transRot(vector2df& vec2d);
	void				invTransform(vector2df& vec2d);
	void				invTransRot(vector2df& vec2d);
	void				invertT0();

	CTransform			operator*(CTransform& rhs);
	CTransform&			operator*=(CTransform& rhs);

	bool				fileRead(FILE* fp);
	bool				fileWrite(FILE* fp);

	void				setValue(CTransform* other_ptr);
	i32					memSize();
	i32					memWrite(u8*& buffer_ptr, i32& buffer_size);
	i32					memRead(u8*& buffer_ptr, i32& buffer_size);

	template <class T>
	void transform(T& x, T& y)
	{
		T x1 = x;
		T y1 = y;
		x = (T)(m_t0[0] * x1 + m_t0[1] * y1 + m_t0[2]);
		y = (T)(m_t0[3] * x1 + m_t0[4] * y1 + m_t0[5]);
	}

	template <class T>
	void transRot(T & x, T & y)
	{
		T x1 = x;
		T y1 = y;
		x = (T)(m_t0[0] * x1 + m_t0[1] * y1);
		y = (T)(m_t0[3] * x1 + m_t0[4] * y1);
	}

	template <class T>
	void invTransform(T & x, T & y)
	{
		T x1 = x;
		T y1 = y;
		x = (T)(m_t1[0] * x1 + m_t1[1] * y1 + m_t1[2]);
		y = (T)(m_t1[3] * x1 + m_t1[4] * y1 + m_t1[5]);
	}

	template <class T>
	void invTransRot(T & x, T & y)
	{
		T x1 = x;
		T y1 = y;
		x = (T)(m_t1[0] * x1 + m_t1[1] * y1);
		y = (T)(m_t1[3] * x1 + m_t1[4] * y1);
	}

public:
	f32					m_t0[9];
	f32					m_t1[9];
	bool				m_has_invert;
};

class CTransRect : public CTransform
{
public:
	CTransRect();
	CTransRect(f32* T);
	CTransRect(Rectf rt);

	void				init(f32* T = NULL);
	void				inflateRect(f32 val);
	void				deflateRect(f32 val);

	void				setBoundingType(i32 type);
	void				setRegionData(i32 type, Rectf rt);
	void				setRegionData(i32 type, Pixelu* pixels, i32 count);
	void				setRegionData(i32 type, Pixelf* pixels, i32 count);
	void				makeBoundingBox(f32 x, f32 y);
	void				getBoundingBox(f32& x1, f32& y1, f32& x2, f32& y2);
	void				getWorldRotationBox(vector2df& x1, vector2df& x2, vector2df& x3, vector2df& x4);
	i32					getPtCount();
	i32					getRegionType();
	Rectf				getBoundingBox();
	Rectf				getWorldBoundingBox();
	Rectf				getWorldBoundingBox(Rectf* pbox);

	bool				fileRead(FILE* fp);
	bool				fileWrite(FILE* fp);

	void				setValue(CTransRect* other_ptr);
	i32					memSize();
	i32					memWrite(u8*& buffer_ptr, i32& buffer_size);
	i32					memRead(u8*& buffer_ptr, i32& buffer_size);

public:
	i32					m_rect_type;		//bounding type in own rect
	Rectf				m_bounding_box;		//own coordniate
	ptfvector           m_shape_point_vec;	//point vector for free-shape
};
typedef std::vector<CTransRect*> TransRectPVector;

struct ROIData
{
public:
	ROIData();
	~ROIData();

	void				setValue(ROIData* other_ptr);

	virtual i32			memSize();
	virtual i32			memWrite(u8*& buffer_ptr, i32& buffer_size);
	virtual i32			memRead(u8*& buffer_ptr, i32& buffer_size);

	bool				fileWrite(FILE* fp);
	bool				fileRead(FILE* fp);

	inline CTransRect*	getTr() { return &roi_tr; };
	inline DefectParam*	getMoldParam() { return &roi_param; };

public:
	DefectParam			roi_param;
	CTransRect			roi_tr;
};
typedef std::vector<ROIData*> ROIPVector;

class CMoldSnapBase
{
public:
	CMoldSnapBase();
	~CMoldSnapBase();

	void				initBuffer();
	void				freeBuffer();

	virtual i32			memSize();
	virtual i32			memWrite(u8*& buffer_ptr, i32& buffer_size);
	virtual i32			memRead(u8*& buffer_ptr, i32& buffer_size);

	inline i32			getROICount() { return (i32)m_roi_tr.size(); };
	inline i32			getMaskCount() { return (i32)m_mask_tr.size(); };
	inline ROIPVector&	getROIPVector() { return m_roi_tr; };

public:
	ROIPVector			m_roi_tr;
	TransRectPVector	m_mask_tr;
};

struct CROIMaskData
{
public:
	CROIMaskData();
	~CROIMaskData();

	void				initBuffer(Recti range);
	void				freeBuffer();

	void				initAutoCheckBuffer();

	bool				fileWrite(FILE* fp);
	bool				fileRead(FILE* fp);
	
	u8*					getMaskImage(bool use_autochk_mask);

public:
	Recti				m_range;
	i32					m_area;
	u8*					m_mask_img_ptr;

	u8*					m_autochk_mask_ptr; //tempoary
};
typedef std::vector<CROIMaskData*> ROIMaskPVector;

class CAutoCheckBase
{
public:
	CAutoCheckBase();
	~CAutoCheckBase();

	void				initBuffer();
	void				freeBuffer();

	virtual i32			memSize();
	virtual i32			memRead(u8*& buffer_ptr, i32& buffer_size);
	virtual i32			memWrite(u8*& buffer_ptr, i32& buffer_size);

public:
	Recti				m_range;
	shapeuvector		m_auto_region_vec;
};

#define MVIO_MAXIN					8
#define MVIO_MAXOUT					8

#define IOOUT_EQUIP					9
#define IOOUT_SCHEMENUM				4

#define EQCTRL_FREE					0
#define EQCTRL_PRODUCTTRIGGER		1
#define EQCTRL_PRODUCTOK			2
#define EQCTRL_PRODUCTNG			3
#define EQCTRL_EJECTTRIGGER			4
#define EQCTRL_EJECTOK				5
#define EQCTRL_EJECTNG				6
#define EQCTRL_THIRDTRIGGER			7
#define EQCTRL_THIRDOK				8
#define EQCTRL_THIRDNG				9
#define EQCTRL_COUNT				10

#define MVIO_NORMALMODE				0x00
#define MVIO_INVERSEMODE			0x01
#define MVIO_NCMODE					0x02

#define IOIN_NONE					0
#define IOIN_MOLDOPEN				1
#define IOIN_EJECTOROUT				2
#define IOIN_EJECTORBACK			3
#define IOIN_SAFEDOOR				4
#define IOIN_THIRDTRIGGER			5
#define IOIN_RESERVED1				6
#define IOIN_RESERVED2				7
#define IOIN_RESERVED3				8
#define IOIN_COUNT					10
#define IOIN_EJECTORBACKSIM			11

#define IOOUT_NONE					0
#define IOOUT_MOLDCLOSEPERMIT		1
#define IOOUT_EJECTORPERMIT			2
#define IOOUT_MANIPPERMIT			3
#define IOOUT_ALARMPERMIT			4
#define IOOUT_RESERVED1				5
#define IOOUT_RESERVED2				6
#define IOOUT_RESERVED3				7
#define IOOUT_RESERVED4				8
#define IOOUT_COUNT					10

#define EQIO_OFFMODE				0x00
#define EQIO_ONMODE					0x01
#define EQIO_KEEPMODE				0x02

struct IOInfo
{
	//device input pin -> mold protect functions(mode, pair)
	i32					in_count;
	i32					in_address[MVIO_MAXIN];		//slot->addr
	i32					in_method[MVIO_MAXIN];		//slot->method
	u8					in_mode[MVIO_MAXIN];		//slot->mode

	//device output pin -> mold protect functions(mode, pair)
	i32					out_count;
	i32					out_address[MVIO_MAXOUT];	//slot->addr
	i32					out_method[MVIO_MAXOUT];	//slot->method
	u8					out_mode[MVIO_MAXOUT];		//slot->mode

	//equioment scheme
	i32					scheme_count;
	i32					scheme_index;
	postring			scheme_name[IOOUT_SCHEMENUM];
	u8					equip_scheme[IOOUT_SCHEMENUM*EQCTRL_COUNT*IOOUT_EQUIP];

public:
	IOInfo();
	~IOInfo();

	void				init();
	void				initIOAddress();
	void				initEquipMode();

	void				setIOInAddress(i32 method, i32 addr);
	void				setIOOutAddress(i32 method, i32 addr);
	bool				setEquipMode(i32 dev_event, i32 out_signal, i32 is_allow_mode);

	i32					getIOInAddress(i32 method);
	i32					getIOOutAddress(i32 method);
	u8					getEquipMode(i32 dev_event, i32 out_signal);
	u8*					getEquipmentControl(i32 stp);
	void				checkIOParam(IOInfo& param);

	i32					memSize();
	i32					memWrite(u8*& buffer_ptr, i32& buffer_size);
	i32					memRead(u8*& buffer_ptr, i32& buffer_size);

	bool				fileRead(FILE* fp);
	bool				fileWrite(FILE* fp);
};

#pragma pack(pop)
