#include <assert.h>
#include "memory_map.h"
#include "xil_printf.h"

//////////////////////////////////////////////////////////////////////////////////
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: Address Translator
// File Name: por.c
//
// Description:
//   - manage l2v mapping table information in NAND device
//////////////////////////////////////////////////////////////////////////////////

void FlushDataBuffer()
{
	unsigned int reqSlotTag, bufEntry, virtualSliceAddr;
	int index;

	xil_printf("flush data buffer entry \r\n");
	for(index = 0 ; index < AVAILABLE_DATA_BUFFER_ENTRY_COUNT ; index ++)
	{
		bufEntry = dataBufHashTablePtr->dataBufHash[index].headEntry;

		while(bufEntry != DATA_BUF_NONE)
		{
			if(dataBufMapPtr->dataBuf[bufEntry].dirty == DATA_BUF_DIRTY)
			{
				reqSlotTag = GetFromFreeReqQ();
				virtualSliceAddr = AddrTransWrite (dataBufMapPtr->dataBuf[bufEntry].logicalSliceAddr);

				reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
				reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_WRITE;
				reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = dataBufMapPtr->dataBuf[bufEntry].logicalSliceAddr;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ENTRY;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_VSA;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_ON;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_MAIN;
				reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry = bufEntry;
				UpdateDataBufEntryInfoBlockingReq(bufEntry, reqSlotTag);
				reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr = virtualSliceAddr;

				SelectLowLevelReqQ(reqSlotTag);

				dataBufMapPtr->dataBuf[bufEntry].dirty = DATA_BUF_CLEAN;

			}

			bufEntry = dataBufMapPtr->dataBuf[bufEntry].hashNextEntry;
		}
	}

	SyncAllLowLevelReqDone();

	// read test
	//dummyReadCommand();
}

void ReadSystemMeta(unsigned int tempSysBufAddr[], unsigned int tempSysBufEntrySize, int dataSize, int page)
{
	unsigned int tempPage, reqSlotTag, dieNo, blockNo; //, rowAddr;
	int loop;

	loop = 0;
	tempPage = page;

	blockNo = 1;

	while(dataSize>0)
	{
		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
		{
			reqSlotTag = GetFromFreeReqQ();

			reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
			reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_READ;
			if(tempSysBufEntrySize > BYTES_PER_DATA_REGION_OF_PAGE)
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ADDR;
			else
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_FOR_POR;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_PHY_ORG;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_OFF;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_NONE;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_TOTAL;

			reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.addr = tempSysBufAddr[dieNo] + loop * tempSysBufEntrySize;

			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalCh = Vdie2PchTranslation(dieNo);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalWay = Vdie2PwayTranslation(dieNo);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalBlock = blockNo;
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalPage = Vpage2PlsbPageTranslation(tempPage);

			SelectLowLevelReqQ(reqSlotTag);
/*
 * 	for debugging
			if(dieNo == 0 )
			{
				rowAddr = GenerateNandRowAddr(reqSlotTag);
				xil_printf("rowAddr: %x\r\n", rowAddr);
			}
 */
		}

		tempPage++;
		loop++;
		dataSize -= BYTES_PER_DATA_REGION_OF_PAGE;
	}

	SyncAllLowLevelReqDone();

}


void SaveSystemMeta(unsigned int tempSysBufAddr[], unsigned int tempSysBufEntrySize, int dataSize, int page)
{
	unsigned int dieNo, reqSlotTag, blockNo; //, rowAddr;
	int loop, tempPage;

	loop = 0;
	tempPage = page;
	blockNo = 1;

	while(dataSize>0)
	{
		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
		{
			reqSlotTag = GetFromFreeReqQ();

			reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
			reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_WRITE;
			if(tempSysBufEntrySize > BYTES_PER_DATA_REGION_OF_PAGE)
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ADDR;
			else
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_FOR_POR;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_PHY_ORG;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_OFF;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_NONE;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_TOTAL;

			reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.addr = tempSysBufAddr[dieNo] + loop * tempSysBufEntrySize;

			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalCh = Vdie2PchTranslation(dieNo);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalWay = Vdie2PwayTranslation(dieNo);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalBlock = blockNo;
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalPage =  Vpage2PlsbPageTranslation(tempPage);

			SelectLowLevelReqQ(reqSlotTag);
/*
 * 	for debugging
			if(dieNo == 0 )
			{
				rowAddr = GenerateNandRowAddr(reqSlotTag);
				xil_printf("rowAddr: %x\r\n", rowAddr);
			}
*/
		}
		tempPage++;
		loop++;
		dataSize -= BYTES_PER_DATA_REGION_OF_PAGE;
	}

	SyncAllLowLevelReqDone();

}

