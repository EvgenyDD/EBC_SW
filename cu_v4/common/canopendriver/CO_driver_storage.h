#ifndef CO_DRIVER_STORAGE_H_
#define CO_DRIVER_STORAGE_H_

#include "storage/CO_storage.h"

void CO_driver_storage_error_report(CO_EM_t *em);

void CO_driver_storage_init(OD_entry_t *OD_1010_StoreParameters,
							OD_entry_t *OD_1011_RestoreDefaultParameters);

#endif // CO_DRIVER_STORAGE_H_
