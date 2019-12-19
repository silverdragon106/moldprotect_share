#include "mv_engine.h"
#include "base.h"
#include "proc/image_proc.h"
#include "logger/logger.h"
#include "mv_base.h"
#include "mv_template.h"
#include "roi_processor.h"
#include "auto_check_region.h"
#include "mv_application.h"

//#define POR_TESTMODE

#if defined(POR_WITH_OVX)
#include "openvx/mv_graph_pool.h"
#endif

CMVEngine::CMVEngine()
{
	m_is_inited = false;
	m_engine_param_ptr = NULL;
    m_img_group.freeBuffer();

	m_img_proc_ptr = NULL;
	m_auto_region_ptr = NULL;
	m_template_manager_ptr = NULL;

	for (i32 i = 0; i < PO_CAM_COUNT; i++)
	{
		m_roi_proc_ptr[i] = NULL;
	}
}

CMVEngine::~CMVEngine()
{
	exitInstance();
}

void CMVEngine::initInstance(EngineParam* engine_param_ptr, CTemplateManager* pTemplateMgr)
{
	singlelog_lv0("The MVEngine Module InitInstance");
	exitInstance();

	m_engine_param_ptr = engine_param_ptr;
	m_template_manager_ptr = pTemplateMgr;
	m_checked_step.initResult();

	for (i32 i = 0; i < PO_CAM_COUNT; i++)
	{
		m_roi_proc_ptr[i] = new CROIProcessor();
		m_roi_proc_ptr[i]->initInstance(i, m_engine_param_ptr);
	}

	m_img_proc_ptr = new CImageProc();
	m_auto_region_ptr = new CAutoCheckRegion(m_engine_param_ptr);
	m_is_inited = true;
}

void CMVEngine::exitInstance()
{
	if (m_is_inited)
	{
		singlelog_lv0("The MVEngine Module ExitInstance");

		m_is_inited = false;
		m_engine_param_ptr = NULL;
        m_img_group.freeBuffer();
		m_template_manager_ptr = NULL;
		m_checked_step.freeResult();

		for (i32 i = 0; i < PO_CAM_COUNT; i++)
		{
			POSAFE_DELETE(m_roi_proc_ptr[i]);
		}
		POSAFE_DELETE(m_img_proc_ptr);
		POSAFE_DELETE(m_auto_region_ptr);
	}
}

f32 CMVEngine::getMeasuredFocus(u8* img_ptr, i32 w, i32 h)
{
	if (!m_img_proc_ptr)
	{
		return -1.0f;
	}
	return m_img_proc_ptr->getImageFocus(img_ptr, w, h, m_engine_param_ptr->low_focus);
}

f32 CMVEngine::getMeasuredFocus(u8* img_ptr, i32 w, i32 h, Recti rt)
{
	if (!m_img_proc_ptr || rt.isEmpty())
	{
		return -1.0f;
	}
	return m_img_proc_ptr->getImageFocus(img_ptr, w, h, rt, m_engine_param_ptr->low_focus);
}

CCheckedStep& CMVEngine::getCheckedStepResult()
{
	return m_checked_step;
}

void CMVEngine::setImage(ImageGroup* img_group_ptr)
{
    m_img_group.setImageSnap(img_group_ptr);
}

u8* CMVEngine::getImage(i32 index, i32& w, i32& h)
{
    return m_img_group.getImage(index, w, h);
}

Img CMVEngine::getImg(i32 index)
{
	i32 w, h;
    u8* img_ptr = m_img_group.getImage(index, w, h);
	if (!img_ptr)
	{
		return Img();
	}
	return Img(img_ptr, w, h);
}

ImageGroup* CMVEngine::getImageGroup()
{
    return &m_img_group;
}

CROIProcessor* CMVEngine::getROIProcessor(i32 index)
{
	if (index < 0 || index >= PO_CAM_COUNT)
	{
		return NULL;
	}
	return m_roi_proc_ptr[index];
}

i32 CMVEngine::getCheckedResult()
{
	return m_checked_step.getResult();
}

