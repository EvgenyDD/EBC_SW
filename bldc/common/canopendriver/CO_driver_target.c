#include "301/CO_driver.h"
#include "can_driver.h"

uint32_t prev_primask = 0;

void CO_CANsetConfigurationMode(void *CANptr)
{
	can_drv_enter_init_mode((CAN_TypeDef *)CANptr);
}

void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule)
{
	can_drv_leave_init_mode((CAN_TypeDef *)CANmodule->CANptr);
	CANmodule->CANnormal = true;
}

CO_ReturnError_t CO_CANmodule_init(
	CO_CANmodule_t *CANmodule,
	void *CANptr,
	CO_CANrx_t rxArray[],
	uint16_t rxSize,
	CO_CANtx_t txArray[],
	uint16_t txSize,
	uint16_t can_bitrate)
{
	/* verify arguments */
	if(CANmodule == NULL || rxArray == NULL || txArray == NULL) return CO_ERROR_ILLEGAL_ARGUMENT;

	/* Configure object variables */
	CANmodule->CANptr = CANptr;
	CANmodule->rxArray = rxArray;
	CANmodule->rxSize = rxSize;
	CANmodule->txArray = txArray;
	CANmodule->txSize = txSize;
	CANmodule->CANerrorStatus = 0;
	CANmodule->CANnormal = false;
	CANmodule->useCANrxFilters = (rxSize <= CAN_FILT_NUM) ? true : false;
	CANmodule->bufferInhibitFlag = false;
	CANmodule->firstCANtxMessage = true;
	CANmodule->CANtxCount = 0U;
	CANmodule->errOld = 0U;

	for(uint16_t i = 0U; i < rxSize; i++)
	{
		rxArray[i].ident = 0U;
		rxArray[i].mask = 0xFFFFU;
		rxArray[i].object = NULL;
		rxArray[i].CANrx_callback = NULL;
	}
	for(uint16_t i = 0U; i < txSize; i++)
	{
		txArray[i].bufferFull = false;
	}

	/* Configure CAN module registers */
	can_drv_start((CAN_TypeDef *)CANptr);

	can_drv_enter_init_mode((CAN_TypeDef *)CANptr);

	/* Configure CAN timing */
	if(can_bitrate < 10 || can_bitrate > 1000) return CO_ERROR_ILLEGAL_ARGUMENT;
	if(can_drv_check_set_bitrate((CAN_TypeDef *)CANptr, can_bitrate * 1000, true) != can_bitrate * 1000) return CO_ERROR_ILLEGAL_BAUDRATE;

	/* Configure CAN module hardware filters */
	if(CANmodule->useCANrxFilters == false)
	{
		can_drv_set_rx_filter((CAN_TypeDef *)CANptr, 0, 0, 0); // set filter0 as allpass
	}

	return CO_ERROR_NO;
}

void CO_CANmodule_disable(CO_CANmodule_t *CANmodule)
{
	if(CANmodule != NULL)
	{
		can_drv_reset_module((CAN_TypeDef *)CANmodule->CANptr);
	}
}

CO_ReturnError_t CO_CANrxBufferInit(
	CO_CANmodule_t *CANmodule,
	uint16_t index,
	uint16_t ident,
	uint16_t mask,
	bool_t rtr,
	void *object,
	void (*CANrx_callback)(void *object, void *message))
{
	CO_ReturnError_t ret = CO_ERROR_NO;
	CAN_TypeDef *dev = (CAN_TypeDef *)CANmodule->CANptr;

	if((CANmodule != NULL) && (object != NULL) && (CANrx_callback != NULL) && (index < CANmodule->rxSize))
	{
		/* buffer, which will be configured */
		CO_CANrx_t *buffer = &CANmodule->rxArray[index];

		/* Configure object variables */
		buffer->object = object;
		buffer->CANrx_callback = CANrx_callback;

		buffer->ident = ((uint32_t)(ident & 0x07FFU)) << 21;
		if(rtr)
		{
			buffer->ident |= CAN_RI0R_RTR;
		}
		// IDE (0x4) & RTR (0x2) have to match
		buffer->mask = (((uint32_t)(mask & 0x07FFU)) << 21) |
					   CAN_RI0R_RTR |
					   CAN_RI0R_IDE;

		/* Set CAN hardware module filter and mask. */
		if(CANmodule->useCANrxFilters)
		{
			can_drv_set_rx_filter(dev, index, buffer->ident, buffer->mask);
		}
	}
	else
	{
		ret = CO_ERROR_ILLEGAL_ARGUMENT;
	}

	return ret;
}

CO_CANtx_t *CO_CANtxBufferInit(
	CO_CANmodule_t *CANmodule,
	uint16_t index,
	uint16_t ident,
	bool_t rtr,
	uint8_t noOfBytes,
	bool_t syncFlag)
{
	CO_CANtx_t *buffer = NULL;

	if((CANmodule != NULL) && (index < CANmodule->txSize))
	{
		/* get specific buffer */
		buffer = &CANmodule->txArray[index];

		buffer->ident = (((uint32_t)(ident & 0x07FFU)) << 21) |
						(rtr ? CAN_TI0R_RTR : 0);
		buffer->DLC = noOfBytes & 0xFU;
		buffer->bufferFull = false;
		buffer->syncFlag = syncFlag;
	}

	return buffer;
}

int co_drv_send_ex(void *dev, uint32_t ident, uint8_t *data, uint32_t dlc)
{
	CO_LOCK_CAN_SEND(CANmodule);
	int sts = can_drv_tx(dev, ident << 21, dlc, data);
	CO_UNLOCK_CAN_SEND(CANmodule);

	return sts;
}

CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
	CO_ReturnError_t err = CO_ERROR_NO;

	if(buffer->bufferFull) /* Verify overflow */
	{
		if(!CANmodule->firstCANtxMessage)
		{
			CANmodule->CANerrorStatus |= CO_CAN_ERRTX_OVERFLOW; /* don't set error, if bootup message is still on buffers */
		}
		err = CO_ERROR_TX_OVERFLOW;
	}

	CO_LOCK_CAN_SEND(CANmodule);
	CAN_TypeDef *dev = (CAN_TypeDef *)CANmodule->CANptr;
	/* if CAN TX buffer is free, copy message to it */
	if(can_drv_tx(dev, buffer->ident, buffer->DLC, buffer->data) > 0 &&
	   CANmodule->CANtxCount == 0)
	{
		CANmodule->bufferInhibitFlag = buffer->syncFlag;
	}
	/* if no buffer is free, message will be sent by interrupt */
	else
	{
		buffer->bufferFull = true;
		CANmodule->CANtxCount++;
	}
	CO_UNLOCK_CAN_SEND(CANmodule);

	return err;
}

void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule)
{
	uint32_t tpdoDeleted = 0U;
	CAN_TypeDef *dev = (CAN_TypeDef *)CANmodule->CANptr;

	CO_LOCK_CAN_SEND(CANmodule);
	/* Abort message from CAN module, if there is synchronous TPDO. Take special care with this functionality. */
	if(can_drv_is_transmitting(dev) && CANmodule->bufferInhibitFlag)
	{
		/* clear TXREQ */
		can_drv_tx_abort(dev);
		CANmodule->bufferInhibitFlag = false; /* clear TXREQ */
		tpdoDeleted = 1U;
	}

	if(CANmodule->CANtxCount != 0U) // delete also pending synchronous TPDOs in TX buffers
	{
		CO_CANtx_t *buffer = &CANmodule->txArray[0];
		for(uint16_t i = CANmodule->txSize; i > 0U; i--)
		{
			if(buffer->bufferFull)
			{
				if(buffer->syncFlag)
				{
					buffer->bufferFull = false;
					CANmodule->CANtxCount--;
					tpdoDeleted = 2U;
				}
			}
			buffer++;
		}
	}
	CO_UNLOCK_CAN_SEND(CANmodule);

	if(tpdoDeleted != 0U)
	{
		CANmodule->CANerrorStatus |= CO_CAN_ERRTX_PDO_LATE;
	}
}

