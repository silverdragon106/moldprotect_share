#include "auto_check_region.h"
#include "logger/logger.h"
#include "proc/image_proc.h"
#include "proc/connected_components.h"
#include "mv_base.h"
#include "mv_template.h"
#include "roi_processor.h"

//#define POR_TESTMODE

//////////////////////////////////////////////////////////////////////////
CAutoCheckRegion::CAutoCheckRegion(EngineParam* engine_param_ptr)
{
	m_engine_param_ptr = engine_param_ptr;
}

CAutoCheckRegion::~CAutoCheckRegion()
{
	m_engine_param_ptr = NULL;
}

void CAutoCheckRegion::preUpdateAutoChkRegion(CAutoCheckData* autochk_data_ptr, CROIProcessor* roi_processor_ptr)
{
	if (!autochk_data_ptr)
	{
		return;
	}
	
	Recti range;
	i32 i, j, point_count;
	autochk_data_ptr->freeMaskBuffer();

	shapeuvector& auto_region_vec = autochk_data_ptr->m_auto_region_vec;
	i32 region_count = (i32)auto_region_vec.size();
	if (region_count <= 0)
	{
		return;
	}

	/* 자동검사구역목록의 전체정점들을 순환하면서 외접4각형을 구한다. */
	for (i = 0; i < region_count; i++)
	{
		ptuvector& point_vec = auto_region_vec[i];
		point_count = (i32)point_vec.size();

		for (j = 0; j < point_count; j++)
		{
			range.insertPoint(vector2di(point_vec[j].x, point_vec[j].y));
		}
	}

	/* 자동검사구역목록의 개별적구역들에 대한 마스크화상들을 생성조립한다. */
	CTransRect trans_shape;
	i32 width = range.getWidth();
	i32 height = range.getHeight();
	if (width <= 0 && height <= 0)
	{
		return;
	}

	autochk_data_ptr->initMaskBuffer(range);
	for (i = 0; i < region_count; i++)
	{
		ptuvector& point_vec = auto_region_vec[i];
		trans_shape.setRegionData(kTransTypePolygon, point_vec.data(), (i32)point_vec.size());
		roi_processor_ptr->updateRegionMap(autochk_data_ptr->m_auto_mask_ptr, range, range, 
									&trans_shape, MV_REGION_MASKPLUS, MV_REGION_MASKPLUS);
	}

#ifdef POR_TESTMODE
	CImageProc::saveBinImgOpenCV(PO_DEBUG_PATH"autochk.bmp", autochk_data_ptr->m_auto_mask_ptr, width, height);
#endif
}

bool CAutoCheckRegion::makeAutoChkRegion(CMVTemplate* model_ptr, i32 cam_index)
{
	if (!model_ptr || !CPOBase::checkIndex(cam_index, PO_CAM_COUNT))
	{
		return false;
	}

	ImgData* product_img_ptr = model_ptr->getModelImage(kMVStepProduct, cam_index);
	ImgData* eject_img_ptr = model_ptr->getModelImage(kMVStepEject, cam_index);
	if (!product_img_ptr || !eject_img_ptr)
	{
		return false;
	}
	if (!product_img_ptr->isEqualFormat(eject_img_ptr))
	{
		return false;
	}

	CAutoCheckData& auto_chk_data = model_ptr->m_auto_chk_data[cam_index];
	auto_chk_data.freeBuffer();

	i32 w = product_img_ptr->w;
	i32 h = product_img_ptr->h;
	makeAutoChkRegion(product_img_ptr->img_ptr, eject_img_ptr->img_ptr, w, h, auto_chk_data.m_auto_region_vec);

	if (auto_chk_data.m_auto_region_vec.size() > 0)
	{
		return true;
	}
	return false;
}