void EraseSystemMeta()
{
	unsigned int reqSlotTag, dieNo;

	for(dieNo = 0; dieNo < USER_DIES; dieNo++)
	{
		reqSlotTag = GetFromFreeReqQ();

		reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
		reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_ERASE;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_PHY_ORG;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_NONE;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_NONE;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_TOTAL;

		reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalCh = Vdie2PchTranslation(dieNo);
		reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalWay = Vdie2PwayTranslation(dieNo);
		reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalBlock = 1;
		reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalPage = 0;	//dummy

		SelectLowLevelReqQ(reqSlotTag);
	}
}

void RecoverGCMap()
{
	unsigned int dieNo, tempSysBaseAddr, tempSysEntrySize, marker, invalidSliceCnt;
	unsigned int tempSysBufAddr[USER_DIES];
	GC_VICTIM_LIST_ENTRY* gcUpdater;

	if( mtInfoMapPtr->mtInfo[0].format == POR_MAKER_TRIGGER )
	{
		xil_printf("init GC map \r\n");
		InitGcVictimMap();
	} else
	{
		marker = POR_MAKER_IDLE;
		// Read mapping_table_info_entry
		tempSysBaseAddr = RESERVED_DATA_BUFFER_BASE_ADDR;
		tempSysEntrySize = BYTES_PER_DATA_REGION_OF_PAGE + BYTES_PER_SPARE_REGION_OF_PAGE;
		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
		{
			tempSysBufAddr[dieNo] = tempSysBaseAddr + dieNo * USED_PAGES_FOR_GC_MAP_PER_DIE * tempSysEntrySize;
		}

		ReadSystemMeta(tempSysBufAddr, tempSysEntrySize, DATA_SIZE_OF_GC_MAP_PER_DIE, START_PAGE_NO_OF_GC_MAP_BLOCK);

		for (dieNo = 0 ; dieNo < USER_DIES ; dieNo++)
		{
			for (invalidSliceCnt = 0 ; invalidSliceCnt <= SLICES_PER_BLOCK ; invalidSliceCnt ++)
			{
				gcUpdater = (GC_VICTIM_LIST_ENTRY*)tempSysBufAddr[dieNo] + invalidSliceCnt;
				gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt] = *gcUpdater;

				if((gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock != BLOCK_NONE)
						&& (gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock < 0
							|| gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock > TOTAL_BLOCKS_PER_DIE))
				{
					xil_printf("[GCMap]tail block invalid %d (%d)\r\n", gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock, dieNo );
					marker = POR_MAKER_TRIGGER;
					break;
				} else if ((gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock != BLOCK_NONE)
						&& (gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock < 0
							|| gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock > TOTAL_BLOCKS_PER_DIE))
				{
					xil_printf("[GCMap]tail block invalid %d (%d)\r\n", gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock, dieNo );
					marker = POR_MAKER_TRIGGER;
					break;
				}
			}
			if(invalidSliceCnt != SLICES_PER_BLOCK + 1)
				break;
		}

		if (marker == POR_MAKER_TRIGGER)
		{
			xil_printf("Recover GC Map Fail \r\n");
		}
	}
}

