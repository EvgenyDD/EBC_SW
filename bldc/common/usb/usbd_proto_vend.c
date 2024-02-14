#include "usbd_proto_vend.h"
#include "usbd_proto_core.h"
#include "usbd_req.h"

enum
{
	ST_IDLE = 0,
	ST_DWL,
	ST_UPL,
} st = 0;

uint16_t wLength = 0;

void usbd_proto_vendor_handler(void *pdev, USB_SETUP_REQ *req)
{
	if(st != ST_IDLE)
	{
		USBD_CtlError(pdev, req);
		return;
	}
	if(req->bRequest == 0) // download
	{
		if(req->wLength > 0)
		{
			wLength = req->wLength;
			if(wLength > sizeof(usbd_buffer))
			{
				USBD_CtlError(pdev, req);
				return;
			}
			USBD_CtlPrepareRx(pdev, usbd_buffer, wLength);
			st = ST_DWL;
		}
	}
	else // upload
	{
		USBD_CtlSendData(pdev, usbd_buffer, wLength);
		st = ST_UPL;
	}
}

bool usbd_proto_is_idle(void) { return st == ST_IDLE; }

void usbd_proto_vendor_tx_sent(void *pdev)
{
	st = ST_IDLE;
}

void usbd_proto_vendor_rx_received(void *pdev)
{
}