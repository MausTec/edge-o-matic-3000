#ifndef __system__action_manager_c
#define __system__action_manager_c

#ifdef __cplusplus
extern "C" {
#endif

#include "mt_actions.h"
#include <stddef.h>

void action_manager_init(void);
void action_manager_load_plugin(const char* filename);
void action_manager_load_all_plugins(void);
void action_manager_dispatch_event(const char* event, int val);

size_t action_manager_get_plugin_count(void);
mta_plugin_t* action_manager_get_plugin(size_t index);
mta_plugin_t* action_manager_find_plugin(const char* name);

/**
 * @brief Save plugin config to disk
 *
 * Serializes plugin config and writes to /plugincfg/<name>.json
 *
 * @param plugin Plugin instance
 * @return true if saved successfully, false otherwise
 */
bool action_manager_save_plugin_config(mta_plugin_t* plugin);

#ifdef __cplusplus
}
#endif

#endif
