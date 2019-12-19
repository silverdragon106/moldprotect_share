/*
 * Copyright (c) 2013-2014, Texas Instruments Incorporated
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

/*
 *  ======== AppCommon.h ========
 *
 */

#ifndef AppCommon__include
#define AppCommon__include
#if defined (__cplusplus)
extern "C" {
#endif


/*
 *  ======== Application Configuration ========
 */
#define NotifyMsg_Evt_DSP          7       /* dsp event number */
#define NotifyMsg_Evt_Appl         15      /* application event number */
#define Notify_Evt_PING         0xFFFE0000      /* FFFE eeee */
#define Notify_Evt_PONG         0xFFFFFFFF
#define NotifyMsg_S_EMPTY          0x00000000
#define NotifyMsg_S_SUCCESS        0x00000000
#define NotifyMsg_E_FAILURE        0x80000000
#define NotifyMsg_E_OVERFLOW       0x90000000
#define NotifyMsg_E_NOEVENT        0xA0000000

/* notify commands 00 - FF */
#define App_CMD_MASK            0xFF000000
#define App_CMD_NOP             0x00000000  /* cc------ */
#define App_CMD_SHUTDOWN        0x02000000  /* cc------ */
#define App_CMD_HOST            0x03000000
#define App_CMD_SLAVE           0x04000000
#define App_CMD_SLAVE2          0x05000000
#define App_CMD_TEST            0x06000000
#define App_CMD_CFG             0x07000000

#define MAX_NAMELEN             20
#define MAX_PAYLOAD             8

typedef struct {
    MessageQ_MsgHeader  reserved;
    UInt32              cmd;
    UInt32              payload[MAX_PAYLOAD];
} tiovxMsg_t;

#define App_MsgHeapId           0

#define TIOVX_RxHost "QUE_RX_HOST"
#define TIOVX_RxDsp1 "QUE_RX_DSP1"
#define TIOVX_RxDsp2 "QUE_RX_DSP2"
#define TIOVX_RxIpu1 "QUE_RX_IPU1"

#define App_SrHeapId      1
#define App_SrHeapName    "myHeapBuf"

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
#endif /* AppCommon__include */
