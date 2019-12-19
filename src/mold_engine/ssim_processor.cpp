#include "ssim_processor.h"
#include "mv_base.h"
#include "logger/logger.h"
#include "proc/image_proc.h"

//#define POR_TESTMODE
const i32 kConstSSIM_C2 = 328;

//////////////////////////////////////////////////////////////////////////
CSSIMProcessor::CSSIMProcessor()
{
	initBuffer();
	initInstance(NULL);
}

CSSIMProcessor::~CSSIMProcessor()
{
	freeBuffer();
	exitInstance();
}

void CSSIMProcessor::initInstance(EngineParam* engine_param_ptr)
{
	m_engine_param_ptr = engine_param_ptr;
}

void CSSIMProcessor::exitInstance()
{

}

void CSSIMProcessor::prepareStructSqSimilar(CMoldROIData* roi_data_ptr, bool init_ssim_map, bool use_mp)
{
	i32 w = roi_data_ptr->getROIWidth();
	i32 h = roi_data_ptr->getROIHeight();
	u8* roi_img_ptr = roi_data_ptr->getROIImage();
	u8* mask_img_ptr = roi_data_ptr->getMaskImage();

	if (w <= 0 || h <= 0 || !roi_img_ptr || !mask_img_ptr)
	{
		return;
	}

	i32 wh = w*h;
	SSIMMap& map = roi_data_ptr->getSSIMMap();

	if (use_mp)
	{
		map.initBuffer(w, h, init_ssim_map, true);
		CPOMemPool::getBuffer(map.mean_stsq_ptr, wh);
		CPOMemPool::getBuffer(map.edge_img_ptr, wh);
		CPOMemPool::getBuffer(map.ssim_map_ptr, wh);
	}
	else
	{
		CPOMemPool::releaseBuffer();
		map.initBuffer(w, h, init_ssim_map, false);
	}
	
 	//make edge image
 	u8* edge_img_ptr = map.edge_img_ptr;
 	u16* mean_stsq_ptr = map.mean_stsq_ptr;
	cv::Mat cv_roi_img(h, w, CV_8UC1, roi_img_ptr);
	cv::Mat cv_edge_img(h, w, CV_8UC1, edge_img_ptr);
 	CImageProc::makeRobertGradImage(edge_img_ptr, roi_img_ptr, w, h);

	//make mean_edge_sq_image
	i32 ssim_window = m_engine_param_ptr->ssim_window;
	cv::Mat cv_mean_edge_sq2(h, w, CV_16UC1, mean_stsq_ptr);

	cv::Mat cv_edge, cv_edge_sq2;
	cv_edge_img.convertTo(cv_edge, CV_16UC1);
	cv_edge_sq2 = cv_edge.mul(cv_edge);
    cv::GaussianBlur(cv_edge_sq2, cv_mean_edge_sq2, cv::Size(ssim_window, ssim_window), 0.5f);
}

void CSSIMProcessor::makeStructSqSimilar(CMoldROIData* roi_model_ptr, CMoldROIData* roi_data_ptr)
{
	if (roi_data_ptr == NULL || roi_model_ptr == NULL)
	{
		return;
	}

	i32 w = roi_data_ptr->getROIWidth();
	i32 h = roi_data_ptr->getROIHeight();
	i32 ssim_window = m_engine_param_ptr->ssim_window;
	SSIMMap& ssim_map1 = roi_model_ptr->getSSIMMap();
	SSIMMap& ssim_map2 = roi_data_ptr->getSSIMMap();

	cv::Mat cv_edge_img1(h, w, CV_8UC1, ssim_map1.edge_img_ptr);
	cv::Mat cv_edge_img2(h, w, CV_8UC1, ssim_map2.edge_img_ptr);

	cv::Mat cv_edge1, cv_edge2;
	cv_edge_img1.convertTo(cv_edge1, CV_16UC1);
	cv_edge_img2.convertTo(cv_edge2, CV_16UC1);
	cv::Mat cv_edge_conv = cv_edge1.mul(cv_edge2);
	cv::Mat cv_mean_edge_conv;
    cv::GaussianBlur(cv_edge_conv, cv_mean_edge_conv, cv::Size(ssim_window, ssim_window), 0.5f);

	u16* tmp_mean_sq1_ptr = ssim_map1.mean_stsq_ptr;
	u16* tmp_mean_sq2_ptr = ssim_map2.mean_stsq_ptr;
	u16* tmp_edge_conv_ptr = (u16*)cv_mean_edge_conv.data;
	u16* tmp_edge_similar = ssim_map2.ssim_map_ptr;

	i32 i, tmp, wh = w*h;
	for (i = 0; i < wh; i++)
	{
        tmp = (i32)(2 * (*tmp_edge_conv_ptr) + kConstSSIM_C2) * 255;
        tmp = tmp / (*tmp_mean_sq1_ptr + *tmp_mean_sq2_ptr + kConstSSIM_C2);
		*tmp_edge_similar = tmp;
		tmp_mean_sq1_ptr++;
		tmp_mean_sq2_ptr++;
		tmp_edge_conv_ptr++;
		tmp_edge_similar++;
	}

#ifdef POR_TESTMODE
	cv::Mat edge1(h, w, CV_8UC1, ssim_map1.edge_img_ptr);
	cv::Mat edge2(h, w, CV_8UC1, ssim_map2.edge_img_ptr);
	cv::Mat cv_edge_similar(h, w, CV_16UC1, ssim_map2.ssim_map_ptr);
	cv::imwrite(PO_DEBUG_PATH"edge1.bmp", edge1);
	cv::imwrite(PO_DEBUG_PATH"edge2.bmp", edge2);
	cv::imwrite(PO_DEBUG_PATH"st.bmp", cv_edge_similar);
#endif
}

void CSSIMProcessor::updateSimilar(CMoldROIData* roi_data_ptr, u8* sub_img_ptr, u8* mask_img_ptr)
{
	if (!roi_data_ptr || !sub_img_ptr)
	{
		return;
	}

	i32 w = roi_data_ptr->getROIWidth();
	i32 h = roi_data_ptr->getROIHeight();
	i32 i, tmp, wh = w*h;

	u8* tmp_sub_img_ptr = sub_img_ptr;
	u8* tmp_mask_img_ptr = mask_img_ptr;
	u16* tmp_edge_similar_ptr = roi_data_ptr->getSSIMMap().ssim_map_ptr;

	for (i = 0; i < wh; i++)
	{
		if (*tmp_mask_img_ptr == MV_REGION_MASKPLUS)
		{
			tmp = (i32)(*tmp_edge_similar_ptr * (255 - *tmp_sub_img_ptr)) >> 8;
			*tmp_sub_img_ptr = 255 - po::_min(tmp, 255);
		}
		tmp_sub_img_ptr++;
		tmp_mask_img_ptr++;
		tmp_edge_similar_ptr++;
	}

#ifdef POR_TESTMODE
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"similar.bmp", sub_img_ptr, w, h);
#endif
}
