#include "301/CO_Emergency.h"
#include "config_system.h"
#include "storage/CO_storage.h"

#include "CO_driver_storage.h"

#if(CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE

#define PARAM_STORE_PASSWORD 0x73617665	  // s a v e
#define PARAM_RESTORE_PASSWORD 0x6C6F6164 // l o a d

int cfg_init_err_code = 0xFFF;

static int cfg_read(void)
{
	int err = config_validate();
	if(err == CONFIG_STS_OK) config_read_storage();
	return err;
}

static ODR_t drv_store(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten)
{
	if(stream == NULL || stream->subIndex == 0 || buf == NULL || count != 4 || countWritten == NULL) return ODR_DEV_INCOMPAT;
	if(stream->subIndex == 0) return ODR_READONLY;

	if(CO_getUint32(buf) != PARAM_STORE_PASSWORD) return ODR_NO_DATA;

	CO_LOCK_OD(CANmodule);
	int err = config_write_storage();
	if(err == 0) err = config_validate();
	CO_UNLOCK_OD(CANmodule);

	return err ? ODR_GENERAL : ODR_OK;
}

static ODR_t drv_restore(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten)
{
	if(stream == NULL || stream->subIndex == 0 || buf == NULL || count != 4 || countWritten == NULL) return ODR_DEV_INCOMPAT;
	if(stream->subIndex == 0) return ODR_READONLY;

	if(CO_getUint32(buf) != PARAM_RESTORE_PASSWORD) return ODR_NO_DATA;

	cfg_init_err_code = cfg_read();
	return cfg_init_err_code ? ODR_GENERAL : ODR_OK;
}

static OD_extension_t OD_1010_extension = {.object = NULL, .read = OD_readOriginal, .write = drv_store};
static OD_extension_t OD_1011_extension = {.object = NULL, .read = OD_readOriginal, .write = drv_restore};

void CO_driver_storage_error_report(CO_EM_t *em)
{
	if(cfg_init_err_code) CO_errorReport(em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE, (uint32_t)cfg_init_err_code);
}

void CO_driver_storage_init(OD_entry_t *OD_1010_StoreParameters,
							OD_entry_t *OD_1011_RestoreDefaultParameters)
{
	cfg_init_err_code = cfg_read();

	OD_extension_init(OD_1010_StoreParameters, &OD_1010_extension);
	OD_extension_init(OD_1011_RestoreDefaultParameters, &OD_1011_extension);
}

#endif /* (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE */
