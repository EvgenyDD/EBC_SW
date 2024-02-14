#ifndef CO_DRIVER_TARGET_H_
#define CO_DRIVER_TARGET_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f4xx.h"

extern uint32_t prev_primask;

/* Basic definitions. If big endian, CO_SWAP_xx macros must swap bytes. */
#define CO_LITTLE_ENDIAN
#define CO_SWAP_16(x) x
#define CO_SWAP_32(x) x
#define CO_SWAP_64(x) x
/* NULL is defined in stddef.h */
/* true and false are defined in stdbool.h */
/* int8_t to uint64_t are defined in stdint.h */
typedef uint_fast8_t bool_t;
typedef float float32_t;
typedef double float64_t;

/* Access to received CAN message */
#define CO_CANrxMsg_readIdent(msg) (((CO_CANrx_t *)msg)->ident >> 21)
#define CO_CANrxMsg_readDLC(msg) (((CO_CANrx_t *)msg)->DLC)
#define CO_CANrxMsg_readData(msg) (((CO_CANrx_t *)msg)->data)

/* Received message object */
typedef struct
{
	uint32_t ident;
	uint32_t mask;
	uint8_t DLC;
	uint8_t data[8];
	void *object;
	void (*CANrx_callback)(void *object, void *message);
} CO_CANrx_t;

/* Transmit message object */
typedef struct
{
	uint32_t ident;
	uint8_t DLC;
	uint8_t data[8];
	volatile bool_t bufferFull;
	volatile bool_t syncFlag;
} CO_CANtx_t;

/* CAN module object */
typedef struct
{
	void *CANptr;
	CO_CANrx_t *rxArray;
	uint16_t rxSize;
	CO_CANtx_t *txArray;
	uint16_t txSize;
	uint16_t CANerrorStatus;
	volatile bool_t CANnormal;
	volatile bool_t useCANrxFilters;
	volatile bool_t bufferInhibitFlag;
	volatile bool_t firstCANtxMessage;
	volatile uint16_t CANtxCount;
	uint32_t errOld;
	bool rx_ovf;
} CO_CANmodule_t;

/* Data storage object for one entry */
typedef struct
{
	void *addr;
	size_t len;
	uint8_t subIndexOD;
	uint8_t attr;
	/* Additional variables (target specific) */
	void *addrNV;
} CO_storage_entry_t;

#define __LOCK_IRQ()                    \
	{                                   \
		prev_primask = __get_PRIMASK(); \
		__disable_irq();                \
	}

#define __UNLOCK_IRQ()               \
	{                                \
		__set_PRIMASK(prev_primask); \
	}

/* (un)lock critical section in CO_CANsend() */
#define CO_LOCK_CAN_SEND(CAN_MODULE) __LOCK_IRQ()
#define CO_UNLOCK_CAN_SEND(CAN_MODULE) __UNLOCK_IRQ()

/* (un)lock critical section in CO_errorReport() or CO_errorReset() */
#define CO_LOCK_EMCY(CAN_MODULE) __LOCK_IRQ()
#define CO_UNLOCK_EMCY(CAN_MODULE) __UNLOCK_IRQ()

/* (un)lock critical section when accessing Object Dictionary */
#define CO_LOCK_OD(CAN_MODULE) __LOCK_IRQ()
#define CO_UNLOCK_OD(CAN_MODULE) __UNLOCK_IRQ()

/* Synchronization between CAN receive and message processing threads. */
#define CO_MemoryBarrier()
#define CO_FLAG_READ(rxNew) ((rxNew) != NULL)
#define CO_FLAG_SET(rxNew)  \
	{                       \
		CO_MemoryBarrier(); \
		rxNew = (void *)1L; \
	}
#define CO_FLAG_CLEAR(rxNew) \
	{                        \
		CO_MemoryBarrier();  \
		rxNew = NULL;        \
	}

int co_drv_send_ex(void *dev, uint32_t ident, uint8_t *data, uint32_t dlc);

void CO_CANinterrupt(CO_CANmodule_t *CANmodule);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CO_DRIVER_TARGET_H_
