#include "roi_processor.h"
#include "ssim_processor.h"
#include "mv_base.h"
#include "proc/image_proc.h"
#include "logger/logger.h"

#ifdef POR_SUPPORT_TIOPENCV
#include "proc/connected_components.h"
#endif

#if defined(POR_WITH_OVX)
#include "openvx/mv_graph_pool.h"
#elif defined(POR_WITH_OCLCV)
#include <opencv2/core/ocl.hpp>
#endif

//#define POR_TESTMODE

CROIProcessor::CROIProcessor()
{
	m_index = -1;

	m_roi_data_ptr = NULL;
	m_roi_param_ptr = NULL;
	m_engine_param_ptr = NULL;

	initLKPairs(0);
	m_image.freeBuffer();
}

CROIProcessor::~CROIProcessor()
{
	exitInstance();
}

void CROIProcessor::initInstance(i32 index, EngineParam* engine_param_ptr)
{
	m_index = index;

	m_roi_data_ptr = NULL;
	m_engine_param_ptr = engine_param_ptr;

	initLKPairs(0);
	m_image.freeBuffer();
	m_ssim_processor.initInstance(engine_param_ptr);
}

void CROIProcessor::exitInstance()
{
	freeLKPairs();
	m_image.freeBuffer();
	m_ssim_processor.exitInstance();
}

void CROIProcessor::setBasedROIData(CMoldROIData* roi_data_ptr, Img* img_ptr, DefectParam* mold_param_ptr)
{ 
	//cache ROIData
	if (roi_data_ptr)
	{
		m_roi_data_ptr = roi_data_ptr;
	}
	//cache MoldParam
	if (mold_param_ptr)
	{
		m_roi_param_ptr = mold_param_ptr;
	}
	//cache SnapImage
	if (img_ptr)
	{
		m_image = Img(img_ptr->img_ptr, img_ptr->w, img_ptr->h);
	}
}

void CROIProcessor::initLKPairs(i32 count)
{
	m_lk_pair_count = 0;
	m_lk_lpoint_vec.resize(count);
	m_lk_rpoint_vec.resize(count);
}

void CROIProcessor::freeLKPairs()
{
	m_lk_lpoint_vec.clear();
	m_lk_rpoint_vec.clear();
}

void CROIProcessor::collectCornerPoints(CMoldROIData* roi_model_ptr)
{
	if (!roi_model_ptr)
	{
		return;
	}

	ptfvector& point_vec = roi_model_ptr->m_corner_vec;
	i32 i, count = (i32)point_vec.size();
	i32 x0 = roi_model_ptr->m_range.x1;
	i32 y0 = roi_model_ptr->m_range.y1;

	for (i = 0; i < count; i++)
	{
		m_lk_lpoint_vec[m_lk_pair_count].x = point_vec[i].x + x0;
		m_lk_lpoint_vec[m_lk_pair_count].y = point_vec[i].y + y0;
		m_lk_pair_count++;
	}
}

i32	CROIProcessor::makeMaskImageInROI(CMoldSnapData* mold_snap_ptr)
{
	if (!mold_snap_ptr)
	{
		return kEngineErrInvalidData;
	}

	ROIPVector& roi_vec = mold_snap_ptr->m_roi_tr;
	TransRectPVector& mask_vec = mold_snap_ptr->m_mask_tr;
	ROIMaskPVector& roi_mask_vec = mold_snap_ptr->m_roi_mask_vec;
	POSAFE_CLEAR(roi_mask_vec);

	i32 area = 0;
	i32 i, roi_count = (i32)roi_vec.size();
	mold_snap_ptr->m_roi_area = 0;
	if (!CPOBase::isPositive(roi_count))
	{
		kEngineResultOK;
	}

	Recti roi_range, cross_range;
	i32 j, mask_count = (i32)mask_vec.size();
	for (i = 0; i < roi_count; i++)
	{
		CTransRect* cur_roi_ptr = roi_vec[i]->getTr();
		Rectf rt = cur_roi_ptr->getWorldBoundingBox();
		roi_range.x1 = (i32)po_max(rt.x1, 0);
		roi_range.y1 = (i32)po_max(rt.y1, 0);
		roi_range.x2 = (i32)po_max(rt.x2 + 0.5f, 0);
		roi_range.y2 = (i32)po_max(rt.y2 + 0.5f, 0);

		CROIMaskData* roi_mask_ptr = CPOBase::pushBackNew(roi_mask_vec);
		roi_mask_ptr->initBuffer(roi_range);

		for (j = 0; j < mask_count; j++)
		{
			CTransRect* cur_mask_ptr = mask_vec[j];
			cross_range = roi_range.intersectRect(cur_mask_ptr->getWorldBoundingBox());
			if (cross_range.isEmpty())
			{
				continue;
			}

			bool is_processed = false;
#if defined(POR_IMVS2_ON_NVIDIA)
			CGMakeROIRegion* region_graph_ptr = (CGMakeROIRegion*)g_vx_graph_pool.fetchGraph(kMGMakeROIRegion,
					roi_mask_ptr, cross_range, cur_mask_ptr, MV_REGION_MASKMINUS, MV_REGION_MASKPLUS);
			if (region_graph_ptr)
			{
				is_processed = region_graph_ptr->process();
				g_vx_graph_pool.releaseGraph(region_graph_ptr);
			}
#endif
			if (!is_processed)
			{
				updateRegionMap(roi_mask_ptr->m_mask_img_ptr, roi_range, cross_range, cur_mask_ptr,
							MV_REGION_MASKMINUS, MV_REGION_MASKPLUS);
			}
		}

		bool is_processed = false;
#if defined(POR_IMVS2_ON_NVIDIA)
		CGMakeROIRegion* region_graph_ptr = (CGMakeROIRegion*)g_vx_graph_pool.fetchGraph(kMGMakeROIRegion,
				roi_mask_ptr, roi_range, cur_roi_ptr, MV_REGION_MASKPLUS, MV_REGION_MASKMINUS);
		if (region_graph_ptr)
		{
			is_processed = region_graph_ptr->process();
			g_vx_graph_pool.releaseGraph(region_graph_ptr);
		}
#endif
		if (!is_processed)
		{
			roi_mask_ptr->m_area = updateRegionMap(roi_mask_ptr->m_mask_img_ptr, roi_range, roi_range, cur_roi_ptr,
												MV_REGION_MASKPLUS, MV_REGION_MASKMINUS);
		}
		area += roi_mask_ptr->m_area;

#ifdef POR_TESTMODE
		i32 rw = roi_range.getWidth();
		i32 rh = roi_range.getHeight();
		CImageProc::saveBinImgOpenCV(PO_DEBUG_PATH"mask.bmp", roi_mask_ptr->m_mask_img_ptr, rw, rh);
#endif
	}
	mold_snap_ptr->m_roi_area = area;
	return kEngineResultOK;
}

void CROIProcessor::updateAutoCheckMaskImage(CMoldSnapData* mold_snap_ptr, CAutoCheckData* autochk_data_ptr)
{
	if (!mold_snap_ptr || !autochk_data_ptr)
	{
		return;
	}
	if (!autochk_data_ptr->isValid())
	{
		return;
	}

	Recti autochk_range = autochk_data_ptr->m_range;
	u8* autochk_img_ptr = autochk_data_ptr->m_auto_mask_ptr;

	ROIMaskPVector& roi_mask_vec = mold_snap_ptr->getROIMaskPVector();
	i32 i, roi_count = (i32)roi_mask_vec.size();

	for (i = 0; i < roi_count; i++)
	{
		CROIMaskData* roi_mask_ptr = roi_mask_vec[i];
		Recti roi_range = roi_mask_ptr->m_range;
		roi_mask_ptr->initAutoCheckBuffer();
		
		/* 자동검사구역자료로 마스크를 갱신한다. */
		Recti rt = roi_range.intersectRect(autochk_range);
		if (!rt.isEmpty())
		{
			u8* tmp_mask_img_ptr;
			u8* tmp_autochk_mask_ptr;
			u8* tmp_autochk_img_ptr;
			i32 x1 = rt.x1 - roi_range.x1;
			i32 y1 = rt.y1 - roi_range.y1;
			i32 x2 = rt.x1 - autochk_range.x1;
			i32 y2 = rt.y1 - autochk_range.y1;

			i32 x, y, w, h, w1, w2;
			w = rt.getWidth();
			h = rt.getHeight();
			w1 = roi_range.getWidth();
			w2 = autochk_range.getWidth();

			for (y = 0; y < h; y++)
			{
				tmp_mask_img_ptr = roi_mask_ptr->m_mask_img_ptr + (y + y1)*w1 + x1;
				tmp_autochk_img_ptr = autochk_img_ptr + (y + y2)*w2 + x2;
				tmp_autochk_mask_ptr = roi_mask_ptr->m_autochk_mask_ptr + (y + y1)*w1 + x1;
				for (x = 0; x < w; x++)
				{
					if (*tmp_autochk_img_ptr == MV_REGION_MASKPLUS && *tmp_mask_img_ptr == MV_REGION_MASKPLUS)
					{
						*tmp_autochk_mask_ptr = MV_REGION_MASKMARK;
					}
					tmp_mask_img_ptr++;
					tmp_autochk_img_ptr++;
					tmp_autochk_mask_ptr++;
				}
			}
		}

		/* 마스크구역을 업데이트한다. */
		i32 w = roi_range.getWidth();
		i32 h = roi_range.getHeight();
		cv::Mat cv_autochk_mask(h, w, CV_8UC1, roi_mask_ptr->m_autochk_mask_ptr);
		cv::threshold(cv_autochk_mask, cv_autochk_mask, MV_REGION_MASKMARK - 1, MV_REGION_MASKPLUS, cv::THRESH_BINARY);
	}
}

i32 CROIProcessor::makeMaskImageInROI(CMoldROIData* roi_model_ptr, u8* mask_img_ptr, CMoldROIData* roi_data_ptr)
{
	if (!roi_data_ptr || !roi_model_ptr)
	{
		return kEngineErrInvalidData;
	}

	roi_data_ptr->initBuffer(roi_model_ptr);
	roi_data_ptr->setROIMask(mask_img_ptr);
	return kEngineResultOK;
}

