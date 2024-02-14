#include "sdo.h"
#include "platform.h"

CO_SDO_abortCode_t read_SDO(CO_SDOclient_t *SDO_C, uint8_t nodeId,
							uint16_t index, uint8_t subIndex,
							uint8_t *buf, size_t bufSize, size_t *readSize,
							uint32_t timeout_ms)
{
	// for(uint32_t nd = 0; nd < CO->HBcons->numberOfMonitoredNodes; nd++)
	{
// #warning "write me" TODO!!!!!!!!!!!
	}

	CO_SDO_return_t SDO_ret = CO_SDOclient_setup(SDO_C, CO_CAN_ID_SDO_CLI + nodeId, CO_CAN_ID_SDO_SRV + nodeId, nodeId);
	if(SDO_ret != CO_SDO_RT_ok_communicationEnd) return CO_SDO_AB_GENERAL;

	SDO_ret = CO_SDOclientUploadInitiate(SDO_C, index, subIndex, timeout_ms, false);
	if(SDO_ret != CO_SDO_RT_ok_communicationEnd) return CO_SDO_AB_GENERAL;

	do
	{
		uint32_t timeDifference_us = 1000;
		CO_SDO_abortCode_t abortCode = CO_SDO_AB_NONE;

		SDO_ret = CO_SDOclientUpload(SDO_C, timeDifference_us, false, &abortCode, NULL, NULL, NULL);
		if(SDO_ret < 0) return abortCode;

		delay_ms_w_can_poll(timeDifference_us / 1000);
	} while(SDO_ret > 0);

	// copy data to the user buffer (for long data function must be called
	// several times inside the loop)
	*readSize = CO_SDOclientUploadBufRead(SDO_C, buf, bufSize);

	return CO_SDO_AB_NONE;
}

CO_SDO_abortCode_t write_SDO(CO_SDOclient_t *SDO_C, uint8_t nodeId,
							 uint16_t index, uint8_t subIndex,
							 uint8_t *data, size_t dataSize,
							 uint32_t timeout_ms)
{
	bool_t bufferPartial = false;

	CO_SDO_return_t SDO_ret = CO_SDOclient_setup(SDO_C, CO_CAN_ID_SDO_CLI + nodeId, CO_CAN_ID_SDO_SRV + nodeId, nodeId);
	if(SDO_ret != CO_SDO_RT_ok_communicationEnd) return CO_SDO_AB_GENERAL;

	SDO_ret = CO_SDOclientDownloadInitiate(SDO_C, index, subIndex, dataSize, timeout_ms, false);
	if(SDO_ret != CO_SDO_RT_ok_communicationEnd) return CO_SDO_AB_GENERAL;

	size_t nWritten = CO_SDOclientDownloadBufWrite(SDO_C, data, dataSize);
	if(nWritten < dataSize) bufferPartial = true;

	do
	{
		uint32_t timeDifference_us = 1000;
		CO_SDO_abortCode_t abortCode = CO_SDO_AB_NONE;

		SDO_ret = CO_SDOclientDownload(SDO_C, timeDifference_us, false, bufferPartial, &abortCode, NULL, NULL);
		if(SDO_ret < 0) return abortCode;

		delay_ms_w_can_poll(timeDifference_us / 1000);
	} while(SDO_ret > 0);

	return CO_SDO_AB_NONE;
}