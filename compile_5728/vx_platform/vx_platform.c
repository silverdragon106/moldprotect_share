/*
 * Copyright (c) 2013-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* this define must precede inclusion of any xdc header file */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ti/cmem.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
/* Ipc Socket Protocol Family */
//#include <net/rpmsg.h>
#include <sys/eventfd.h>
#include <sched.h>
#include <assert.h>
#include <pthread.h>
/* package header files */
#include <ti/ipc/Std.h>
#include <ti/ipc/MessageQ.h>
#include "app_common.h"

#define IPU_PRIMARY_CORE_IPU1

#include "vx_platform.h"
#ifndef ALLOC_CMEM_BASED
#include "tialloc.h"
#endif

#define PROC_DSP1_INCLUDE
#define PROC_DSP2_INCLUDE

#undef  PROC_IPU1_0_INCLUDE
#undef  PROC_IPU1_1_INCLUDE
#undef  PROC_IPU2_INCLUDE

#define CMEM_SHMEM_BLOCK_SIZE (32*1024*1024)

extern MultiProc_Config _MultiProc_cfg;

/*** Protoype declaration ***/
int OSA_thrCreate(OSA_ThrHndl *hndl, OSA_ThrEntryFunc entryFunc, UInt32 pri, UInt32 stackSize, void *prm);
Void* System_ipcNotifyRecvFxn(Void * prm);
Void System_openVxIpcHandler(UInt32 payload, Ptr arg);
Int32 System_ipcNotifyInit(void);
int Vps_printf(const char *format, ...);

#define MAX_CMEM_REGIONS 2
System_openVxNotifyHandler govx_ipcobj_notifyHandler;
UInt32 govx_cmem_phys_base_region[MAX_CMEM_REGIONS];
UInt32 govx_cmem_size[MAX_CMEM_REGIONS];

System_openVxIpcObj govx_openVxIpcObj;

typedef struct {
  MessageQ_Handle     hndl; /* created locally */
  MessageQ_QueueId    id;   /* opened remotely */
  char                name[32];
  UInt16              heapId;      /* MessageQ heapId */
  UInt32              msgSize;
} tiovxMsgQueue_t;

typedef struct {
  OSA_ThrHndl    thrRxHndl;
  tiovxMsgQueue_t rxQue;
  tiovxMsgQueue_t txQue[SYSTEM_PROC_MAX];
} tiovxMessaging_t;

tiovxMessaging_t govx_hostMsg;

/* OCMC3 block is used as shared memory region */
UInt32 govx_shmem_virt_addr = 0;
UInt32 govx_shmem_phys_addr = 0;
UInt32 govx_shmem_size      = 0;
UInt32 govx_shmem_virt_addr_bios = 0;
UInt32 govx_shmem_phys_addr_bios = 0;
UInt32 govx_shmem_bios_size = 0;

UInt32 govx_ipcEnableProcId[SYSTEM_PROC_MAX + 2U] = {
#ifdef PROC_IPU1_0_INCLUDE
    SYSTEM_PROC_IPU1_0,
#endif
#ifdef PROC_IPU1_1_INCLUDE
    SYSTEM_PROC_IPU1_1,
#endif
#ifdef PROC_IPU2_INCLUDE
    SYSTEM_PROC_IPU2,
#endif
#ifdef PROC_DSP1_INCLUDE
    SYSTEM_PROC_DSP1,
#endif
#ifdef PROC_DSP2_INCLUDE
    SYSTEM_PROC_DSP2,
#endif
#ifdef PROC_EVE1_INCLUDE
    SYSTEM_PROC_EVE1,
#endif
#ifdef PROC_EVE2_INCLUDE
    SYSTEM_PROC_EVE2,
#endif
#ifdef PROC_EVE3_INCLUDE
    SYSTEM_PROC_EVE3,
#endif
#ifdef PROC_EVE4_INCLUDE
    SYSTEM_PROC_EVE4,
#endif
#ifdef PROC_A15_0_INCLUDE
    SYSTEM_PROC_A15_0,
#endif
    SYSTEM_PROC_MAX,
#ifndef PROC_IPU1_0_INCLUDE
    SYSTEM_PROC_INVALID,
#endif
#ifndef PROC_IPU1_1_INCLUDE
    SYSTEM_PROC_INVALID,
#endif
#ifndef PROC_IPU2_INCLUDE
    SYSTEM_PROC_INVALID,
#endif
#ifndef PROC_DSP1_INCLUDE
    SYSTEM_PROC_INVALID,
#endif
#ifndef PROC_DSP2_INCLUDE
    SYSTEM_PROC_INVALID,
#endif
#ifndef PROC_EVE1_INCLUDE
    SYSTEM_PROC_INVALID,
#endif
#ifndef PROC_EVE2_INCLUDE
    SYSTEM_PROC_INVALID,
#endif
#ifndef PROC_EVE3_INCLUDE
    SYSTEM_PROC_INVALID,
#endif
#ifndef PROC_EVE4_INCLUDE
    SYSTEM_PROC_INVALID,
#endif
#ifndef PROC_A15_0_INCLUDE
    SYSTEM_PROC_INVALID,
#endif

    SYSTEM_PROC_INVALID
};

