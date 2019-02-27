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



#define DIE_STATE_MAPPING_TABLE_NOT_EXIST		0
#define DIE_STATE_MAPPING_TABLE_EXIST			1

#define MAPPING_TABLE_MAKER_IDLE				0
#define MAPPING_TABLE_MAKER_TRIGGER			1
#define DIE_STATE_MAPPING_TABLE_HOLD			2
#define DIE_STATE_MAPPING_TABLE_UPDATE		3

// check: Logical slice map size: 4MB per die (total 4MB*32 = 128MB)
// SLICES_PER_SSD: 32M --> 1M LSN ������ 32���� ����� die ���� �����ϱ�
#define DATA_SIZE_OF_MAPPING_TABLE_PER_DIE		(sizeof(LOGICAL_SLICE_MAP)/USER_DIES) //
#define USED_PAGES_FOR_MAPPING_TABLE_PER_DIE	(DATA_SIZE_OF_MAPPING_TABLE_PER_DIE / BYTES_PER_DATA_REGION_OF_PAGE + 1) // 4MB/16K+1 = 257 (257 pages per die)
#define USED_BLOCKS_FOR_MAPPING_TABLE_PER_DIE	(USED_PAGES_FOR_MAPPING_TABLE_PER_DIE / USER_PAGES_PER_BLOCK  + 1) // 257/128 + 1 = 3 (3 blocks per die)
#define START_PAGE_NO_OF_MAPPING_TABLE_BLOCK	(1)		//bad block table begins at second page for preserving a bad block mark of the block allocated to save bad block table

#define MAPPING_TABLE_SAVED_BLOCK_NUM sizeof(VIRTUAL_SLICE_MAP)

typedef struct _MAPPING_TABLE_INFO_ENTRY{
	unsigned int phyBlock : 16;
	unsigned int grownBadUpdate : 1;
	unsigned int reserved0 : 15;
} MAPPING_TABLE_INFO_ENTRY, *P_MAPPING_TABLE_ENTRY;

typedef struct _MAPPING_TABLE_INFO_MAP{
	MAPPING_TABLE_INFO_ENTRY mtInfo[USER_DIES];
} MAPPING_TABLE_INFO_MAP, *P_MAPPING_TABLE_INFO_MAP;

extern P_MAPPING_TABLE_INFO_MAP mtInfoMapPtr;

void SaveMappingTable(unsigned char dieState[], unsigned int tempMtBufAddr[], unsigned int tempMtBufEntrySize);
