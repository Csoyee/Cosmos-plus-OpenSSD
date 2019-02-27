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


void ReadMappingTable(unsigned int tempMtBufAddr[], unsigned int tempMtBufEntrySize)
{
	// TODO: die ���� �о���� �Լ� ����? (mapping table�� �ʹ� ũ�� buffer �ʹ� ���� �����.)
	unsigned int tempPage, reqSlotTag, dieNo;
	int loop, dataSize, blockNo;

	loop = 0;
	blockNo = 0;
	dataSize = DATA_SIZE_OF_MAPPING_TABLE_PER_DIE;
	tempPage = PlsbPage2VpageTranslation(START_PAGE_NO_OF_MAPPING_TABLE_BLOCK); 	//mapping table is saved at lsb pages

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

			reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.addr = tempMtBufAddr[dieNo] + loop * tempMtBufEntrySize + blockNo * (tempMtBufEntrySize * USER_PAGES_PER_BLOCK);

			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalCh = Vdie2PchTranslation(dieNo);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalWay = Vdie2PwayTranslation(dieNo);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalBlock = mtInfoMapPtr->mtInfo[dieNo].phyBlock + blockNo;
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalPage = Vpage2PlsbPageTranslation(tempPage);

			SelectLowLevelReqQ(reqSlotTag);
		}

		tempPage++;
		loop++;
		if(tempPage % USER_PAGES_PER_BLOCK == 0)
		{
			blockNo ++;
			tempPage = PlsbPage2VpageTranslation(START_PAGE_NO_OF_MAPPING_TABLE_BLOCK);
			loop = 0;
		}
		dataSize -= BYTES_PER_DATA_REGION_OF_PAGE;
	}

	SyncAllLowLevelReqDone();
}

void SaveMappingTable(unsigned char dieState[], unsigned int tempMtBufAddr[], unsigned int tempMtBufEntrySize)
{
	unsigned int dieNo, reqSlotTag;
	int loop, dataSize, tempPage, blockNo;

	loop = 0;
	blockNo = 0;
	dataSize = DATA_SIZE_OF_MAPPING_TABLE_PER_DIE;
	tempPage = PlsbPage2VpageTranslation(START_PAGE_NO_OF_MAPPING_TABLE_BLOCK);	//bad block table is saved at lsb pages

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
					reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalBlock = bbtInfoMapPtr->bbtInfo[dieNo].phyBlock + blockNo;
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

				reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.addr = tempMtBufAddr[dieNo] + loop * tempMtBufEntrySize + blockNo * (tempMtBufEntrySize * USER_PAGES_PER_BLOCK);

				reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalCh = Vdie2PchTranslation(dieNo);
				reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalWay = Vdie2PwayTranslation(dieNo);
				reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalBlock = mtInfoMapPtr->mtInfo[dieNo].phyBlock + blockNo;
				reqPoolPtr->reqPool[reqSlotTag].nandInfo.physicalPage =  Vpage2PlsbPageTranslation(tempPage);

				SelectLowLevelReqQ(reqSlotTag);
			}

		tempPage++;
		loop++;
		if(tempPage % USER_PAGES_PER_BLOCK == 0)
		{
			blockNo ++;
			tempPage = PlsbPage2VpageTranslation(START_PAGE_NO_OF_MAPPING_TABLE_BLOCK);
			loop = 0;
		}
		dataSize -= BYTES_PER_DATA_REGION_OF_PAGE;
	}

	SyncAllLowLevelReqDone();
}


void RecoverMappingTable(unsigned int tempBufAddr)
{
	/*
	 * �� �Լ��� mapping table �� NAND�� ����� �����ִ��� Ȯ���ϱ� ���� NAND���� table�� �о����
	 * ���� NAND�� mapping table�� flush �Ǿ����� ���� ��� mapping table�� ����� �Լ��̴�.
	 * */

	unsigned int mtMarker, dieNo, tempMtBufEntrySize, pageNo;
	unsigned int tempMtBufAddr[USER_DIES];

	/*
	 * TODO: NAND�κ��� mapping table�� �о�´�.
	 * > ReadMappingTable �Լ��� ���� �о��
	 *
	 * - NOTE: ��� mapping table�� init �� �� �о���� ���� high overhead (metadata�� NAND�� flush �� �������� ����Ű�� �����͸� �����ؼ� �ֱ�?)
	 * */
	tempMtBufEntrySize = BYTES_PER_DATA_REGION_OF_PAGE;
	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo ++ ) {
		tempMtBufAddr[dieNo] = tempBufAddr + dieNo * DATA_SIZE_OF_MAPPING_TABLE_PER_DIE; //tempBufAddr == LOGICAL_SLICE_MAP_ADDR
	}

	ReadMappingTable(tempMtBufAddr, tempMtBufEntrySize);

	mtMarker = MAPPING_TABLE_MAKER_IDLE;
	for (pageNo=0 ; pageNo<SLICES_PER_SSD; pageNo++)
	{
		if(logicalSliceMapPtr->logicalSlice[pageNo].virtualSliceAddr != VSA_NONE &&
				logicalSliceMapPtr->logicalSlice[pageNo].virtualSliceAddr < 0 &&
				logicalSliceMapPtr->logicalSlice[pageNo].virtualSliceAddr >= SLICES_PER_SSD)
		{
			xil_printf("mapping table of lsa %d does not exist", pageNo);
			mtMarker = MAPPING_TABLE_MAKER_TRIGGER;
			break;
		}
	}

	// ���� NAND�� mapping table�� �������� �ʴ� ���
	if(mtMarker == MAPPING_TABLE_MAKER_TRIGGER)
		InitSliceMap();
}

void UpdateMappingTable(unsigned int tempBufAddr)
{
	unsigned int dieNo, tempMtBufEntrySize;
	unsigned char dieState[USER_DIES];
	unsigned int tempMtBufAddr[USER_DIES];
	/*
	 * TODO: l2v mapping table�� ���ӵ� buffer ������ �Ҵ��ؼ� NAND�� flush ��
	 *
	 * SaveMappingTable �Լ��� Ȱ���� NAND�� write ��û�� ������.
	 * */
	tempMtBufEntrySize = BYTES_PER_DATA_REGION_OF_PAGE;
	for (dieNo = 0 ; dieNo < USER_DIES ; dieNo ++ ) {
		tempMtBufAddr[dieNo] = tempBufAddr + dieNo * DATA_SIZE_OF_MAPPING_TABLE_PER_DIE; //tempBufAddr == LOGICAL_SLICE_MAP_ADDR
		dieState[dieNo] = DIE_STATE_MAPPING_TABLE_UPDATE;
	}

	SaveMappingTable(dieState, tempMtBufAddr, tempMtBufEntrySize);
}

