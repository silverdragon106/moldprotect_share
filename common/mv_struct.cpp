#include "mv_struct.h"
#include "mv_base.h"

//////////////////////////////////////////////////////////////////////////
ModelSnapMode::ModelSnapMode()
{
	memset(this, 0, sizeof(ModelSnapMode));
}

void ModelSnapMode::init()
{
	snap_mode = kModelSnapModeNone;
	auto_snap_step = kMVStepNone;
}

bool ModelSnapMode::isDupAutoSnapStep(i32 stp)
{
	if (stp <= kMVStepNone || stp >= kMVStepCount)
	{
		return false;
	}

	if (auto_snap_step == kMVStepNone)
	{
		auto_snap_step = stp;
		return false;
	}
	return (auto_snap_step == stp);
}


//////////////////////////////////////////////////////////////////////////
MvControl::MvControl()
{
	initControl(kMVStateNone);
}

void MvControl::initControl(i32 state)
{
	m_app_state = state;
	m_model_edit_mode = kMVEditUnknown;
	m_model_snap_mode.init();

	m_snap_step = kMVStepNone;
	m_snap_cam = -1;
	m_cam_error = false;
	m_snap_time = 0;
	m_require_img_id = 0;
	m_step_time = 0;
}

void MvControl::initModelControl()
{
	m_model_edit_mode = kMVEditUnknown;
	m_model_snap_mode.init();
}

bool MvControl::isModelAutoSnapMode()
{
	if (m_app_state != kMVStateModel)
	{
		return false;
	}
	if (m_model_edit_mode != kMVEditModelImage || m_model_snap_mode.snap_mode != kModelSnapModeAuto)
	{
		return false;
	}
	return true;
}

bool MvControl::isModelTestMode()
{
	return m_model_edit_mode == kMVEditROITest;
}

bool MvControl::isDupAutoSnapStep(i32 stp)
{
	return m_model_snap_mode.isDupAutoSnapStep(stp);
}

void MvControl::setModelSnapMode(i32 mode, i32 stp)
{
	m_model_snap_mode.snap_mode = mode;
	m_model_snap_mode.auto_snap_step = stp;
}

void MvControl::setModelEditMode(i32 mode)
{
	m_model_edit_mode = mode;
}

bool MvControl::setModelROITest(i32 stp, i32 cam)
{
	if (m_model_edit_mode == kMVEditROITest && m_snap_step == stp && m_snap_cam == cam)
	{
		return false;
	}

	m_snap_step = stp;
	m_snap_cam = cam;
	m_cam_error = false;
	m_model_edit_mode = kMVEditROITest;
	return true;
}

i32 MvControl::getSnapCamIndex()
{
	for (i32 i = 0; i < PO_CAM_COUNT; i++)
	{
		if (CMVBase::isCamMask(m_snap_cam, i))
		{
			return i;
		}
	}
	return -1;
}

i32 MvControl::memSize()
{
	i32 len = 0;
	len += sizeof(m_app_state);
	len += sizeof(m_snap_step);
	len += sizeof(m_snap_cam);
	len += sizeof(m_snap_time);
	len += sizeof(m_step_time);
	return len;
}

i32 MvControl::memWrite(u8 * &buffer_ptr, i32 & buffer_size)
{
	u8* buffer_pos = buffer_ptr;
	CPOBase::memWrite(m_app_state, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_snap_step, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_snap_cam, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_snap_time, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_step_time, buffer_ptr, buffer_size);
	return buffer_ptr - buffer_pos;
}

i32 MvControl::memRestore(u8 * &buffer_ptr, i32 & buffer_size)
{
	u8* buffer_pos = buffer_ptr;
	CPOBase::memRead(m_app_state, buffer_ptr, buffer_size);
	CPOBase::memRead(m_snap_step, buffer_ptr, buffer_size);
	CPOBase::memRead(m_snap_cam, buffer_ptr, buffer_size);
	CPOBase::memRead(m_snap_time, buffer_ptr, buffer_size);
	CPOBase::memRead(m_step_time, buffer_ptr, buffer_size);
	return buffer_ptr - buffer_pos;
}

bool MvControl::fileRead(FILE * fp)
{
	if (!CPOBase::fileSignRead(fp, PO_SIGN_CODE))
	{
		printlog_lv1("The MVControl start-sign is uncorrectly");
		return false;
	}

	CPOBase::fileRead(m_app_state, fp);

	if (!CPOBase::fileSignRead(fp, PO_SIGN_ENDCODE))
	{
		printlog_lv1("The MVControl end-sign is uncorrectly");
		return false;
	}
	return true;
}

bool MvControl::fileWrite(FILE * fp)
{
	CPOBase::fileSignWrite(fp, PO_SIGN_CODE);
	CPOBase::fileWrite(m_app_state, fp);
	CPOBase::fileSignWrite(fp, PO_SIGN_ENDCODE);
	return true;
}

//////////////////////////////////////////////////////////////////////////
DefectParam::DefectParam()
{
	init();
}

void DefectParam::init()
{
	th_b_sensitive = MVPARAM_BLACK_SENSITIVE;
	th_w_sensitive = MVPARAM_WHITE_SENSITIVE;
	th_max_mean = MVPARAM_DIFF_MAXMEAN;
	th_max_st_dev = MVPARAM_DIFF_MAXDEV;

	th_defect_area = MVPARAM_DEFECT_AREA;
	th_defect_thin = MVPARAM_DEFECT_THIN;
}

i32 DefectParam::getThGHSubDiff()
{
	i32 th_sub_diff = getThGBDiff();
	f32 dc_offset = (1 - std::exp(-(f32)th_sub_diff / 15.0f)) * MVPARAM_HSUBDIFF_OFFSET;
	f32 offset = (dc_offset + MVPARAM_HSUBDIFF_RATE * th_sub_diff) * (std::sqrt(1 - (th_sub_diff / 260)));
	return po::_min(offset, MVPARAM_GRAY_WHITE - th_sub_diff);
}

i32 DefectParam::getThGBDiff()
{
	return  2 * (100 - th_b_sensitive);
}