i32 CMVEngine::studyModelAllROI(CMVTemplate* model_ptr)
{
	if (!model_ptr)
	{
		return kEngineErrInvalidData;
	}

	i32 i, j;
	for (i = 0; i < kMVStepCount; i++)
	{
		for (j = 0; j < PO_CAM_COUNT; j++)
		{
			if (studyROIRegions(model_ptr, i, j) != kEngineResultOK)
			{
				model_ptr->resetUpdate();
				return kEngineResultFail;
			}
		}
	}
	model_ptr->resetUpdate();
	return kEngineResultOK;
}

i32 CMVEngine::studyROIRegions(CMVTemplate* model_ptr, i32 stp, i32 cam)
{
	if (!model_ptr)
	{
		printlog_lv1("Can't study with empty model");
		return kEngineResultFail;
	}

	CMoldSnapData* mold_snap_ptr = model_ptr->getSnapData(stp, cam);
	if (!mold_snap_ptr)
	{
		printlog_lv1(QString("Can't study with invalid snap, stp%1, cam%2").arg(stp).arg(cam));
		return kEngineResultFail;
	}
	
	CMoldPiece* mold_piece_ptr;
	CMoldROIData* roi_data_ptr;
	CROIProcessor* roi_processor_ptr = getROIProcessor(0);
	singlelog_lvs3(QString("StuduyROIRegions. stp:%1, cam:%2").arg(stp).arg(cam), LOG_SCOPE_APP);

	i32 area = 0, hull = 0;
	i32 wsize = m_engine_param_ptr->lk_win_size;
	i32 max_level = m_engine_param_ptr->lk_max_level;
	i32 piece_count = mold_snap_ptr->getPieceCount();
	i32 roi_count = mold_snap_ptr->getROICount();
	i32 corner_count = 0;
	i32 i, j, update = mold_snap_ptr->getUpdate();

	//generation roi regions
	if (CPOBase::bitCheck(update, MDUPDATE_STUDY))
	{
		roi_processor_ptr->makeMaskImageInROI(mold_snap_ptr);
		roi_processor_ptr->updateAutoCheckMaskImage(mold_snap_ptr, model_ptr->getAutoChkRegion(cam));
	}
	area = mold_snap_ptr->getROIArea();
	printlog_lvs3(QString("StudyROIRegion, generation mask."), LOG_SCOPE_APP);

	for (i = 0; i < piece_count; i++)
	{
		//check update bit
		mold_piece_ptr = mold_snap_ptr->m_piece_vec[i];
		update = (mold_snap_ptr->getUpdate() | mold_piece_ptr->getUpdate());
		if (!CPOBase::bitCheck(update, MDUPDATE_STUDY))
		{
			continue;
		}

		//mold piece forget(include IgnoreRegion)
		mold_piece_ptr->freeLearnData();
		if (roi_count <= 0)
		{
			continue;
		}

		//check image buffer
		Img img = mold_piece_ptr->m_snap_image;
		if (!img.isValid())
		{
			ImgData* img_data_ptr = model_ptr->loadModelImage(mold_piece_ptr->getImgID());
			if (!img_data_ptr)
			{
				continue;
			}
			img = img_data_ptr->getImg();
			mold_piece_ptr->setSnapImage(img);
		}

		//build pyramid for motion estimation
#if defined(POR_IMVS2_ON_NVIDIA)
		i32 seed = g_vx_graph_pool.getNextSeed();
		mold_piece_ptr->m_ovx_pyramid = g_vx_resource_pool.createPyramid(
				g_vx_context.getVxContext(), img.w, img.h, VX_SCALE_PYRAMID_HALF, max_level, VX_DF_IMAGE_U8);

		CGPreUpdateImage* preupdate_graph_ptr = (CGPreUpdateImage*)g_vx_graph_pool.fetchGraph(
					kMGPreUpdateImage, mold_piece_ptr, &img);
		if (preupdate_graph_ptr)
		{
			preupdate_graph_ptr->schedule(seed);
		}
#else
		cv::Mat cv_model_img(img.h, img.w, CV_8UC1, img.img_ptr);
		std::vector<cv::Mat>& cv_img_pyramid = mold_piece_ptr->m_cv_pyramid;
		cv::buildOpticalFlowPyramid(cv_model_img, cv_img_pyramid, cv::Size(wsize, wsize), max_level);
#endif
		printlog_lvs3(QString("StudyPiece:%1, build pyramid for motion estimation.").arg(i), LOG_SCOPE_APP);

		//set ROIMaskRegion in Piece
		for (j = 0; j < roi_count; j++)
		{
			roi_data_ptr = CPOBase::pushBackNew(mold_piece_ptr->m_roi_data);
			roi_data_ptr->setROIMask(mold_snap_ptr->m_roi_mask_vec[j]);
			roi_data_ptr->initROIImage();
		}

		//study each ROI regions
		corner_count = 0;
		roi_processor_ptr->setBasedROIData(NULL, &img);
#if defined(POR_IMVS2_ON_NVIDIA)
		for (j = 0; j < roi_count; j++)
		{
			roi_data_ptr = mold_piece_ptr->m_roi_data[j];
			roi_processor_ptr->setBasedROIData(roi_data_ptr);
			if (roi_processor_ptr->makeCheckDataInROI(MV_ROIPROC_CROPIMG, area) != kEngineResultOK)
			{
				return kEngineResultFail;
			}

			CGStudyROIRegion* roistudy_graph_ptr = (CGStudyROIRegion*)g_vx_graph_pool.fetchGraph(
					kMGStudyROIRegion, roi_data_ptr, m_engine_param_ptr);
			if (roistudy_graph_ptr)
			{
				roistudy_graph_ptr->schedule(seed);
			}
			if (roi_processor_ptr->makeCheckDataInROI(MV_ROIPROC_CORNER, area) != kEngineResultOK)
			{
				return kEngineResultFail;
			}
			corner_count += roi_data_ptr->getCornerCount();
		}
#else 
		for (j = 0; j < roi_count; j++)
		{
			roi_data_ptr = mold_piece_ptr->m_roi_data[j];
			roi_processor_ptr->setBasedROIData(roi_data_ptr);
			if (roi_processor_ptr->makeCheckDataInROI(MV_ROIPROC_STUDY, area) != kEngineResultOK)
			{
				return kEngineResultFail;
			}
			corner_count += roi_data_ptr->getCornerCount();
		}
#endif
		printlog_lvs3(QString("StudyPiece:%1, study all regions(%2).").arg(i).arg(roi_count), LOG_SCOPE_APP);

		//calc convex-hull area for motion estimation
		if (corner_count >= MVENGINE_MINPAIRNUM)
		{
			roi_processor_ptr->initLKPairs(corner_count);
			for (j = 0; j < roi_count; j++)
			{
				roi_data_ptr = mold_piece_ptr->m_roi_data[j];
				roi_processor_ptr->collectCornerPoints(roi_data_ptr);
			}
			hull = (i32)roi_processor_ptr->calcConvexHull(roi_processor_ptr->m_lk_lpoint_vec, 
													mold_piece_ptr->m_border_pts, img.w, img.h);
			roi_processor_ptr->freeLKPairs();
		}

		mold_piece_ptr->m_area = area;
		mold_piece_ptr->m_hull_area = hull;
		mold_piece_ptr->m_corner_count = corner_count;
		printlog_lvs3(QString("StudyPiece:%1, finished.").arg(i), LOG_SCOPE_APP);

#if defined(POR_IMVS2_ON_NVIDIA)
		g_vx_graph_pool.releaseSeedGraphs(seed);
#endif
	}
	return kEngineResultOK;
}

