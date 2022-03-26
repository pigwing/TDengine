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

#include "tdbInt.h"

typedef struct __attribute__((__packed__)) {
  u8 cellNum[3];
  u8 cellBody[3];
  u8 cellFree[3];
  u8 nFree[3];
} SPageHdrL;

typedef struct __attribute__((__packed__)) {
  u8 szCell[3];
  u8 nxOffset[3];
} SFreeCellL;

// cellNum
static inline int  getPageCellNum(SPage *pPage) { return TDB_GET_U24(((SPageHdrL *)(pPage->pPageHdr))[0].cellNum); }
static inline void setPageCellNum(SPage *pPage, int cellNum) {
  TDB_PUT_U24(((SPageHdrL *)(pPage->pPageHdr))[0].cellNum, cellNum);
}

// cellBody
static inline int  getPageCellBody(SPage *pPage) { return TDB_GET_U24(((SPageHdrL *)(pPage->pPageHdr))[0].cellBody); }
static inline void setPageCellBody(SPage *pPage, int cellBody) {
  TDB_PUT_U24(((SPageHdrL *)(pPage->pPageHdr))[0].cellBody, cellBody);
}

// cellFree
static inline int  getPageCellFree(SPage *pPage) { return TDB_GET_U24(((SPageHdrL *)(pPage->pPageHdr))[0].cellFree); }
static inline void setPageCellFree(SPage *pPage, int cellFree) {
  TDB_PUT_U24(((SPageHdrL *)(pPage->pPageHdr))[0].cellFree, cellFree);
}

// nFree
static inline int  getPageNFree(SPage *pPage) { return TDB_GET_U24(((SPageHdrL *)(pPage->pPageHdr))[0].nFree); }
static inline void setPageNFree(SPage *pPage, int nFree) {
  TDB_PUT_U24(((SPageHdrL *)(pPage->pPageHdr))[0].nFree, nFree);
}

// cell offset
static inline int getPageCellOffset(SPage *pPage, int idx) {
  ASSERT(idx >= 0 && idx < getPageCellNum(pPage));
  return TDB_GET_U24(pPage->pCellIdx + 3 * idx);
}

static inline void setPageCellOffset(SPage *pPage, int idx, int offset) {
  TDB_PUT_U24(pPage->pCellIdx + 3 * idx, offset);
}

// free cell info
static inline void getPageFreeCellInfo(SCell *pCell, int *szCell, int *nxOffset) {
  SFreeCellL *pFreeCell = (SFreeCellL *)pCell;
  *szCell = TDB_GET_U24(pFreeCell->szCell);
  *nxOffset = TDB_GET_U24(pFreeCell->nxOffset);
}

static inline void setPageFreeCellInfo(SCell *pCell, int szCell, int nxOffset) {
  SFreeCellL *pFreeCell = (SFreeCellL *)pCell;
  TDB_PUT_U24(pFreeCell->szCell, szCell);
  TDB_PUT_U24(pFreeCell->nxOffset, nxOffset);
}

SPageMethods pageLargeMethods = {
    3,                    // szOffset
    sizeof(SPageHdrL),    // szPageHdr
    sizeof(SFreeCellL),   // szFreeCell
    getPageCellNum,       // getCellNum
    setPageCellNum,       // setCellNum
    getPageCellBody,      // getCellBody
    setPageCellBody,      // setCellBody
    getPageCellFree,      // getCellFree
    setPageCellFree,      // setCellFree
    getPageNFree,         // getFreeBytes
    setPageNFree,         // setFreeBytes
    getPageCellOffset,    // getCellOffset
    setPageCellOffset,    // setCellOffset
    getPageFreeCellInfo,  // getFreeCellInfo
    setPageFreeCellInfo   // setFreeCellInfo
};