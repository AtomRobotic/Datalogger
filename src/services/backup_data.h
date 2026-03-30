#ifndef BACKUP_DATA_MANAGER_H
#define BACKUP_DATA_MANAGER_H

#include <common.h>
#include <app_assert.h>
#include <app_log.h>
#include "sd_card.h"

void init_backup_manager(void);
void backup_manager_handle_data(const char *data);

#endif