bool CMVEngine::preUpdateROI(CMoldPiece* mdl_piece_ptr, ImgData* img_data_ptr, ROIMaskPVector& roi_mask_vec)
{
	if (!mdl_piece_ptr || !img_data_ptr)
	{
		return false;
	}

	i32 wsize = m_engine_param_ptr->lk_win_size;
	i32 max_level = m_engine_param_ptr->lk_max_level;

	Img img = img_data_ptr->getImg();
	mdl_piece_ptr->setSnapImage(img);
	mdl_piece_ptr->freePyramidData();

	CMoldROIData* roi_data_ptr;
	MoldROIPProcs& roi_data_vec = mdl_piece_ptr->m_roi_data;
	i32 i, roi_count = (i32)roi_data_vec.size();
	if (roi_count != roi_mask_vec.size())
	{
		return false;
	}

	//update each ROI mask
	for (i = 0; i < roi_count; i++)
	{
		roi_data_ptr = roi_data_vec[i];
		roi_data_ptr->setROIMask(roi_mask_vec[i]);
	}

#if defined(POR_IMVS2_ON_NVIDIA)
	mdl_piece_ptr->m_ovx_pyramid = g_vx_resource_pool.createPyramid(
			g_vx_context.getVxContext(), img.w, img.h, VX_SCALE_PYRAMID_HALF, max_level, VX_DF_IMAGE_U8);
	
	CGPreUpdateImage* preupdate_graph_ptr = (CGPreUpdateImage*)g_vx_graph_pool.fetchGraph(
			kMGPreUpdateImage, mdl_piece_ptr, &img);
	if (preupdate_graph_ptr)
	{
		preupdate_graph_ptr->process();
	}
#else
	cv::Mat cv_mdl_img(img.h, img.w, CV_8UC1, img.img_ptr);
	std::vector<cv::Mat>& img_pyramid = mdl_piece_ptr->m_cv_pyramid;
	cv::buildOpticalFlowPyramid(cv_mdl_img, img_pyramid, cv::Size(wsize, wsize), max_level);
#endif

#if defined(POR_IMVS2_ON_NVIDIA)
	//update each ROI
	i32 seed = g_vx_graph_pool.getNextSeed();
	for (i = 0; i < roi_count; i++)
	{
		roi_data_ptr = roi_data_vec[i];
		if (!roi_data_ptr->hasSSIMMap())
		{
			CGPrepStSqSimilar* prep_stsq_graph_ptr = (CGPrepStSqSimilar*)g_vx_graph_pool.fetchGraph(
					kMGPrepStSqSimilar, roi_data_ptr, m_engine_param_ptr);
			if (prep_stsq_graph_ptr)
			{
				prep_stsq_graph_ptr->schedule(seed);
			}
		}
	}
	g_vx_graph_pool.releaseSeedGraphs(seed);
#else
	//update each ROI
	CROIProcessor* roi_processor_ptr = getROIProcessor(0);
	for (i = 0; i < roi_count; i++)
	{
		roi_data_ptr = roi_data_vec[i];
		if (!roi_data_ptr->hasSSIMMap())
		{
			roi_processor_ptr->updateSSIMMap(roi_data_ptr);
		}
	}
#endif
	return true;
}