void UpdateGCMap()
{
	unsigned int dieNo, tempSysBaseAddr, tempSysEntrySize, invalidSliceCnt;
	unsigned int tempSysBufAddr[USER_DIES];
	GC_VICTIM_LIST_ENTRY* gcUpdater;

	tempSysBaseAddr = RESERVED_DATA_BUFFER_BASE_ADDR;
	tempSysEntrySize = BYTES_PER_DATA_REGION_OF_PAGE + BYTES_PER_SPARE_REGION_OF_PAGE;

	for(dieNo = 0 ; dieNo < USER_DIES ; dieNo ++)
		tempSysBufAddr[dieNo] = tempSysBaseAddr + dieNo * USED_PAGES_FOR_GC_MAP_PER_DIE * tempSysEntrySize;

	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo++)
	{
		for (invalidSliceCnt = 0 ; invalidSliceCnt < SLICES_PER_BLOCK + 1 ; invalidSliceCnt ++)
		{
			gcUpdater = (GC_VICTIM_LIST_ENTRY*) tempSysBufAddr[dieNo] + invalidSliceCnt;
			*gcUpdater = gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt];
		}
	}

	SaveSystemMeta(tempSysBufAddr, tempSysEntrySize, DATA_SIZE_OF_GC_MAP_PER_DIE, START_PAGE_NO_OF_GC_MAP_BLOCK);

}


void RecoverBlockMap()
{
	unsigned int dieNo, tempSysEntrySize, marker;
	unsigned int tempSysBufAddr[USER_DIES];

	if( mtInfoMapPtr->mtInfo[0].format == POR_MAKER_TRIGGER )
	{
		xil_printf("init block map \r\n");
		InitBlockMap();
	} else
	{
		marker = POR_MAKER_IDLE;
		// Read block map
		tempSysEntrySize = BYTES_PER_DATA_REGION_OF_PAGE ;

		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
		{
			tempSysBufAddr[dieNo] = VIRTUAL_BLOCK_MAP_ADDR + dieNo * DATA_SIZE_OF_BLOCK_MAP_PER_DIE;
		}

		ReadSystemMeta(tempSysBufAddr, tempSysEntrySize, DATA_SIZE_OF_BLOCK_MAP_PER_DIE, START_PAGE_NO_OF_BLOCK_MAP_BLOCK);

		for(dieNo = 0 ; dieNo < USER_DIES ; dieNo++)
		{
			if (virtualBlockMapPtr->block[dieNo][0].bad != BLOCK_STATE_NORMAL && virtualBlockMapPtr->block[dieNo][0].bad != BLOCK_STATE_BAD)
			{
				xil_printf("[BlockMap]bad flag invalid %d (%d)\r\n", virtualBlockMapPtr->block[dieNo][0].bad, dieNo );
				marker = POR_MAKER_TRIGGER;
				break;
			} else if (virtualBlockMapPtr->block[dieNo][0].currentPage < 0 || virtualBlockMapPtr->block[dieNo][0].currentPage > USER_PAGES_PER_BLOCK)
			{
				xil_printf("[BlockMap]currentPage invalid %d (%d)\r\n", virtualBlockMapPtr->block[dieNo][0].currentPage, dieNo );
				marker = POR_MAKER_TRIGGER;
				break;
			}
		}

		if (marker == POR_MAKER_TRIGGER)
		{
			xil_printf("Recover Block Map Fail \r\n");
		}
	}
}

void UpdateBlockMap()
{
	unsigned int dieNo, tempSysEntrySize;
	unsigned int tempSysBufAddr[USER_DIES];

	tempSysEntrySize = BYTES_PER_DATA_REGION_OF_PAGE;

	for(dieNo = 0 ; dieNo < USER_DIES ; dieNo ++)
		tempSysBufAddr[dieNo] = VIRTUAL_BLOCK_MAP_ADDR + dieNo * DATA_SIZE_OF_BLOCK_MAP_PER_DIE;


	SaveSystemMeta(tempSysBufAddr, tempSysEntrySize, DATA_SIZE_OF_BLOCK_MAP_PER_DIE, START_PAGE_NO_OF_BLOCK_MAP_BLOCK);
}