i32 CROIProcessor::makeCheckDataInROI(i32 mode, i32 area)
{
	//check based-ROIData
	singlelog_lvs2("makeCheckDataInROI", LOG_SCOPE_ENGINE);
	if (!m_roi_data_ptr || !m_roi_data_ptr->getMaskImage())
	{
		return kEngineErrInvalidData;
	}

	Recti& range = m_roi_data_ptr->m_range;
	u8* roi_img_ptr = m_roi_data_ptr->getROIImage();
	u8* mask_img_ptr = m_roi_data_ptr->getMaskImage();

	//crop image
	i32 w = range.getWidth();
	i32 h = range.getHeight();
	if (CPOBase::bitCheck(mode, MV_ROIPROC_CROPIMG))
	{
		if (!m_image.isValid() || !CImageProc::cropImage(roi_img_ptr, m_image, range))
		{
			return kEngineErrInvalidImage;
		}
	}

	//get strong corners on an current frame
	if (CPOBase::bitCheck(mode, MV_ROIPROC_CORNER) && area > 0)
	{
		singlelog_lvs3("Calcualte strong corners", LOG_SCOPE_ENGINE);

		cvPtVector corner_vec;
		i32 corner_count = po::_max(m_engine_param_ptr->corner_min_count,
								m_engine_param_ptr->corner_count*m_roi_data_ptr->getROIArea() / area);
		f64 min_dist = po::_max(m_engine_param_ptr->corner_interval,
								std::sqrt(0.20f*m_roi_data_ptr->getROIArea() / corner_count));

		cv::Mat img(h, w, CV_8UC1, roi_img_ptr);
#ifdef POR_SUPPORT_TIOPENCV
        cv::Mat cv_mask_img(h, w, CV_8UC1, mask_img_ptr);
        cv::goodFeaturesToTrack(img, corner_vec, corner_count, m_engine_param_ptr->corner_quality,
                        min_dist, cv_mask_img, 3, true, 0.04f);
#else
        cv::Mat cv_mask_img(h, w, CV_8UC1, mask_img_ptr);
        cv::goodFeaturesToTrack(img, corner_vec, corner_count, m_engine_param_ptr->corner_quality,
                        min_dist, cv_mask_img, 7, 7, true, 0.04f);
#endif

		i32 i, count = (i32)corner_vec.size();
		ptfvector& point_vec = m_roi_data_ptr->m_corner_vec;
		point_vec.resize(count);
		for (i = 0; i < count; i++)
		{
			point_vec[i].x = corner_vec[i].x;
			point_vec[i].y = corner_vec[i].y;
		}

#ifdef POR_TESTMODE
		testPtsWithImg(PO_DEBUG_PATH"std.bmp", point_vec, roi_img_ptr, w, h);
#endif
	}

	//gaussian blur
	if (CPOBase::bitCheck(mode, MV_ROIPROC_NOISE))
	{
		CImageProc::gaussianBlur(roi_img_ptr, w, h, m_engine_param_ptr->blur_window);
	}

	//precalcuate ssim features for ROI similarity index
	if (m_engine_param_ptr->useSafeCheck() || mode == MV_ROIPROC_STUDY)
	{
		if (CPOBase::bitCheck(mode, MV_ROIPROC_SSIM))
		{
			singlelog_lvs3("Precalcuate ssim features", LOG_SCOPE_ENGINE);
			m_ssim_processor.prepareStructSqSimilar(m_roi_data_ptr, true, 
											CPOBase::bitCheck(mode, MV_ROIPROC_USE_MEMPOOL));
		}
	}
	return kEngineResultOK;
}

bool CROIProcessor::checkValidEstimation(f32* tr, i32 w, i32 h)
{
	if (w <= 0 || h <= 0)
	{
		return false;
	}

	f64 delta = sin(1 / std::sqrt(w*w + h*h));
	if (std::abs(tr[2]) >= 0.77f || std::abs(tr[5]) >= 0.77f)
	{
		return true;
	}
	if (std::abs(tr[0] - 1.0f) > delta || std::abs(tr[1]) > delta ||
		std::abs(tr[3]) > delta || std::abs(tr[4] - 1.0f) > delta)
	{
		return true;
	}
	return false;
}

bool CROIProcessor::estimateGlobalMotion(CMoldPiece* mold_piece_ptr, Img& cur_img, f32* tr)
{
	if (m_lk_pair_count < 4 || m_lk_lpoint_vec.size() <= 0)
	{
		return false;
	}
	if (mold_piece_ptr->m_hull_area < mold_piece_ptr->m_area * m_engine_param_ptr->lk_disable_area)
	{
		return false;
	}

	cvPtVector pts1, pts2, pts3;
	pts1.resize(m_lk_pair_count);
	pts2.resize(m_lk_pair_count);
	pts3.resize(m_lk_pair_count);

	i32 i = 0, count = 0;
	i32 wsize = m_engine_param_ptr->lk_win_size;
	i32 maxlevel = m_engine_param_ptr->lk_max_level;
	f32 eps = m_engine_param_ptr->lk_epsilon;
	f32 outlier = m_engine_param_ptr->lk_outlier;
    f32 thoffsetx = m_engine_param_ptr->lk_offset_x;
	f32 thoffsety = m_engine_param_ptr->lk_offset_y;
	f32 thscale = m_engine_param_ptr->lk_scale;
	f32 thangle = sinf(CPOBase::degToRad(m_engine_param_ptr->lk_angle));
	f32 thskew = sinf(CPOBase::degToRad(m_engine_param_ptr->lk_angle/2));

#if defined(POR_IMVS2_ON_NVIDIA)
	CGEstimateMotion* est_motion_graph_ptr = (CGEstimateMotion*)g_vx_graph_pool.fetchGraph(
			kMGMotionEstimation, mold_piece_ptr->m_ovx_pyramid, &cur_img, m_engine_param_ptr,
			&m_lk_lpoint_vec, &m_lk_rpoint_vec);
	if (!est_motion_graph_ptr)
	{
		return false;
	}
	est_motion_graph_ptr->process();
	g_vx_graph_pool.releaseGraph(est_motion_graph_ptr);
#else
	std::vector<f32> err;
    std::vector<uchar> status;

	std::vector<cv::Mat> cur_pyramid;
	std::vector<cv::Mat>& mdl_pyramid = mold_piece_ptr->m_cv_pyramid;
	cv::Mat* cv_model_img_ptr = mdl_pyramid.data();

	if (!cv_model_img_ptr)
	{
		printlog_lvs2("MoldPiece image invalid.", LOG_SCOPE_ENGINE);
		return false;
	}
	if (cv_model_img_ptr->cols != cur_img.w || cv_model_img_ptr->rows != cur_img.h)
	{
		printlog_lvs2(QString("MoldPiece image size(%1,%2) is not same with current frame(%3,%4)")
						.arg(mold_piece_ptr->m_snap_image.w).arg(mold_piece_ptr->m_snap_image.h)
						.arg(cur_img.w).arg(cur_img.h), LOG_SCOPE_ENGINE);
		return false;
	}

	cv::Mat cv_cur_img(cur_img.h, cur_img.w, CV_8UC1, cur_img.img_ptr);
	cv::buildOpticalFlowPyramid(cv_cur_img, cur_pyramid, cv::Size(wsize, wsize), maxlevel);

	//mdl->cur optical flow
	cv::calcOpticalFlowPyrLK(mdl_pyramid, cur_pyramid, m_lk_lpoint_vec, m_lk_rpoint_vec,
						status, err, cv::Size(wsize, wsize), maxlevel,
						cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 0.01));

	count = 0;
	for (i = 0; i < m_lk_pair_count; i++)
	{
		if (status[i] != 1)
		{
			continue;
		}
		pts1[count] = m_lk_lpoint_vec[i];
		pts3[count] = m_lk_lpoint_vec[i];
		pts2[count] = m_lk_rpoint_vec[i];
		count++;
	}
	pts1.resize(count);
	pts2.resize(count);
	pts3.resize(count);
	if (count <= 0)
	{
		return false;
	}

	//cur->mdl optical flow
	cv::calcOpticalFlowPyrLK(cur_pyramid, mdl_pyramid, pts2, pts3, status, err, cv::Size(wsize, wsize), maxlevel,
				cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 0.01), cv::OPTFLOW_USE_INITIAL_FLOW);

	if (getValidLKPairs(pts1, pts3, status, err) < 4)
	{
		return false;
	}

	i32 pcount = 0;
	for (i = 0; i < count; i++)
	{
		if (status[i] != 1)
		{
			continue;
		}
		m_lk_lpoint_vec[pcount] = pts1[i];
		m_lk_rpoint_vec[pcount] = pts2[i];
		pcount++;
	}
	m_lk_lpoint_vec.resize(pcount);
	m_lk_rpoint_vec.resize(pcount);
	if (pcount <= 0)
	{
		return false;
	}
#endif

#ifdef POR_TESTMODE
	{
		Img& mdl_img = mold_piece_ptr->m_snap_image;
		testPtsWithImg(PO_DEBUG_PATH"pair_l.bmp", m_lk_lpoint_vec, mdl_img.img_ptr, mdl_img.w, mdl_img.h);
		testPtsWithImg(PO_DEBUG_PATH"pair_r.bmp", m_lk_rpoint_vec, cur_img.img_ptr, cur_img.w, cur_img.h);
	}