i32 CMVEngine::checkMoldPiece(i32 stp, i32 cam, ImageData* img_data_ptr,
						CMoldSnapData* mdl_snap_ptr, CMoldPiece* mdl_piece_ptr, bool use_autochk_mask,
						CMoldResult* mold_result_ptr)
{
	singlelog_lvs3(QString("CheckPiece, stp:%1 cam:%2").arg(stp).arg(cam), LOG_SCOPE_APP);
	if (!mdl_snap_ptr || !mdl_piece_ptr || !mold_result_ptr || !img_data_ptr)
	{
		return kEngineResultFail;
	}

	ROIPVector& roi_ptr_vec = mdl_snap_ptr->getROIPVector();
	ROIMaskPVector& roi_mask_vec = mdl_snap_ptr->getROIMaskPVector();

	Img img = img_data_ptr->toImg();
	CROIProcessor* roi_processor_ptr = m_roi_proc_ptr[cam];
	if (!roi_processor_ptr || !img.isValid())
	{
		printlog_lvs2(QString("ROIProcessor%1 is not exist or image is invalid.").arg(cam), LOG_SCOPE_APP);
		return kEngineResultFail;
	}

	DefectParam* roi_param_ptr;
	CMoldROIData* roi_data_ptr;
	CMoldROIData* roi_mdl_ptr;
	CMoldROIResult* roi_result_ptr;
	i32 i, count = mdl_piece_ptr->getROIDataCount();
	mold_result_ptr->initBuffer(stp, cam, count);

	//check ROI count in piece
	if (count <= 0 || count != roi_ptr_vec.size())
	{
		mold_result_ptr->addResultDesc(kMoldRDescNoneROIPass);
		return kEngineResultOK;
	}

	//1.prebuild mask image of each ROI
	keep_time(10);
	roi_processor_ptr->setBasedROIData(NULL, &img);

	for (i = 0; i < count; i++)
	{
		roi_mdl_ptr = mdl_piece_ptr->m_roi_data[i];
		roi_data_ptr = mold_result_ptr->m_roi_data_vec[i];
		if (roi_processor_ptr->makeMaskImageInROI(roi_mdl_ptr,
					roi_mask_vec[i]->getMaskImage(use_autochk_mask), roi_data_ptr) != kEngineResultOK)
		{
			mold_result_ptr->setResult(kMoldResultFail);
			return kEngineResultFail;
		}
	}
	leave_time(10);

	//2.estimation motion each ROI regions between model data and input frame
	keep_time(11);
	if (m_engine_param_ptr->useMotionEstimation())
	{
		f32 tr[9];
		CPOBase::init3x3(tr);
		roi_processor_ptr->initLKPairs(mdl_piece_ptr->m_corner_count);
		for (i = 0; i < count; i++)
		{
			roi_mdl_ptr = mdl_piece_ptr->m_roi_data[i];
			roi_processor_ptr->collectCornerPoints(roi_mdl_ptr);
		}

		//estimate global motion transform on current version
		if (roi_processor_ptr->estimateGlobalMotion(mdl_piece_ptr, img, tr))
		{
			for (i = 0; i < count; i++)
			{
				roi_result_ptr = mold_result_ptr->m_roi_result_vec[i];
				roi_result_ptr->setTransform(tr, 9);
			}
			mold_result_ptr->setTransform(tr, 9);
			mold_result_ptr->addResultDesc(kMoldRDescEstMotion);
		}
		roi_processor_ptr->freeLKPairs();
	}
	leave_time(11);

	//3.check each ROI in current frame
	keep_time(12);
	for (i = 0; i < count; i++)
	{
		roi_mdl_ptr = mdl_piece_ptr->m_roi_data[i];
		roi_data_ptr = mold_result_ptr->m_roi_data_vec[i];
		roi_result_ptr = mold_result_ptr->m_roi_result_vec[i];
		roi_param_ptr = roi_ptr_vec[i]->getMoldParam();
		roi_processor_ptr->setBasedROIData(roi_data_ptr, NULL, roi_param_ptr);
		if (roi_processor_ptr->checkROIRegion(roi_mdl_ptr, roi_result_ptr) != kEngineResultOK)
		{
			mold_result_ptr->setResult(kMoldResultFail);
			return kEngineResultFail;
		}
	}
	leave_time(12);

	//4.update result with all ROI results
	mold_result_ptr->updateResult();
	return kEngineResultOK;
}

