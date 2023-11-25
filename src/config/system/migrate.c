/**
 * @file migrate.c
 * @brief System Config Migrations
 *
 * Config migrations happen on JSON data, which are called during any deserialization operation to
 * ensure an old configuration file is compatible with any changes to the config datastructure or
 * function.
 *
 * Please define each migration as `void migrate_to_X(cJSON* root)` where X is your new version.
 * Additionally, be sure to update the current version macro in the config header file alongside the
 * base struct.
 *
 * Please note: Migrations are NOT required if you are adding a new configuration option, or
 * removing an option. The only use case for a migration is if you are renaming an option and wish
 * to preserve the value, or if you are changing the function of an option and need to reprocess it
 * (such as changing units).
 *
 * Please note that the JSON root passed into this may be a partial structure, and so you should
 * only operate on keys that you check to be present.
 *
 */

#include "config_defs.h"

migration_result_t migrate_to_1(cJSON* root) {
    // noop
    return MIGRATION_COMPLETE;
}

migration_result_t config_system_migrate(cJSON* root) {
    START_MIGRATION();

    // List all migration calls below, in numeric order:
    MIGRATE(1);

    END_MIGRATION();
}