void CAutoCheckRegion::makeAutoChkRegion(u8* prev_img_ptr, u8* cur_img_ptr, i32 w, i32 h, shapeuvector& shape_vec)
{
	shape_vec.clear();
	if (!prev_img_ptr || !cur_img_ptr || w <= 0 || h <= 0)
	{
		return;
	}

	i32 cov_pixels = m_engine_param_ptr->auto_chk_cov_pixels;
	u8* sub_img_ptr = new u8[w*h];
	u16* cc_img_ptr = new u16[w*h];

	/* 두 화상의 차분값을 계산하고 축소/팽창연산을 진행한다. */
	cv::Mat cv_prev_img(h, w, CV_8UC1, prev_img_ptr);
	cv::Mat cv_cur_img(h, w, CV_8UC1, cur_img_ptr);
	cv::Mat cv_sub_img(h, w, CV_8UC1, sub_img_ptr);
	cv::absdiff(cv_prev_img, cv_cur_img, cv_sub_img);

	f32 dmn = CImageProc::getThresholdLowBand(sub_img_ptr, NULL, w, h) + MVPARAM_GAL_DIFF;
	f32 dmn_delta = MVPARAM_GAH_DIFF - MVPARAM_GAL_DIFF;
	cv::subtract(cv_sub_img, (i32)(dmn + 0.5f), cv_sub_img);

	if (m_engine_param_ptr->useSpotDetection())
	{
		cv::Mat cv_max_img;
		cv::max(cv_prev_img, cv_cur_img, cv_max_img);
		cv::threshold(cv_max_img, cv_max_img, m_engine_param_ptr->spot_intensity, 0xFF, cv::THRESH_BINARY_INV);
		cv::bitwise_and(cv_max_img, cv_sub_img, cv_sub_img);
	}
	CImageProc::erodeImage(sub_img_ptr, w, h, cov_pixels / 2);

	cv::Mat cv_prev_edge(h, w, CV_8UC1);
	cv::Mat cv_cur_edge(h, w, CV_8UC1);
	cv::Mat cv_sub_edge;
	CImageProc::makeRobertGradImage((u8*)cv_prev_edge.data, prev_img_ptr, w, h);
	CImageProc::makeRobertGradImage((u8*)cv_cur_edge.data, cur_img_ptr, w, h);
	cv::absdiff(cv_prev_edge, cv_cur_edge, cv_sub_edge);

	f32 edn = CImageProc::getThresholdLowBand((u8*)cv_sub_edge.data, NULL, w, h) + MVPARAM_GAL_DIFF;
	cv::subtract(cv_sub_edge, edn, cv_sub_edge);

	cv_sub_img += cv_sub_edge;
	CImageProc::dilateImage(sub_img_ptr, w, h, cov_pixels);
	printlog_lvs2("makeAutoChkRegion image operation done.", LOG_SCOPE_APP);

#ifdef POR_TESTMODE
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"auto_prev.bmp", prev_img_ptr, w, h);
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"auto_next.bmp", cur_img_ptr, w, h);
	CImageProc::saveImgOpenCV(PO_DEBUG_PATH"auto_chk_diff.bmp", sub_img_ptr, w, h);
	cv::imwrite(PO_DEBUG_PATH"auto_edge_diff.bmp", cv_sub_edge);
#endif

	/* 검사구역을 추정한다. */
	i32 cc_count = 0;
	ConnComp* cc_ptr;
	CConnectedComponents conn_comp;
	conn_comp.checkBuffer(w, h);

	cc_ptr = conn_comp.getConnectedComponents(sub_img_ptr, cc_img_ptr, w, h, cc_count);
	if (cc_count <= 0 || !cc_ptr)
	{
		POSAFE_DELETE_ARRAY(sub_img_ptr);
		POSAFE_DELETE_ARRAY(cc_img_ptr);
		return;
	}

#ifdef POR_TESTMODE
	cv::imwrite(PO_DEBUG_PATH"auto_chk_ccidx.bmp", cv::Mat(h, w, CV_16UC1, cc_img_ptr));