void RecoverDieMap()
{
	unsigned int dieNo, tempSysBaseAddr, tempSysEntrySize, marker;
	unsigned int tempSysBufAddr[USER_DIES];
	VIRTUAL_DIE_ENTRY* dieUpdater;

	if( mtInfoMapPtr->mtInfo[0].format == POR_MAKER_TRIGGER )
	{
		xil_printf("init die map \r\n");
		InitDieMap();
	} else
	{
		marker = POR_MAKER_IDLE;
		// Read mapping_table_info_entry
		tempSysBaseAddr = RESERVED_DATA_BUFFER_BASE_ADDR;
		tempSysEntrySize = BYTES_PER_DATA_REGION_OF_PAGE + BYTES_PER_SPARE_REGION_OF_PAGE;
		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
		{
			tempSysBufAddr[dieNo] = tempSysBaseAddr + dieNo * USED_PAGES_FOR_DIE_MAP_PER_DIE * tempSysEntrySize;
		}

		ReadSystemMeta(tempSysBufAddr, tempSysEntrySize, DATA_SIZE_OF_DIE_MAP_PER_DIE, START_PAGE_NO_OF_DIE_MAP_BLOCK);

		for (dieNo = 0 ; dieNo < USER_DIES ; dieNo++)
		{
			dieUpdater = (VIRTUAL_DIE_ENTRY*) tempSysBufAddr[dieNo];
			virtualDieMapPtr->die[dieNo] = *dieUpdater;

			if(virtualDieMapPtr->die[dieNo].currentBlock != BLOCK_NONE
				&& (virtualDieMapPtr->die[dieNo].currentBlock < 0
				|| virtualDieMapPtr->die[dieNo].currentBlock >= USER_BLOCKS_PER_DIE))
			{
				xil_printf("[DieMap]curBlock invalid %d (%d)\r\n", virtualDieMapPtr->die[dieNo].currentBlock, dieNo );
				marker = POR_MAKER_TRIGGER;
				break;
			}
			else if(virtualDieMapPtr->die[dieNo].headFreeBlock != BLOCK_NONE
					&& (virtualDieMapPtr->die[dieNo].headFreeBlock < 0
					|| virtualDieMapPtr->die[dieNo].headFreeBlock >= USER_BLOCKS_PER_DIE) )
			{
				xil_printf("[DieMap]startBlock invalid %d (%d)\r\n", virtualDieMapPtr->die[dieNo].headFreeBlock, dieNo);
				marker = POR_MAKER_TRIGGER;
				break;
			}
			else if(virtualDieMapPtr->die[dieNo].tailFreeBlock != BLOCK_NONE
					&& (virtualDieMapPtr->die[dieNo].tailFreeBlock < 0
					|| virtualDieMapPtr->die[dieNo].tailFreeBlock >= USER_BLOCKS_PER_DIE))
			{
				xil_printf("[DieMap]curPage invalid %d (%d)\r\n", virtualDieMapPtr->die[dieNo].tailFreeBlock, dieNo);
				marker = POR_MAKER_TRIGGER;
				break;
			}
			else if(virtualDieMapPtr->die[dieNo].freeBlockCnt > USER_BLOCKS_PER_DIE
					|| virtualDieMapPtr->die[dieNo].freeBlockCnt <= 0)
			{
				xil_printf("[DieMap]freeBlockCnt invalid %d (%d)\r\n",virtualDieMapPtr->die[dieNo].freeBlockCnt, dieNo);
				marker = POR_MAKER_TRIGGER;
				break;
			}
		}

		if (marker == POR_MAKER_TRIGGER)
		{
			xil_printf("Recover Die Map Fail \r\n");
		}
	}
}

void UpdateDieMap()
{
	unsigned int dieNo, tempSysBaseAddr, tempSysEntrySize;
	unsigned int tempSysBufAddr[USER_DIES];
	VIRTUAL_DIE_ENTRY* dieUpdater;

	tempSysBaseAddr = RESERVED_DATA_BUFFER_BASE_ADDR;
	tempSysEntrySize = BYTES_PER_DATA_REGION_OF_PAGE + BYTES_PER_SPARE_REGION_OF_PAGE;

	for(dieNo = 0 ; dieNo < USER_DIES ; dieNo ++)
		tempSysBufAddr[dieNo] = tempSysBaseAddr + dieNo * USED_PAGES_FOR_DIE_MAP_PER_DIE * tempSysEntrySize;

	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo++)
	{
		dieUpdater = (VIRTUAL_DIE_ENTRY*) tempSysBufAddr[dieNo];
		*dieUpdater = virtualDieMapPtr->die[dieNo];
	}

	SaveSystemMeta(tempSysBufAddr, tempSysEntrySize, DATA_SIZE_OF_DIE_MAP_PER_DIE, START_PAGE_NO_OF_DIE_MAP_BLOCK);

}