i32 DefectParam::getThGWDiff()
{
	return 2 * (100 - th_w_sensitive);
}

void DefectParam::getThGDiffs(i32* th_diff_ptr)
{
	if (!th_diff_ptr)
	{
		return;
	}

	f32 tmp;
	i32 bth = getThGBDiff();
	i32 wth = getThGWDiff();
	for (i32 i = 0; i < 256; i++)
	{
		tmp = 1.0f / (1.0f + exp(-0.04f * ((f32)i - 128.0f)));
		th_diff_ptr[i] = bth + tmp * (wth - bth);
	}
}

f32 DefectParam::getThMaxMean()
{
	return th_max_mean;
}

f32 DefectParam::getThMaxStdDev()
{
	return th_max_st_dev;
}

i32 DefectParam::memSize()
{
	i32 len = 0;
	len += sizeof(th_b_sensitive);
	len += sizeof(th_w_sensitive);
	len += sizeof(th_defect_area);
	return len;
}

i32 DefectParam::memRead(u8 * &buffer_ptr, i32 & buffer_size)
{
	u8* buffer_pos = buffer_ptr;

	CPOBase::memRead(th_b_sensitive, buffer_ptr, buffer_size);
	CPOBase::memRead(th_w_sensitive, buffer_ptr, buffer_size);
	CPOBase::memRead(th_defect_area, buffer_ptr, buffer_size);

	return buffer_ptr - buffer_pos;
}

i32 DefectParam::memWrite(u8 * &buffer_ptr, i32 & buffer_size)
{
	u8* buffer_pos = buffer_ptr;

	CPOBase::memWrite(th_b_sensitive, buffer_ptr, buffer_size);
	CPOBase::memWrite(th_w_sensitive, buffer_ptr, buffer_size);
	CPOBase::memWrite(th_defect_area, buffer_ptr, buffer_size);

	return buffer_ptr - buffer_pos;
}

//////////////////////////////////////////////////////////////////////////
CTransform::CTransform()
{
	init();
}

CTransform::CTransform(f32* T)
{
	init(T);
}

CTransform::CTransform(f32* T, i32 count)
{
	initEx(T, count);
}

void CTransform::init(f32* T)
{
	m_has_invert = false;
	CPOBase::init3x3(m_t0);
	CPOBase::init3x3(m_t1);

	if (T)
	{
		CPOBase::memCopy(m_t0, T, 6);
	}
}

void CTransform::initEx(f32* T, i32 count)
{
	m_has_invert = false;
	CPOBase::init3x3(m_t0);
	CPOBase::init3x3(m_t1);

	if (T)
	{
		CPOBase::memCopy(m_t0, T, count);
	}
}

void CTransform::initLP2WP(f32 angle, f32 cx, f32 cy)
{
	initLP2WP(cos(angle), sin(angle), cx, cy);
}

void CTransform::initLP2WP(f32 costh, f32 sinth, f32 cx, f32 cy)
{
	m_t0[0] = costh; m_t0[1] = -sinth; m_t0[2] = cx;
	m_t0[3] = sinth; m_t0[4] = costh; m_t0[5] = cy;
}

void CTransform::initWP2LP(f32 ax, f32 ay, f32 cx, f32 cy)
{
	m_t0[0] = ax;	m_t0[1] = ay; m_t0[2] = -cx * ax - cy * ay;
	m_t0[3] = -ay; m_t0[4] = ax; m_t0[5] = cx * ay - ax * cy;
}

void CTransform::update()
{
	cv::Mat TransForward(3, 3, CV_32FC1, m_t0);
	cv::Mat TransBackward = TransForward.inv();
	memcpy(m_t1, (f32*)TransBackward.data, sizeof(f32) * 6);
	m_has_invert = true;
}

void CTransform::transform(vector2df& vec2d)
{
	f32 x1 = vec2d.x;
	f32 y1 = vec2d.y;
	vec2d.x = m_t0[0] * x1 + m_t0[1] * y1 + m_t0[2];
	vec2d.y = m_t0[3] * x1 + m_t0[4] * y1 + m_t0[5];
}

void CTransform::transRot(vector2df& vec2d)
{
	f32 x1 = vec2d.x;
	f32 y1 = vec2d.y;
	vec2d.x = m_t0[0] * x1 + m_t0[1] * y1;
	vec2d.y = m_t0[3] * x1 + m_t0[4] * y1;
}

void CTransform::invTransform(vector2df& vec2d)
{
	f32 x1 = vec2d.x;
	f32 y1 = vec2d.y;
	vec2d.x = m_t1[0] * x1 + m_t1[1] * y1 + m_t1[2];
	vec2d.y = m_t1[3] * x1 + m_t1[4] * y1 + m_t1[5];
}

void CTransform::invTransRot(vector2df& vec2d)
{
	f32 x1 = vec2d.x;
	f32 y1 = vec2d.y;
	vec2d.x = m_t1[0] * x1 + m_t1[1] * y1;
	vec2d.y = m_t1[3] * x1 + m_t1[4] * y1;
}

void CTransform::invertT0()
{
	m_t0[0] = -m_t0[0]; m_t0[1] = -m_t0[1]; m_t0[2] = -m_t0[2];
	m_t0[3] = -m_t0[3]; m_t0[4] = -m_t0[4]; m_t0[5] = -m_t0[5];
}

f32* CTransform::getMatrix()
{
	return m_t0;
}

f32* CTransform::getInvMatrix()
{
	return m_t1;
}

void CTransform::getTransform(f32& tx, f32& ty, f32& angle)
{
	angle = 0;
	tx = m_t0[2];
	ty = m_t0[5];

	f32 scale = sqrt(m_t0[0] * m_t0[0] + m_t0[1] * m_t0[1]);
	if (scale < PO_EPSILON)
	{
		return;
	}
	angle = acos((m_t0[0] + m_t0[4]) / (2 * scale));
}

