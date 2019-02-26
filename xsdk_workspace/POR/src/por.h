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

#define DATA_SIZE_OF_MAPPING_TABLE_PER_DIE		(sizeof(VIRTUAL_SLICE_MAP)/USER_DIES) // FIXME
#define USED_PAGES_FOR_MAPPING_TABLE_PER_DIE	(DATA_SIZE_OF_MAPPING_TABLE_PER_DIE / BYTES_PER_DATA_REGION_OF_PAGE + 1) // FIXME
#define USED_BLOCKS_FOR_MAPPING_TABLE_PER_DIE	(DATA_SIZE_OF_MAPPING_TABLE_PER_DIE / TOTAL_BLOCKS_PER_DIE + 1)
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