void RecoverMappingTableInfoMap()
{
	unsigned int dieNo, tempSysBaseAddr, tempSysEntrySize, mtMarker;
	unsigned int tempSysBufAddr[USER_DIES];
	MAPPING_TABLE_INFO_ENTRY* mtUpdater;

	// Read mapping_table_info_entry
	tempSysBaseAddr = RESERVED_DATA_BUFFER_BASE_ADDR;
	tempSysEntrySize = BYTES_PER_DATA_REGION_OF_PAGE + BYTES_PER_SPARE_REGION_OF_PAGE;
	for(dieNo = 0; dieNo < USER_DIES; dieNo++)
	{
		tempSysBufAddr[dieNo] = tempSysBaseAddr + dieNo * USED_PAGES_FOR_MT_INFO_PER_DIE * tempSysEntrySize;
	}

	ReadSystemMeta(tempSysBufAddr, tempSysEntrySize, DATA_SIZE_OF_MT_INFO_PER_DIE, START_PAGE_NO_OF_MT_INFO_BLOCK);

	mtMarker = POR_MAKER_IDLE;

	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo++)
	{
		mtUpdater = (MAPPING_TABLE_INFO_ENTRY*)(tempSysBufAddr[dieNo]);
		mtInfoMapPtr->mtInfo[dieNo] = *mtUpdater;

		if(mtInfoMapPtr->mtInfo[dieNo].curBlock < START_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE
				|| mtInfoMapPtr->mtInfo[dieNo].curBlock > END_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE)
		{
			xil_printf("[MtInfo]invalid Current Block number %d %d\r\n", mtInfoMapPtr->mtInfo[dieNo].curBlock, dieNo);
			mtMarker = POR_MAKER_TRIGGER;
			break;
		}
		else if(mtInfoMapPtr->mtInfo[dieNo].startBlock < START_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE
				|| mtInfoMapPtr->mtInfo[dieNo].startBlock > END_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE)
		{
			xil_printf("[MtInfo]invalid Start Block number %d %d\r\n", mtInfoMapPtr->mtInfo[dieNo].startBlock, dieNo);
			mtMarker = POR_MAKER_TRIGGER;
			break;
		}
		else if(mtInfoMapPtr->mtInfo[dieNo].curPage < START_PAGE_NO_OF_MAPPING_TABLE_BLOCK
				|| mtInfoMapPtr->mtInfo[dieNo].curPage > USER_PAGES_PER_BLOCK)
		{
			xil_printf("[MtInfo]invalid Current Page number %d %d\r\n", mtInfoMapPtr->mtInfo[dieNo].curPage, dieNo);
			mtMarker = POR_MAKER_TRIGGER;
			break;
		}
		else if(mtInfoMapPtr->mtInfo[dieNo].startPage < START_PAGE_NO_OF_MAPPING_TABLE_BLOCK
				|| mtInfoMapPtr->mtInfo[dieNo].startPage > USER_PAGES_PER_BLOCK)
		{
			xil_printf("[MtInfo]invalid Start Page number %d %d\r\n", mtInfoMapPtr->mtInfo[dieNo].startPage, dieNo);
			mtMarker = POR_MAKER_TRIGGER;
			break;
		}
	}

	if (mtMarker == POR_MAKER_TRIGGER)
	{
		xil_printf("Recover Mapping Table Info Fail \r\n");

		mtInfoMapPtr->mtInfo[0].format = POR_MAKER_TRIGGER;

		for(dieNo=0 ; dieNo < USER_DIES ; dieNo ++)
		{
			mtInfoMapPtr->mtInfo[dieNo].startBlock = mtInfoMapPtr->mtInfo[dieNo].curBlock = START_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE;	// 0 for bbt, 1 for sys meta
			mtInfoMapPtr->mtInfo[dieNo].startPage = mtInfoMapPtr->mtInfo[dieNo].curPage = PlsbPage2VpageTranslation(START_PAGE_NO_OF_MAPPING_TABLE_BLOCK);
		}
	}
}


