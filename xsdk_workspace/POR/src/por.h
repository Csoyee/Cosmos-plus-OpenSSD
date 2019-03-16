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

#define DIE_STATE_MAPPING_TABLE_HOLD			2
#define DIE_STATE_MAPPING_TABLE_UPDATE		3

#define POR_MAKER_IDLE				0
#define POR_MAKER_TRIGGER			1

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
 * - MAPPING_TABLE_INFO_MAP_PER_DIE	[size: 9 Bytes]
 *
 * > virtualDieMapPtr[dieNo]
 * > virtualBlockMapPtr[dieNo][block loop..]
 * > mtInfoMapPtr[dieNo]
 *
 * - Slice 16K (page1: mapping table info map, page2: die map, page 3-8: block map)
 * */

#define DATA_SIZE_OF_MT_INFO_PER_DIE	sizeof(MAPPING_TABLE_INFO_ENTRY)
#define USED_PAGES_FOR_MT_INFO_PER_DIE	(DATA_SIZE_OF_MT_INFO_PER_DIE / BYTES_PER_DATA_REGION_OF_PAGE + 1)
#define DATA_SIZE_OF_DIE_MAP_PER_DIE	sizeof(VIRTUAL_DIE_ENTRY)
#define USED_PAGES_FOR_DIE_MAP_PER_DIE	(DATA_SIZE_OF_DIE_MAP_PER_DIE / BYTES_PER_DATA_REGION_OF_PAGE + 1)
#define DATA_SIZE_OF_BLOCK_MAP_PER_DIE	sizeof(VIRTUAL_BLOCK_ENTRY) * USER_BLOCKS_PER_DIE
#define USED_PAGES_FOR_BLOCK_MAP_PER_DIE	(DATA_SIZE_OF_BLOCK_MAP_PER_DIE / BYTES_PER_DATA_REGION_OF_PAGE + 1)

#define START_PAGE_NO_OF_MT_INFO_BLOCK		(1)		//각 block의 0번 페이지는 bad block mark page.
#define START_PAGE_NO_OF_DIE_MAP_BLOCK		(START_PAGE_NO_OF_MT_INFO_BLOCK + 1)
#define START_PAGE_NO_OF_BLOCK_MAP_BLOCK	(START_PAGE_NO_OF_DIE_MAP_BLOCK + 1)		// 96K / 16K + 1 = 7 pages
#define END_PAGE_NO_OF_BLOCK_MAP_BLOCK		(START_PAGE_NO_OF_BLOCK_MAP_BLOCK + USED_PAGES_FOR_BLOCK_MAP_PER_DIE - 1)

/////////////////////////////////////////////


typedef struct _MAPPING_TABLE_INFO_ENTRY{
	unsigned int format : 1;
	unsigned int reserved : 7;
	unsigned int curBlock : 16;
	unsigned int curPage : 16;
	unsigned int startBlock : 16;
	unsigned int startPage : 16;
} MAPPING_TABLE_INFO_ENTRY, *P_MAPPING_TABLE_INFO_ENTRY;

typedef struct _MAPPING_TABLE_INFO_MAP{
	MAPPING_TABLE_INFO_ENTRY mtInfo[USER_DIES];
} MAPPING_TABLE_INFO_MAP, *P_MAPPING_TABLE_INFO_MAP;



extern P_MAPPING_TABLE_INFO_MAP mtInfoMapPtr;

void UpdateMappingTable(unsigned int tempBufAddr);
void RecoverMappingTable(unsigned int tempBufAddr);
void SaveMappingTable(unsigned int tempMtBufAddr[], unsigned int tempMtBufEntrySize);
void UpdateSystemMeta();
void RecoverDieMap();
void RecoverBlockMap();