#endif

	f32 th_area = cvConvexHullArea(m_lk_lpoint_vec);
    if (th_area < mold_piece_ptr->m_hull_area * m_engine_param_ptr->lk_rect_rate)
	{
		return false;
	}

	//mdl->snap transform
	th_area = po_max(10, th_area*0.07f); // > 0.2*0.2 = 0.04
	if (!cvGetRansacTransform(m_lk_lpoint_vec, m_lk_rpoint_vec, tr, eps, outlier, th_area))
	{
		return false;
	}

	f32 cos_delta = std::abs(tr[0] - tr[4]);
    f32 sin_delta = std::abs(tr[1] + tr[3]);
	if (std::abs(tr[0] - 1.0f) > thangle*thscale || std::abs(tr[1]) > thangle || std::abs(tr[2]) > thoffsetx ||
		std::abs(tr[3]) > thangle || std::abs(tr[4] - 1.0f) > thangle*thscale || std::abs(tr[5]) > thoffsety ||
		std::abs(tr[6]) > thangle || std::abs(tr[7]) > thangle || cos_delta > thskew || sin_delta > thskew)
	{
		CPOBase::init3x3(tr);
		return false;
	}

	if (!cvSolveDetailTransform(mold_piece_ptr, cur_img, m_lk_lpoint_vec, m_lk_rpoint_vec, tr))
 	{
 		CPOBase::init3x3(tr);
 		return false;
 	}

#ifdef POR_TESTMODE
	{
		Img& mdl_img = mold_piece_ptr->m_snap_image;
		count = 0;
		f32 px, py, eps2 = eps*eps;
		i32 pcount = (i32)m_lk_lpoint_vec.size();
		for (i = 0; i < pcount; i++)
		{
			px = m_lk_lpoint_vec[i].x;
			py = m_lk_lpoint_vec[i].y;
			CPOBase::trans3x3(tr, px, py);
			if (CPOBase::distL2Norm(px, py, m_lk_rpoint_vec[i].x, m_lk_rpoint_vec[i].y) > eps2)
			{
				continue;
			}

			pts1[count] = m_lk_lpoint_vec[i];
			pts2[count] = m_lk_rpoint_vec[i];
			count++;
		}
		pts1.resize(count);
		pts2.resize(count);
		testPtsWithImg(PO_DEBUG_PATH"motion_l.bmp", pts1, mdl_img.img_ptr, mdl_img.w, mdl_img.h);
		testPtsWithImg(PO_DEBUG_PATH"motion_r.bmp", pts2, cur_img.img_ptr, cur_img.w, cur_img.h);
	}
#endif
	return true;
}

i32 CROIProcessor::checkROIRegion(CMoldROIData* roi_model_ptr, CMoldROIResult* roi_result_ptr)
{
	if (!m_roi_param_ptr || !m_roi_data_ptr)
	{
		return kPOErrInvalidOper;
	}

	Recti range = m_roi_data_ptr->m_range;
	i32 w = range.getWidth();
	i32 h = range.getHeight();
	i32 max_defect_size = roi_model_ptr->getROIArea();
	roi_result_ptr->m_range = range;

	u8* roi_img_ptr = m_roi_data_ptr->getROIImage();
	u8* igr_img_ptr = roi_model_ptr->getIgrImage();
	f32* trans_ptr = roi_result_ptr->getTransform();

#if defined(POR_IMVS2_ON_NVIDIA)
	//1.unwarp current image and calc ROI features with model space
	keep_time(13);
	if (checkValidEstimation(trans_ptr, m_image.w, m_image.h))
	{
		//build warpped image from current frame to model frame
		vxWarpAffineImage(trans_ptr);
	}
	else
	{
		//copy sub image(crop image) from current frame to model frame
		CImageProc::cropImage(roi_img_ptr, m_image, range);
	}
	leave_time(13);
#if defined(POR_TESTMODE)
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"crop_l.bmp", roi_model_ptr->getROIImage(), w, h);
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"crop_r.bmp", m_roi_data_ptr->getROIImage(), w, h);
#endif

	//2.build substract image between model and current frame
	keep_time(14);
	bool is_processed = false;
	u8* sub_img_ptr = new u8[w*h];
	u8* erode_img_ptr = new u8[w*h];
	f32 acc_score = 0;

	CGDiffROIRegion* chk_graph_ptr = (CGDiffROIRegion*)g_vx_graph_pool.fetchGraph(
			kMGDiffROIRegion, roi_model_ptr, m_roi_data_ptr, m_roi_param_ptr, m_engine_param_ptr,
			sub_img_ptr, erode_img_ptr, &acc_score);
	if (chk_graph_ptr)
	{
		is_processed = chk_graph_ptr->process();
		g_vx_graph_pool.releaseGraph(chk_graph_ptr);
	}
	if (!is_processed)
	{
		roi_result_ptr->setResult(kMoldResultNG, max_defect_size);
		roi_result_ptr->setResultDesc(kMoldRDescLargeStDev);
		POSAFE_DELETE_ARRAY(sub_img_ptr);
		POSAFE_DELETE_ARRAY(erode_img_ptr);
		return kEngineResultOK;
	}

	//오유로 검출되는 화소개수측정
	f32 max_defect_area = m_roi_param_ptr->th_defect_area * 5.0f;
	f32 min_ignore_roi = m_engine_param_ptr->ignore_add_rate * roi_model_ptr->getROIArea();
	f32 lot_th_pixels = po::_max(max_defect_area, min_ignore_roi);
	if (acc_score > 0.7f*lot_th_pixels*m_roi_param_ptr->getThGHSubDiff())
	{
		roi_result_ptr->addResultDesc(kMoldRDescLotDiffPixels);
	}
	leave_time(14);

#if defined(POR_TESTMODE)
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"diff_proc.bmp", sub_img_ptr, w, h);
#endif

	//4.find defect region hubo with
	keep_time(15);
	cv::Mat label;
	cv::Mat cv_sub_img(h, w, CV_8UC1, sub_img_ptr);
	i32 cc_count = cv::connectedComponents(cv_sub_img, label, 8, CV_16U) - 1;
	leave_time(15);

#else
	//////////////////////////////////////////////////////////////////////////
	bool is_processed = false;
	m_ssim_processor.releaseBuffer();

	//1.unwarp current image and calc ROI features with model space
	keep_time(13);
	if (checkValidEstimation(trans_ptr, m_image.w, m_image.h))
	{
		//build warpped image from current frame to model frame
		CImageProc::affineWarpImageOpenCV(roi_img_ptr, range, m_image, trans_ptr);
	}
	else
	{
		//copy sub image(crop image) from current frame to model frame
		CImageProc::cropImage(roi_img_ptr, m_image, range);
	}
	leave_time(13);

	keep_time(14);
	if (makeCheckDataInROI(MV_ROIPROC_CHECK) != kEngineResultOK)
	{
		roi_result_ptr->setResult(kMoldResultFail, max_defect_size);
		return kEngineResultFail;
	}

#ifdef POR_TESTMODE
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"crop_l.bmp", roi_model_ptr->getROIImage(), w, h);
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"crop_r.bmp", m_roi_data_ptr->getROIImage(), w, h);
#endif

	//2.build substract image between model and current frame
	u8* cur_img_ptr = m_roi_data_ptr->getROIImage();
	u8* mdl_img_ptr = roi_model_ptr->getROIImage();
	u8* mask_img_ptr = m_roi_data_ptr->getMaskImage();
	u8* sub_img_ptr = new u8[w*h];

	cv::Mat cv_cur_img(h, w, CV_8UC1, cur_img_ptr);
	cv::Mat cv_mdl_img(h, w, CV_8UC1, mdl_img_ptr);
	cv::Mat cv_sub_img(h, w, CV_8UC1, sub_img_ptr);
	cv::Mat cv_mask_img(h, w, CV_8UC1, mask_img_ptr);
	cv::Mat cv_max_img, cv_min_img;

	//abs diff
	cv::absdiff(cv_cur_img, cv_mdl_img, cv_sub_img);

	//반사광억제기능을 수행한다. 차분화상에서 반사광화소들을 제거한다.
	if (m_engine_param_ptr->useSpotDetection())
	{
		cv::max(cv_cur_img, cv_mdl_img, cv_max_img);
		cv::threshold(cv_max_img, cv_max_img, m_engine_param_ptr->spot_intensity, 0xFF, cv::THRESH_BINARY_INV);
	}

	//check structure similar
	if (m_engine_param_ptr->useSafeCheck())
	{
		m_ssim_processor.makeStructSqSimilar(roi_model_ptr, m_roi_data_ptr);
	}

#ifdef POR_TESTMODE
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"diff.bmp", sub_img_ptr, w, h);
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"ignore.bmp", igr_img_ptr, w, h);
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"mask.bmp", mask_img_ptr, w, h);
#endif

	f32 dmn = CImageProc::getThresholdLowBand(sub_img_ptr, mask_img_ptr, w, h, 0.5f);
	if (dmn > m_roi_param_ptr->th_max_mean)
	{
		roi_result_ptr->setResult(kMoldResultNG, max_defect_size);
		roi_result_ptr->setResultDesc(kMoldRDescLargeStDev);
		POSAFE_DELETE_ARRAY(sub_img_ptr);
		return kEngineResultOK;
	}

	//차분화상을 계산
	if (m_engine_param_ptr->useSafeCheck())
	{
		m_ssim_processor.updateSimilar(m_roi_data_ptr, sub_img_ptr, mask_img_ptr);
	}

	i32 th_suppression[256];
	m_roi_param_ptr->getThGDiffs(th_suppression);
	cv::min(cv_cur_img, cv_mdl_img, cv_min_img);
	subtractByHistogram((u8*)cv_min_img.data, sub_img_ptr, sub_img_ptr, w, h, w, w, w, th_suppression, dmn);

	//마스크, 무시령력, 반사광무시령역설정
	cv::bitwise_and(cv_sub_img, cv_mask_img, cv_sub_img);
	if (igr_img_ptr)
	{
		cv::Mat cv_igr_img(h, w, CV_8UC1, igr_img_ptr);
		cv::bitwise_and(cv_sub_img, cv_igr_img, cv_sub_img);
	}

	if (m_engine_param_ptr->useSpotDetection())
	{
		cv::bitwise_and(cv_max_img, cv_sub_img, cv_sub_img);
#ifdef POR_TESTMODE
		cv::imwrite(PO_DEBUG_PATH"spot.bmp", cv_max_img);
#endif
	}

#ifdef POR_TESTMODE
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"org_diff.bmp", sub_img_ptr, w, h);
#endif

	//오유구역확정
	keep_time(17);
	u8* erode_img_ptr = new u8[w*h];
	cv::Mat cv_erode_img(h, w, CV_8UC1, erode_img_ptr);

	i32 dilate_size = m_roi_param_ptr->th_defect_thin;
	i32 erode_size = dilate_size + (m_engine_param_ptr->useSafeCheck() ? 2 : 0);