void UpdateMappingTableInfoMap()
{

	unsigned int dieNo, tempSysBaseAddr, tempSysEntrySize;
	unsigned int tempSysBufAddr[USER_DIES];
	MAPPING_TABLE_INFO_ENTRY* mtUpdater;

	mtInfoMapPtr->mtInfo[0].format = POR_MAKER_IDLE;

	tempSysBaseAddr = RESERVED_DATA_BUFFER_BASE_ADDR;
	tempSysEntrySize = BYTES_PER_DATA_REGION_OF_PAGE + BYTES_PER_SPARE_REGION_OF_PAGE;

	for(dieNo = 0 ; dieNo < USER_DIES ; dieNo ++)
		tempSysBufAddr[dieNo] = tempSysBaseAddr + dieNo * USED_PAGES_FOR_MT_INFO_PER_DIE * tempSysEntrySize;

	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo++)
	{
		mtUpdater = (MAPPING_TABLE_INFO_ENTRY*) tempSysBufAddr[dieNo];
		*mtUpdater = mtInfoMapPtr->mtInfo[dieNo];
	}

	SaveSystemMeta(tempSysBufAddr, tempSysEntrySize, DATA_SIZE_OF_MT_INFO_PER_DIE, START_PAGE_NO_OF_MT_INFO_BLOCK);

}

void UpdateSystemMeta()
{
	EraseSystemMeta();
	UpdateMappingTableInfoMap();
	UpdateDieMap();
	UpdateBlockMap();
	UpdateGCMap();

	/*
	 * test code for check
	 *
	RecoverMappingTableInfoMap();
	mtInfoMapPtr->mtInfo[0].format = POR_MAKER_IDLE;
	xil_printf("recover die map\r\n");
	RecoverDieMap();
	xil_printf("recover block map\r\n");
	RecoverBlockMap();
	xil_printf("recover GC map \r\n");
	RecoverGCMap();

	*/
}

void ReadMappingTable(unsigned int tempMtBufAddr[], unsigned int tempMtBufEntrySize)
{
	unsigned int reqSlotTag, dieNo;
	int loop, dataSize;
	unsigned int readBlock, readPage;


	readBlock = mtInfoMapPtr->mtInfo[0].startBlock ;
	readPage = mtInfoMapPtr->mtInfo[0].startPage ;

	loop = 0;
	dataSize = DATA_SIZE_OF_MAPPING_TABLE_PER_DIE;

	while(dataSize>0)
	{
		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
		{
			reqSlotTag = GetFromFreeReqQ();

			reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
			reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_READ;
			if(tempMtBufEntrySize > BYTES_PER_DATA_REGION_OF_PAGE)
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ADDR;
			else
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_FOR_POR;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_PHY_ORG;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_OFF;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_NONE;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_TOTAL;

			reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.addr = tempMtBufAddr[dieNo] + loop * tempMtBufEntrySize;

			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalCh = Vdie2PchTranslation(dieNo);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalWay = Vdie2PwayTranslation(dieNo);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalBlock = readBlock;
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalPage =  Vpage2PlsbPageTranslation(readPage);

			SelectLowLevelReqQ(reqSlotTag);
		}
		loop++;
		readPage ++;
		if(readPage % USER_PAGES_PER_BLOCK == 0)
		{
			readBlock ++ ;
			readPage = PlsbPage2VpageTranslation(START_PAGE_NO_OF_MAPPING_TABLE_BLOCK);
			if (readBlock > END_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE)
			{
				readBlock = START_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE;
			}
		}
		dataSize -= BYTES_PER_DATA_REGION_OF_PAGE;
	}

	for(dieNo = 0; dieNo < USER_DIES; dieNo++)
	{
		mtInfoMapPtr->mtInfo[dieNo].curBlock = readBlock;
		mtInfoMapPtr->mtInfo[dieNo].curPage = readPage;
	}
	SyncAllLowLevelReqDone();

}

