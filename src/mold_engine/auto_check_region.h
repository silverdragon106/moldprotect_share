#pragma once
#include "mv_data.h"

struct ConnComp;
class CMVTemplate;
class CROIProcessor;

struct ACRegion
{
	Recti	rt;
	f32		area;
	i32		hull_index;
	bool	is_used;
};

class CAutoCheckRegion
{
public:
	CAutoCheckRegion(EngineParam* engine_param_ptr);
	~CAutoCheckRegion();

public:
	void					preUpdateAutoChkRegion(CAutoCheckData* autochk_data_ptr, CROIProcessor* roi_processor_ptr);
	bool					makeAutoChkRegion(CMVTemplate* model_ptr, i32 cam_index);
	void					makeAutoChkRegion(u8* prev_img_ptr, u8* cur_img_ptr, i32 w, i32 h, shapeuvector& shape_vec);
	bool					buildAutoCheckRegion(ConnComp* cc_ptr, i32 cc_count, i32 w, i32 h, shapeuvector& shape_vec);

private:
	void					checkValidAutoChkConnComps(u8* sub_img_ptr, u16* cc_img_ptr, i32 w, i32 h, f32 dmn_high,
													i32 cc_count, ConnComp* cc_ptr);
	void					sortAutoCheckRegion(std::vector<ACRegion>& region_vec);
	void					testConvexHullVector(std::vector<std::vector<cv::Point>>& convex_hull_vec, i32 w, i32 h);
	void					testAutoCheckRegionVector(shapeuvector& shape_vec, i32 w, i32 h);

public:
	EngineParam*			m_engine_param_ptr;
};