#endif

	checkValidAutoChkConnComps(sub_img_ptr, cc_img_ptr, w, h, dmn_delta, cc_count, cc_ptr);
	conn_comp.getConnectedComponentEdge(sub_img_ptr, cc_img_ptr, w, h,
							cc_ptr, cc_count, kPOPixelOperOutterEdge | kPOPixelOperClosedEdge);
	buildAutoCheckRegion(cc_ptr, cc_count, w, h, shape_vec);
	printlog_lvs2("makeAutoChkRegion build regions.", LOG_SCOPE_APP);

	/* 할당된 림시기억들을 해방한다. */
	POSAFE_DELETE_ARRAY(sub_img_ptr);
	POSAFE_DELETE_ARRAY(cc_img_ptr);
	POSAFE_DELETE_ARRAY(cc_ptr);
}

void CAutoCheckRegion::checkValidAutoChkConnComps(u8* sub_img_ptr, u16* cc_img_ptr, i32 w, i32 h, f32 dmn_high,
											i32 cc_count, ConnComp* cc_ptr)
{
	if (!sub_img_ptr || !cc_img_ptr || w <= 0 || h <= 0 || cc_count <= 0 || !cc_ptr)
	{
		return;
	}

	i32 i, wh = w*h;
	ConnComp* conn_ptr;
	i32 min_pixels = m_engine_param_ptr->auto_chk_min_pixels;
	i32 min_size = m_engine_param_ptr->auto_chk_min_size;
	min_size = po::_max(min_size, po::_max(w, h)*m_engine_param_ptr->auto_chk_min_rate);

	/* 모든 련결성분들을 무효화한다.*/
	for (i = 0; i < cc_count; i++)
	{
		cc_ptr[i].is_valid = false;
	}

	/* 현재의 련결성분에 씨드(차분이 dmn_high보다 큰)화소점이 존재하는가를 검사한다. */
	for (i = 0; i < wh; i++)
	{
		if (*sub_img_ptr > dmn_high)
		{
			conn_ptr = cc_ptr + (*cc_img_ptr - 1);
			conn_ptr->is_valid = true;
		}
		sub_img_ptr++;
		cc_img_ptr++;
	}

	/* 유효한 련결성분에 대하여 크기검사를 진행한다. */
	for (i = 0; i < cc_count; i++)
	{
		if (!cc_ptr[i].is_valid)
		{
			continue;
		}
		if (po::_min(cc_ptr[i].width, cc_ptr[i].height) < min_size)
		{
			cc_ptr[i].is_valid = false;
		}
		if (cc_ptr[i].area < min_pixels)
		{
			cc_ptr[i].is_valid = false;
		}
	}
}

