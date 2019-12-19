#pragma once

#include "define.h"

enum MVPacketCmdTypes
{
	//model management packets types
	kMVCmdModelOpen = kPOCmdExtend,
	kMVCmdCurModelOpen,
	kMVCmdCurModelEdit,
	kMVCmdCurModelReset,
	kMVCmdModelImport,
	kMVCmdModelExport,
	kMVCmdModelClose,
	kMVCmdModelGet,
	kMVCmdModelStudy,

	//modelcontents edit packets types
	kMVCmdModelSnapMode = kPOCmdExtend + 20,
	kMVCmdModelSnapAdd,
	kMVCmdModelSnapDelete,
	kMVCmdModelSnapGet,
	kMVCmdModelROIUpdate,
	kMVCmdModelAutoSnapEnd,
	kMVCmdModelTuneParam,

	//system status management packets types
	kMVCmdContinue = kPOCmdExtend + 40,
	kMVCmdStop,
	kMVCmdCheckRetry,
	kMVCmdCheckAddAlarm,
	kMVCmdCheckAddTemplate,
	kMVCmdCheckNG2OK,
	kMVCmdCheckNGSkip,
	kMVCmdCheckFail2Skip,

	kMVCmdCameraSetting = kPOCmdExtend + 60,	// 카메라설정명령
	kMVCmdSetting,
	kMVCmdResetRuntime,
	kMVCmdResetHistory,
	kMVCmdResetLog,
	kMVCmdIORawCmd,
	kMVCmdCaptureStream,
	kMVCmdDBOperation,

	//real-time packet types from server to client
	kMVCmdCamImage = kPOCmdExtend + 80,
	kMVCmdCheckResult,
	kMVCmdTestResult,
	kMVCmdAppState,
	kMVCmdDevState,
};

enum MVPacketSubTypes
{
	kMVSubTypeDeleteHistory = kPOSubTypeExtend,
	kMVSubTypeResetHistory,
	kMVSubTypeResetActionLog,
	kMVSubTypeResetAlarmLog,

	kMVSubTypeIORawSync = kPOSubTypeExtend + 20,
	kMVSubTypeIORawUnsync,

	kMVSubTypeEnableCapStream = kPOSubTypeExtend + 30,
	kMVSubTypeDisableCapStream,

	kMVSubTypeSyncSetting = kPOSubTypeExtend + 50,
	kMVSubTypeSyncCamSetting,
	kMVSubTypeSyncRuntimeHistory,
	kMVSubTypeSyncTestHistory,
	kMVSubTypeSyncModelInfo,

	kMVSubTypeImageRaw = kPOSubTypeExtend + 70,
	kMVSubTypeImageStream,
	kMVSubTypeCheckedResult,
	kMVSubTypeModelData,
	kMVSubTypeModelSnapImage,
	kMVSubTypeAutoRegion,

	kMVSubTypeDevError = kPOSubTypeExtend + 80,
	kMVSubTypeDevPluged,
	kMVSubTypeDevUnpluged,
	kMVSubTypeDevIORaw,
	kMVSubTypeDevCamSetting,

	kMVSubTypeChkState = kPOSubTypeExtend + 100,
	kMVSubTypeNeedTimer,
	kMVSubTypeNGCancel,
	kMVSubTypeStudyStart,
	kMVSubTypeStudyFinish,
	kMVSubTypeTemplateAdd,

	kMVSubTypeGetCurHistory = kPOSubTypeExtend + 120,
	kMVSubTypeGetHistory,
	kMVSubTypeGetTableRows,
	kMVSubTypeGetOperLog,
	kMVSubTypeGetAlarmLog,
	kMVSubTypeGetTroubleshoot,
	kMVSubTypeExecQuery
};

enum MVPacketSubFlagType
{
	kMVSubFlagSettingGeneral = 0x0100,
	kMVSubFlagSettingMeasure = 0x0200,
	kMVSubFlagSettingPermission = 0x0400,
	kMVSubFlagSettingIOWork = 0x0800,
	kMVSubFlagSettingAdvanced = 0x1000
};

//////////////////////////////////////////////////////////////////////////
enum MVState
{
	kMVStateNone = 0,
	kMVStateFree,
	kMVStateMonitor,
	kMVStateModel,
	kMVStateStop,
	kMVStateOffline
};

enum MVModelEdit
{
	kMVEditUnknown = 0,
	kMVEditModelImage,
	kMVEditROIParam,
	kMVEditROITest
};

enum ModelSnapModeTypes
{
	kModelSnapModeNone = 0,
	kModelSnapModeManual,
	kModelSnapModeAuto
};

enum MoldStepTypes
{
	kMVStepNone = -1,
	kMVStepProduct = 0,
	kMVStepEject,
	kMVStepThird,
	kMVStepCount
};

enum MoldResultTypes
{
	kMoldResultFail = 0,
	kMoldResultNG,
	kMoldResultPassed,
	kMoldResultNone,
};

enum MoldResultDesc
{
	kMoldRDescNone = 0x00,
	kMoldRDescPass = 0x01,
	kMoldRDescFail = 0x02,
	kMoldRDescLargeStDev = 0x04,
	kMoldRDescLotDiffPixels = 0x08,
	kMoldRDescUserCommit = 0x20,
	kMoldRDescEstMotion = 0x40,
	kMoldRDescNoneROIPass = (0x80 | kMoldRDescPass)
};
