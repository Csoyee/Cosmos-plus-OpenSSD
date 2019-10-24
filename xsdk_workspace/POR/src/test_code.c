/*
 * test_code.c
 *
 *  Created on: 2019. 5. 10.
 *      Author: Soyee
 */
#include <assert.h>
#include "memory_map.h"
#include "xil_printf.h"


// this is just dummy read command for test
void dummyWriteCommand()
{
	// POR function check 를 위한 dummy write, host buffer 을 읽어올 수 없음.. 그냥 buffer에 직접 쓰기????ㅎ_ㅎ
	// buffer entry, address : DATA_BUFFER_BASE_ADDR + reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry * BYTES_PER_DATA_REGION_OF_SLICE
	// spare_entry, address  : SPARE_DATA_BUFFER_BASE_ADDR + reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry * BYTES_PER_SPARE_REGION_OF_SLICE
	unsigned int dataBufEntry;
	int *checker;

	xil_printf("dummy Write Command\r\n");

	// dummy lsn:
	dataBufEntry = AllocateDataBuf();
	dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr = 3;
	PutToDataBufHashList(dataBufEntry);
	dataBufMapPtr->dataBuf[dataBufEntry].dirty = DATA_BUF_DIRTY;

	dataBufEntry = AllocateDataBuf();
	dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr = 1035;
	PutToDataBufHashList(dataBufEntry);
	dataBufMapPtr->dataBuf[dataBufEntry].dirty = DATA_BUF_DIRTY;


	dataBufEntry = AllocateDataBuf();
	dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr = 30030;
	PutToDataBufHashList(dataBufEntry);
	dataBufMapPtr->dataBuf[dataBufEntry].dirty = DATA_BUF_DIRTY;
	// checker write dummy data (123123) to buffer region
	checker = (int*) (DATA_BUFFER_BASE_ADDR + dataBufEntry * BYTES_PER_DATA_REGION_OF_SLICE);
	*checker = 123123;


	dataBufEntry = AllocateDataBuf();
	dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr = 528193;
	PutToDataBufHashList(dataBufEntry);
	dataBufMapPtr->dataBuf[dataBufEntry].dirty = DATA_BUF_DIRTY;
}

// this is just dummy read command for test
void dummyReadCommand()
{
	unsigned int reqSlotTag, virtualSliceAddr, dataBufEntry;
	int *checker;
	int check;

	dataBufEntry = AllocateDataBuf();
	dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr = 30030; // change LSN value for test
	PutToDataBufHashList(dataBufEntry);

	virtualSliceAddr = AddrTransRead(30030);  // change LSN value for test
	if(virtualSliceAddr != VSA_FAIL)
	{
		reqSlotTag = GetFromFreeReqQ();
		reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
		reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_READ;
		reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = 3;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ENTRY;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_VSA;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_ON;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_MAIN;

		reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry = dataBufEntry;
		UpdateDataBufEntryInfoBlockingReq(reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry, reqSlotTag);
		reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr = virtualSliceAddr;

		SelectLowLevelReqQ(reqSlotTag);

	} else {
		xil_printf("get virtualSliceAddr fail \r\n");
	}

	SyncAllLowLevelReqDone();

	checker = (int*) (DATA_BUFFER_BASE_ADDR + dataBufEntry * BYTES_PER_DATA_REGION_OF_SLICE);
	check = *checker;

	xil_printf("buffer read: %d\r\n", check);
}