bool CAutoCheckRegion::buildAutoCheckRegion(ConnComp* conn_comp_ptr, i32 conn_comp_count, i32 w, i32 h, shapeuvector& shape_vec)
{
	if (!conn_comp_ptr || conn_comp_count == 0)
	{
		return false;
	}

	i32 i, j, point_count;
	ConnComp* tmp_cc_ptr;
	std::vector<cv::Point> contour;
	std::vector<cv::Point> convex_hull;
	std::vector<std::vector<cv::Point>> convex_hull_vec;

	/* OpenCV를 리용하여 불룩포들을 생성한다. */
	for (i = 0; i < conn_comp_count; i++)
	{
		tmp_cc_ptr = conn_comp_ptr + i;
		if (!tmp_cc_ptr->is_valid || tmp_cc_ptr->contour_count <= 0)
		{
			continue;
		}
	
		ptfvector& mv_contour = tmp_cc_ptr->pixel_vec;
		point_count = (i32)mv_contour.size();
		contour.resize(point_count);

		for (j = 0; j < point_count; j++)
		{
			contour[j] = cv::Point(mv_contour[j].x, mv_contour[j].y);
		}
		cv::convexHull(contour, convex_hull);
		convex_hull_vec.push_back(convex_hull);
	}
	testConvexHullVector(convex_hull_vec, w, h);

	/* 생성된 불룩포들을 위상관계에 따라 통합한다. */
	i32 region_count = (i32)convex_hull_vec.size();
	std::vector<ACRegion> region_vec;
	region_vec.resize(region_count);

	for (i = 0; i < region_count; i++)
	{
		cv::Rect rt = cv::boundingRect(convex_hull_vec[i]);
		region_vec[i].rt = Recti(rt.x, rt.y, rt.x + rt.width, rt.y + rt.height);
		region_vec[i].area = cv::contourArea(convex_hull_vec[i]);
		region_vec[i].hull_index = i;
		region_vec[i].is_used = false;
	}

	sortAutoCheckRegion(region_vec);

	Recti rt1, rt2, intersect_rect;
	ptuvector auto_chk_region;

	for (i = 0; i < region_count; i++)
	{
		if (region_vec[i].is_used)
		{
			continue;
		}

		rt1 = region_vec[i].rt;
		convex_hull = convex_hull_vec[region_vec[i].hull_index];

		for (j = i + 1; j < region_count; j++)
		{
			rt2 = region_vec[j].rt;
			intersect_rect = rt1.intersectRect(rt2);
			if (intersect_rect.getArea() > 0.4f*rt2.getArea())
			{
				region_vec[j].is_used = true;
				contour = convex_hull_vec[region_vec[j].hull_index];
				convex_hull.insert(convex_hull.end(), contour.begin(), contour.end());
			}
		}

		cv::convexHull(convex_hull, contour);
		cv::approxPolyDP(contour, convex_hull, 3.0f, true);
		point_count = (i32)convex_hull.size();
		auto_chk_region.resize(point_count);
		for (j = 0; j < point_count; j++)
		{
			auto_chk_region[j].x = convex_hull[j].x;
			auto_chk_region[j].y = convex_hull[j].y;
		}

		shape_vec.push_back(auto_chk_region);
	}

	testAutoCheckRegionVector(shape_vec, w, h);
	return shape_vec.size() > 0;
}

void CAutoCheckRegion::sortAutoCheckRegion(std::vector<ACRegion>& region_vec)
{
	i32 i, j, region_count = (i32)region_vec.size();
	ACRegion tmp_region;

	for (i = 0; i < region_count; i++)
	{
		for (j = i + 1; j < region_count; j++)
		{
			if (region_vec[i].area < region_vec[j].area)
			{
				tmp_region = region_vec[i];
				region_vec[i] = region_vec[j];
				region_vec[j] = tmp_region;
			}
		}
	}
}

void CAutoCheckRegion::testConvexHullVector(std::vector<std::vector<cv::Point>>& convex_hull_vec, i32 w, i32 h)
{
#ifdef POR_TESTMODE
	cv::RNG rng(12345);
	cv::Mat drawing = cv::Mat::zeros(cv::Size(w, h), CV_8UC3);

	for (i32 i = 0; i < convex_hull_vec.size(); i++)
	{
		cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		cv::drawContours(drawing, convex_hull_vec, i, color);
	}

	cv::imwrite(PO_DEBUG_PATH"convex_hull.bmp", drawing);
#endif
}

void CAutoCheckRegion::testAutoCheckRegionVector(shapeuvector& shape_vec, i32 w, i32 h)
{
#ifdef POR_TESTMODE
	cv::RNG rng(12345);
	cv::Mat drawing = cv::Mat::zeros(cv::Size(w, h), CV_8UC3);
	std::vector<std::vector<cv::Point>> contours;
	contours.push_back(std::vector<cv::Point>());

	for (i32 i = 0; i < shape_vec.size(); i++)
	{
		contours[0].resize(shape_vec[i].size());
		for (i32 j = 0; j < shape_vec[i].size(); j++)
		{
			contours[0][j].x = shape_vec[i][j].x;
			contours[0][j].y = shape_vec[i][j].y;
		}

		cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		cv::drawContours(drawing, contours, 0, color);
	}

	cv::imwrite(PO_DEBUG_PATH"auto_check_region.bmp", drawing);
#endif
}