void CTransform::getInvTransform(f32 & tx, f32 & ty, f32 & angle)
{
	if (!m_has_invert)
		update();

	angle = 0;
	tx = m_t1[2];
	ty = m_t1[5];

	f32 scale = sqrt(m_t1[0] * m_t1[0] + m_t1[1] * m_t1[1]);
	if (scale < PO_EPSILON)
	{
		return;
	}
	angle = acos((m_t1[0] + m_t1[4]) / (2 * scale));
}

CTransform & CTransform::operator*=(CTransform & rhs)
{
	cv::Mat P0(3, 3, CV_32FC1, m_t0);
	cv::Mat P1(3, 3, CV_32FC1, rhs.m_t0);
	P0 *= P1;

	if (m_has_invert)
	{
		update();
	}
	return *this;
}

CTransform CTransform::operator*(CTransform & rhs)
{
	CTransform lhs;
	cv::Mat P0(3, 3, CV_32FC1, m_t0);
	cv::Mat P1(3, 3, CV_32FC1, rhs.m_t0);
	cv::Mat P2(3, 3, CV_32FC1, lhs.m_t0);
	P2 = P0 * P1;

	if (m_has_invert || rhs.m_has_invert)
	{
		lhs.update();
	}
	return lhs;
}

CTransform CTransform::invert()
{
	if (!m_has_invert)
	{
		update();
	}
	return CTransform(this->m_t1);
}

bool CTransform::fileRead(FILE * fp)
{
	if (!CPOBase::fileSignRead(fp, PO_SIGN_CODE))
	{
		return false;
	}

	CPOBase::fileRead(m_t0, 9, fp);
	CPOBase::fileRead(m_t1, 9, fp);
	CPOBase::fileRead(m_has_invert, fp);
	return CPOBase::fileSignRead(fp, PO_SIGN_ENDCODE);
}

bool CTransform::fileWrite(FILE * fp)
{
	CPOBase::fileSignWrite(fp, PO_SIGN_CODE);
	CPOBase::fileWrite(m_t0, 9, fp);
	CPOBase::fileWrite(m_t1, 9, fp);
	CPOBase::fileWrite(m_has_invert, fp);
	CPOBase::fileSignWrite(fp, PO_SIGN_ENDCODE);
	return true;
}

void CTransform::setValue(CTransform * other_ptr)
{
	m_has_invert = other_ptr->m_has_invert;
	CPOBase::memCopy(m_t0, other_ptr->m_t0, 9);
	CPOBase::memCopy(m_t1, other_ptr->m_t1, 9);
}

i32 CTransform::memSize()
{
	i32 len = 0;
	len += sizeof(m_t0);
	len += sizeof(m_t1);
	len += sizeof(m_has_invert);
	return len;
}

i32 CTransform::memWrite(u8 * &buffer_ptr, i32 & buffer_size)
{
	u8* buffer_pos = buffer_ptr;

	CPOBase::memWrite(m_t0, 9, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_t1, 9, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_has_invert, buffer_ptr, buffer_size);
	return buffer_ptr - buffer_pos;
}

i32 CTransform::memRead(u8 * &buffer_ptr, i32 & buffer_size)
{
	u8* buffer_pos = buffer_ptr;

	CPOBase::memRead(m_t0, 9, buffer_ptr, buffer_size);
	CPOBase::memRead(m_t1, 9, buffer_ptr, buffer_size);
	CPOBase::memRead(m_has_invert, buffer_ptr, buffer_size);
	return buffer_ptr - buffer_pos;
}

//////////////////////////////////////////////////////////////////////////
CTransRect::CTransRect()
{
	init();
}

CTransRect::CTransRect(f32 * T) : CTransform(T)
{
	init();
}

CTransRect::CTransRect(Rectf rt)
{
	init();
	m_bounding_box = rt;
}

void CTransRect::init(f32 * tr)
{
	m_rect_type = kTransTypeRect;
	m_bounding_box.reset();
	m_shape_point_vec.clear();
	CTransform::init(tr);
}

void CTransRect::inflateRect(f32 val)
{
	m_bounding_box.x1 -= val; m_bounding_box.x2 += val;
	m_bounding_box.y1 -= val; m_bounding_box.y2 += val;
}

void CTransRect::deflateRect(f32 val)
{
	m_bounding_box.x1 += val; m_bounding_box.x2 -= val;
	m_bounding_box.y1 += val; m_bounding_box.y2 -= val;
	m_bounding_box.x1 = po_min(m_bounding_box.x1, m_bounding_box.x2);
	m_bounding_box.y1 = po_min(m_bounding_box.y1, m_bounding_box.y2);
}

void CTransRect::setRegionData(i32 type, Rectf rt)
{
	init();
	m_bounding_box = rt;
	this->m_rect_type = type;
}

void CTransRect::setRegionData(i32 type, Pixelu * pixe_ptr, i32 count)
{
	init();
	m_shape_point_vec.resize(count);
	for (i32 i = 0; i < count; i++)
	{
		m_shape_point_vec[i].x = pixe_ptr[i].x;
		m_shape_point_vec[i].y = pixe_ptr[i].y;
		makeBoundingBox(pixe_ptr[i].x, pixe_ptr[i].y);
	}
	this->m_rect_type = type;
}

void CTransRect::setRegionData(i32 type, Pixelf * pixels, i32 count)
{
	init();
	m_shape_point_vec.resize(count);
	for (i32 i = 0; i < count; i++)
	{
		m_shape_point_vec[i] = pixels[i];
		makeBoundingBox(pixels[i].x, pixels[i].y);
	}
	this->m_rect_type = type;
}

Rectf CTransRect::getBoundingBox()
{
	return m_bounding_box;
}

void CTransRect::getBoundingBox(f32 & x1, f32 & y1, f32 & x2, f32 & y2)
{
	x1 = m_bounding_box.x1; x2 = m_bounding_box.x2;
	y1 = m_bounding_box.y1; y2 = m_bounding_box.y2;
}