void SaveMappingTable(unsigned int tempMtBufAddr[], unsigned int tempMtBufEntrySize)
{
	unsigned int dieNo, reqSlotTag;
	int loop, dataSize;

	loop = 0;
	dataSize = DATA_SIZE_OF_MAPPING_TABLE_PER_DIE;

	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo ++)
	{
		mtInfoMapPtr->mtInfo[dieNo].startBlock = mtInfoMapPtr->mtInfo[dieNo].curBlock;
		mtInfoMapPtr->mtInfo[dieNo].startPage = mtInfoMapPtr->mtInfo[dieNo].curPage;
	}

	while(dataSize>0)
	{
		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
			if( logicalSliceMapPtr->logicalSlice[dieNo*tempMtBufEntrySize].virtualSliceAddr == VSA_NONE ||
					logicalSliceMapPtr->logicalSlice[dieNo*tempMtBufEntrySize].virtualSliceAddr <= SLICES_PER_SSD ||
					logicalSliceMapPtr->logicalSlice[dieNo*tempMtBufEntrySize].virtualSliceAddr >= 0)
			{
				if(mtInfoMapPtr->mtInfo[dieNo].curPage == PlsbPage2VpageTranslation(START_PAGE_NO_OF_MAPPING_TABLE_BLOCK))
				{
					reqSlotTag = GetFromFreeReqQ();

					reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
					reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_ERASE;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_PHY_ORG;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_NONE;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_NONE;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_TOTAL;

					reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalCh = Vdie2PchTranslation(dieNo);
					reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalWay = Vdie2PwayTranslation(dieNo);
					reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalBlock = mtInfoMapPtr->mtInfo[dieNo].curBlock;
					reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalPage = 0;	//dummy

					SelectLowLevelReqQ(reqSlotTag);
				}

				reqSlotTag = GetFromFreeReqQ();

				reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
				reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_WRITE;
				if(tempMtBufEntrySize > BYTES_PER_DATA_REGION_OF_PAGE)
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ADDR;
				else
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_FOR_POR;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_PHY_ORG;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_OFF;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_NONE;
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_TOTAL;

				reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.addr = tempMtBufAddr[dieNo] + loop * tempMtBufEntrySize;

				reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalCh = Vdie2PchTranslation(dieNo);
				reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalWay = Vdie2PwayTranslation(dieNo);
				reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalBlock = mtInfoMapPtr->mtInfo[dieNo].curBlock;
				reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalPage =  Vpage2PlsbPageTranslation(mtInfoMapPtr->mtInfo[dieNo].curPage);

				SelectLowLevelReqQ(reqSlotTag);
			}

		loop++;
		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
		{
			mtInfoMapPtr->mtInfo[dieNo].curPage ++;
			if(mtInfoMapPtr->mtInfo[dieNo].curPage % USER_PAGES_PER_BLOCK == 0)
			{
				mtInfoMapPtr->mtInfo[dieNo].curBlock ++ ;
				mtInfoMapPtr->mtInfo[dieNo].curPage = PlsbPage2VpageTranslation(START_PAGE_NO_OF_MAPPING_TABLE_BLOCK);
				if (mtInfoMapPtr->mtInfo[dieNo].curBlock > END_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE)
				{
					mtInfoMapPtr->mtInfo[dieNo].curBlock = START_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE;
				}
			}
		}

		dataSize -= BYTES_PER_DATA_REGION_OF_PAGE;
	}

	SyncAllLowLevelReqDone();

}