/***********************************************************************/
Int32 System_openvxIsProcEnabled(UInt32 cpu_enabled)
{
Int32 retval = 0;

    switch(cpu_enabled)
    {
      case SYSTEM_PROC_DSP1:
      case SYSTEM_PROC_DSP2:
      case SYSTEM_PROC_A15_0:
        retval = 1;
        break;

      case SYSTEM_PROC_EVE1:
      case SYSTEM_PROC_EVE2:
      case SYSTEM_PROC_EVE3:
      case SYSTEM_PROC_EVE4:
      case SYSTEM_PROC_IPU1_0:
      case SYSTEM_PROC_IPU1_1:
      case SYSTEM_PROC_IPU2:
      default:
        retval = 0;
        break;
    }
    return retval;
}
/***********************************************************************/
UInt32 System_ovxGetObjDescShm(void)
{ /* TODO: Initialization kitchen sink */
  CMEM_AllocParams prms;
  CMEM_BlockAttrs block_attrs;
  int num_cmem_blocks, i;

  int ret_val = CMEM_init();
  if (ret_val == -1)
  {
     Vps_printf(" tivxMemInit: CMEM_init Failed !!!\n");
     return 0;
  }
  else
  {
     int version = CMEM_getVersion();
     if (-1 == version)
     {
        fprintf(stderr, "Failed to retrieve CMEM version\n");
        exit(EXIT_FAILURE);
     }
  }
  if(CMEM_getNumBlocks(&num_cmem_blocks) < 0) return 0; //NO CMEM blocks!
  if(num_cmem_blocks > MAX_CMEM_REGIONS) num_cmem_blocks = MAX_CMEM_REGIONS; //Take first two only!

  for (i = 0; i < num_cmem_blocks; i ++)
  {
    if(CMEM_getBlockAttrs(i, &block_attrs) < 0) break;
    govx_cmem_phys_base_region[i] = block_attrs.phys_base;
    govx_cmem_size[i]             = block_attrs.size;
  }

  prms.type  = CMEM_HEAP;
  prms.flags = CMEM_NONCACHED;
  prms.alignment = 4096;
  /* Use OCMC3 from CMEM */
  govx_shmem_size = 512 * 1024;
  govx_shmem_virt_addr = (UInt32)CMEM_alloc2 (1, govx_shmem_size, &prms);
  govx_shmem_phys_addr = (UInt32)CMEM_getPhys((void *)govx_shmem_virt_addr);
  memset((char *)govx_shmem_virt_addr, 0, govx_shmem_size);  

#ifdef ALLOC_CMEM_BASED
  prms.type  = CMEM_HEAP;
  prms.flags = CMEM_CACHED;
  prms.alignment = 4096;
  govx_shmem_bios_size      =  CMEM_SHMEM_BLOCK_SIZE;
  govx_shmem_virt_addr_bios = (UInt32)CMEM_alloc2 (0, CMEM_SHMEM_BLOCK_SIZE, &prms);
  govx_shmem_phys_addr_bios = (UInt32)CMEM_getPhys((void *)govx_shmem_virt_addr_bios);
#else
  prms.type  = CMEM_POOL;
  prms.flags = CMEM_CACHED;
  prms.alignment = 4096;
  govx_shmem_bios_size      = CMEM_SHMEM_BLOCK_SIZE;
  govx_shmem_virt_addr_bios = (UInt32)CMEM_allocPool2(0, 0, &prms);
  govx_shmem_phys_addr_bios = (UInt32)CMEM_getPhys((void *)govx_shmem_virt_addr_bios);
  HeapMem_init(0, (phys_addr_t)(govx_shmem_virt_addr_bios + CMEM_SHMEM_BLOCK_SIZE), (govx_cmem_size[0] - CMEM_SHMEM_BLOCK_SIZE)); 
#endif

  Vps_printf ("SYSTEM NOTIFY_INIT: starting");
  Vps_printf (">>>> ipcNotifyInit returned: %d\n", System_ipcNotifyInit());
  Vps_printf ("SYSTEM NOTIFY_INIT: done");

  return (UInt32)govx_shmem_virt_addr;
}

