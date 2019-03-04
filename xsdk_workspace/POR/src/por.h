//////////////////////////////////////////////////////////////////////////////////
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: Address Translator
// File Name: por.h
//
//
// Description:
//   - define parameters, data structure and functions for por
//////////////////////////////////////////////////////////////////////////////////


#include "ftl_config.h"
#include "nvme/nvme.h"



// Mapping Table definition

#define DIE_STATE_MAPPING_TABLE_NOT_EXIST		0
#define DIE_STATE_MAPPING_TABLE_EXIST			1

#define MAPPING_TABLE_MAKER_IDLE				0
#define MAPPING_TABLE_MAKER_TRIGGER			1
#define DIE_STATE_MAPPING_TABLE_HOLD			2
#define DIE_STATE_MAPPING_TABLE_UPDATE		3

#define START_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE	2
#define END_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE		16

// check: Logical slice map size: 4MB per die (total 4MB*32 = 128MB)
// SLICES_PER_SSD: 32M --> 1M LSN 단위로 32개로 나누어서 die 마다 저장하기
#define DATA_SIZE_OF_MAPPING_TABLE_PER_DIE		(sizeof(LOGICAL_SLICE_MAP)/USER_DIES) //
#define USED_PAGES_FOR_MAPPING_TABLE_PER_DIE	(DATA_SIZE_OF_MAPPING_TABLE_PER_DIE / BYTES_PER_DATA_REGION_OF_PAGE + 1) // 4MB/16K+1 = 257 (257 pages per die)
#define USED_BLOCKS_FOR_MAPPING_TABLE_PER_DIE	(USED_PAGES_FOR_MAPPING_TABLE_PER_DIE / USER_PAGES_PER_BLOCK  + 1) // 257/128 + 1 = 3 (3 blocks per die)
#define START_PAGE_NO_OF_MAPPING_TABLE_BLOCK	(1)		//각 block의 0번 페이지는 bad block mark page.
#define RESERVED_BLOCK_FOR_MAPPING_TABLE_PER_DIE (USED_BLOCKS_FOR_MAPPING_TABLE_PER_DIE) * 5 // 5 배 여유 공간 .

/////////////////////////////////////////////

// System Metadata definition

#define BLOCK_NO_OF_SYSTEM_META	1

/*
 * TODO: 추가적으로 저장해야하는 system metadata
 *
 * - VIRTUAL_BLOCK_MAP_PER_DIE		[size: 11.5 Bytes * 8K ==> 92KB ]: free block list, currentPage, eraseCnt 등을 가지고 있음.
 * - VIRTUAL_DIE_MAP_PER_DIE		[size: 12 Bytes]
 * - MAPPING_TABLE_INFO_MAP_PER_DIE	[size: 8 Bytes]
 *
 * > virtualDieMapPtr[dieNo]
 * > virtualBlockMapPtr[dieNo][block loop..]
 * > mtInfoMapPtr[dieNo]
 *
 * */

#define DATA_SIZE_OF_SYSTEM_META_PER_DIE	(sizeof(VIRTUAL_DIE_ENTRY) + sizeof(VIRTUAL_BLOCK_ENTRY) + sizeof(SYSTEM_META_INFO_ENTRY)))
#define USED_PAGES_FOR_SYSTEM_META_PER_DIE	(DATA_SIZE_OF_SYSTEM_META_PER_DIE / BYTES_PER_DATA_REGION_OF_PAGE + 1)
#define USED_BLOCKS_FOR_SYSTEM_META_PER_DIE	(USED_PAGES_FOR_SYSTEM_META_PER_DIE / USER_PAGES_PER_BLOCK  + 1) // 257/128 + 1 = 3 (3 blocks per die)
#define START_PAGE_NO_OF_SYSTEM_META_BLOCK	(1)		//각 block의 0번 페이지는 bad block mark page.

/////////////////////////////////////////////


typedef struct _MAPPING_TABLE_INFO_ENTRY{
	unsigned int curBlock : 16;
	unsigned int curPage : 16;
	unsigned int startBlock : 16;
	unsigned int startPage : 16;
} MAPPING_TABLE_INFO_ENTRY, *P_MAPPING_TABLE_ENTRY;

typedef struct _MAPPING_TABLE_INFO_MAP{
	MAPPING_TABLE_INFO_ENTRY mtInfo[USER_DIES];
} MAPPING_TABLE_INFO_MAP, *P_MAPPING_TABLE_INFO_MAP;

typedef struct _SYSTEM_META_INFO_ENTRY{
	unsigned int phyBlock : 16;
} SYSTEM_META_INFO_ENTRY, *P_SYSTEM_META_ENTRY;

typedef struct _SYSTEM_META_INFO_MAP{
	SYSTEM_META_INFO_ENTRY sysInfo[USER_DIES];
} SYSTEM_META_INFO_MAP, *P_SYSTEM_META_INFO_MAP;

/*
 * TODO: flush all requests in data buffer
 * */

extern P_MAPPING_TABLE_INFO_MAP mtInfoMapPtr;
extern P_SYSTEM_META_INFO_MAP sysInfoMapPtr;

void SaveMappingTable(unsigned char dieState[], unsigned int tempMtBufAddr[], unsigned int tempMtBufEntrySize);