Rectf CTransRect::getWorldBoundingBox(Rectf * pbox)
{
	vector2df pt[4];
	pt[0] = vector2df(pbox->x1, pbox->y1);
	pt[1] = vector2df(pbox->x1, pbox->y2);
	pt[2] = vector2df(pbox->x2, pbox->y2);
	pt[3] = vector2df(pbox->x2, pbox->y1);

	invTransform(pt[0]);
	invTransform(pt[1]);
	invTransform(pt[2]);
	invTransform(pt[3]);

	Rectf rt(PO_MAXINT, PO_MAXINT, 0, 0);
	for (i32 i = 0; i < 4; i++)
	{
		rt.x1 = po_min(rt.x1, pt[i].x);
		rt.y1 = po_min(rt.y1, pt[i].y);
		rt.x2 = po_max(rt.x2, pt[i].x);
		rt.y2 = po_max(rt.y2, pt[i].y);
	}
	return rt;
}

Rectf CTransRect::getWorldBoundingBox()
{
	if (m_rect_type < kTransTypePolygon)
	{
		return getWorldBoundingBox(&m_bounding_box);
	}

	// case transrect's type is polygon
	Rectf range;
	f32 px, py;
	i32 i, count = (i32)m_shape_point_vec.size();
	for (i = 0; i < count; i++)
	{
		px = m_shape_point_vec[i].x;
		py = m_shape_point_vec[i].y;
		invTransform(px, py);

		range.insertPoint(px, py);
	}
	return range;
}

void CTransRect::getWorldRotationBox(vector2df & x1, vector2df & x2, vector2df & x3, vector2df & x4)
{
	x1 = vector2df(m_bounding_box.x1, m_bounding_box.y1);
	x2 = vector2df(m_bounding_box.x1, m_bounding_box.y2);
	x3 = vector2df(m_bounding_box.x2, m_bounding_box.y2);
	x4 = vector2df(m_bounding_box.x2, m_bounding_box.y1);

	invTransform(x1);
	invTransform(x2);
	invTransform(x3);
	invTransform(x4);
}

void CTransRect::makeBoundingBox(f32 x, f32 y)
{
	if (m_bounding_box.x1 == 0 && m_bounding_box.x2 == 0 && m_bounding_box.y1 == 0 && m_bounding_box.y2 == 0)
	{
		m_bounding_box.x1 = x;  m_bounding_box.x2 = x;
		m_bounding_box.y1 = y;  m_bounding_box.y2 = y;
		return;
	}
	m_bounding_box.x1 = po_min(m_bounding_box.x1, x); m_bounding_box.y1 = po_min(m_bounding_box.y1, y);
	m_bounding_box.x2 = po_max(m_bounding_box.x2, x); m_bounding_box.y2 = po_max(m_bounding_box.y2, y);
}

void CTransRect::setBoundingType(i32 type)
{
	this->m_rect_type = type;
}

i32 CTransRect::getRegionType()
{
	return m_rect_type;
}

i32 CTransRect::getPtCount()
{
	switch (m_rect_type)
	{
		case kTransTypeRect:	return 4;
		case kTransTypeCircle:	return 2;
		case kTransTypeEllipse:	return 3;
		case kTransTypePolygon:	return (i32)m_shape_point_vec.size();
	}
	return 0;
}

bool CTransRect::fileRead(FILE * fp)
{
	if (!CPOBase::fileSignRead(fp, PO_SIGN_CODE))
	{
		return false;
	}
	if (!CTransform::fileRead(fp))
	{
		return false;
	}

	CPOBase::fileRead(m_rect_type, fp);
	CPOBase::fileRead(m_bounding_box, fp);
	CPOBase::fileReadVector(m_shape_point_vec, fp);
	return CPOBase::fileSignRead(fp, PO_SIGN_ENDCODE);
}

bool CTransRect::fileWrite(FILE * fp)
{
	CPOBase::fileSignWrite(fp, PO_SIGN_CODE);
	CTransform::fileWrite(fp);

	CPOBase::fileWrite(m_rect_type, fp);
	CPOBase::fileWrite(m_bounding_box, fp);
	CPOBase::fileWriteVector(m_shape_point_vec, fp);
	CPOBase::fileSignWrite(fp, PO_SIGN_ENDCODE);
	return true;
}

void CTransRect::setValue(CTransRect * other_ptr)
{
	m_rect_type = other_ptr->m_rect_type;
	m_bounding_box = other_ptr->m_bounding_box;
	m_shape_point_vec = other_ptr->m_shape_point_vec;
	CTransform::setValue((CTransform*)other_ptr);
}

i32 CTransRect::memSize()
{
	i32 len = 0;
	len += sizeof(m_rect_type);
	len += sizeof(m_bounding_box);
	len += CPOBase::getVectorMemSize(m_shape_point_vec);
	len += CTransform::memSize();
	return len;
}

i32 CTransRect::memWrite(u8 * &buffer_ptr, i32 & buffer_size)
{
	u8* buffer_pos = buffer_ptr;
	CTransform::memWrite(buffer_ptr, buffer_size);

	CPOBase::memWrite(m_rect_type, buffer_ptr, buffer_size);
	CPOBase::memWrite(m_bounding_box, buffer_ptr, buffer_size);
	CPOBase::memWriteVector(m_shape_point_vec, buffer_ptr, buffer_size);
	return buffer_ptr - buffer_pos;
}

i32 CTransRect::memRead(u8 * &buffer_ptr, i32 & buffer_size)
{
	u8* buffer_pos = buffer_ptr;
	CTransform::memRead(buffer_ptr, buffer_size);

	CPOBase::memRead(m_rect_type, buffer_ptr, buffer_size);
	CPOBase::memRead(m_bounding_box, buffer_ptr, buffer_size);
	CPOBase::memReadVector(m_shape_point_vec, buffer_ptr, buffer_size);
	return buffer_ptr - buffer_pos;
}

//////////////////////////////////////////////////////////////////////////
CheckedResult::CheckedResult()
{
	init();
}

void CheckedResult::init()
{
	chk_ok_count = 0;
	chk_ng_count = 0;
	chk_fail_count = 0;
	chk_unk_count = 0;
}