/* Get error counters from the module. If necessary, function may use different way to determine errors. */
void CO_CANmodule_process(CO_CANmodule_t *CANmodule)
{
	CAN_TypeDef *dev = (CAN_TypeDef *)CANmodule->CANptr;

	uint16_t overflow = can_drv_check_bus_off(dev) ? 1 : 0;
	uint16_t rxErrors = can_drv_get_rx_error_counter(dev);
	uint16_t txErrors = can_drv_get_tx_error_counter(dev);

	uint32_t err = ((uint32_t)txErrors << 16) | ((uint32_t)rxErrors << 8) | overflow;

	if(CANmodule->errOld != err)
	{
		uint16_t status = CANmodule->CANerrorStatus;
		CANmodule->errOld = err;

		if(txErrors >= 256U)
		{
			status |= CO_CAN_ERRTX_BUS_OFF;
		}
		else
		{
			/* recalculate CANerrorStatus, first clear some flags */
			status &= 0xFFFF ^ (CO_CAN_ERRTX_BUS_OFF |
								CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE |
								CO_CAN_ERRTX_WARNING | CO_CAN_ERRTX_PASSIVE);
			if(rxErrors >= 128)
			{
				status |= CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE;
			}
			else if(rxErrors >= 96)
			{
				status |= CO_CAN_ERRRX_WARNING;
			}

			if(txErrors >= 128)
			{
				status |= CO_CAN_ERRTX_WARNING | CO_CAN_ERRTX_PASSIVE;
			}
			else if(rxErrors >= 96)
			{
				status |= CO_CAN_ERRTX_WARNING;
			}

			/* if not tx passive clear also overflow */
			if((status & CO_CAN_ERRTX_PASSIVE) == 0)
			{
				status &= 0xFFFF ^ CO_CAN_ERRTX_OVERFLOW;
			}
		}

		if(overflow != 0)
		{
			status |= CO_CAN_ERRRX_OVERFLOW;
		}

		CANmodule->CANerrorStatus = status;
	}
}

void CO_CANinterrupt(CO_CANmodule_t *CANmodule)
{
	CAN_TypeDef *dev = (CAN_TypeDef *)CANmodule->CANptr;
	uint32_t txMask = 0;

	if(can_drv_is_rx_pending(dev)) // receive interrupt
	{
		do
		{
			CO_CANrx_t *buffer = NULL; /* receive message buffer from CO_CANmodule_t object. */
			bool_t msgMatched = false;

			if(can_drv_check_rx_overrun(dev)) return;

			uint16_t index = can_drv_get_rx_filter_index(dev); /* get index of the received message here. Or something similar */

			CO_CANrx_t rcv_msg = {
				.ident = can_drv_get_rx_ident(dev),
				.DLC = can_drv_get_rx_dlc(dev)};
			can_drv_get_rx_data(dev, rcv_msg.data);
			can_drv_release_rx_message(dev);

			if(CANmodule->useCANrxFilters)
			{
				// CAN module filters are used. Message with known 11-bit identifier has been received
				if(index < CANmodule->rxSize)
				{
					buffer = &CANmodule->rxArray[index];

					if(((rcv_msg.ident ^ buffer->ident) & buffer->mask) == 0U)
					{
						msgMatched = true;
					}
				}
			}
			else
			{
				// CAN module filters are not used, message with any standard 11-bit identifier
				// has been received. Search rxArray form CANmodule for the same CAN-ID
				buffer = &CANmodule->rxArray[0];
				for(index = CANmodule->rxSize; index > 0U; index--)
				{
					if(((rcv_msg.ident ^ buffer->ident) & buffer->mask) == 0U)
					{
						msgMatched = true;
						break;
					}
					buffer++;
				}
			}

			if(msgMatched && (buffer != NULL) && (buffer->CANrx_callback != NULL))
			{
				buffer->CANrx_callback(buffer->object, (void *)&rcv_msg);
			}
		} while(0);
	}
	else if((txMask = can_drv_is_message_sent(dev))) //  transmit interrupt
	{
		can_drv_release_tx_message(dev, txMask); // clear interrupt flag
		CANmodule->firstCANtxMessage = false;	 // first CAN message (bootup) was sent successfully
		CANmodule->bufferInhibitFlag = false;	 // clear flag from previous message

		if(CANmodule->CANtxCount > 0U) // Are there any new messages waiting to be send?
		{
			CO_CANtx_t *buffer = &CANmodule->txArray[0];
			uint16_t i;
			for(i = CANmodule->txSize; i > 0U; i--)
			{
				if(buffer->bufferFull)
				{
					buffer->bufferFull = false;
					CANmodule->CANtxCount--;

					CANmodule->bufferInhibitFlag = buffer->syncFlag;
					can_drv_tx(dev, buffer->ident, buffer->DLC, buffer->data);
					break;
				}
				buffer++;
			}

			if(i == 0U)
			{
				CANmodule->CANtxCount = 0U;
			}
		}
	}
	else
	{
		// some other interrupt reason
	}
}