i32 CMVEngine::addROIIgnore(CMoldPiece* mold_piece_ptr, AlarmPVector& alarm_vec)
{
	i32 al_count = (i32)alarm_vec.size();
	i32 roi_count = mold_piece_ptr->getROIDataCount();
	if (!CPOBase::isPositive(al_count) || !CPOBase::isPositive(roi_count))
	{
		return kEngineResultFail;
	}

	i32 i, j;
	i32 sum = 0;
	i32 msum = 0;
	i32 area = mold_piece_ptr->getROIPixels();
	i32 cov_pixel = m_engine_param_ptr->ignore_cov_pixel;
	f32 igr_acc_rate = m_engine_param_ptr->ignore_acc_rate;
	bool is_copied = false;
	bool is_updated = false;

	//accumulate alarm and igr pixels
	for (i = 0; i < al_count; i++)
	{
		sum += alarm_vec[i]->getAlarmPixels();
	}
	msum += mold_piece_ptr->getIgrPixels();

	//check igr pixel count
	if ((sum + msum) > igr_acc_rate*area)
	{
		return kEngineErrInvalidData;
	}

	for (i = 0; i < roi_count; i++)
	{
		CMoldROIData* roi_data_ptr = mold_piece_ptr->m_roi_data[i];
		Recti range = roi_data_ptr->m_range;
		u8* img_ptr = CImageProc::makeZeroImage(range);

		is_copied = false;
		for (j = 0; j < al_count; j++)
		{
			is_copied |= alarm_vec[j]->copyToImage(img_ptr, range);
		}
		if (is_copied)
		{
			is_updated = true;
			i32 w = range.getWidth();
			i32 h = range.getHeight();
			u8* igr_img_ptr = roi_data_ptr->getIgrImage();
			CImageProc::dilateImage(img_ptr, w, h, cov_pixel);
			CImageProc::addInvImage(img_ptr, igr_img_ptr, w, h);
			roi_data_ptr->setROIIgnore(img_ptr, range);

#ifdef POR_TESTMODE
			CImageProc::saveImgOpenCV(PO_DEBUG_PATH"updated_igr.bmp", roi_data_ptr->getIgrImage(), roi_data_ptr->getROIWidth(), roi_data_ptr->getROIHeight());
#endif
		}
		POSAFE_DELETE_ARRAY(img_ptr);
	}

	if (is_updated)
	{
		mold_piece_ptr->updateIgrPixels();
		mold_piece_ptr->setUpdate(MDUPDATE_KEEP);
	}
	return kEngineResultOK;
}

