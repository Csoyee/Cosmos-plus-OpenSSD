//////////////////////////////////////////////////////////////////////////////////
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// File Name: test_code.c
//
// Description:
//   - share implementation testing code
//
//////////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include "memory_map.h"
#include "xil_printf.h"
#include "ftl_config.h"

void dummyBufWriteCommand(unsigned int logicalSliceAddr);
void dummyNANDReadCommand(unsigned int logicalSliceAddr);
void shareCommand(unsigned int sourceSliceAddr, unsigned int targetSliceAddr);
void checkShareList(unsigned int logicalSliceAddr);
void FlushDataBuffer();

void dummyEvictCommand(unsigned int dataBufEntry)
{
	unsigned int reqSlotTag, virtualSliceAddr;

	if(dataBufMapPtr->dataBuf[dataBufEntry].dirty == DATA_BUF_DIRTY)
	{
		xil_printf("Evicted LSN: %d \r\n", dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr);
		reqSlotTag = GetFromFreeReqQ();
		virtualSliceAddr =  AddrTransWrite(dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr);

		reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
		reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_WRITE;
		reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ENTRY;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_VSA;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_ON;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK;
		reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_MAIN;
		reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry = dataBufEntry;
		UpdateDataBufEntryInfoBlockingReq(dataBufEntry, reqSlotTag);
		reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr = virtualSliceAddr;

		SelectLowLevelReqQ(reqSlotTag);

		dataBufMapPtr->dataBuf[dataBufEntry].dirty = DATA_BUF_CLEAN;
	}
}

void dummyBufWriteCommand(unsigned int logicalSliceAddr)
{
	unsigned int dataBufEntry;
	int * checker;

	dataBufEntry = AllocateDataBuf();
	dummyEvictCommand(dataBufEntry);
	dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr = logicalSliceAddr;
	PutToDataBufHashList(dataBufEntry);
	dataBufMapPtr->dataBuf[dataBufEntry].dirty = DATA_BUF_DIRTY;
	checker = (int*) (DATA_BUFFER_BASE_ADDR + dataBufEntry * BYTES_PER_DATA_REGION_OF_SLICE);
	*checker = 123123 ; // dummy data for test
}