#if defined(POR_WITH_OVX)
	CGImgProcClose* close_graph_ptr = (CGImgProcClose*)g_vx_graph_pool.fetchGraph(
			kGImgProcClose, dilate_size, sub_img_ptr, w, h, sub_img_ptr, erode_size);
	CGImgProcErode* erode_graph_ptr = (CGImgProcErode*)g_vx_graph_pool.fetchGraph(
			kGImgProcErode, erode_size, sub_img_ptr, w, h, erode_img_ptr);
	if (close_graph_ptr && erode_graph_ptr)
	{
		close_graph_ptr->schedule();
		erode_graph_ptr->schedule();
		is_processed = true;
	}
#elif defined(POR_WITH_OCLCV)
	cv::ocl::setUseOpenCL(true);
	is_processed = true;

	i32 sub_w = cv_sub_img.cols;
	i32 sub_h = cv_sub_img.rows;
	cv::UMat u_close_img, u_tmp_img, u_erode_img;
	cv::UMat u_sub_img = cv::UMat::zeros(CPOBase::round(sub_h, 256), CPOBase::round(sub_w, 256), CV_8UC1);
	cv_sub_img.copyTo(u_sub_img(cv::Rect(0, 0, sub_w, sub_h)));

	cv::Mat cv_element_dilate = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(dilate_size, dilate_size));
	cv::Mat cv_element_erode = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(erode_size, erode_size));
	cv::dilate(u_sub_img, u_tmp_img, cv_element_dilate);
	cv::erode(u_tmp_img, u_close_img, cv_element_erode);
	cv::erode(u_sub_img, u_erode_img, cv_element_erode);
#endif

	//오유로 검출되는 화소개수측정
	f32 max_defect_area = m_roi_param_ptr->th_defect_area * 5.0f;
	f32 min_ignore_roi = m_engine_param_ptr->ignore_add_rate * roi_model_ptr->getROIArea();
	f32 lot_th_pixels = po::_max(max_defect_area, min_ignore_roi);

	f32 dth = m_roi_param_ptr->getThGHSubDiff();
	f32 acc_intensity = getDiffPixels(sub_img_ptr, w, h, w, dth);
	if (acc_intensity > 0.7f*dth*lot_th_pixels)
	{
		roi_result_ptr->addResultDesc(kMoldRDescLotDiffPixels);
	}

	if (is_processed)
	{
#if defined(POR_WITH_OVX)
		g_vx_graph_pool.releaseGraph(close_graph_ptr);
#elif defined(POR_WITH_OCLCV)
		u_close_img(cv::Rect(0, 0, sub_w, sub_h)).copyTo(cv_sub_img);
#endif
	}
	else
	{
		cv::Mat cv_tmp_img;
		cv::Mat cv_element_dilate = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(dilate_size, dilate_size));
		cv::Mat cv_element_erode = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(erode_size, erode_size));
		cv::dilate(cv_sub_img, cv_tmp_img, cv_element_dilate);
		cv::erode(cv_tmp_img, cv_sub_img, cv_element_erode);
	}
	leave_time(17);
	leave_time(14);

#ifdef POR_TESTMODE
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"diff_proc.bmp", sub_img_ptr, w, h);
#endif

	//////////////////////////////////////////////////////////////////////////
	//4.find defect region hubo with
	cv::Mat label;
	i32 cc_count = 0;

	keep_time(15);
#ifdef POR_SUPPORT_TIOPENCV
	cc_count = myConnectedComponents(cv_sub_img, label, 8, CV_16U) - 1;
#else
	cc_count = cv::connectedComponents(cv_sub_img, label, 8, CV_16U) - 1;
#endif
	leave_time(15);

	if (is_processed)
	{
#if defined(POR_WITH_OVX)
		g_vx_graph_pool.releaseGraph(erode_graph_ptr);
#elif defined(POR_WITH_OCLCV)
		u_erode_img(cv::Rect(0, 0, sub_w, sub_h)).copyTo(cv_erode_img);
		cv::ocl::setUseOpenCL(false);
#endif
	}
	else
	{
		cv::Mat cv_element_erode = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(erode_size, erode_size));
		cv::erode(cv_sub_img, cv_erode_img, cv_element_erode);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	if (cc_count <= 0)
	{
		roi_result_ptr->setResult(kMoldResultPassed, 0);
		roi_result_ptr->addResultDesc(kMoldRDescPass);
		POSAFE_DELETE_ARRAY(sub_img_ptr);
		POSAFE_DELETE_ARRAY(erode_img_ptr);
		return kEngineResultOK;
	}
#ifdef POR_TESTMODE
	cv::imwrite(PO_DEBUG_PATH"cc.bmp", label);
#endif

	keep_time(16);
	DefectVector defect_vec;
	defect_vec.resize(cc_count);
	i32 dh_count = findDefectRegions(Img(sub_img_ptr, w, h), erode_img_ptr, (u16*)label.data, defect_vec);

#if defined(POR_TESTMODE)
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"diff_erode.bmp", erode_img_ptr, w, h);
#endif
	POSAFE_DELETE_ARRAY(erode_img_ptr);

	if (dh_count <= 0)
	{
		roi_result_ptr->setResult(kMoldResultPassed, 0);
		roi_result_ptr->addResultDesc(kMoldRDescPass);
		POSAFE_DELETE_ARRAY(sub_img_ptr);
		return kEngineResultOK;
	}

	//5.extract exactly defect region in all hubo
	i32 defect_pixels = getExactlyDefects(defect_vec, (u16*)label.data, roi_result_ptr);
	roi_result_ptr->setResult(kMoldResultNG, defect_pixels);
	leave_time(16);

#ifdef POR_TESTMODE
	testDefectRegions(Img(sub_img_ptr, w, h), (u16*)label.data, defect_vec);
#endif

	//free buffer
	POSAFE_DELETE_ARRAY(sub_img_ptr);
	return kEngineResultOK;
}

bool CROIProcessor::subtractByHistogram(u8* min_img_ptr, u8* sub_img_ptr, u8* hist_diff_ptr, i32 w, i32 h, 
								i32 min_stride, i32 sub_stride, i32 hist_diff_stride, i32* th_sub_ptr, f32 dmn)
{
	i32 wh = w*h;
	if (!min_img_ptr || !sub_img_ptr || !hist_diff_ptr || !th_sub_ptr || wh <= 0)
	{
		return false;
	}

	i32 i, delta = (i32)(dmn * MVPARAM_SUBDIFF_RATE);
	for (i = 0; i < 256; i++)
	{
		th_sub_ptr[i] = po::_max(0, th_sub_ptr[i] + delta);
	}

	i32 x, y;
	u8* scn_min_ptr;
	u8* scn_sub_ptr;
	u8* scn_hist_diff_ptr;
	for (y = 0; y < h; y++)
	{
		scn_min_ptr = min_img_ptr + y * min_stride;
		scn_sub_ptr = sub_img_ptr + y * sub_stride;
		scn_hist_diff_ptr = hist_diff_ptr + y * hist_diff_stride;
		for (x = 0; x < w; x++)
		{
			*scn_hist_diff_ptr = po::_max(0, (i32)(*scn_sub_ptr) - th_sub_ptr[*scn_min_ptr]);
			scn_sub_ptr++;
			scn_min_ptr++;
			scn_hist_diff_ptr++;
		}
	}
	return true;
}

i32 CROIProcessor::getDiffPixels(u8* img_ptr, i32 w, i32 h, i32 stride, f32 dth)
{
	i32 dth_min = dth*0.2f;
	i32 dth_max = (i32)(dth*1.8f);
	i32 x, y, tmp, acc_intensity = 0, pixels = 0;
	u8* scan_img_ptr;

	for (y = 0; y < h; y++)
	{
		scan_img_ptr = img_ptr + y * stride;
		for (x = 0; x < w; x++)
		{
			tmp = scan_img_ptr[x];
			if (tmp > 0)
			{
				acc_intensity += po_min(dth_max, tmp);
				pixels++;
			}
		}
	}
	acc_intensity += pixels*dth_min;
	return acc_intensity;
}

i32 CROIProcessor::getExactlyDefects(DefectVector& defect_vec, u16* label_ptr, CMoldROIResult* roi_result_ptr)
{
	CMoldDefect* defect_ptr;
	CMoldDefect* defect_data_ptr = defect_vec.data();

	Recti& range = roi_result_ptr->m_range;
	i32 w = range.getWidth();
	i32 h = range.getHeight();
	
	i32 x, y;
	i32 dcount = 0, dsum = 0;
	i32 i, count = (i32)defect_vec.size();
	for (i = 0; i < count; i++)
	{
		defect_ptr = defect_data_ptr + i;
		if (!defect_ptr->m_is_used)
		{
			continue;
		}

		dcount++;
		dsum += defect_ptr->m_defect_area;
		defect_ptr->m_reserved = -1;
	}
	if (dcount <= 0)
	{
		return 0;
	}

	//build alarm data
	i32 index;
	Alarm* alarm_ptr;
	AlarmPVector& alarm_vec = roi_result_ptr->m_alarm_data;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			index = (i32)(*label_ptr) - 1;
			label_ptr++;
			if (index < 0)
			{
				continue;
			}

			defect_ptr = defect_data_ptr + index;
			if (!defect_ptr->m_is_used)
			{
				continue;
			}

			index = defect_ptr->m_reserved;
			if (index < 0)
			{
				//add new alarm region to MoldROIResult
				alarm_ptr = CPOBase::pushBackNew(alarm_vec);
				alarm_ptr->initBuffer(defect_ptr->m_range);

				index = (i32)alarm_vec.size() - 1;
				defect_ptr->m_reserved = index;
			}

			alarm_ptr = alarm_vec[index];
			alarm_ptr->addAlarmPoint(x, y);
		}
	}

#ifdef POR_TESTMODE
	testROIResult(roi_result_ptr);
