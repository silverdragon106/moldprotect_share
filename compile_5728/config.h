#pragma once

#define POR_DEVICE
//#define POR_WINDOWS
#define POR_LINUX

//Targeted Device
#define POR_IMVS2_ON_AM5728

//////////////////////////////////////////////////////////////////////////
/* unused defines */
//#define POR_INITCOMPILE
//#define POR_TESTMODE
//#define POR_PRODUCT
//#define POR_EMULATOR
//#define POR_WITH_DONGLE

//////////////////////////////////////////////////////////////////////////
/* subcompile defines */
#define POR_WITH_CAMERA
#define POR_WITH_IOMODULE
#define POR_WITH_STREAM
//#define POR_WITH_GUI
#define POR_WITH_LOG
//#define POR_WITH_MYSQL
#define POR_WITH_SQLITE
//#define POR_WITH_OVX
//#define POR_WITH_DSP
#define POR_WITH_OCLCV
#define POR_WITH_AUTHENTICATION

//////////////////////////////////////////////////////////////////////////
#ifdef POR_WITH_CAMERA
	#define POR_SUPPORT_MINDVISION
	//#define POR_SUPPORT_BASLER
#endif

#ifdef POR_WITH_STREAM
    //#define POR_SUPPORT_GSTREAMER
    //#define POR_SUPPORT_FFMPEG
    #define POR_SUPPORT_RAWENCODER
#endif

//#define POR_SUPPORT_UNICODE
#define POR_SUPPORT_TIOPENCV

#define PO_MVIO_MAXINADDR				8	//InputLine (0~MaxAddress-1)
#define PO_MVIO_MAXOUTADDR				8	//OutputLine (0~MaxAddress-1)

#define PO_LOWLEVEL_NAME				"mv_device.exe"
#define PO_HIGHLEVEL_NAME				"mold_vision.exe"

#ifdef POR_WINDOWS
	#define PO_LOG_PATH					"c:\\log"
	#define PO_LOG_FILENAME				"c:\\log\\mv_log.txt"
	#define PO_TIMELOG_FILENAME			"c:\\log\\mv_timelog.txt"
	#define PO_DATABASE_PATH			"c:\\database"
#else
	#define PO_DEBUG_PATH				"/home/root/nfs/tmp/imvs2/"
	#define PO_LOG_PATH					"/opt/am572x/imvs2/log"
	#define PO_LOG_FILENAME				"/opt/am572x/imvs2/log/mv_log.txt"
	#define PO_TIMELOG_FILENAME			"/opt/am572x/imvs2/log/mv_timelog.txt"
	#define PO_DATABASE_PATH			"/opt/am572x/imvs2/database"
#endif

#define PO_PROJECT_ID1					402000001	//MVS1.6 Product Key
#define PO_PROJECT_ID2					1989108		//Tester Key
#define PO_PROJECT_GUID					"mv7aae71-a2db-46c9-af4e-42c4d1163d21"

#define PO_CUR_DEVICE					kPOAppMVS
#define PO_NETWORK_PORT					12345
#define PO_CAM_COUNT					2
#define PO_CAM_WIDTH					1440
#define PO_CAM_HEIGHT					1080
#define PO_MAX_CAM_COUNT				8
#define PO_VX_UNIT_COUNT				2
#define PO_INIT_PASSWORD				""
#define PO_UPDATE_DELAYTIME				5000
#define PO_LOG_FILESIZE					4096000	//about 4MB

#define PO_DEVICE_ID					1001
#define PO_DEVICE_NAME					"MVPorIPDev"
#define PO_DEVICE_MODELNAME				"MV2017FD"
#define PO_DEVICE_VERSION				"1.00"
#define PO_DEVICE_CAMPORT				"UNKPORT"

#define PO_DATABASE_FILENAME			"imvs2_sqlite.db"
#define PO_MYSQL_HOSTNAME				"localhost"
#define PO_MYSQL_DATABASE				"imvs2_db"
#define PO_MYSQL_USERNAME				"root"
#define PO_MYSQL_PASSWORD				"123456"
#define PO_MYSQL_REMOTEUSERNAME			"imvs2_user"
#define PO_MYSQL_REMOTEPASSWORD			"123456"
#define PO_MYSQL_PORT					3306
#define PO_MYSQL_RECLIMIT				1000
#define PO_MYSQL_LOGLIMIT				100000

#define PO_MODNET_TCP_PORT				502
#define PO_MODNET_UDP_PORT				504
#define PO_MODBUS_ADDR					0
#define PO_MODBUS_PORT					"COM1"
#define PO_MODBUS_RS232					1
#define PO_MODBUS_RS485					2
#define PO_MODBUS_RS422					3
#define PO_MODBUS_BAUDBAND				115200
#define PO_MODBUS_DATABITS				8
#define PO_MODBUS_PARITY				0
#define PO_MODBUS_STOPBITS				1
#define PO_MODBUS_FLOWCTRL				0

//////////////////////////////////////////////////////////////////////////
#ifdef POR_DEBUG
	#undef POR_WITH_DONGLE
#endif

#if defined(POR_INITCOMPILE)
	#undef POR_WITH_CAMERA
	#undef POR_WITH_IOMODULE
	#undef POR_WITH_SQLITE
#elif defined(POR_EMULATOR)
	#undef POR_WITH_CAMERA
	#undef POR_WITH_MYSQL
	#undef POR_WITH_SQLITE
	#undef POR_WITH_IOMODULE
	#undef POR_WITH_DONGLE
#endif

#ifdef POR_WINDOWS
#ifdef POR_DEBUG
#pragma comment(lib, "opencv_core341d.lib")
#pragma comment(lib, "opencv_imgproc341d.lib")
#pragma comment(lib, "opencv_highgui341d.lib")
#pragma comment(lib, "opencv_imgcodecs341d.lib")
#pragma comment(lib, "opencv_imgproc341d.lib")
#pragma comment(lib, "opencv_video341d.lib")
#pragma comment(lib, "opencv_calib3d341d.lib")
#else
#pragma comment(lib, "opencv_core341.lib")
#pragma comment(lib, "opencv_imgproc341.lib")
#pragma comment(lib, "opencv_highgui341.lib")
#pragma comment(lib, "opencv_imgcodecs341.lib")
#pragma comment(lib, "opencv_imgproc341.lib")
#pragma comment(lib, "opencv_video341.lib")
#pragma comment(lib, "opencv_calib3d341.lib")
#endif

  #pragma comment(lib, "zlibstatic.lib")

  #pragma warning(disable: 4018)
  #pragma warning(disable: 4305)
  #pragma warning(disable: 4819)
#endif

#define PO_IPC_SHMNAME				"MVShared-46f92966-4764-a721-da2211a16637"
#define PO_IPC_SHMSIZE				(30*1024*1024) //30MB
