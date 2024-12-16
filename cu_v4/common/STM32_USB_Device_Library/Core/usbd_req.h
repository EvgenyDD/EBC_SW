/* @file    usbd_req.h
 * @author  MCD Application Team
 * @version V1.2.1
 * @date    17-March-2018
 * @brief   header file for the usbd_req.c file
 */
#ifndef __USB_REQUEST_H_
#define __USB_REQUEST_H_

#include "usbd_conf.h"
#include "usbd_core.h"
#include "usbd_def.h"

USBD_Status USBD_StdDevReq(USB_OTG_CORE_HANDLE *pdev, USB_SETUP_REQ *req);
USBD_Status USBD_StdItfReq(USB_OTG_CORE_HANDLE *pdev, USB_SETUP_REQ *req);
USBD_Status USBD_StdEPReq(USB_OTG_CORE_HANDLE *pdev, USB_SETUP_REQ *req);
USBD_Status USBD_VendDevReq(USB_OTG_CORE_HANDLE *pdev, USB_SETUP_REQ *req);
void USBD_ParseSetupRequest(USB_OTG_CORE_HANDLE *pdev, USB_SETUP_REQ *req);
void USBD_CtlError(USB_OTG_CORE_HANDLE *pdev, USB_SETUP_REQ *req);

void USBD_GetString(const uint8_t *desc, uint8_t *unicode, uint16_t *len);

#endif // __USB_REQUEST_H_
