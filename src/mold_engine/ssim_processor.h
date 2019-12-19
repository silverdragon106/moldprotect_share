#pragma once
#include "mv_data.h"
#include "proc/memory_pool.h"

class CSSIMProcessor : public CPOMemPool
{
public:
	CSSIMProcessor();
	~CSSIMProcessor();

	void				initInstance(EngineParam* engine_param_ptr);
	void				exitInstance();

	void				prepareStructSqSimilar(CMoldROIData* roi_data_ptr, bool init_ssim_map, bool use_mempool);
	void				makeStructSqSimilar(CMoldROIData* roi_model_ptr, CMoldROIData* roi_data_ptr);
	void				updateSimilar(CMoldROIData* roi_data_ptr, u8* sub_img_ptr, u8* mask_img_ptr);

public:
	EngineParam*		m_engine_param_ptr;
};