/***********************************************************************/
UInt16 MultiProc_getId(char *name)
{
    Int    i;
    UInt16 id;

    assert(name != NULL);

    id = MultiProc_INVALIDID;
    for (i = 0; i < _MultiProc_cfg.numProcessors; i++) {
        if ((_MultiProc_cfg.nameList[i] != NULL) &&
                (strcmp(name, _MultiProc_cfg.nameList[i]) == 0)) {
            id = i;
        }
    }
    return (id);
}
/***********************************************************************/
char *System_getProcName(UInt32 procId)
{
    if(procId==SYSTEM_PROC_DSP1)
        return SYSTEM_IPC_PROC_NAME_DSP1;

    if(procId==SYSTEM_PROC_DSP2)
        return SYSTEM_IPC_PROC_NAME_DSP2;

    if(procId==SYSTEM_PROC_EVE1)
        return SYSTEM_IPC_PROC_NAME_EVE1;

    if(procId==SYSTEM_PROC_EVE2)
        return SYSTEM_IPC_PROC_NAME_EVE2;

    if(procId==SYSTEM_PROC_EVE3)
        return SYSTEM_IPC_PROC_NAME_EVE3;

    if(procId==SYSTEM_PROC_EVE4)
        return SYSTEM_IPC_PROC_NAME_EVE4;

    if(procId==SYSTEM_PROC_IPU1_0)
        return SYSTEM_IPC_PROC_NAME_IPU1_0;

    if(procId==SYSTEM_PROC_IPU1_1)
        return SYSTEM_IPC_PROC_NAME_IPU1_1;

    if(procId==SYSTEM_PROC_IPU2)
        return SYSTEM_IPC_PROC_NAME_IPU2;

    if(procId==SYSTEM_PROC_A15_0)
        return SYSTEM_IPC_PROC_NAME_A15_0;

    return SYSTEM_IPC_PROC_NAME_INVALID;
}
/***********************************************************************/
UInt32 System_getSelfProcId(void)
{
  return SYSTEM_PROC_A15_0;
}
/***********************************************************************/
UInt32 System_getSyslinkProcId(UInt32 procId)
{
    char *procName = System_getProcName(procId);

    if(strcmp(procName, SYSTEM_IPC_PROC_NAME_INVALID)!=0)
    {
        return MultiProc_getId(procName);
    }

    return MultiProc_INVALIDID;
}
/***********************************************************************/
Int32 System_ipcNotifyInit(void)
{
    Int32 status = SYSTEM_LINK_STATUS_SOK;
    Int32 i;
    UInt32 procId;
    MessageQ_Params     msgqParams;
    Vps_printf(" SYSTEM: IPC: Notify init in progress !!!\n");

    memset(&govx_hostMsg, 0, sizeof(govx_hostMsg));
    i = 0;
    while (govx_ipcEnableProcId[i] != SYSTEM_PROC_MAX)
    {
        procId = govx_ipcEnableProcId[i];
        Vps_printf ("[%d] DUMP ALL PROCS[%d]=%s", i, procId, System_getProcName(procId));
        i++;
    }
    /* Create single incoming RX thread and queue */
    sprintf (govx_hostMsg.rxQue.name, "%s", TIOVX_RxHost);
    procId = System_getSelfProcId();
    MessageQ_Params_init(&msgqParams);
    Vps_printf("Next rx queue to open:%s\n", govx_hostMsg.rxQue.name);
    govx_hostMsg.rxQue.hndl = MessageQ_create(govx_hostMsg.rxQue.name, &msgqParams);
    Vps_printf("Just created MsgQue");
    if (govx_hostMsg.rxQue.hndl == NULL) {
       Vps_printf("Failed creating inbound MessageQ");
       return -1;
    }
    /* open the remote message queues (inbound messages), RX */
    status = OSA_thrCreate(
                &govx_hostMsg.thrRxHndl,
                System_ipcNotifyRecvFxn,
                SYSTEM_RPMSG_NOTIFY_TSK_PRI,
                OSA_THR_STACK_SIZE_DEFAULT,
                &procId);
    OSA_assertSuccess(status);
    Vps_printf ("Created RX task");
    /* Open Tx queues for sending messages  */
    i = 0;
    while (govx_ipcEnableProcId[i] != SYSTEM_PROC_MAX)
    {
        procId = govx_ipcEnableProcId[i];
        if ((procId != System_getSelfProcId()) &&
            (procId != SYSTEM_PROC_INVALID)    &&
            (procId != SYSTEM_PROC_EVE1)       &&
            (procId != SYSTEM_PROC_EVE2)       &&
            (procId != SYSTEM_PROC_EVE3)       &&
            (procId != SYSTEM_PROC_EVE4))
        {
           /* open the remote message queues (outbound messages), TX */
           /* Firmware is up and running already, so no delay expected here */
           sprintf (govx_hostMsg.txQue[procId].name, "QUE_RX_%s",  System_getProcName(procId));

           govx_hostMsg.txQue[procId].msgSize = sizeof(tiovxMsg_t);
           govx_hostMsg.txQue[procId].heapId  = App_MsgHeapId;
           Vps_printf("Next tx queue to open:%s, procId=%d", govx_hostMsg.txQue[procId].name, procId);
           do {
             status = MessageQ_open(govx_hostMsg.txQue[procId].name, &govx_hostMsg.txQue[procId].id);
             usleep(10000);
           } while (status == MessageQ_E_NOTFOUND);
           if (status < 0) {
             Vps_printf("Failed opening outbound MessageQ, to procId=%d, procName=%s\n", procId, System_getProcName(procId));
             return -1;
           }
        }
        i++;
    }

    for (i = 0; i < SYSTEM_PROC_MAX; i++) {        
        if(govx_ipcEnableProcId[i] == SYSTEM_PROC_MAX) break;
        procId = govx_ipcEnableProcId[i];
        Vps_printf("Dump all TX queues: procId=%d name=%s queId=%d, msgSize=%d, heapId=%d ", 
          procId, govx_hostMsg.txQue[procId].name, govx_hostMsg.txQue[procId].id, govx_hostMsg.txQue[procId].msgSize, govx_hostMsg.txQue[procId].heapId);
    }

    procId = 3;
    tiovxMsg_t *msg = (tiovxMsg_t *)MessageQ_alloc(govx_hostMsg.txQue[procId].heapId, govx_hostMsg.txQue[procId].msgSize);
    if (msg == NULL) {
        return -1;
    }
    /* set the return address in the message header */
    MessageQ_setReplyQueue(govx_hostMsg.rxQue.hndl, (MessageQ_Msg)msg);
    /* fill in message payload */
    msg->cmd        = App_CMD_CFG;
    msg->payload[0] = govx_shmem_phys_addr;
    msg->payload[1] = govx_shmem_size;
    msg->payload[2] = govx_shmem_phys_addr_bios;
    msg->payload[3] = govx_shmem_bios_size;
    Vps_printf ("SYSTEM: IPC: SentCfgMsg, procId=%d queuId=%d", procId, govx_hostMsg.txQue[procId].id);
    /* send message */
    MessageQ_put(govx_hostMsg.txQue[procId].id, (MessageQ_Msg)msg);
    usleep(300000); //Make sure DSP1 gets message first - to create SharedRegion 2
    procId = 4;
    msg = (tiovxMsg_t *)MessageQ_alloc(govx_hostMsg.txQue[procId].heapId, govx_hostMsg.txQue[procId].msgSize);
    if (msg == NULL) {
        return -1;
    }
    /* set the return address in the message header */
    MessageQ_setReplyQueue(govx_hostMsg.rxQue.hndl, (MessageQ_Msg)msg);
    /* fill in message payload */
    msg->cmd        = App_CMD_CFG;
    msg->payload[0] = govx_shmem_phys_addr;
    msg->payload[1] = govx_shmem_size;
    msg->payload[2] = govx_shmem_phys_addr_bios;
    msg->payload[3] = govx_shmem_bios_size;
    Vps_printf ("SYSTEM: IPC: SentCfgMsg, procId=%d queuId=%d", procId, govx_hostMsg.txQue[procId].id);
    /* send message */
    MessageQ_put(govx_hostMsg.txQue[procId].id, (MessageQ_Msg)msg);
    Vps_printf(" SYSTEM: IPC: Notify init DONE !!!\n");

    return status;
}
/***********************************************************************/
Int32 System_ipcSendNotify(UInt32 linkId)
{
    UInt32 procId = SYSTEM_GET_PROC_ID(linkId);
    tiovxMsg_t *msg;

    if(procId == SYSTEM_PROC_EVE1 ||
       procId == SYSTEM_PROC_EVE2 ||
       procId == SYSTEM_PROC_EVE3 ||
       procId == SYSTEM_PROC_EVE4)
    { /* A15 cannot communicate directly with EVEs, so it has to use IPU as proxy */
        SYSTEM_LINK_ID_SET_ROUTE_BIT(linkId);
        procId = SYSTEM_IPU_PROC_PRIMARY;
    }

    /* allocate message */
    msg = (tiovxMsg_t *)MessageQ_alloc(govx_hostMsg.txQue[procId].heapId, govx_hostMsg.txQue[procId].msgSize);

    if (msg == NULL) {
        return -1;
    }

    /* set the return address in the message header */
    MessageQ_setReplyQueue(govx_hostMsg.rxQue.hndl, (MessageQ_Msg)msg);

    /* fill in message payload */
    msg->cmd     = App_CMD_TEST;
    msg->payload[0] = linkId & 0x00ffffff; //Only bits 0:23 matter

    /* send message */
    MessageQ_put(govx_hostMsg.txQue[procId].id, (MessageQ_Msg)msg);


    return SYSTEM_LINK_STATUS_SOK;
}
/***********************************************************************/

