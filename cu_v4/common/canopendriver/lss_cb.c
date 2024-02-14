
#include "lss_cb.h"
#include "can_driver.h"
#include "config_system.h"

// LSS Callbacks

extern bool g_stay_in_boot;

static void set_br(void *obj /* LSS_cb_obj_t* */)
{
	can_drv_enter_init_mode(((LSS_cb_obj_t *)obj)->co->CANmodule->CANptr);
	can_drv_check_set_bitrate(((LSS_cb_obj_t *)obj)->co->CANmodule->CANptr, (int32_t)(*((LSS_cb_obj_t *)obj)->co->LSSslave->pendingBitRate) * 1000, true);
	can_drv_leave_init_mode(((LSS_cb_obj_t *)obj)->co->CANmodule->CANptr);
}

static bool_t lss_check_br(void *obj /* LSS_cb_obj_t* */, uint16_t bitRate)
{
	g_stay_in_boot = true;
	if(bitRate == 0) return false;
	return can_drv_check_set_bitrate(((LSS_cb_obj_t *)obj)->co->CANmodule->CANptr, (int32_t)(bitRate * 1000U), false) == (int32_t)(bitRate * 1000U);
}

static void lss_activate_br(void *obj /* LSS_cb_obj_t* */, uint16_t switch_delay_ms)
{
	g_stay_in_boot = true;
	if(switch_delay_ms)
	{
		set_br(obj);
	}
	else
	{
		((LSS_cb_obj_t *)obj)->lss_br_set_delay_counter = (uint32_t)switch_delay_ms * 1000U;
	}
}

static bool_t lss_store_cfg(void *obj /* LSS_cb_obj_t* */, uint8_t id, uint16_t bitRate)
{
	// Note: id & bitRate are already in config table "g_device_config"
	g_stay_in_boot = true;

	CO_LOCK_OD(CANmodule);
	int err = config_write_storage();
	CO_UNLOCK_OD(CANmodule);

	return err == 0;
}

void lss_cb_init(LSS_cb_obj_t *obj)
{
	CO_LSSslave_initCheckBitRateCallback(obj->co->LSSslave, obj, lss_check_br);
	CO_LSSslave_initActivateBitRateCallback(obj->co->LSSslave, obj, lss_activate_br);
	CO_LSSslave_initCfgStoreCallback(obj->co->LSSslave, obj, lss_store_cfg);
}

void lss_cb_poll(LSS_cb_obj_t *obj, uint32_t diff_us) // delayed baudrate set
{
	if(obj->lss_br_set_delay_counter != 0)
	{
		if(obj->lss_br_set_delay_counter <= diff_us)
		{
			obj->lss_br_set_delay_counter = 0;
			set_br(obj);
		}
		else
		{
			obj->lss_br_set_delay_counter -= diff_us;
		}
	}
}