void CheckedResult::updateCheckResult(i32 result)
{
	switch (result)
	{
		case kMoldResultPassed:
		{
			chk_ok_count++;
			break;
		}
		case kMoldResultNG:
		{
			chk_ng_count++;
			break;
		}
		case kMoldResultFail:
		{
			chk_fail_count++;
			break;
		}
		case kMoldResultNone:
		default:
		{
			chk_unk_count++;
			break;
		}
	}
}

CheckedResult& CheckedResult::operator+=(const CheckedResult& other)
{
	chk_ok_count += other.chk_ok_count;
	chk_ng_count += other.chk_ng_count;
	chk_fail_count += other.chk_fail_count;
	chk_unk_count += other.chk_unk_count;
	return *this;
}

//////////////////////////////////////////////////////////////////////////
ChkTimerResult::ChkTimerResult()
{
	init();
}

void ChkTimerResult::init()
{
	chk_count = 0;
	cur_proc_time = 0;
	avg_proc_time = 0;
	min_proc_time = 1E+10;
	max_proc_time = 0;
}

void ChkTimerResult::updateChkTimeResult(f32 ptime)
{
	cur_proc_time = ptime;
	avg_proc_time = (avg_proc_time * chk_count + cur_proc_time) / (chk_count + 1);
	min_proc_time = po_min(min_proc_time, ptime);
	max_proc_time = po_max(max_proc_time, ptime);
	chk_count++;
}

ChkTimerResult & ChkTimerResult::operator+=(const ChkTimerResult & other)
{
	if (chk_count + other.chk_count <= 0)
	{
		return *this;
	}

	cur_proc_time = other.cur_proc_time;
	avg_proc_time = (avg_proc_time * chk_count + other.avg_proc_time * other.chk_count) / (chk_count + other.chk_count);
	min_proc_time = po_min(min_proc_time, other.min_proc_time);
	max_proc_time = po_max(max_proc_time, other.max_proc_time);
	chk_count += other.chk_count;
	return *this;
}

//////////////////////////////////////////////////////////////////////////
RuntimeHistory::RuntimeHistory()
{
	init();
}

void RuntimeHistory::init()
{
	snap_total_count = 0;
	boot_count = 0;
	acc_up_time = 0;
	backup_acc_up_time = 0;
	cur_up_time = 0;

	initRuntime();
}

void RuntimeHistory::initRuntime()
{
	snap_count = 0;
	snap_ok_count = 0;
	snap_ng_count = 0;
	acc_result = kMoldResultNone;

	i32 i, j;
	for (i = 0; i < kMVStepCount; i++)
	{
		for (j = 0; j < PO_CAM_COUNT; j++)
		{
			checked_result[i][j].init();
			checked_proc_time[i][j].init();
		}
	}
}

void RuntimeHistory::setAccResult(i32 result)
{
	acc_result = po_min(acc_result, result);
}

i32	RuntimeHistory::memSize(i32 stp)
{
	i32 len = 0;
	len += sizeof(snap_total_count);
	len += sizeof(boot_count);
	len += sizeof(acc_up_time);
	len += sizeof(snap_count);
	len += sizeof(snap_ok_count);
	len += sizeof(snap_ng_count);
	len += sizeof(cur_up_time);

	len += sizeof(stp);
	if (CPOBase::checkIndex(stp, kMVStepCount))
	{
		len += sizeof(CheckedResult) * PO_CAM_COUNT;	//array size
		len += sizeof(ChkTimerResult) * PO_CAM_COUNT;	//array size
	}
	else
	{
		len += sizeof(CheckedResult) * kMVStepCount * PO_CAM_COUNT;	 //array size
		len += sizeof(ChkTimerResult) * kMVStepCount * PO_CAM_COUNT; //array size
	}
	return len;
}

void RuntimeHistory::memWrite(u8*& buffer_ptr, i32& buffer_size, i32 stp)
{
	CPOBase::memWrite(snap_total_count, buffer_ptr, buffer_size);
	CPOBase::memWrite(boot_count, buffer_ptr, buffer_size);
	CPOBase::memWrite(acc_up_time, buffer_ptr, buffer_size);

	CPOBase::memWrite(snap_count, buffer_ptr, buffer_size);
	CPOBase::memWrite(snap_ok_count, buffer_ptr, buffer_size);
	CPOBase::memWrite(snap_ng_count, buffer_ptr, buffer_size);
	CPOBase::memWrite(cur_up_time, buffer_ptr, buffer_size);

	CPOBase::memWrite(stp, buffer_ptr, buffer_size);
	if (CPOBase::checkIndex(stp, kMVStepCount))
	{
		CPOBase::memWrite(checked_result[stp], PO_CAM_COUNT, buffer_ptr, buffer_size);
		CPOBase::memWrite(checked_proc_time[stp], PO_CAM_COUNT, buffer_ptr, buffer_size);
	}
	else
	{
		for (i32 i = 0; i < kMVStepCount; i++)
		{
			CPOBase::memWrite(checked_result[i], PO_CAM_COUNT, buffer_ptr, buffer_size);
			CPOBase::memWrite(checked_proc_time[i], PO_CAM_COUNT, buffer_ptr, buffer_size);
		}
	}
}

bool RuntimeHistory::fileRead(FILE* fp)
{
	if (!CPOBase::fileSignRead(fp, PO_SIGN_CODE))
	{
		printlog_lv1("The RUNTIMEHISTORY start-sign is uncorrectly in setting.dat");
		return false;
	}

	CPOBase::fileRead(snap_total_count, fp);
	CPOBase::fileRead(boot_count, fp);
	CPOBase::fileRead(backup_acc_up_time, fp);

	boot_count++;
	acc_up_time = backup_acc_up_time;

	if (!CPOBase::fileSignRead(fp, PO_SIGN_ENDCODE))
	{
		printlog_lv1("The RUNTIMEHISTORY end-sign is uncorrectly in setting.dat");
		return false;
	}
	return true;
}

bool RuntimeHistory::fileWrite(FILE* fp)
{
	CPOBase::fileSignWrite(fp, PO_SIGN_CODE);

	CPOBase::fileWrite(snap_total_count, fp);
	CPOBase::fileWrite(boot_count, fp);
	CPOBase::fileWrite(acc_up_time, fp);

	CPOBase::fileSignWrite(fp, PO_SIGN_ENDCODE);
	return true;
}
//////////////////////////////////////////////////////////////////////////
CMoldSnapBase::CMoldSnapBase()
{
	initBuffer();
}