Int32 System_openVxSendNotify(UInt32 cpuId, UInt32 payload)
{
    UInt32 linkId = 0U;

    linkId = SYSTEM_LINK_ID_MAKE_NOTIFY_TYPE( cpuId, payload, SYSTEM_LINK_ID_NOTIFY_TYPE_OPENVX);
    return System_ipcSendNotify(linkId);
}
/***********************************************************************/
void System_registerOpenVxNotifyCb(System_openVxNotifyHandler notifyCb)
{
    govx_openVxIpcObj.notifyHandler = notifyCb;
}
/***********************************************************************/
int System_ovxCacheInv(UInt32 virtAddr, UInt32 length)
{
    return CMEM_cacheInv ((void *)virtAddr, (size_t)length);
}
/***********************************************************************/
int System_ovxCacheWb(UInt32 virtAddr, UInt32 length)
{
    return CMEM_cacheWb ((void *)virtAddr, (size_t)length);
}
/***********************************************************************/
UInt32 System_ovxVirt2Phys (UInt32 virt_addr)
{
   return (UInt32)CMEM_getPhys((void *)virt_addr);
}
/***********************************************************************/
UInt32 System_ovxPhys2Virt (UInt32 phys_addr)
{
  /* This works for shared segment region only! */
  if((phys_addr >= govx_shmem_phys_addr) && (phys_addr < govx_shmem_phys_addr + govx_shmem_size))
  {
    return (govx_shmem_virt_addr + (phys_addr - govx_shmem_phys_addr));
  }
  return 0;
}
/***********************************************************************/
UInt32 System_ovxIsValidCMemVirtAddr(UInt32 virt_addr)
{
   return 1; //Only CMEM is used!
}
/***********************************************************************/
UInt32 System_ovxIsValidCMemPhysAddr(UInt32 phys_addr)
{
int i;

   for (i = 0; i < MAX_CMEM_REGIONS; i++)
   {
      if((phys_addr >= govx_cmem_phys_base_region[i]) && (phys_addr < (govx_cmem_phys_base_region[i] +  govx_cmem_size[i]))) return 1;
   }
   return 0;

}
/**
 *******************************************************************************
 *
 * \brief Put variable arguments into shared memory
 *        Provides a C style printf which puts variable arguments as a string
 *        into a shared memory
 *
 * \param  format   [IN] variable argument list
 *
 * \return  returns 0 on success
 *
 *******************************************************************************
 */
