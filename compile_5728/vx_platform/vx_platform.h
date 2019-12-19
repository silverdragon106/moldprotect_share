#include <include/link_api/system_linkId.h>
#include <include/link_api/system_common.h>

#define OSA_SOK (0)
#define OSA_assert(x)  \
{ \
  if( (x) == 0) { \
    fprintf(stderr, " ASSERT (%s|%s|%d)\r\n", __FILE__, __func__, __LINE__); \
    abort();\
  } \
}

// printf wrapper that can be used to display errors. Prefixes all text with
// "ERROR" and inserts the file and line number of where in the code it was
// called
#define OSA_ERROR(...) \
  do \
  { \
  fprintf(stderr, " ERROR  (%s|%s|%d): ", __FILE__, __func__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); \
  } \
  while(0);

#define OSA_assertSuccess(ret)  OSA_assert(ret==OSA_SOK)

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })



//#define SYSTEM_LINK_ID_MAX                  (128U)
#define SYSTEM_LINK_ID_PROCK_LINK_ID        (SYSTEM_LINK_ID_MAX-1U)

#define SYSTEM_LINK_STATUS_SOK              0

/**
 * \brief Remote end point, This will be created on slave cores
 */
#define SYSTEM_RPMSG_ENDPT_REMOTE          (80U)

/**
 * \brief This will be created at host and used by rpmsg notify module
 *        to receive notifications from slave
 */
#define SYSTEM_RPMSG_NOTIFY_ENDPT_HOST     (81U)

/**
 * \brief This will be created at host and used by rpmsg msgQ module
 *        to receive data messages from slave
 */
#define SYSTEM_RPMSG_MSGQ_DATA_ENDPT_HOST  (82U)

/**
 * \brief This will be created at host and used by rpmsg msgQ module
 *        to receive ack messages from slave
 */
#define SYSTEM_RPMSG_MSGQ_ACK_ENDPT_HOST   (83U)

/**
 *******************************************************************************
 *
 * \brief Notify task priority
 *
 *
 *******************************************************************************
 */
#define SYSTEM_RPMSG_NOTIFY_TSK_PRI              (OSA_THR_PRI_MAX)


/**
 *******************************************************************************
 *
 * \brief Message Q task priority
 *
 *
 *******************************************************************************
 */
#define SYSTEM_RPMSG_MSGQ_TSK_PRI                (OSA_THR_PRI_DEFAULT)

/**
 *******************************************************************************
 *
 * \brief IPC Link task priority
 *
 *
 *******************************************************************************
 */
#define IPC_LINK_TSK_PRI                         (OSA_THR_PRI_DEFAULT)

/*!
 *  @brief  Max name length for a processor name.
 */
#define MultiProc_MAXNAMELENGTH 32

/*!
 *  @brief  Max number of processors supported.
 */
#define MultiProc_MAXPROCESSORS 10

/**
 *******************************************************************************
 *
 * \brief System task priority
 *
 *
 *******************************************************************************
 */
#define SYSTEM_TSK_PRI                  (OSA_THR_PRI_DEFAULT)

#define OSA_THR_PRI_MAX                 sched_get_priority_max(SCHED_FIFO)
#define OSA_THR_PRI_MIN                 sched_get_priority_min(SCHED_FIFO)
#define OSA_THR_PRI_DEFAULT             ( OSA_THR_PRI_MIN + (OSA_THR_PRI_MAX-OSA_THR_PRI_MIN)/2 )
#define OSA_THR_STACK_SIZE_DEFAULT      0

typedef unsigned int UInt32;
typedef unsigned short UInt16;
typedef int Int;
typedef char Char;
typedef int Int32;
typedef void* Ptr;

typedef struct MultiProc_Config_tag {
    Int32  numProcessors;
    /*!< Max number of procs for particular system */
    Char   nameList [MultiProc_MAXPROCESSORS][MultiProc_MAXNAMELENGTH];
    /*!< Name List for processors in the system */
    Int32  rprocList[MultiProc_MAXPROCESSORS];
    /*!< Linux "remoteproc index" for processors in the system */
    UInt16 id;
    /*!< Local Proc ID. This needs to be set before calling any other APIs */
} MultiProc_Config;

typedef struct {
  pthread_t      hndl;
} OSA_ThrHndl;
typedef Void(*System_ipcNotifyCb) (OSA_ThrHndl pTthread);
typedef struct {
    Int32 sockFdRx[SYSTEM_PROC_MAX];
    /**< socket fds for Rx channels */
    Int32 sockFdTx[SYSTEM_PROC_MAX];
    /**< socket fds for Tx channels */
    Int32 unblockFd;
    /**< descriptor unblocking select()  */
    OSA_ThrHndl thrHndl;
    /**< receive thread handle */
    System_ipcNotifyCb notifyCb[SYSTEM_LINK_ID_MAX];
} System_IpcNotifyObj;
typedef Void(*System_openVxNotifyHandler) (UInt32 payload);
typedef Void * (*OSA_ThrEntryFunc)(void *);

typedef struct
{
  System_openVxNotifyHandler notifyHandler;
} System_openVxIpcObj;

/***********************************************************************/
typedef enum
{
    SYSTEM_RPMSG_RX_CHANNEL = 0,
    /**< Used to create RP Message endpoint to receive messages from
         remote core */
    SYSTEM_RPMSG_TX_CHANNEL,
    /**< Used to create RP Message endpoint to transmit messages to
         remote core */
} SYSTEM_RPMSG_SOCKETTYPE;

/*------------------------------------------------------------------------------*/