#endif

	//offset alarm rect from ROI coordinate to model image coordinate
	count = (i32)alarm_vec.size();
	for (i = 0; i < count; i++)
	{
		alarm_vec[i]->range.translate(range.x1, range.y1);
	}
	return dsum;
}

i32 CROIProcessor::findDefectRegions(Img sub_img, u8* erode_img_ptr, u16* label_ptr, DefectVector& defect_vec)
{
	CMoldDefect* defect_ptr;
	CMoldDefect* defect_data_ptr = defect_vec.data();

	i32 w = sub_img.w;
	i32 h = sub_img.h;
	i32 i, index, wh = w*h;
	i32 th_sub_diff = m_roi_param_ptr->getThGHSubDiff();
	i32 th_defect_area = m_roi_param_ptr->th_defect_area;
	i32 th_erode_area = po::_max(1, (sqrt(th_defect_area) - m_roi_param_ptr->th_defect_thin) / 2);
	th_erode_area *= th_erode_area;

	//check cc has high response of substract image
	bool is_defected = false;
	u16* tmp_label_ptr = label_ptr;
	u8* tmp_img_ptr = sub_img.img_ptr;

	for (i = 0; i < wh; i++)
	{
		index = (i32)(*tmp_label_ptr) - 1;
		if (index >= 0 && *tmp_img_ptr > th_sub_diff)
		{
			is_defected = true;
			defect_ptr = defect_data_ptr + index;
			defect_ptr->m_is_used = true;
		}
		tmp_img_ptr++; tmp_label_ptr++;
	}
	if (!is_defected)
	{
		return 0;
	}

	//collect cc information such as area, center point in substract image
	i32 x, y;
	tmp_label_ptr = label_ptr;
	tmp_img_ptr = sub_img.img_ptr;
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			index = (i32)(*tmp_label_ptr) - 1;
			if (index >= 0)
			{
				defect_ptr = defect_data_ptr + index;
				if (defect_ptr->m_is_used)
				{
					defect_ptr->m_range.insertPoint(x, y);
					defect_ptr->m_defect_area++;
					if (*erode_img_ptr > 0)
					{
						defect_ptr->m_reserved++;
					}
				}
			}
			tmp_img_ptr++; tmp_label_ptr++; erode_img_ptr++;
		}
	}

	//update defect hubo
	i32 dcount = 0;
	i32 count = (i32)defect_vec.size();
	for (i = 0; i < count; i++)
	{
		defect_ptr = defect_data_ptr + i;
		if (!defect_ptr->m_is_used)
		{
			continue;
		}

		if (defect_ptr->m_defect_area < th_defect_area || defect_ptr->m_reserved < th_erode_area)
		{
			defect_ptr->m_is_used = false;
			continue;
		}
		defect_ptr->m_reserved = 0;
		dcount++;
	}
	return dcount;
}

void CROIProcessor::updateSSIMMap(CMoldROIData* roi_data_ptr)
{
	i32 w = roi_data_ptr->getROIWidth();
	i32 h = roi_data_ptr->getROIHeight();
	u8* roi_img_ptr = roi_data_ptr->getROIImage();
	u8* mask_img_ptr = roi_data_ptr->getMaskImage();
	if (w*h <= 0 || roi_img_ptr == NULL || mask_img_ptr == NULL)
	{
		return;
	}

	m_ssim_processor.prepareStructSqSimilar(roi_data_ptr, false, false);
}

i32 CROIProcessor::getValidLKPairs(cvPtVector& cv_pts_vec1, cvPtVector& cv_pts_vec2, u8vector& status, f32vector& err_vec)
{
 	i32 i, pair_count = 0, count1 = 0;
 	i32 count = (i32)cv_pts_vec1.size();
 	f32 eps = m_engine_param_ptr->lk_epsilon;
 	f32 eps2 = eps*eps;
// 	f32 min_dis, max_dis;
// 	f32 mdis = 0, sdis2 = 0;
// 
// 	for (i = 0; i < count; i++)
// 	{
// 		if (status[i] != 1)
// 		{
// 			continue;
// 		}
// 		mdis += err_vec[i];
// 		sdis2 += err_vec[i] * err_vec[i];
// 		count1++;
// 	}
// 
// 	if (count1 <= 0)
// 	{
// 		return 0;
// 	}
// 
// 	mdis = mdis / count1;
// 	sdis2 = po_max(2.0f*sqrt(sdis2 / count1 - mdis*mdis), 2.0f);
// 	min_dis = po_max(0, mdis - sdis2);
// 	max_dis = mdis + sdis2;

	for (i = 0; i < count; i++)
	{
		if (status[i] != 1)
		{
			continue;
		}
		//if (err_vec[i] > max_dis || err_vec[i] < min_dis)
		//{
		//	continue;
		//}
		if (CPOBase::distL2Norm(cv_pts_vec1[i].x, cv_pts_vec1[i].y, cv_pts_vec2[i].x, cv_pts_vec2[i].y) > eps2)
		{
			status[i] = 0;
			continue;
		}
		pair_count++;
	}
	return pair_count;
}

f32 CROIProcessor::calcConvexHull(cvPtVector& pts, ptvector2df& border_pts, i32 w, i32 h)
{
	i32 i, count = (i32)pts.size();
	if (!CPOBase::isPositive(count))
	{
		return 0;
	}

	//calc border points
	cv::Point2f mean_pt;
	for (i = 0; i < count; i++)
	{
		mean_pt += pts[i];
	}

	f64 dis2 = 0;
	mean_pt /= count;
	for (i = 0; i < count; i++)
	{
		dis2 += CPOBase::distanceSQ(mean_pt.x, mean_pt.y, pts[i].x, pts[i].y);
	}

	i32 angle_index = 0;
	f32 angle, angle_step = PO_PI_HALF / 4;
	f32 mean_dis2 = dis2 / (16 * count);

	struct bin_points
	{
		bin_points() {
			dis = -1.0f;
			pt = vector2df(0, 0);
		}
		f32 dis;
		vector2df pt;
	};
	bin_points bin_pt[20];

	for (i = 0; i < count; i++)
	{
		dis2 = CPOBase::distanceSQ(mean_pt.x, mean_pt.y, pts[i].x, pts[i].y);
		if (dis2 < mean_dis2)
		{
			continue;
		}

		angle = CPOBase::getAbsAngle(atan2(pts[i].y - mean_pt.y, pts[i].x - mean_pt.x));
		angle_index = (i32)(angle / angle_step);

		if (dis2 > bin_pt[angle_index].dis && 
			CPOBase::checkRange(pts[i].x, (f32)5, (f32)(w - 5)) &&
			CPOBase::checkRange(pts[i].y, (f32)5, (f32)(h - 5)))
		{
			bin_pt[angle_index].dis = dis2;
			bin_pt[angle_index].pt = vector2df(pts[i].x, pts[i].y);
		}
	}

	//calc 16-points foreach directions
	ptvector2df bin_pts;
	for (i = 0; i < 20; i++)
	{
		if (bin_pt[i].dis <= 0)
		{
			continue;
		}
		bin_pts.push_back(bin_pt[i].pt);
	}

	//calc six points for border matching
	border_pts.clear();
	count = (i32)bin_pts.size();
	f32 stp = po::_max((f32)count / 8.0f, 1.0f);
	for (f32 fi = 0; fi < count; fi += stp)
	{
		border_pts.push_back(bin_pts[(i32)fi]);
	}

#if defined(POR_TESTMODE)
	testCornerPts(border_pts, w, h);
#endif
	return cvConvexHullArea(pts);
}

f32 CROIProcessor::cvConvexHullArea(cvPtVector& pts)
{
	cvPtVector hull;
	f64 epsilon = 0.001; // Contour approximation accuracy
	
	// Calculate convex hull of original points (which points positioned on the boundary)
	cv::convexHull(pts, hull);
	return std::abs(cv::contourArea(hull));
}