char govx_print_debug[1024];
int Vps_printf(const char *format, ...)
{
    va_list vaArgPtr;
    char *buf = NULL;
    buf = govx_print_debug;
    va_start(vaArgPtr, format);
    vsnprintf (buf, 1024, format, vaArgPtr);
    va_end(vaArgPtr);
    printf ("VSPRINTF_DBG:%s\n", buf); fflush(stdout);
    return (0);
}

/***********************************************************************/
/**
 *******************************************************************************
 *
 * \brief System wide notify handler function.
 *
 *        This function blocks on recvfrom() UNIX API to get messages from the
 *        kernel based on the message received it calls appropriate registered
 *        call back
 *
 * \return  NULL.
 *
 *******************************************************************************
 */
Void* System_ipcNotifyRecvFxn(Void * prm)
{
    Int32 status;
    //Int32 procId = *((Int32 *)prm);
    Int32 done = 0;
    tiovxMsg_t *msg;

    while(!done)
    {
       status = MessageQ_get(govx_hostMsg.rxQue.hndl, (MessageQ_Msg *)&msg, MessageQ_FOREVER);
       if (govx_openVxIpcObj.notifyHandler)
       {
          UInt32 payload = SYSTEM_GET_LINK_ID(msg->payload[0]);
          govx_openVxIpcObj.notifyHandler(payload);
       }
    }   /* while(! done) */
    return(NULL);
}
/***********************************************************************/
int OSA_thrCreate(OSA_ThrHndl *hndl, OSA_ThrEntryFunc entryFunc, UInt32 pri, UInt32 stackSize, void *prm)
{
  int status=OSA_SOK;
  pthread_attr_t thread_attr;
  struct sched_param schedprm;

  // initialize thread attributes structure
  status = pthread_attr_init(&thread_attr);

  if(status!=OSA_SOK) {
    OSA_ERROR("OSA_thrCreate() - Could not initialize thread attributes\n");
    return status;
  }

  if(stackSize!=OSA_THR_STACK_SIZE_DEFAULT)
    pthread_attr_setstacksize(&thread_attr, stackSize);

  status |= pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
  status |= pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);

  if(pri>OSA_THR_PRI_MAX)
    pri=OSA_THR_PRI_MAX;
  else
  if(pri<OSA_THR_PRI_MIN)
    pri=OSA_THR_PRI_MIN;

  schedprm.sched_priority = pri;
  status |= pthread_attr_setschedparam(&thread_attr, &schedprm);

  if(status!=OSA_SOK) {
    OSA_ERROR("OSA_thrCreate() - Could not initialize thread attributes\n");
    goto error_exit;
  }

  status = pthread_create(&hndl->hndl, &thread_attr, entryFunc, prm);

  if(status != OSA_SOK) {
    OSA_ERROR("OSA_thrCreate() - Could not create thread [%d]\n", status);
    OSA_assert(status == OSA_SOK);
  }