void dummyNANDReadCommand(unsigned int logicalSliceAddr)
{
	unsigned int reqSlotTag, virtualSliceAddr, dataBufEntry;
	int *checker;
	int check;

	dataBufEntry = AllocateDataBuf();
	dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr = logicalSliceAddr; // change LSN value for test
	PutToDataBufHashList(dataBufEntry);

	virtualSliceAddr = AddrTransRead(logicalSliceAddr);  // change LSN value for test
	if(virtualSliceAddr != VSA_FAIL)
	{
		reqSlotTag = GetFromFreeReqQ();
		reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
		reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_READ;
		reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = logicalSliceAddr;
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

	xil_printf("Buffer Read: %d\r\n", check);
}

void dummyShareCommand(unsigned int sourceSliceAddr, unsigned int targetSliceAddr)
{
	unsigned int dataBufEntry, reqSlotTag, virtualSliceAddr, tempSliceAddr;

	dataBufEntry = CheckDataBufHit(sourceSliceAddr);
	if(dataBufEntry != DATA_BUF_FAIL)
	{
		// is in buffer
		if(dataBufMapPtr->dataBuf[dataBufEntry].dirty == DATA_BUF_DIRTY)
		{
			xil_printf("sourSliceAddr is in buffer and is dirty\r\n");
			reqSlotTag = GetFromFreeReqQ();
			virtualSliceAddr = AddrTransWrite(sourceSliceAddr);

			reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
			reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_WRITE;
			reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = dataBufMapPtr->dataBuf[dataBufEntry].logicalSliceAddr;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_ENTRY;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_VSA;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_ON;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK;
			reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_MAIN;
			reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry = dataBufEntry;
			UpdateDataBufEntryInfoBlockingReq(dataBufEntry, reqSlotTag);
			reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr = virtualSliceAddr;

			SelectLowLevelReqQ(reqSlotTag);

			dataBufMapPtr->dataBuf[dataBufEntry].dirty = DATA_BUF_CLEAN;

			virtualSliceMapPtr->virtualSlice[virtualSliceAddr].logicalSliceAddr = setShareBit(targetSliceAddr);
			logicalSliceMapPtr->logicalSlice[targetSliceAddr].virtualSliceAddr = setShareBit(sourceSliceAddr);
		} else
		{
			xil_printf("sourSliceAddr is in buffer and is clean\r\n");
			tempSliceAddr = sourceSliceAddr;
			while (getShareBit(logicalSliceMapPtr->logicalSlice[tempSliceAddr].virtualSliceAddr))
			{
				tempSliceAddr = getAddress(logicalSliceMapPtr->logicalSlice[tempSliceAddr].virtualSliceAddr);
			}

			virtualSliceAddr = logicalSliceMapPtr->logicalSlice[tempSliceAddr].virtualSliceAddr;
			sourceSliceAddr = virtualSliceMapPtr->virtualSlice[virtualSliceAddr].logicalSliceAddr;

			virtualSliceMapPtr->virtualSlice[virtualSliceAddr].logicalSliceAddr = setShareBit(targetSliceAddr);
			logicalSliceMapPtr->logicalSlice[targetSliceAddr].virtualSliceAddr = setShareBit(sourceSliceAddr);
		}
	} else {
		// is not in buffer
		xil_printf("sourSliceAddr is not in buffer\r\n");
		tempSliceAddr = sourceSliceAddr;
		while (getShareBit(logicalSliceMapPtr->logicalSlice[tempSliceAddr].virtualSliceAddr))
		{
			tempSliceAddr = getAddress(logicalSliceMapPtr->logicalSlice[tempSliceAddr].virtualSliceAddr);
		}

		virtualSliceAddr = logicalSliceMapPtr->logicalSlice[tempSliceAddr].virtualSliceAddr;
		sourceSliceAddr = virtualSliceMapPtr->virtualSlice[virtualSliceAddr].logicalSliceAddr;

		virtualSliceMapPtr->virtualSlice[virtualSliceAddr].logicalSliceAddr = setShareBit(targetSliceAddr);
		logicalSliceMapPtr->logicalSlice[targetSliceAddr].virtualSliceAddr = setShareBit(sourceSliceAddr);
	}

	checkShareList(targetSliceAddr);
}

// Flush whole data in data buffer
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

				bufEntry = dataBufMapPtr->dataBuf[bufEntry].hashNextEntry;
			}
		}
	}

	SyncAllLowLevelReqDone();
}

void checkShareList(unsigned int logicalSliceAddr)
{
	unsigned int tempSliceAddr;

	tempSliceAddr = logicalSliceAddr;

	xil_printf("LSN: %d  ", tempSliceAddr);
	while (getShareBit(logicalSliceMapPtr->logicalSlice[tempSliceAddr].virtualSliceAddr))
	{
		tempSliceAddr = getAddress(logicalSliceMapPtr->logicalSlice[tempSliceAddr].virtualSliceAddr);
		xil_printf("%d (press enter) ", tempSliceAddr);
		inbyte();
	}
	xil_printf("\r\nVSN: %d\r\n", logicalSliceMapPtr->logicalSlice[tempSliceAddr].virtualSliceAddr);
}

void testCode()
{
	xil_printf("test Code\r\n");
	// 1. write dummy data
	dummyBufWriteCommand(1);
	dummyBufWriteCommand(2);
	dummyBufWriteCommand(3);
	dummyBufWriteCommand(4);
	dummyBufWriteCommand(5);
	dummyBufWriteCommand(6);
	dummyBufWriteCommand(7);
	dummyBufWriteCommand(8);
	dummyBufWriteCommand(9);
	dummyBufWriteCommand(10);

	// 2. check buffered data share works well
	dummyShareCommand(8, 13);
	dummyShareCommand(13, 15);
	dummyShareCommand(8, 14);

	dummyNANDReadCommand(8);
	dummyNANDReadCommand(14);
	// 3. check unbuffered data share works well

}