i32 CROIProcessor::updateRegionMap(u8* mask_img_ptr, Recti& mask_range, Recti& crop_range,
								CTransRect* trans_ptr, i32 val, i32 mask)
{
	if (!mask_img_ptr)
	{
		return 0;
	}

	u8* tmp_mask_img_ptr;
	i32 x, y, pixel_count = 0;
	i32 mask_w = mask_range.getWidth();
	i32 mask_h = mask_range.getHeight();
	i32 crop_w = crop_range.getWidth();
	i32 crop_h = crop_range.getHeight();
	i32 dx = crop_range.x1 - mask_range.x1;
	i32 dy = crop_range.y1 - mask_range.y1;

	i32 type = trans_ptr->getRegionType();
	Rectf& range = trans_ptr->m_bounding_box;

	bool is_in_region;
	i32 i, ni = 0, count = 0;
	f32 x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	f32 cx = 0, cy = 0, a2 = 0, b2 = 0, r2 = 0;
	f32* poly_mul_ptr = NULL;
	f32* poly_con_ptr = NULL;
	Pixelf* shape_point_ptr = trans_ptr->m_shape_point_vec.data();

	//prepare for each region
	switch (type)
	{
		case kTransTypeCircle:
		{
			range.getCenterPoint(cx, cy);
			r2 = po_min(range.getWidth(), range.getHeight()) / 2;
			r2 = r2*r2;
			break;
		}
		case kTransTypeEllipse:
		{
			range.getCenterPoint(cx, cy);
			a2 = range.getWidth() / 2; a2 = a2*a2;
			b2 = range.getHeight() / 2; b2 = b2*b2;
			break;
		}
		case kTransTypePolygon:
		{
			//see more detail... http://alienryderflex.com/polygon/
			count = trans_ptr->getPtCount();
			poly_mul_ptr = new f32[count];
			poly_con_ptr = new f32[count];
			for (i = 0; i < count; i++)
			{
				ni = (i + 1) % count;
				if (shape_point_ptr[i].y == shape_point_ptr[ni].y)
				{
					poly_con_ptr[i] = shape_point_ptr[i].x;
					poly_mul_ptr[i] = 0;
				}
				else
				{
					x1 = shape_point_ptr[ni].x - shape_point_ptr[i].x;
					y1 = shape_point_ptr[ni].y - shape_point_ptr[i].y;
					poly_con_ptr[i] = shape_point_ptr[i].x - (shape_point_ptr[i].y * x1) / y1;
					poly_mul_ptr[i] = x1 / y1;
				}
			}
			break;
		}
	}

	//check all pixels
	for (y = 0; y < crop_h; y++)
	{
		tmp_mask_img_ptr = mask_img_ptr + (y + dy)*mask_w + dx;
		for (x = 0; x < crop_w; x++)
		{
			if (tmp_mask_img_ptr[x] == mask)
			{
				tmp_mask_img_ptr[x] = MV_REGION_MASKNONE;
				continue;
			}

			x1 = (f32)x + crop_range.x1;
			y1 = (f32)y + crop_range.y1;
			trans_ptr->transform(x1, y1);
			if (!range.pointInRect(x1, y1))
			{
				continue;
			}

			is_in_region = true;
			switch (type)
			{
				case kTransTypeCircle:
				{
					x1 = x1 - cx; x2 = x1*x1;
					y1 = y1 - cy; y2 = y1*y1;
					if (x2 + y2 > r2)
					{
						is_in_region = false;
					}
					break;
				}
				case kTransTypeEllipse:
				{
					x1 = x1 - cx; x2 = (x1*x1) / a2;
					y1 = y1 - cy; y2 = (y1*y1) / b2;
					if (x2 + y2 > 1)
					{
						is_in_region = false;
					}
					break;
				}
				case kTransTypePolygon:
				{
					//see more detail... http://alienryderflex.com/polygon/
					is_in_region = false;
					for (i = 0; i < count; i++)
					{
						ni = (i + 1) % count;
						if ((shape_point_ptr[i].y < y1 && shape_point_ptr[ni].y >= y1) ||
							(shape_point_ptr[ni].y < y1 && shape_point_ptr[i].y >= y1))
						{
							is_in_region ^= ((y1*poly_mul_ptr[i] + poly_con_ptr[i]) < x1);
						}
					}
					break;
				}
			}

			if (!is_in_region)
			{
				continue;
			}
			pixel_count++;
			tmp_mask_img_ptr[x] = val;
		}
	}

	POSAFE_DELETE_ARRAY(poly_mul_ptr);
	POSAFE_DELETE_ARRAY(poly_con_ptr);
	return pixel_count;
}

bool CROIProcessor::cvGetRansacTransform(cvPtVector& cv_pts_vec1, cvPtVector& cv_pts_vec2, f32* tr, f32 eps, f32 outlier, i32 area)
{
#if (0)
	i32 count1 = cv_pts_vec1.size();
	i32 count2 = cv_pts_vec2.size();
	if (count1 != count2 || count1 < 4)
	{
		return false;
	}

	//calc homography transform matrix with RANSAC
	cv::Mat h = cv::findHomography(cv_pts_vec1, cv_pts_vec2, CV_RANSAC, eps);
	for (i32 i = 0; i < 9; i++)
	{
		tr[i] = (f32)((f64*)h.data)[i];
	}
	return getHTransformLMSE(cv_pts_vec1, cv_pts_vec2, tr, 1.0f, 0.3f);

#else
	//coverage is 99.99%, inliner coverage is outlier%, n point is 4
	f32 max_iter = (1 + log(1 - 0.9999) / log(1 - pow(outlier, 4)));
	i32 count1 = (i32)cv_pts_vec1.size();
	i32 count2 = (i32)cv_pts_vec2.size();
	if (count1 != count2 || count1 < 4)
	{
		return false;
	}

	i32 i, j, nj;
	i32 tcount = 4;
	cv::Mat hmat;
	cvPtVector tmp_pts_vec1, tmp_pts_vec2;
	i32vector tmp;
	f32 htr[9], px, py, dist;
	f32 eps2 = eps*eps;
	f32 th_offset_x = m_engine_param_ptr->lk_offset_x;
	f32 th_offset_y = m_engine_param_ptr->lk_offset_y;
	f32 th_scale = m_engine_param_ptr->lk_scale;
	f32 th_angle = sin(CPOBase::degToRad(m_engine_param_ptr->lk_angle));

	f32 sa, sb, sc, max_sc = 0;

	tmp.resize(tcount);
	tmp_pts_vec1.resize(tcount);
	tmp_pts_vec2.resize(tcount);
	CPOBase::randomInit();

	//calc correctly transform with ransac method
	for (i = 0; i < max_iter; i++)
	{
		if (!getRandomSample(cv_pts_vec1, tmp, tcount, area))
		{
			max_iter += 0.5f;
			continue;
		}

		for (j = 0; j < tcount; j++)
		{
			nj = tmp[j];
			tmp_pts_vec1[j] = cv_pts_vec1[nj];
			tmp_pts_vec2[j] = cv_pts_vec2[nj];
		}

		//check area of convex full
		sa = cvConvexHullArea(tmp_pts_vec1);
		sb = cvConvexHullArea(tmp_pts_vec2);
		if (po_min(sa, sb) < area || CPOBase::ratio(sa, sb) < 0.8f)
		{
			max_iter += 0.7f;
			continue;
		}

		//calc perspective transform
		hmat = cv::getPerspectiveTransform(tmp_pts_vec1, tmp_pts_vec2);
		for (j = 0; j < 9; j++)
		{
			htr[j] = (f32)(((f64*)hmat.data)[j]);
		}

		if (std::abs(htr[0] - 1.0f) > th_angle*th_scale || std::abs(htr[1]) > th_angle || std::abs(htr[2]) > th_offset_x ||
			std::abs(htr[3]) > th_angle || std::abs(htr[4] - 1.0f) > th_angle*th_scale || std::abs(htr[5]) > th_offset_y ||
			std::abs(htr[6]) > th_angle || std::abs(htr[7]) > th_angle)
		{
			continue;
		}

		sc = 0;
		for (j = 0; j < count1; j++)
		{
			px = cv_pts_vec1[j].x;
			py = cv_pts_vec1[j].y;
			CPOBase::trans3x3(htr, px, py);
			dist = CPOBase::distL2Norm(px, py, cv_pts_vec2[j].x, cv_pts_vec2[j].y);
			if (dist > eps2)
			{
				continue;
			}
			sc += (1.0f - dist / eps2);
		}

		if (sc > max_sc)
		{
			max_sc = sc;
			CPOBase::memCopy(tr, htr, 9);
		}
	}
	if (max_sc < 0.3f*count1)
	{
		return false;
	}

	//calc optimized transform with LMSE
	return getHTransformLMSE(cv_pts_vec1, cv_pts_vec2, tr, 2.0f, 0.3f);
#endif
}

//calc the rigid transformation (rotation and tranlation matrix), for more inforation see http://nghiaho.com/?page_id=671
bool CROIProcessor::getATransformLMSE(cvPtVector& cv_pts_vec1, cvPtVector& cv_pts_vec2, f32* trans_ptr)
{
	i32 count1 = (i32)cv_pts_vec1.size();
	i32 count2 = (i32)cv_pts_vec2.size();
	if (count1 != count2 || count1 < 3)
	{
		return false;
	}

	i32 i;
	f32 H[4], U[4], W[4], VT[4];
	memset(H, 0, sizeof(f32) * 4);

	vector2df ca, cb;
	cv::Mat h(2, 2, CV_32FC1, H);
	cv::Mat u(2, 2, CV_32FC1, U);
	cv::Mat w(2, 2, CV_32FC1, W);
	cv::Mat vt(2, 2, CV_32FC1, VT);

	cv::Point2f* point_ptr1 = cv_pts_vec1.data();
	cv::Point2f* point_ptr2 = cv_pts_vec2.data();
	for (i = 0; i < count1; i++, point_ptr1++, point_ptr2++)
	{
		ca.x += point_ptr1->x;
		ca.y += point_ptr1->y;
		cb.x += point_ptr2->x;
		cb.y += point_ptr2->y;
	}

	ca.x /= (f32)count1; ca.y /= (f32)count1;
	cb.x /= (f32)count1; cb.y /= (f32)count1;

	point_ptr1 = cv_pts_vec1.data();
	point_ptr2 = cv_pts_vec2.data();
	for (i = 0; i < count1; i++, point_ptr1++, point_ptr2++)
	{
		f32 AX = (point_ptr1->x - ca.x);
		f32 AY = (point_ptr1->y - ca.y);
		f32 BX = (point_ptr2->x - cb.x);
		f32 BY = (point_ptr2->y - cb.y);
		H[0] += AX*BX;
		H[1] += AX*BY;
		H[2] += AY*BX;
		H[3] += AY*BY;
	}

	cv::SVD solver;
	solver.compute(h, w, u, vt, CV_SVD_MODIFY_A);

	//R = V * transpose(U)
	trans_ptr[0] = VT[0] * U[0] + VT[2] * U[1];
	trans_ptr[1] = VT[0] * U[2] + VT[2] * U[3];
	trans_ptr[3] = VT[1] * U[0] + VT[3] * U[1];
	trans_ptr[4] = VT[1] * U[2] + VT[3] * U[3];

	//special reflection case
	if (CPOBase::transDet2x3(trans_ptr) < 0)
	{
		trans_ptr[0] = VT[0] * U[0] - VT[2] * U[1];
		trans_ptr[1] = VT[0] * U[2] - VT[2] * U[3];
		trans_ptr[3] = VT[1] * U[0] - VT[3] * U[1];
		trans_ptr[4] = VT[1] * U[2] - VT[3] * U[3];
	}

	//T = -R*meanA+meanB
	trans_ptr[2] = -trans_ptr[0] * ca.x - trans_ptr[1] * ca.y + cb.x;
	trans_ptr[5] = -trans_ptr[3] * ca.x - trans_ptr[4] * ca.y + cb.y;
	return true;
}

