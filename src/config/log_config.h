#pragma once
#include "config.h"
#include "database/mysql_manager.h"

#if (defined(POR_WITH_MYSQL) || defined(POR_WITH_SQLITE))
#define actionlog(a)			g_mv_logger.addActionLog(a);
#define alarmlog1(a)			g_mv_logger.addAlarmLog((POECode)a, "");
#define alarmlog2(a, b)			g_mv_logger.addAlarmLog((POECode)a, b);

#if defined(POR_IMVS2_ON_AM5728)
  #define infolog(a)
#else
  #define infolog(a)			g_mv_logger.addInfoLog(a);
#endif

#else
#define actionlog(a)
#define alarmlog1(a)
#define alarmlog2(a, b)
#endif

extern CMySQLLogger g_mv_logger;