void CMVEngine::preUpdateAutoChkRegion(CMVTemplate* model_ptr, i32 cam_index)
{
	if (m_auto_region_ptr)
	{
		CROIProcessor* roi_processor_ptr = getROIProcessor(cam_index);
		CAutoCheckData* autochk_data_ptr = model_ptr->getAutoChkRegion(cam_index);
		m_auto_region_ptr->preUpdateAutoChkRegion(autochk_data_ptr, roi_processor_ptr);

		for (i32 i = 0; i < kMVStepCount; i++)
		{
			CMoldSnapData* mold_snap_ptr = model_ptr->getSnapData(i, cam_index);
			roi_processor_ptr->updateAutoCheckMaskImage(mold_snap_ptr, autochk_data_ptr);
		}
	}
}

bool CMVEngine::makeAutoChkRegion(CMVTemplate* model_ptr, i32 cam_index)
{
	if (model_ptr && m_auto_region_ptr)
	{
		bool is_success = m_auto_region_ptr->makeAutoChkRegion(model_ptr, cam_index);
		if (is_success)
		{
			preUpdateAutoChkRegion(model_ptr, cam_index);
		}
		return is_success;
	}
	return false;
}

bool CMVEngine::preProcessCheck(i32 stp, ImageGroup* img_group_ptr)
{
	m_checked_step.initResult(stp, img_group_ptr->m_time_stamp);
	setImage(img_group_ptr);

	CMVTemplate* model_ptr = m_template_manager_ptr->getCurModel();
	if (!model_ptr || !model_ptr->hasModelData(kModelContent))
	{
		return false;
	}
	return true;
}

void CMVEngine::postProcessCheck()
{
	CMVTemplate* model_ptr = m_template_manager_ptr->getCurModel();
	if (!model_ptr)
	{
		return;
	}
	m_checked_step.updateResult(model_ptr);
}

i32 CMVEngine::checkMoldSnap(i32 stp, i32 cam, bool use_autochk_region, ImageData* img_data_ptr)
{
	CMVTemplate* model_ptr = m_template_manager_ptr->getCurModel();
	if (!model_ptr)
	{
		return kEngineResultFail;
	}

	CMoldResult tmp_result;
	CMoldPiece* model_piece_ptr;
	CMoldResult* mold_result_ptr = &m_checked_step.m_snap_result_vec[cam];
	CMoldSnapData* model_snap_ptr = model_ptr->getSnapData(stp, cam);
	if (!model_snap_ptr)
	{
		return kEngineResultFail;
	}

	i32 i, piece;
	i32 ibest_ok, iok = -1;
	i32vector& piece_order_vec = model_snap_ptr->m_piece_order;
	i32 count = (i32)piece_order_vec.size();

	//all piece of modelsnap
	sys_keep_time;
	for (i = 0; i < count; i++)
	{
		piece = piece_order_vec[i];
		model_piece_ptr = model_snap_ptr->getMoldPiece(piece);

		if (checkMoldPiece(stp, cam, img_data_ptr, model_snap_ptr,
					model_piece_ptr, use_autochk_region, &tmp_result) == kEngineResultOK)
		{
			if (tmp_result.isGoodThan(mold_result_ptr))
			{
				iok = i;
				mold_result_ptr->setResult(&tmp_result, piece);
				if (mold_result_ptr->isOKResult())
				{
					break;
				}
			}

			//if don't use multi-template model, app should be break;
			if (!m_engine_param_ptr->useMultiTemplate())
			{
				break;
			}
		}
		else
		{
			mold_result_ptr->setResult(kMoldResultFail);
		}

		//free buffer unused result
		tmp_result.freeBuffer();
	}

	//update piece order for improve checking speed...
	if (iok > 0)
	{
		//calc new index
		ibest_ok = (i32)(iok*m_engine_param_ptr->ahead_rate);
		model_snap_ptr->changeOrder(iok, ibest_ok);
	}

	mold_result_ptr->m_proc_time = sys_get_time;
	return kEngineResultOK;
}