bool CROIProcessor::calcHTransformLMSE(cvPtVector& cv_pts_vec1, cvPtVector& cv_pts_vec2, f32* tr_ptr)
{
	i32 count = (i32)cv_pts_vec2.size();
	if (!tr_ptr || count < 4 || cv_pts_vec1.size() != cv_pts_vec2.size())
	{
		return false;
	}

	//calc exactly transform with LMSE
	f64* data_ptr1 = new f64[2 * count * 8];
	f64* data_ptr2 = new f64[2 * count];
	f64* mat_ptr = new f64[8];
	memset(data_ptr1, 0, sizeof(f64)*(2 * count * 8));
	memset(data_ptr2, 0, sizeof(f64)*(2 * count));

	i32 i, index;
	f32 px, py, qx, qy;
	for (i = 0; i < count; i++)
	{
		px = cv_pts_vec1[i].x;
		py = cv_pts_vec1[i].y;
		qx = cv_pts_vec2[i].x;
		qy = cv_pts_vec2[i].y;

		index = i * 8;
		data_ptr1[index] = px;
		data_ptr1[index + 1] = py;
		data_ptr1[index + 2] = 1;
		data_ptr1[index + 6] = -px*qx;
		data_ptr1[index + 7] = -py*qx;

		index = (i + count) * 8;
		data_ptr1[index + 3] = px;
		data_ptr1[index + 4] = py;
		data_ptr1[index + 5] = 1;
		data_ptr1[index + 6] = -px*qy;
		data_ptr1[index + 7] = -py*qy;

		data_ptr2[i] = qx;
		data_ptr2[i + count] = qy;
	}

	cv::Mat A(2 * count, 8, CV_64F, data_ptr1);
	cv::Mat b(2 * count, 1, CV_64F, data_ptr2);
	cv::Mat x(8, 1, CV_64F, mat_ptr);
	cv::solve(A, b, x, cv::DECOMP_SVD);

	for (i = 0; i < 8; i++)
	{
		tr_ptr[i] = (f32)mat_ptr[i];
	}

	POSAFE_DELETE_ARRAY(mat_ptr);
	POSAFE_DELETE_ARRAY(data_ptr1);
	POSAFE_DELETE_ARRAY(data_ptr2);
	return true;
}

bool CROIProcessor::getHTransformLMSE(cvPtVector& cv_pts_vec1, cvPtVector& cv_pts_vec2, f32* tr_ptr, f32 eps, f32 rate, i32 fixed_count)
{
	i32 count1 = (i32)cv_pts_vec1.size();
	i32 count2 = (i32)cv_pts_vec2.size();
	if (count1 < 4 || count1 != count2)
	{
		return false;
	}

	i32 i;
	f32 px, py;
	f32 eps2 = eps*eps;
	cvPtVector tmp_pts_vec1;
	cvPtVector tmp_pts_vec2;

	//collect positive samples in all samples with ransac result transform
	count2 = 0;
	tmp_pts_vec1.resize(count1);
	tmp_pts_vec2.resize(count1);
	for (i = 0; i < count1; i++)
	{
		if (i > fixed_count)
		{
			px = cv_pts_vec1[i].x;
			py = cv_pts_vec1[i].y;
			CPOBase::trans3x3(tr_ptr, px, py);
			if (CPOBase::distL2Norm(px, py, cv_pts_vec2[i].x, cv_pts_vec2[i].y) > eps2)
			{
				continue;
			}
		}

		tmp_pts_vec1[count2] = cv_pts_vec1[i];
		tmp_pts_vec2[count2] = cv_pts_vec2[i];
		count2++;
	}
	tmp_pts_vec1.resize(count2);
	tmp_pts_vec2.resize(count2);
	if (count2 < count1*rate || count2 < 4)
	{
		return false;
	}

	if (!calcHTransformLMSE(tmp_pts_vec1, tmp_pts_vec2, tr_ptr))
	{
		return false;
	}

#ifdef POR_TESTMODE
	testPointMatch(tmp_pts_vec1, tmp_pts_vec2, 1280, 1024);
	testPointLKMatch(tmp_pts_vec1, tmp_pts_vec2, tr_ptr, 1.0, 1280, 1024);
#endif
	return true;
}

bool CROIProcessor::getRandomSample(cvPtVector& cv_pts_vec, i32vector& ind, i32 count, i32 thlen2)
{
	i32 rcount = 0;
	i32 kcount = 1;
	i32 point_count = (i32)cv_pts_vec.size();
	cv::Point2f* cv_point_ptr = cv_pts_vec.data();

	i32 x0, x1, y0, y1, index;
	ind[0] = CPOBase::random() % point_count;
	x0 = (i32)cv_point_ptr[ind[0]].x;
	y0 = (i32)cv_point_ptr[ind[0]].y;

	while (kcount < count)
	{
		if (rcount++ > point_count)
		{
			return false;
		}

		index = CPOBase::random() % point_count;
		x1 = (i32)cv_point_ptr[index].x;
		y1 = (i32)cv_point_ptr[index].y;
		if (CPOBase::distL2Norm(x0, y0, x1, y1) < thlen2)
		{
			continue;
		}

		ind[kcount++] = index;
		x0 = x1; y0 = y1;
	}
	return true;
}

bool CROIProcessor::cvSolveDetailTransform(CMoldPiece* mold_piece_ptr, Img cur_img,
									cvPtVector& cv_pts_vec1, cvPtVector& cv_pts_vec2, f32* tr_ptr)
{
	if (!mold_piece_ptr || !tr_ptr)
	{
		return false;
	}
	ptvector2df& pts = mold_piece_ptr->m_border_pts;
	if (pts.size() < 4)
	{
		return false;
	}

	i32 founds = 0;
	i32 i, count = (i32)pts.size();
	i32 x1, y1, x2, y2;
	f32 px, py, dx, dy;
	f64 min_val, max_val;
	cv::Point min_loc, max_loc;
	cvPtVector mdl_pt_vec;
	cvPtVector cur_pt_vec;
	cv::Mat cv_result(7, 7, CV_32FC1);

	u8* mdl_img_ptr = mold_piece_ptr->m_snap_image.img_ptr;
	u8* cur_img_ptr = cur_img.img_ptr;
	i32 w1 = mold_piece_ptr->m_snap_image.w;
	i32 w2 = cur_img.w;
	i32 h2 = cur_img.h;

	for (i = 0; i < count; i++)
	{
		px = pts[i].x;
		py = pts[i].y;
		CPOBase::trans3x3(tr_ptr, px, py, dx, dy);
		if (CPOBase::checkRange(dx, (f32)7, (f32)(w2 - 7)) && CPOBase::checkRange(dy, (f32)7, (f32)(h2 - 7)))
		{
			x1 = (i32)(px + 0.5f) - 3;
			y1 = (i32)(py + 0.5f) - 3;
			x2 = (i32)(dx + 0.5f) - 6;
			y2 = (i32)(dy + 0.5f) - 6;
			
			cv::Mat cv_templ(7, 7, CV_8UC1, mdl_img_ptr + y1 * w1 + x1, w1);
			cv::Mat cv_img(13, 13, CV_8UC1, cur_img_ptr + y2 * w2 + x2, w2);
			cv::matchTemplate(cv_img, cv_templ, cv_result, CV_TM_CCOEFF_NORMED);
			cv::minMaxLoc(cv_result, &min_val, &max_val, &min_loc, &max_loc, cv::Mat());
			if (max_val > 0.90f)
			{
				mdl_pt_vec.push_back(cv::Point2f(x1 + 3, y1 + 3));
				cur_pt_vec.push_back(cv::Point2f(x2 + max_loc.x + 3, y2 + max_loc.y + 3));
				founds++;
			}
		}
	}

#if defined(POR_TESTMODE)
	{
		Img& mdl_img = mold_piece_ptr->m_snap_image;
		testPtsWithImg(PO_DEBUG_PATH"detail_match_mdl.bmp", mdl_pt_vec, mdl_img.img_ptr, mdl_img.w, mdl_img.h);
		testPtsWithImg(PO_DEBUG_PATH"detail_match_cur.bmp", cur_pt_vec, cur_img.img_ptr, cur_img.w, cur_img.h);
	}
#endif
	if (founds < count*0.75f)
	{
		return false;
	}

	if (!calcHTransformLMSE(mdl_pt_vec, cur_pt_vec, tr_ptr))
	{
		return false;
	}
	
 	mdl_pt_vec.insert(mdl_pt_vec.end(), cv_pts_vec1.begin(), cv_pts_vec1.end());
 	cur_pt_vec.insert(cur_pt_vec.end(), cv_pts_vec2.begin(), cv_pts_vec2.end());
 	return getHTransformLMSE(mdl_pt_vec, cur_pt_vec, tr_ptr, 1.0f, 0.0f, founds);
}

//////////////////////////////////////////////////////////////////////////
void CROIProcessor::testDefectRegions(Img img, u16* label_ptr, DefectVector& defect_vec)
{
	i32 w = img.w;
	i32 h = img.h;
	u8* img_ptr = new u8[w*h];
	memset(img_ptr, 0, w*h);

	i32 i, wh = w*h;
	CMoldDefect* tmp_defect_ptr;
	CMoldDefect* defect_ptr = defect_vec.data();
	for (i = 0; i < wh; i++)
	{
		if (label_ptr[i] == 0)
		{
			continue;
		}

		tmp_defect_ptr = defect_ptr + label_ptr[i] - 1;
		if (tmp_defect_ptr->m_is_used)
		{
			img_ptr[i] = 0xFF;
		}
	}

	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"defects.bmp", img_ptr, w, h);
	POSAFE_DELETE_ARRAY(img_ptr);
}

void CROIProcessor::testCornerPts(ptvector2df& pts_vec, i32 w, i32 h)
{
	u8* img_ptr = new u8[w*h];
	memset(img_ptr, 0, w*h);

	i32 i, x, y;
	for (i = 0; i < pts_vec.size(); i++)
	{
		x = (i32)(pts_vec[i].x + 0.5f);
		y = (i32)(pts_vec[i].y + 0.5f);
		img_ptr[y*w + x] = 0xFF;
	}

	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"border_pts.bmp", img_ptr, w, h);
	POSAFE_DELETE_ARRAY(img_ptr);
}

