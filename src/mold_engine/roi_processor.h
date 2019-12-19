#pragma once
#include "mv_data.h"
#include "ssim_processor.h"

class CROIProcessor
{
public:
	CROIProcessor();
	~CROIProcessor();

	void					initInstance(i32 index, EngineParam* engine_param_ptr);
	void					exitInstance();

	void					initLKPairs(i32 num);
	void					freeLKPairs();
	void					collectCornerPoints(CMoldROIData* roi_model_ptr);
	f32						calcConvexHull(cvPtVector& pts, ptvector2df& border_pts, i32 w, i32 h);
	f32						cvConvexHullArea(cvPtVector& pts);
	i32						updateRegionMap(u8* mask_img_ptr, Recti& range0, Recti& range1, CTransRect* ptr, i32 val, i32 mask);
	bool					cvGetRansacTransform(cvPtVector& pts1, cvPtVector& pts2, f32* tr, f32 eps, f32 outlier, i32 area);
	bool					cvSolveDetailTransform(CMoldPiece* mold_piece_ptr, Img cur_img, 
											cvPtVector& cv_pts_vec1, cvPtVector& cv_pts_vec2, f32* tr);

	bool					getHTransformLMSE(cvPtVector& pts1, cvPtVector& pts2, f32* tr, f32 eps, f32 rate, i32 fixed_count = 0);
	bool					getATransformLMSE(cvPtVector& pts1, cvPtVector& pts2, f32* tr);
	bool					getRandomSample(cvPtVector& pts, i32vector& ind, i32 num, i32 thlen2);
	bool					calcHTransformLMSE(cvPtVector& cv_pts_vec1, cvPtVector& cv_pts_vec2, f32* tr_ptr);

	void					setBasedROIData(CMoldROIData* roi_data_ptr, Img* img_ptr = NULL, DefectParam* pParam = NULL);

	i32						makeMaskImageInROI(CMoldSnapData* mold_snap_ptr);
	i32						makeMaskImageInROI(CMoldROIData* roi_model_ptr, u8* mask_img_ptr, CMoldROIData* roi_data_ptr);
	i32						makeCheckDataInROI(i32 mode, i32 snum = 0);
	void					updateAutoCheckMaskImage(CMoldSnapData* mold_snap_ptr, CAutoCheckData* autochk_data_ptr);

	i32						checkROIRegion(CMoldROIData* roi_model_ptr, CMoldROIResult* roi_result_ptr);
	void					updateSSIMMap(CMoldROIData* roi_data_ptr);
	bool					estimateGlobalMotion(CMoldPiece* mold_piece_ptr, Img& curImg, f32* tr);

	i32						findDefectRegions(Img sub_img, u8* erode_img_ptr, u16* pLabel, DefectVector& defects);
	i32						getExactlyDefects(DefectVector& defects, u16* plabel, CMoldROIResult* roi_result_ptr);
	bool					checkValidEstimation(f32* tr, i32 w, i32 h);
	i32						getValidLKPairs(cvPtVector& pts1, cvPtVector& pts2, u8vector& status, f32vector& err);

	void					testPtsWithImg(const char* filename, cvPtVector& pts_vec, u8* img_ptr, i32 w, i32 h);
	void					testPtsWithImg(const char* filename, ptfvector& pts_vec, u8* img_ptr, i32 w, i32 h);
	void					testROIResult(CMoldROIResult* roi_result_ptr);
	void					testCornerPts(ptvector2df& pts_vec, i32 w, i32 h);
	void					testCornerPts(cvPtVector& cv_pts_vec, i32 w, i32 h);
	void					testPointMatch(cvPtVector& cv_pts_vec1, cvPtVector& cv_pts_vec2, i32 w, i32 h);
	void					testPointLKMatch(cvPtVector& cv_pts_vec1, cvPtVector& cv_pts_vec2, f32* tr, f32 ratio, i32 w, i32 h);
	void					testImgLKMatch(u8* img_ptr1, u8* img_ptr2, i32 w, i32 h, f32* tr);
	void					testDefectRegions(Img img, u16* pLabel, DefectVector& defects);
	
	inline i32				getProcID() { return m_index; };

	static bool				subtractByHistogram(u8* min_img_ptr, u8* sub_img_ptr, u8* hist_diff_ptr, i32 w, i32 h,
										i32 min_stride, i32 sub_stride, i32 hist_diff_stride, i32* th_sub_ptr, f32 dmn);
	static i32				getDiffPixels(u8* img_ptr, i32 w, i32 h, i32 stride, f32 dth);

private:
	bool					vxWarpAffineImage(f32* trans_ptr);

public:
	//cache data for processing
	Img						m_image;
	CMoldROIData*			m_roi_data_ptr;
	DefectParam*			m_roi_param_ptr;

	//engine 
	EngineParam*			m_engine_param_ptr;

	//dynamic collected data
	i32						m_lk_pair_count;
	cvPtVector				m_lk_lpoint_vec;
	cvPtVector				m_lk_rpoint_vec;

	i32						m_index;
	CSSIMProcessor			m_ssim_processor;
};
