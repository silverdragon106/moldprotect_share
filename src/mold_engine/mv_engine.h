#pragma once

#include "mv_data.h"
#include "mv_struct_int.h"
#include "ssim_processor.h"
#include "checked_step.h"

class CImageProc;
class CROIProcessor;
class CAutoCheckRegion;
class CMVTemplate;
class CTemplateManager;

class CMVEngine
{
public:
	CMVEngine();
	~CMVEngine();

	void					initInstance(EngineParam* engine_param_ptr, CTemplateManager* pTemplateMgr);
	void					exitInstance();

	void					setImage(ImageGroup* img_group_ptr);
	u8*						getImage(i32 index, i32& w, i32& h);
	Img						getImg(i32 index);
	ImageGroup*				getImageGroup();
	CCheckedStep&			getCheckedStepResult();
	CROIProcessor*			getROIProcessor(i32 index);
	i32						getCheckedResult();

	i32						checkMoldStepFromImg(i32 stp, u8* autochk_mode_ptr, ImageGroup* img_group_ptr);
	i32						checkMoldSnapFromImg(i32 stp, i32 cam, bool use_autochk_region, ImageGroup* img_group_ptr);
	bool					preProcessCheck(i32 stp, ImageGroup* img_group_ptr);
	void					postProcessCheck();
	i32						checkMoldSnap(i32 stp, i32 cam, bool use_autochk_region, ImageData* img_group_ptr);

	i32						studyModelAllROI(CMVTemplate* model_ptr);
	i32						addROIIgnore(CMoldPiece* mold_piece_ptr, AlarmPVector& alarms);
	void					preUpdateAutoChkRegion(CMVTemplate* model_ptr, i32 cam_index);
	bool					makeAutoChkRegion(CMVTemplate* model_ptr, i32 cam_index);

	f32						getMeasuredFocus(u8* img_ptr, i32 w, i32 h);
	f32						getMeasuredFocus(u8* img_ptr, i32 w, i32 h, Recti rt);

	i32						studyROIRegions(CMVTemplate* model_ptr, i32 stp, i32 cam);
	bool					preUpdateROI(CMoldPiece* mold_piece_ptr, ImgData* img_data_ptr, ROIMaskPVector& roi_mask_vec);
	i32						checkMoldPiece(i32 stp, i32 cam, ImageData* img_data_ptr, CMoldSnapData* mdl_snap_ptr,
									CMoldPiece* mdl_piece_ptr, bool use_autochk_mask, CMoldResult* mold_result_ptr);

private:
	void					testMoldPiece(CMoldPiece* mold_piece_ptr);

public:
	bool					m_is_inited;
    ImageGroup				m_img_group;
	EngineParam*			m_engine_param_ptr;
	CCheckedStep			m_checked_step;
	
	CImageProc*				m_img_proc_ptr;
	CAutoCheckRegion*		m_auto_region_ptr;
	CTemplateManager*		m_template_manager_ptr;
	CROIProcessor*			m_roi_proc_ptr[PO_CAM_COUNT];
};