void CROIProcessor::testCornerPts(std::vector<cv::Point2f>& cv_pts_vec, i32 w, i32 h)
{
	u8* img_ptr = new u8[w*h];
	memset(img_ptr, 0, w*h);

	i32 i, x, y;
	for (i = 0; i < cv_pts_vec.size(); i++)
	{
		x = (i32)(cv_pts_vec[i].x + 0.5f);
		y = (i32)(cv_pts_vec[i].y + 0.5f);
		img_ptr[y*w + x] = 0xFF;
	}

	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"1.bmp", img_ptr, w, h);
	POSAFE_DELETE_ARRAY(img_ptr);
}

void CROIProcessor::testPtsWithImg(const char* filename, cvPtVector& pts_vec, u8* img_ptr, i32 w, i32 h)
{
	u8* new_img_ptr = new u8[3*w*h];
	memset(new_img_ptr, 0, 3*w*h);

	i32 i, x, y;
	for (i = 0; i < pts_vec.size(); i++)
	{
		x = (i32)(pts_vec[i].x + 0.5f);
		y = (i32)(pts_vec[i].y + 0.5f);
		if (x < 0 || y < 0 || x >= w || y >= h)
		{
			continue;
		}

		new_img_ptr[(y*w + x) * 3 + 1] = 0xFF;
	}

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			new_img_ptr[(y*w + x) * 3 + 2] = img_ptr[y*w + x];
		}
	}

	cv::Mat img(h, w, CV_8UC3, new_img_ptr);
	cv::imwrite(filename, img);
	POSAFE_DELETE_ARRAY(new_img_ptr);
}

void CROIProcessor::testPtsWithImg(const char* filename, ptfvector& pts_vec, u8* img_ptr, i32 w, i32 h)
{
	u8* new_img_ptr = new u8[3*w*h];
	memset(new_img_ptr, 0, 3*w*h);

	i32 i, x, y;
	for (i = 0; i < pts_vec.size(); i++)
	{
		x = (i32)(pts_vec[i].x + 0.5f);
		y = (i32)(pts_vec[i].y + 0.5f);
		if (x < 0 || y < 0 || x >= w || y >= h)
		{
			continue;
		}

		new_img_ptr[(y*w + x) * 3 + 1] = 0xFF;
	}

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			new_img_ptr[(y*w + x) * 3 + 2] = img_ptr[y*w + x];
		}
	}

	cv::Mat img(h, w, CV_8UC3, new_img_ptr);
	cv::imwrite(filename, img);
	POSAFE_DELETE_ARRAY(new_img_ptr);
}

void CROIProcessor::testROIResult(CMoldROIResult* roi_result_ptr)
{
	Recti& range = roi_result_ptr->m_range;

	i32 w = range.getWidth();
	i32 h = range.getHeight();
	u8* img_ptr = new u8[w*h];
	memset(img_ptr, 0, w*h);

	AlarmPVector& alarm_vec = roi_result_ptr->m_alarm_data;
	i32 i, j, k;
	i32 count = (i32)alarm_vec.size();
	for (i = 0; i < count; i++)
	{
		Alarm* alarm_ptr = alarm_vec[i];
		i32 x1 = alarm_ptr->range.x1;
		i32 y1 = alarm_ptr->range.y1;
		i32 w1 = alarm_ptr->range.getWidth();
		i32 h1 = alarm_ptr->range.getHeight();
		u8* img_ptr1 = alarm_ptr->alram_img_ptr;

		for (j = 0; j < h1; j++)
		{
			u8* tmp_img_ptr1 = img_ptr1 + j*w1;
			u8* tmp_img_ptr = img_ptr + (y1 + j)*w + x1;
			for (k = 0; k < w1; k++)
			{
				if (tmp_img_ptr1[k]) tmp_img_ptr[k] = tmp_img_ptr1[k];
			}
		}
	}

	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"alarm.bmp", img_ptr, w, h);
	POSAFE_DELETE_ARRAY(img_ptr);
}

void CROIProcessor::testPointMatch(std::vector<cv::Point2f>& cv_pts_vec1, std::vector<cv::Point2f>& cv_pts_vec2, i32 w, i32 h)
{
	i32 i, count = (i32)cv_pts_vec1.size();
	u8* img_ptr = new u8[3 * w*h];
	memset(img_ptr, 0, 3 * w*h);

	i32 x1, y1, x2, y2;
	for (i = 0; i < count; i++)
	{
		x1 = (i32)(cv_pts_vec1[i].x + 0.5f);
		y1 = (i32)(cv_pts_vec1[i].y + 0.5f);
		x2 = (i32)(cv_pts_vec2[i].x + 0.5f);
		y2 = (i32)(cv_pts_vec2[i].y + 0.5f);

		if (x1 >= 0 && x1 < w && y1 >= 0 && y1 < h)
		{
			img_ptr[(y1*w + x1) * 3 + 2] = 255;
		}
		if (x2 >= 0 && x2 < w && y2 >= 0 && y2 < h)
		{
			img_ptr[(y2*w + x2) * 3 + 0] = 255;
		}
	}

	cv::Mat img(h, w, CV_8UC3, img_ptr);
	cv::imwrite(PO_DEBUG_PATH"pts_match.bmp", img);
	POSAFE_DELETE_ARRAY(img_ptr);
}

void CROIProcessor::testPointLKMatch(std::vector<cv::Point2f>& cv_pts_vec1, std::vector<cv::Point2f>& cv_pts_vec2, f32* trans_ptr, f32 ratio, i32 w, i32 h)
{
	i32 i, count = (i32)cv_pts_vec1.size();
	u8* img_ptr = new u8[3 * w*h];
	memset(img_ptr, 0, 3 * w*h);

	f32 px, py;
	i32 x1, y1, x2, y2;
	for (i = 0; i < count; i++)
	{
		px = cv_pts_vec1[i].x;
		py = cv_pts_vec1[i].y;
		CPOBase::trans3x3(trans_ptr, px, py);

		x1 = (i32)(px*ratio + 0.5f);
		y1 = (i32)(py*ratio + 0.5f);
		x2 = (i32)(cv_pts_vec2[i].x*ratio + 0.5f);
		y2 = (i32)(cv_pts_vec2[i].y*ratio + 0.5f);

		if (x1 >= 0 && x1 < w && y1 >= 0 && y1 < h)
		{
			img_ptr[(y1*w + x1) * 3 + 2] = 255;
		}
		if (x2 >= 0 && x2 < w && y2 >= 0 && y2 < h)
		{
			img_ptr[(y2*w + x2) * 3 + 0] = 255;
		}
	}

	cv::Mat img(h, w, CV_8UC3, img_ptr);
	cv::imwrite(PO_DEBUG_PATH"pts_lk_match.bmp", img);
	POSAFE_DELETE_ARRAY(img_ptr);
}

void CROIProcessor::testImgLKMatch(u8* img_ptr1, u8* img_ptr2, i32 w, i32 h, f32* trans_ptr)
{
	CTransform tf(trans_ptr, 9);
	tf.update();

	f32 px, py;
	i32 x, y, x1, y1;
	u8* img_ptr = new u8[w*h];
	memset(img_ptr, 0, w*h);

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			px = (f32)x;
			py = (f32)y;
			tf.invTransform(px, py);
			x1 = (i32)(px + 0.5f);
			y1 = (i32)(py + 0.5f);
			if (x1 >= 0 && y1 >= 0 && x1 < w && y1 < h)
			{
				img_ptr[y*w + x] = img_ptr1[y1*w + x1];
			}
		}
	}

	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"k1.bmp", img_ptr, w, h);
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"k2.bmp", img_ptr2, w, h);
	POSAFE_DELETE_ARRAY(img_ptr);
}

bool CROIProcessor::vxWarpAffineImage(f32* trans_ptr)
{
	bool is_success = false;
	if (!m_roi_data_ptr || !m_image.isValid())
	{
		return false;
	}

#if defined(POR_WITH_OVX)
	Recti range = m_roi_data_ptr->m_range;
	i32 w = range.getWidth();
	i32 h = range.getHeight();

	f32 mtr[9];
	f32 ttr[9];
	CPOBase::init3x3(ttr, 0, 1.0f, range.x1, range.y1);

	cv::Mat cv_mat_tr1(3, 3, CV_32FC1, trans_ptr);
	cv::Mat cv_mat_tr2(3, 3, CV_32FC1, ttr);
	cv::Mat cv_mat_tr3(3, 3, CV_32FC1, mtr);
	cv_mat_tr3 = cv_mat_tr1 * cv_mat_tr2;
	CPOBase::transpose(ttr, mtr, 3, 3);

	vx_context context = g_vx_context.getVxContext();
	OvxResource* ovx_src_ptr = g_vx_resource_pool.fetchImage(context, m_image.w, m_image.h, VX_DF_IMAGE_U8);
	OvxResource* ovx_dst_ptr = g_vx_resource_pool.fetchImage(context, w, h, VX_DF_IMAGE_U8);
	OvxResource* ovx_mat_ptr = g_vx_resource_pool.fetchMatrix(context, 3, 3, VX_TYPE_FLOAT32);

	if (ovx_src_ptr && ovx_dst_ptr && ovx_mat_ptr)
	{
		vx_image vx_src = (vx_image)ovx_src_ptr->m_resource;
		vx_image vx_dst = (vx_image)ovx_dst_ptr->m_resource;
		vx_matrix vx_mat = (vx_matrix)ovx_mat_ptr->m_resource;
		OvxHelper::writeImage(vx_src, m_image.img_ptr, m_image.w, m_image.h, 8);
		OvxHelper::writeMatrix(vx_mat, ttr);

		i32 ret_code = vxuWarpPerspective(context, vx_src, vx_mat, VX_INTERPOLATION_BILINEAR, vx_dst);
		if (ret_code == VX_SUCCESS)
		{
			is_success = true;
			OvxHelper::readImage(m_roi_data_ptr->m_roi_img_ptr, w, h, vx_dst);
		}
	}
	g_vx_resource_pool.releaseResource(ovx_src_ptr);
	g_vx_resource_pool.releaseResource(ovx_mat_ptr);
	g_vx_resource_pool.releaseResource(ovx_dst_ptr);
#endif
	return is_success;
}