void RecoverMappingTable(unsigned int tempBufAddr)
{
	/*
	 * 본 함수는 mapping table 이 NAND에 제대로 쓰여있는지 확인하기 위해 NAND에서 table을 읽어오고
	 * 만일 NAND에 mapping table이 flush 되어있지 않은 경우 mapping table을 만드는 함수이다.
	 * */

	unsigned int mtMarker, dieNo, tempMtBufEntrySize, sliceNo;
	unsigned int tempMtBufAddr[USER_DIES];

	RecoverMappingTableInfoMap();

	if( mtInfoMapPtr->mtInfo[0].format == POR_MAKER_TRIGGER )
	{
		xil_printf("init mapping table \r\n");
		for (dieNo = 0 ; dieNo < USER_DIES ; dieNo ++ ) {
			mtInfoMapPtr->mtInfo[dieNo].curBlock = mtInfoMapPtr->mtInfo[dieNo].startBlock = START_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE;
			mtInfoMapPtr->mtInfo[dieNo].curPage = mtInfoMapPtr->mtInfo[dieNo].startPage = START_PAGE_NO_OF_MAPPING_TABLE_BLOCK;
		}
		InitSliceMap();
		// Debugging Code
		/*UpdateMappingTable(LOGICAL_SLICE_MAP_ADDR);
		ReadMappingTable(tempMtBufAddr, tempMtBufEntrySize);
		sysMetaMaker = POR_MAKER_TRIGGER;
		for(sliceNo=0; sliceNo<SLICES_PER_SSD ; sliceNo++)
		{
			virtualSliceMapPtr->virtualSlice[sliceNo].logicalSliceAddr = LSA_NONE;
		}
		mtMarker = POR_MAKER_IDLE;
		for (sliceNo=0 ; sliceNo<SLICES_PER_SSD; sliceNo++)
		{
			if(	logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr >= 0 &&
					logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr < SLICES_PER_SSD)
			{
				virtualSliceMapPtr->virtualSlice[logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr].logicalSliceAddr = sliceNo;
			}
			else if (logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr != VSA_NONE)
			{
				xil_printf("[ERROR] mapping table of lsa %d does not exist %d\r\n", sliceNo, logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr);
				mtMarker = POR_MAKER_TRIGGER;
			}
		}*/
	} else {
		tempMtBufEntrySize = BYTES_PER_DATA_REGION_OF_PAGE;
		for (dieNo = 0 ; dieNo < USER_DIES ; dieNo ++ )
		{
			tempMtBufAddr[dieNo] = tempBufAddr + dieNo * DATA_SIZE_OF_MAPPING_TABLE_PER_DIE; //tempBufAddr == LOGICAL_SLICE_MAP_ADDR
		}

		ReadMappingTable(tempMtBufAddr, tempMtBufEntrySize);


		for(sliceNo=0; sliceNo<SLICES_PER_SSD ; sliceNo++)
		{
			virtualSliceMapPtr->virtualSlice[sliceNo].logicalSliceAddr = LSA_NONE;
		}

		mtMarker = POR_MAKER_IDLE;
		for (sliceNo=0 ; sliceNo<SLICES_PER_SSD; sliceNo++)
		{
			if(	logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr >= 0 &&
					logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr < SLICES_PER_SSD)
			{
				virtualSliceMapPtr->virtualSlice[logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr].logicalSliceAddr = sliceNo;
			}
			else if (logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr != VSA_NONE)
			{
				xil_printf("mapping table of lsa %d does not exist %d\r\n", sliceNo, logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr);
				mtMarker = POR_MAKER_TRIGGER;
				break;
			}
		}

		// 만일 NAND에 mapping table이 존재하지 않는 경우
		if(mtMarker == POR_MAKER_TRIGGER) {
			for (dieNo = 0 ; dieNo < USER_DIES ; dieNo ++ ) {
				mtInfoMapPtr->mtInfo[dieNo].curBlock = mtInfoMapPtr->mtInfo[dieNo].startBlock = START_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE;
				mtInfoMapPtr->mtInfo[dieNo].curPage = mtInfoMapPtr->mtInfo[dieNo].startPage = START_PAGE_NO_OF_MAPPING_TABLE_BLOCK;
			}
			InitSliceMap();
		}
	}

}

void UpdateMappingTable(unsigned int tempBufAddr)
{
	unsigned int dieNo, tempMtBufEntrySize;
	unsigned int tempMtBufAddr[USER_DIES];

	tempMtBufEntrySize = BYTES_PER_DATA_REGION_OF_PAGE;
	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo ++ ) {
		tempMtBufAddr[dieNo] = tempBufAddr + dieNo * DATA_SIZE_OF_MAPPING_TABLE_PER_DIE; //tempBufAddr == LOGICAL_SLICE_MAP_ADDR
	}

	SaveMappingTable( tempMtBufAddr, tempMtBufEntrySize);
}