i32 CMVEngine::checkMoldStepFromImg(i32 stp, u8* autochk_mode_ptr, ImageGroup* img_group_ptr)
{
	singlelog_lvs3(QString("Check ImageGroup, stp:%1").arg(stp), LOG_SCOPE_APP);

	//preprocess
	if (!preProcessCheck(stp, img_group_ptr))
	{
		return kEngineResultFail;
	}

	//check each mold of frame for all camera
	for (i32 i = 0; i < PO_CAM_COUNT; i++)
	{
		ImageData* img_data_ptr = img_group_ptr->getImgData(i);
		if (!img_data_ptr || !img_data_ptr->isSnapImage())
		{
			continue;
		}

		bool use_autochk_region = false;
		if (autochk_mode_ptr)
		{
			use_autochk_region = (autochk_mode_ptr[i] == kROICheckAutoRegion);
		}

		checkMoldSnap(stp, i, use_autochk_region, img_data_ptr);
	}

	postProcessCheck();
	return kEngineResultOK;
}

i32 CMVEngine::checkMoldSnapFromImg(i32 stp, i32 cam, bool use_autochk_region, ImageGroup* img_group_ptr)
{
	singlelog_lvs3(QString("Check SnapImage, stp:%1, cam:%2").arg(stp).arg(cam), LOG_SCOPE_APP);
	keep_checksnap;

	//preprocess
	if (!preProcessCheck(stp, img_group_ptr))
	{
		return kEngineResultFail;
	}

	//check specified mold of snap
	ImageData* img_data_ptr = img_group_ptr->getImgData(cam);
	if (!img_data_ptr || !img_data_ptr->isSnapImage())
	{
		return kEngineResultFail;
	}

	if (checkMoldSnap(stp, cam, use_autochk_region, img_data_ptr) != kEngineResultOK)
	{
		return kEngineResultFail;
	}

	leave_checksnap;

	//update result of checked step data
	CMVTemplate* model_ptr = m_template_manager_ptr->getCurModel();
	if (!model_ptr)
	{
		return kEngineResultFail;
	}

	m_checked_step.updateResult(model_ptr);
	m_checked_step.m_proc_time = get_checksnap;
	return kEngineResultOK;
}

void CMVEngine::testMoldPiece(CMoldPiece* mold_piece_ptr)
{
	if (!mold_piece_ptr)
	{
		return;
	}

	CROIProcessor* roi_processor_ptr = getROIProcessor(0);
	CMoldROIData* roi_data_ptr;

	i32 i, w, h, count;
	count = mold_piece_ptr->getROIDataCount();

	for (i = 0; i < count; i++)
	{
		roi_data_ptr = mold_piece_ptr->m_roi_data[i];
		if (!roi_data_ptr)
		{
			continue;
		}

		w = roi_data_ptr->getROIWidth();
		h = roi_data_ptr->getROIHeight();

		//save mask and ignore
		CImageProc::saveBinImgOpenCV(PO_DEBUG_PATH"vx_mask.bmp", roi_data_ptr->getMaskImage(), w, h);
		CImageProc::saveImgOpenCV(PO_DEBUG_PATH"vx_igr.bmp", roi_data_ptr->getIgrImage(), w, h);

		//save corners
		roi_processor_ptr->testPtsWithImg(PO_DEBUG_PATH"vx_std.bmp",
							roi_data_ptr->m_corner_vec, roi_data_ptr->getROIImage(), w, h);

		//save ssim map
		SSIMMap& ssim_map = roi_data_ptr->getSSIMMap();
		CImageProc::saveImgOpenCV(PO_DEBUG_PATH"vx_ssim_edge.bmp", ssim_map.edge_img_ptr, w ,h);

		cv::Mat cv_stsq_img(h, w, CV_16UC1, ssim_map.mean_stsq_ptr);
		cv_stsq_img /= 10;
		cv::imwrite(PO_DEBUG_PATH"vx_ssim_sq_edge.bmp", cv_stsq_img);
	}
}
