/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TD_VNODE_DEF_H_
#define _TD_VNODE_DEF_H_

#include "executor.h"
#include "filter.h"
#include "qworker.h"
#include "sync.h"
#include "tchecksum.h"
#include "tcoding.h"
#include "tcompression.h"
#include "tdatablock.h"
#include "tfs.h"
#include "tglobal.h"
#include "tlist.h"
#include "tlockfree.h"
#include "tlosertree.h"
#include "tmacro.h"
#include "tmallocator.h"
#include "tskiplist.h"
#include "tstream.h"
#include "ttime.h"
#include "ttimer.h"
#include "vnode.h"
#include "wal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct STQ STQ;

typedef struct SVState      SVState;
typedef struct SVBufPool    SVBufPool;
typedef struct SQWorkerMgmt SQHandle;

typedef struct SVnodeTask {
  TD_DLIST_NODE(SVnodeTask);
  void* arg;
  int (*execute)(void*);
} SVnodeTask;

typedef struct SVnodeMgr {
  td_mode_flag_t vnodeInitFlag;
  // For commit
  bool          stop;
  uint16_t      nthreads;
  TdThread*     threads;
  TdThreadMutex mutex;
  TdThreadCond  hasTask;
  TD_DLIST(SVnodeTask) queue;
} SVnodeMgr;

typedef struct {
  int8_t  streamType;  // sma or other
  int8_t  dstType;
  int16_t padding;
  int32_t smaId;
  int64_t tbUid;
  int64_t lastReceivedVer;
  int64_t lastCommittedVer;
} SStreamSinkInfo;

typedef struct {
  SVnode*   pVnode;
  SHashObj* pHash;  // streamId -> SStreamSinkInfo
} SSink;

extern SVnodeMgr vnodeMgr;

// SVState
struct SVState {
  int64_t processed;
  int64_t committed;
  int64_t applied;
};

struct SVnode {
  int32_t    vgId;
  char*      path;
  SVnodeCfg  config;
  SVState    state;
  SVBufPool* pBufPool;
  SMeta*     pMeta;
  STsdb*     pTsdb;
  SWal*      pWal;
  STQ*       pTq;
  SSink*     pSink;
  tsem_t     canCommit;
  SQHandle*  pQuery;
  SMsgCb     msgCb;
  STfs*      pTfs;
};

int  vnodeScheduleTask(SVnodeTask* task);
int  vnodeQueryOpen(SVnode* pVnode);
void vnodeQueryClose(SVnode* pVnode);

#define vFatal(...)                                              \
  do {                                                           \
    if (vDebugFlag & DEBUG_FATAL) {                              \
      taosPrintLog("VND FATAL ", DEBUG_FATAL, 255, __VA_ARGS__); \
    }                                                            \
  } while (0)
#define vError(...)                                              \
  do {                                                           \
    if (vDebugFlag & DEBUG_ERROR) {                              \
      taosPrintLog("VND ERROR ", DEBUG_ERROR, 255, __VA_ARGS__); \
    }                                                            \
  } while (0)
#define vWarn(...)                                             \
  do {                                                         \
    if (vDebugFlag & DEBUG_WARN) {                             \
      taosPrintLog("VND WARN ", DEBUG_WARN, 255, __VA_ARGS__); \
    }                                                          \
  } while (0)
#define vInfo(...)                                        \
  do {                                                    \
    if (vDebugFlag & DEBUG_INFO) {                        \
      taosPrintLog("VND ", DEBUG_INFO, 255, __VA_ARGS__); \
    }                                                     \
  } while (0)
#define vDebug(...)                                                  \
  do {                                                               \
    if (vDebugFlag & DEBUG_DEBUG) {                                  \
      taosPrintLog("VND ", DEBUG_DEBUG, tsdbDebugFlag, __VA_ARGS__); \
    }                                                                \
  } while (0)
#define vTrace(...)                                                  \
  do {                                                               \
    if (vDebugFlag & DEBUG_TRACE) {                                  \
      taosPrintLog("VND ", DEBUG_TRACE, tsdbDebugFlag, __VA_ARGS__); \
    }                                                                \
  } while (0)

// vnodeCfg.h
extern const SVnodeCfg defaultVnodeOptions;

int  vnodeValidateOptions(const SVnodeCfg*);
void vnodeOptionsCopy(SVnodeCfg* pDest, const SVnodeCfg* pSrc);

// For commit
#define vnodeShouldCommit vnodeBufPoolIsFull
int vnodeSyncCommit(SVnode* pVnode);
int vnodeAsyncCommit(SVnode* pVnode);

// SVBufPool

int   vnodeOpenBufPool(SVnode* pVnode);
void  vnodeCloseBufPool(SVnode* pVnode);
int   vnodeBufPoolSwitch(SVnode* pVnode);
int   vnodeBufPoolRecycle(SVnode* pVnode);
void* vnodeMalloc(SVnode* pVnode, uint64_t size);
bool  vnodeBufPoolIsFull(SVnode* pVnode);

SMemAllocatorFactory* vBufPoolGetMAF(SVnode* pVnode);

// SVMemAllocator
typedef struct SVArenaNode {
  TD_SLIST_NODE(SVArenaNode);
  uint64_t size;  // current node size
  void*    ptr;
  char     data[];
} SVArenaNode;

typedef struct SVMemAllocator {
  T_REF_DECLARE()
  TD_DLIST_NODE(SVMemAllocator);
  uint64_t     capacity;
  uint64_t     ssize;
  uint64_t     lsize;
  SVArenaNode* pNode;
  TD_SLIST(SVArenaNode) nlist;
} SVMemAllocator;

SVMemAllocator* vmaCreate(uint64_t capacity, uint64_t ssize, uint64_t lsize);
void            vmaDestroy(SVMemAllocator* pVMA);
void            vmaReset(SVMemAllocator* pVMA);
void*           vmaMalloc(SVMemAllocator* pVMA, uint64_t size);
void            vmaFree(SVMemAllocator* pVMA, void* ptr);
bool            vmaIsFull(SVMemAllocator* pVMA);

// init once
int  tqInit();
void tqCleanUp();

// open in each vnode
STQ* tqOpen(const char* path, SVnode* pVnode, SWal* pWal, SMeta* pMeta, STqCfg* tqConfig,
            SMemAllocatorFactory* allocFac);
void tqClose(STQ*);

// required by vnode
int tqPushMsg(STQ*, void* msg, int32_t msgLen, tmsg_t msgType, int64_t version);
int tqCommit(STQ*);

int32_t tqProcessPollReq(STQ* pTq, SRpcMsg* pMsg, int32_t workerId);
int32_t tqProcessSetConnReq(STQ* pTq, char* msg);
int32_t tqProcessRebReq(STQ* pTq, char* msg);
int32_t tqProcessCancelConnReq(STQ* pTq, char* msg);
int32_t tqProcessTaskExec(STQ* pTq, char* msg, int32_t msgLen, int32_t workerId);
int32_t tqProcessTaskDeploy(STQ* pTq, char* msg, int32_t msgLen);
int32_t tqProcessStreamTrigger(STQ* pTq, void* data, int32_t dataLen, int32_t workerId);

// sma
void smaHandleRes(void* pVnode, int64_t smaId, const SArray* data);

#include "meta.h"

#include "tsdb.h"

#include "tq.h"

#include "tsdbSma.h"

#ifdef __cplusplus
}
#endif

#endif /*_TD_VNODE_DEF_H_*/