CMoldSnapBase::~CMoldSnapBase()
{
	freeBuffer();
}

void CMoldSnapBase::freeBuffer()
{
	POSAFE_CLEAR(m_roi_tr);
	POSAFE_CLEAR(m_mask_tr);
}

void CMoldSnapBase::initBuffer()
{
	freeBuffer();
}

i32	CMoldSnapBase::memSize()
{
	i32 len = 0;

	//ROI Plus Regions
	i32 i, count = (i32)m_roi_tr.size();
	for (i = 0; i < count; i++)
	{
		len += m_roi_tr[i]->memSize();
	}
	len += sizeof(count);

	//ROI Minus Regions
	count = (i32)m_mask_tr.size();
	for (i = 0; i < count; i++)
	{
		len += m_mask_tr[i]->memSize();
	}
	len += sizeof(count);
	return len;
}

i32 CMoldSnapBase::memWrite(u8*& buffer_ptr, i32& buffer_size)
{
	u8* buffer_pos = buffer_ptr;

	//ROI Plus Regions
	i32 i, count = (i32)m_roi_tr.size();
	CPOBase::memWrite(count, buffer_ptr, buffer_size);
	if (CPOBase::isCount(count))
	{
		for (i = 0; i < count; i++)
		{
			m_roi_tr[i]->memWrite(buffer_ptr, buffer_size);
		}
	}

	//ROI Mask Regions
	count = (i32)m_mask_tr.size();
	CPOBase::memWrite(count, buffer_ptr, buffer_size);
	if (CPOBase::isCount(count))
	{
		for (i = 0; i < count; i++)
		{
			m_mask_tr[i]->memWrite(buffer_ptr, buffer_size);
		}
	}
	return buffer_ptr - buffer_pos;
}

i32 CMoldSnapBase::memRead(u8*& buffer_ptr, i32& buffer_size)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////
ROIData::ROIData()
{
}

ROIData::~ROIData()
{
}

void ROIData::setValue(ROIData* other_ptr)
{
	if (other_ptr)
	{
		CPOBase::memCopy(&roi_param, other_ptr->getMoldParam());
		roi_tr.setValue(other_ptr->getTr());
	}
}

i32	ROIData::memSize()
{
	i32 len = 0;
	len += roi_param.memSize();
	len += roi_tr.memSize();
	return len;
}

i32	ROIData::memWrite(u8*& buffer_ptr, i32& buffer_size)
{
	u8* buffer_pos = buffer_ptr;

	roi_param.memWrite(buffer_ptr, buffer_size);
	roi_tr.memWrite(buffer_ptr, buffer_size);
	return buffer_ptr - buffer_pos;
}

i32	ROIData::memRead(u8*& buffer_ptr, i32& buffer_size)
{
	u8* buffer_pos = buffer_ptr;

	roi_param.memRead(buffer_ptr, buffer_size);
	roi_tr.memRead(buffer_ptr, buffer_size);
	return buffer_ptr - buffer_pos;
}

bool ROIData::fileWrite(FILE* fp)
{
	CPOBase::fileSignWrite(fp, PO_SIGN_CODE);
	CPOBase::fileWrite(roi_param, fp);
	roi_tr.fileWrite(fp);
	CPOBase::fileSignWrite(fp, PO_SIGN_ENDCODE);
	return true;
}

bool ROIData::fileRead(FILE* fp)
{
	if (!CPOBase::fileSignRead(fp, PO_SIGN_CODE))
	{
		return false;
	}

	CPOBase::fileRead(roi_param, fp);
	if (!roi_tr.fileRead(fp))
	{
		return false;
	}
	return CPOBase::fileSignRead(fp, PO_SIGN_ENDCODE);
}

//////////////////////////////////////////////////////////////////////////
CROIMaskData::CROIMaskData()
{
	m_range.reset();
	m_area = 0;
	m_mask_img_ptr = NULL;

	m_autochk_mask_ptr = NULL;
}

CROIMaskData::~CROIMaskData()
{
	freeBuffer();
}

void CROIMaskData::initBuffer(Recti range)
{
	freeBuffer();
	
	m_range = range;
	m_area = 0;

	i32 size = m_range.getArea();
	m_mask_img_ptr = new u8[size];
	memset(m_mask_img_ptr, 0, size);
}

void CROIMaskData::freeBuffer()
{
	m_range.reset();
	m_area = 0;
	POSAFE_DELETE_ARRAY(m_mask_img_ptr);
	POSAFE_DELETE_ARRAY(m_autochk_mask_ptr);
}

void CROIMaskData::initAutoCheckBuffer()
{
	POSAFE_DELETE_ARRAY(m_autochk_mask_ptr);

	i32 size = m_range.getArea();
	m_autochk_mask_ptr = new u8[size];
	memset(m_autochk_mask_ptr, 0, size);
}

bool CROIMaskData::fileWrite(FILE* fp)
{
	CPOBase::fileSignWrite(fp, PO_SIGN_CODE);
	CPOBase::fileWrite(m_range, fp);
	CPOBase::fileWrite(m_area, fp);

	i32 size = m_range.getArea();
	if (CPOBase::isPositive(size))
	{
		CPOBase::fileWrite(m_mask_img_ptr, size, fp);
	}
	CPOBase::fileSignWrite(fp, PO_SIGN_ENDCODE);
	return true;
}

bool CROIMaskData::fileRead(FILE* fp)
{
	if (!CPOBase::fileSignRead(fp, PO_SIGN_CODE))
	{
		return false;
	}

	freeBuffer();
	CPOBase::fileRead(m_range, fp);
	CPOBase::fileRead(m_area, fp);
	
	i32 size = m_range.getArea();
	if (CPOBase::isPositive(size))
	{
		m_mask_img_ptr = new u8[size];
		CPOBase::fileRead(m_mask_img_ptr, size, fp);
	}
	return CPOBase::fileSignRead(fp, PO_SIGN_ENDCODE);
}