error_exit:
  pthread_attr_destroy(&thread_attr);

  return status;
}

/***********************************************************************/
#define MEM_BUFFER_ALLOC_ALIGN (64U)
void *System_ovxAllocMem(UInt32 size)
{
    void *ptr = NULL;
    CMEM_AllocParams prms;

#ifdef ALLOC_CMEM_BASED
    prms.type = CMEM_HEAP;
    prms.flags = CMEM_CACHED;
    prms.alignment = MEM_BUFFER_ALLOC_ALIGN;

    ptr = CMEM_alloc2(0, size, &prms);
#else
    ptr = HeapMem_alloc (0, size, 4096);
#endif
    if(!ptr) {
        fprintf(stderr, "Failed to allocate buffer from CMEM\n");
        exit(EXIT_FAILURE);
    }
    return (ptr);
}
/***********************************************************************/

void *System_ovxFreeMem(void *ptr, UInt32 size)
{
    CMEM_AllocParams prms;

    if ((NULL != ptr) && (0 != size))
    {
#ifdef ALLOC_CMEM_BASED
        prms.type = CMEM_HEAP;
        prms.flags = CMEM_CACHED;
        prms.alignment = MEM_BUFFER_ALLOC_ALIGN;
        CMEM_free(ptr, &prms);
#else
        HeapMem_free (0, (phys_addr_t)ptr, (size_t)size);
#endif
    }
}
/***********************************************************************/
