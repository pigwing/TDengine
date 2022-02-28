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
#ifndef __INDEX_TFILE_H__
#define __INDEX_TFILE_H__

#include "indexInt.h"
#include "index_fst.h"
#include "index_fst_counting_writer.h"
#include "index_tfile.h"
#include "tlockfree.h"

#ifdef __cplusplus
extern "C" {
#endif

// tfile header content
// |<---suid--->|<---version--->|<-------colName------>|<---type-->|<--fstOffset->|
// |<-uint64_t->|<---int32_t--->|<--TSDB_COL_NAME_LEN-->|<-uint8_t->|<---int32_t-->|

#pragma pack(push, 1)
typedef struct TFileHeader {
  uint64_t suid;
  int32_t  version;
  char     colName[TSDB_COL_NAME_LEN];  //
  uint8_t  colType;
  int32_t  fstOffset;
} TFileHeader;
#pragma pack(pop)

#define TFILE_HEADER_SIZE (sizeof(TFileHeader))
#define TFILE_HEADER_NO_FST (TFILE_HEADER_SIZE - sizeof(int32_t))

typedef struct TFileValue {
  char*   colVal;  // null terminated
  SArray* tableId;
  int32_t offset;
} TFileValue;

// table cache
// refactor to LRU cache later
typedef struct TFileCache {
  SHashObj* tableCache;
  int16_t   capacity;
  // add more param
} TFileCache;

typedef struct TFileWriter {
  FstBuilder* fb;
  WriterCtx*  ctx;
  TFileHeader header;
  uint32_t    offset;
} TFileWriter;

// multi reader and single write
typedef struct TFileReader {
  T_REF_DECLARE()
  Fst*        fst;
  WriterCtx*  ctx;
  TFileHeader header;
  bool        remove;
} TFileReader;

typedef struct IndexTFile {
  char*        path;
  TFileCache*  cache;
  TFileWriter* tw;
} IndexTFile;

typedef struct TFileWriterOpt {
  uint64_t suid;
  int8_t   colType;
  char*    colName;
  int32_t  nColName;
  int32_t  version;
} TFileWriterOpt;

typedef struct TFileReaderOpt {
  uint64_t suid;
  char*    colName;
  int32_t  nColName;
} TFileReaderOpt;

// tfile cache, manage tindex reader
TFileCache*  tfileCacheCreate(const char* path);
void         tfileCacheDestroy(TFileCache* tcache);
TFileReader* tfileCacheGet(TFileCache* tcache, ICacheKey* key);
void         tfileCachePut(TFileCache* tcache, ICacheKey* key, TFileReader* reader);

TFileReader* tfileGetReaderByCol(IndexTFile* tf, uint64_t suid, char* colName);

TFileReader* tfileReaderOpen(char* path, uint64_t suid, int32_t version, const char* colName);
TFileReader* tfileReaderCreate(WriterCtx* ctx);
void         tfileReaderDestroy(TFileReader* reader);
int          tfileReaderSearch(TFileReader* reader, SIndexTermQuery* query, SArray* result);
void         tfileReaderRef(TFileReader* reader);
void         tfileReaderUnRef(TFileReader* reader);

TFileWriter* tfileWriterOpen(char* path, uint64_t suid, int32_t version, const char* colName, uint8_t type);
void         tfileWriterClose(TFileWriter* tw);
TFileWriter* tfileWriterCreate(WriterCtx* ctx, TFileHeader* header);
void         tfileWriterDestroy(TFileWriter* tw);
int          tfileWriterPut(TFileWriter* tw, void* data, bool order);
int          tfileWriterFinish(TFileWriter* tw);

//
IndexTFile* indexTFileCreate(const char* path);
void        indexTFileDestroy(IndexTFile* tfile);
int         indexTFilePut(void* tfile, SIndexTerm* term, uint64_t uid);
int         indexTFileSearch(void* tfile, SIndexTermQuery* query, SArray* result);

Iterate* tfileIteratorCreate(TFileReader* reader);
void     tfileIteratorDestroy(Iterate* iterator);

TFileValue* tfileValueCreate(char* val);

int  tfileValuePush(TFileValue* tf, uint64_t val);
void tfileValueDestroy(TFileValue* tf);

#ifdef __cplusplus
}

#endif

#endif