u8* CROIMaskData::getMaskImage(bool use_autochk_mask)
{
	if (use_autochk_mask && m_autochk_mask_ptr)
	{
		return m_autochk_mask_ptr;
	}
	return m_mask_img_ptr;
}

//////////////////////////////////////////////////////////////////////////
CAutoCheckBase::CAutoCheckBase()
{
	initBuffer();
}

CAutoCheckBase::~CAutoCheckBase()
{
	freeBuffer();
}

void CAutoCheckBase::initBuffer()
{
	freeBuffer();
}

void CAutoCheckBase::freeBuffer()
{
	m_range.reset();
	m_auto_region_vec.clear();
}

i32 CAutoCheckBase::memSize()
{
	i32 len = 0;
	len += sizeof(m_range);

	i32 i, region_count = (i32)m_auto_region_vec.size();
	for (i = 0; i < region_count; i++)
	{
		len += CPOBase::getVectorMemSize(m_auto_region_vec[i]);
	}
	len += sizeof(region_count);
	return len;
}

i32 CAutoCheckBase::memRead(u8*& buffer_ptr, i32& buffer_size)
{
	freeBuffer();
	u8* buffer_pos = buffer_ptr;

	i32 i, region_count = -1;
	CPOBase::memRead(m_range, buffer_ptr, buffer_size);
	CPOBase::memRead(region_count, buffer_ptr, buffer_size);
	if (!CPOBase::isCount(region_count))
	{
		return buffer_ptr - buffer_pos;
	}

	m_auto_region_vec.resize(region_count);
	for (i = 0; i < region_count; i++)
	{
		CPOBase::memReadVector(m_auto_region_vec[i], buffer_ptr, buffer_size);
	}

	return buffer_ptr - buffer_pos;
}

i32 CAutoCheckBase::memWrite(u8*& buffer_ptr, i32& buffer_size)
{
	u8* buffer_pos = buffer_ptr;
	i32 i, region_count = (i32)m_auto_region_vec.size();
	CPOBase::memWrite(m_range, buffer_ptr, buffer_size);
	CPOBase::memWrite(region_count, buffer_ptr, buffer_size);

	for (i = 0; i < region_count; i++)
	{
		CPOBase::memWriteVector(m_auto_region_vec[i], buffer_ptr, buffer_size);
	}
	return buffer_ptr - buffer_pos;
}

//////////////////////////////////////////////////////////////////////////
IOInfo::IOInfo()
{
	init();
}

IOInfo::~IOInfo()
{
}

void IOInfo::init()
{
	scheme_count = 1;
	scheme_index = 0;
	for (i32 i = 0; i < IOOUT_SCHEMENUM; i++)
	{
		scheme_name[i] = "";
	}

	initIOAddress();
	initEquipMode();
}

void IOInfo::initIOAddress()
{
	in_count = MVIO_MAXIN;
	memset(in_address, 0xFF, sizeof(i32)*MVIO_MAXIN);
	memset(in_method, IOIN_NONE, sizeof(i32)*MVIO_MAXIN);
	memset(in_mode, MVIO_NORMALMODE, MVIO_MAXIN);

	out_count = MVIO_MAXOUT;
	memset(out_address, 0xFF, sizeof(i32)*MVIO_MAXIN);
	memset(out_method, IOOUT_NONE, sizeof(i32)*MVIO_MAXOUT);
	memset(out_mode, MVIO_NORMALMODE, MVIO_MAXOUT);
}

void IOInfo::initEquipMode()
{
	if (CPOBase::checkIndex(scheme_index, IOOUT_SCHEMENUM))
	{
		scheme_index = 0;
		scheme_name[scheme_index] = "default";
		memset(equip_scheme, EQIO_OFFMODE, IOOUT_SCHEMENUM*IOOUT_EQUIP*EQCTRL_COUNT);
	}
}

i32 IOInfo::getIOInAddress(i32 method)
{
	for (i32 i = 0; i < in_count; i++)
	{
		if (in_method[i] == method)
		{
			return in_address[i];
		}
	}
	return -1;
}

void IOInfo::setIOInAddress(i32 method, i32 addr)
{
	i32 i, index = -1;
	if (!CPOBase::checkIndex(addr, PO_MVIO_MAXINADDR) || !CPOBase::checkIndex(method, IOIN_COUNT))
	{
		return;
	}

	//check current IO/InAddress table
	for (i = 0; i < in_count; i++)
	{
		if (in_address[i] == addr)
		{
			index = i;
			break;
		}
	}

	//check empty IO/InAddress slot, when addr hasn't slot
	if (index < 0)
	{
		for (i = 0; i < in_count; i++)
		{
			if (in_address[i] < 0)
			{
				index = i;
				break;
			}
		}
	}

	if (CPOBase::checkIndex(index, MVIO_MAXIN))
	{
		in_address[index] = addr;
		in_method[index] = method;
	}
}

i32 IOInfo::getIOOutAddress(i32 method)
{
	for (i32 i = 0; i < in_count; i++)
	{
		if (out_method[i] == method)
		{
			return out_address[i];
		}
	}
	return -1;
}

void IOInfo::setIOOutAddress(i32 method, i32 addr)
{
	i32 i, index = -1;
	if (!CPOBase::checkIndex(addr, PO_MVIO_MAXOUTADDR) || !CPOBase::checkIndex(method, IOOUT_COUNT))
	{
		return;
	}

	//check current IO/InAddress table
	for (i = 0; i < out_count; i++)
	{
		if (out_address[i] == addr)
		{
			index = i;
			break;
		}
	}

	//find empty IO/InAddress slot, when addr hasn't slot
	if (index < 0)
	{
		for (i = 0; i < out_count; i++)
		{
			if (out_address[i] < 0)
			{
				index = i;
				break;
			}
		}
	}

	if (CPOBase::checkIndex(index, MVIO_MAXOUT))
	{
		out_address[index] = addr;
		out_method[index] = method;
	}
}

