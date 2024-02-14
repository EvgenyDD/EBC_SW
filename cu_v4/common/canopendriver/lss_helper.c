#include "lss_helper.h"
#include "305/CO_LSSmaster.h"
#include "CANopen.h"
#include "platform.h"

#define LSS_POLLING_FUNC(f) \
	do                      \
	{                       \
		f;                  \
		interval = 1000;    \
		delay_ms_w_can_poll(1); \
	} while(ret == CO_LSSmaster_WAIT_SLAVE)

extern CO_t *CO;

int lss_helper_inquire(uint32_t field, uint32_t *value)
{
	int interval = 0;
	CO_LSSmaster_return_t ret;
	LSS_POLLING_FUNC({ ret = CO_LSSmaster_Inquire(CO->LSSmaster, interval, field, value); });
	return ret;
}

int lss_helper_inquire_lss_addr(CO_LSS_address_t *addr)
{
	int interval = 0;
	CO_LSSmaster_return_t ret;
	LSS_POLLING_FUNC({ ret = CO_LSSmaster_InquireLssAddress(CO->LSSmaster, interval, addr); });
	return ret;
}

int lss_helper_cfg_node_id(uint8_t node_id)
{
	int interval = 0;
	CO_LSSmaster_return_t ret;
	LSS_POLLING_FUNC({ ret = CO_LSSmaster_configureNodeId(CO->LSSmaster, interval, node_id); });
	return ret;
}

int lss_helper_cfg_bit_timing(uint16_t kbit_s)
{
	int interval = 0;
	CO_LSSmaster_return_t ret;
	LSS_POLLING_FUNC({ ret = CO_LSSmaster_configureBitTiming(CO->LSSmaster, interval, kbit_s); });
	return ret;
}

int lss_helper_activate_bit_timing(uint16_t switch_delay_ms)
{
	return CO_LSSmaster_ActivateBit(CO->LSSmaster, switch_delay_ms);
}

int lss_helper_cfg_store(void)
{
	int interval = 0;
	CO_LSSmaster_return_t ret;
	LSS_POLLING_FUNC({ ret = CO_LSSmaster_configureStore(CO->LSSmaster, interval); });
	return ret;
}

int lss_helper_select(CO_LSS_address_t *addr)
{
	int interval = 0;
	CO_LSSmaster_return_t ret;
	LSS_POLLING_FUNC({ ret = CO_LSSmaster_switchStateSelect(CO->LSSmaster, interval, addr); });
	return ret;
}

void lss_helper_deselect(void)
{
	CO_LSSmaster_switchStateDeselect(CO->LSSmaster);
}

int lss_helper_fastscan_select(uint16_t timeout, uint32_t *vendor_id, uint32_t *product, uint32_t *rev, uint32_t *serial)
{
	CO_LSSmaster_fastscan_t fastscan;
	fastscan.scan[CO_LSS_FASTSCAN_VENDOR_ID] = vendor_id ? CO_LSSmaster_FS_MATCH : CO_LSSmaster_FS_SCAN;
	fastscan.scan[CO_LSS_FASTSCAN_PRODUCT] = product ? CO_LSSmaster_FS_MATCH : CO_LSSmaster_FS_SCAN;
	fastscan.scan[CO_LSS_FASTSCAN_REV] = rev ? CO_LSSmaster_FS_MATCH : CO_LSSmaster_FS_SCAN;
	fastscan.scan[CO_LSS_FASTSCAN_SERIAL] = serial ? CO_LSSmaster_FS_MATCH : CO_LSSmaster_FS_SCAN;

	if(vendor_id) fastscan.match.identity.vendorID = *vendor_id;
	if(product) fastscan.match.identity.productCode = *product;
	if(rev) fastscan.match.identity.revisionNumber = *rev;
	if(serial) fastscan.match.identity.serialNumber = *serial;

	CO_LSSmaster_changeTimeout(CO->LSSmaster, timeout); /** TODO: check on lower baud rates */

	int interval = 0;
	CO_LSSmaster_return_t ret;
	LSS_POLLING_FUNC({ ret = CO_LSSmaster_IdentifyFastscan(CO->LSSmaster, interval, &fastscan); });

	CO_LSSmaster_changeTimeout(CO->LSSmaster, CO_LSSmaster_DEFAULT_TIMEOUT);
	return ret;
}
