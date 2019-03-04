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

P_MAPPING_TABLE_INFO_MAP mtInfoMapPtr;
P_SYSTEM_META_INFO_MAP sysInfoMapPtr;

void ReadSystemMeta(unsigned int tempSysBufAddr[], unsigned int tempSysBufEntrySize)
{
	unsigned int tempPage, reqSlotTag, dieNo;
	int loop, dataSize;

	loop = 0;
	dataSize = DATA_SIZE_OF_BAD_BLOCK_TABLE_PER_DIE;
	tempPage = PlsbPage2VpageTranslation(START_PAGE_NO_OF_BAD_BLOCK_TABLE_BLOCK); 	//bad block table is saved at lsb pages



	while(dataSize>0)
	{
		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
		{
			reqSlotTag = GetFromFreeReqQ();

			reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
			reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_READ;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ADDR;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_PHY_ORG;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_OFF;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_NONE;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_TOTAL;

			reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.addr = tempSysBufAddr[dieNo] + loop * tempSysBufEntrySize;

			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalCh = Vdie2PchTranslation(dieNo);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalWay = Vdie2PwayTranslation(dieNo);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalBlock = sysInfoMapPtr->sysInfo[dieNo].phyBlock;
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalPage = Vpage2PlsbPageTranslation(tempPage);

			SelectLowLevelReqQ(reqSlotTag);
		}

		tempPage++;
		loop++;
		dataSize -= BYTES_PER_DATA_REGION_OF_PAGE;
	}


}

void ReadMappingTable(unsigned int tempMtBufAddr[], unsigned int tempMtBufEntrySize)
{
	unsigned int reqSlotTag, dieNo;
	int loop, dataSize;

	loop = 0;
	dataSize = DATA_SIZE_OF_MAPPING_TABLE_PER_DIE;

	while(dataSize>0)
	{
		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
		{
			reqSlotTag = GetFromFreeReqQ();

			reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
			reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_READ;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ADDR;
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
		mtInfoMapPtr->mtInfo[dieNo].curPage ++;
		if(mtInfoMapPtr->mtInfo[dieNo].curPage % USER_PAGES_PER_BLOCK == 0)
		{
			mtInfoMapPtr->mtInfo[dieNo].curBlock ++ ;
			mtInfoMapPtr->mtInfo[dieNo].curPage = PlsbPage2VpageTranslation(START_PAGE_NO_OF_MAPPING_TABLE_BLOCK);
			if (mtInfoMapPtr->mtInfo[dieNo].curBlock == END_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE)
			{
				mtInfoMapPtr->mtInfo[dieNo].curBlock = START_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE;
			}
		}
		dataSize -= BYTES_PER_DATA_REGION_OF_PAGE;
	}

	SyncAllLowLevelReqDone();

}

void SaveMappingTable(unsigned char dieState[], unsigned int tempMtBufAddr[], unsigned int tempMtBufEntrySize)
{
	unsigned int dieNo, reqSlotTag;
	int loop, dataSize;

	loop = 0;
	dataSize = DATA_SIZE_OF_MAPPING_TABLE_PER_DIE;

	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo ++)
	{
		mtInfoMapPtr[dieNo].mtInfo->startBlock = mtInfoMapPtr[dieNo].mtInfo->curBlock;
		mtInfoMapPtr[dieNo].mtInfo->startPage = mtInfoMapPtr[dieNo].mtInfo->curPage;
	}

	while(dataSize>0)
	{
		for(dieNo = 0; dieNo < USER_DIES; dieNo++)
			if( logicalSliceMapPtr->logicalSlice[dieNo*tempMtBufEntrySize].virtualSliceAddr == VSA_NONE ||
					logicalSliceMapPtr->logicalSlice[dieNo*tempMtBufEntrySize].virtualSliceAddr <= SLICES_PER_SSD ||
					logicalSliceMapPtr->logicalSlice[dieNo*tempMtBufEntrySize].virtualSliceAddr >= 0)
			{
				if(loop == 0)
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
				reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ADDR;
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
		mtInfoMapPtr->mtInfo[dieNo].curPage ++;
		if(mtInfoMapPtr->mtInfo[dieNo].curPage % USER_PAGES_PER_BLOCK == 0)
		{
			mtInfoMapPtr->mtInfo[dieNo].curBlock ++ ;
			mtInfoMapPtr->mtInfo[dieNo].curPage = PlsbPage2VpageTranslation(START_PAGE_NO_OF_MAPPING_TABLE_BLOCK);
			if (mtInfoMapPtr->mtInfo[dieNo].curBlock == END_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE)
			{
				mtInfoMapPtr->mtInfo[dieNo].curBlock = START_BLOCK_NO_OF_MAPPING_TABLE_PER_DIE;
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

	/*
	 * TODO: NAND로부터 mapping table을 읽어온다.
	 * > ReadMappingTable 함수를 통해 읽어옴
	 *
	 * - NOTE: 모든 mapping table을 init 할 때 읽어오는 것은 high overhead (metadata로 NAND가 flush 된 상태인지 가리키는 데이터를 관리해서 넣기?)
	 * */
	tempMtBufEntrySize = BYTES_PER_DATA_REGION_OF_PAGE;
	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo ++ ) {
		tempMtBufAddr[dieNo] = tempBufAddr + dieNo * DATA_SIZE_OF_MAPPING_TABLE_PER_DIE; //tempBufAddr == LOGICAL_SLICE_MAP_ADDR
	}

	ReadMappingTable(tempMtBufAddr, tempMtBufEntrySize);

	// FIXME,
	mtMarker = MAPPING_TABLE_MAKER_IDLE;
	for (sliceNo=0 ; sliceNo<SLICES_PER_SSD; sliceNo++)
	{
		if(	logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr >= 0 ||
				logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr < SLICES_PER_SSD)
		{
			virtualSliceMapPtr->virtualSlice[logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr].logicalSliceAddr = sliceNo;
		}
		else if (logicalSliceMapPtr->logicalSlice[sliceNo].virtualSliceAddr != VSA_NONE)
		{
			xil_printf("mapping table of lsa %d does not exist", sliceNo);
			mtMarker = MAPPING_TABLE_MAKER_TRIGGER;
			break;
		}
	}

	// 만일 NAND에 mapping table이 존재하지 않는 경우
	if(mtMarker == MAPPING_TABLE_MAKER_TRIGGER)
		InitSliceMap();
}

void UpdateMappingTable(unsigned int tempBufAddr)
{
	unsigned int dieNo, tempMtBufEntrySize;
	unsigned char dieState[USER_DIES];
	unsigned int tempMtBufAddr[USER_DIES];
	/*
	 * TODO: buffer에 있는 l2v mapping table 를 NAND로 flush 함.
	 *
	 * SaveMappingTable 함수를 활용해 NAND에 write 요청을 보낸다.
	 * */
	tempMtBufEntrySize = BYTES_PER_DATA_REGION_OF_PAGE;
	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo ++ ) {
		tempMtBufAddr[dieNo] = tempBufAddr + dieNo * DATA_SIZE_OF_MAPPING_TABLE_PER_DIE; //tempBufAddr == LOGICAL_SLICE_MAP_ADDR
		dieState[dieNo] = DIE_STATE_MAPPING_TABLE_UPDATE;
	}

	SaveMappingTable(dieState, tempMtBufAddr, tempMtBufEntrySize);
}