bool IOInfo::setEquipMode(i32 device_event, i32 out_signal, i32 allow_mode)
{
	if (!CPOBase::checkIndex(scheme_index, IOOUT_SCHEMENUM) ||
		!CPOBase::checkIndex(device_event, EQCTRL_COUNT) || !CPOBase::checkIndex(out_signal, IOOUT_EQUIP))
	{
		return false;
	}

	u8* equip_ctrl_ptr = equip_scheme + scheme_index*(EQCTRL_COUNT*IOOUT_EQUIP) + device_event*IOOUT_EQUIP;
	if (equip_ctrl_ptr[out_signal] != allow_mode)
	{
		equip_ctrl_ptr[out_signal] = allow_mode;
		return true;
	}
	return false;
}

u8 IOInfo::getEquipMode(i32 device_event, i32  out_signal)
{
	if (!CPOBase::checkIndex(scheme_index, IOOUT_SCHEMENUM) ||
		!CPOBase::checkIndex(device_event, EQCTRL_COUNT) || !CPOBase::checkIndex(out_signal, IOOUT_EQUIP))
	{
		return EQIO_KEEPMODE;
	}

	u8* equip_ctrl_ptr = equip_scheme + scheme_index*(EQCTRL_COUNT*IOOUT_EQUIP) + device_event*IOOUT_EQUIP;
	return equip_ctrl_ptr[out_signal];
}

i32 IOInfo::memSize()
{
	i32 len = 0;
	len += sizeof(in_count);
	len += sizeof(in_method);
	len += sizeof(in_mode);
	len += sizeof(out_count);
	len += sizeof(out_method);
	len += sizeof(out_mode);

	len += sizeof(scheme_count);
	len += sizeof(scheme_index);
	for (i32 i = 0; i < scheme_count; i++)
	{
		len += CPOBase::getStringMemSize(scheme_name[i]);
	}
	len += scheme_count*IOOUT_EQUIP*EQCTRL_COUNT;
	return len;
}

i32 IOInfo::memWrite(u8*& buffer_ptr, i32& buffer_size)
{
	u8* buffer_pos = buffer_ptr;

	CPOBase::memWrite(in_count, buffer_ptr, buffer_size);
	CPOBase::memWrite(in_method, in_count, buffer_ptr, buffer_size);
	CPOBase::memWrite(in_mode, in_count, buffer_ptr, buffer_size);

	CPOBase::memWrite(out_count, buffer_ptr, buffer_size);
	CPOBase::memWrite(out_method, out_count, buffer_ptr, buffer_size);
	CPOBase::memWrite(out_mode, out_count, buffer_ptr, buffer_size);

	CPOBase::memWrite(scheme_count, buffer_ptr, buffer_size);
	CPOBase::memWrite(scheme_index, buffer_ptr, buffer_size);

	for (i32 i = 0; i < scheme_count; i++)
	{
		CPOBase::memWrite(buffer_ptr, buffer_size, scheme_name[i]);
	}
	CPOBase::memWrite(equip_scheme, scheme_count*IOOUT_EQUIP*EQCTRL_COUNT, buffer_ptr, buffer_size);
	return buffer_ptr - buffer_pos;
}

i32 IOInfo::memRead(u8*& buffer_ptr, i32& buffer_size)
{
	u8* buffer_pos = buffer_ptr;

	CPOBase::memRead(in_count, buffer_ptr, buffer_size);
	CPOBase::memRead(in_method, in_count, buffer_ptr, buffer_size);
	CPOBase::memRead(in_mode, in_count, buffer_ptr, buffer_size);

	CPOBase::memRead(out_count, buffer_ptr, buffer_size);
	CPOBase::memRead(out_method, out_count, buffer_ptr, buffer_size);
	CPOBase::memRead(out_mode, out_count, buffer_ptr, buffer_size);

	CPOBase::memRead(scheme_count, buffer_ptr, buffer_size);
	CPOBase::memRead(scheme_index, buffer_ptr, buffer_size);

	for (i32 i = 0; i < scheme_count; i++)
	{
		CPOBase::memRead(buffer_ptr, buffer_size, scheme_name[i]);
	}
	CPOBase::memRead(equip_scheme, scheme_count*IOOUT_EQUIP*EQCTRL_COUNT, buffer_ptr, buffer_size);
	return buffer_ptr - buffer_pos;
}

bool IOInfo::fileRead(FILE* fp)
{
    if (!CPOBase::fileSignRead(fp, PO_SIGN_CODE))
	{
		printlog_lv1("The IOINFO start-sign is uncorrectly in setting.dat");
		return false;
	}

	CPOBase::fileRead(scheme_count, fp);
	CPOBase::fileRead(scheme_index, fp);

	for (i32 i = 0; i < scheme_count; i++)
	{
		CPOBase::fileRead(fp, scheme_name[i]);
	}
	CPOBase::fileRead(equip_scheme, IOOUT_SCHEMENUM*IOOUT_EQUIP*EQCTRL_COUNT, fp);

    if (!CPOBase::fileSignRead(fp, PO_SIGN_ENDCODE))
	{
		printlog_lv1("The IOINFO end-sign is uncorrectly in setting.dat");
		return false;
	}
	return true;
}

bool IOInfo::fileWrite(FILE* fp)
{
    CPOBase::fileSignWrite(fp, PO_SIGN_CODE);

	CPOBase::fileWrite(scheme_count, fp);
	CPOBase::fileWrite(scheme_index, fp);

	for (i32 i = 0; i < scheme_count; i++)
	{
		CPOBase::fileWrite(fp, scheme_name[i]);
	}
	CPOBase::fileWrite(equip_scheme, IOOUT_SCHEMENUM*IOOUT_EQUIP*EQCTRL_COUNT, fp);
    CPOBase::fileSignWrite(fp, PO_SIGN_ENDCODE);
	return true;
}

u8* IOInfo::getEquipmentControl(i32 stp)
{
	if (!CPOBase::checkIndex(scheme_index, scheme_count))
	{
		return NULL;
	}
	return equip_scheme + scheme_index*(IOOUT_EQUIP*EQCTRL_COUNT) + stp*IOOUT_EQUIP;
}

