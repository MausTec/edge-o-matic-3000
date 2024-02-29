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

/**
 * Rename "clench_threshold_2_orgasm" to "clench_time_to_orgasm_ms" and change value unit
 * from ticks to ms.
 */
migration_result_t migrate_to_2(cJSON* root) {
    cJSON* old = cJSON_GetObjectItem(root, "clench_threshold_2_orgasm");
    if (old == NULL) return MIGRATION_COMPLETE;

    // my tests to validate this has shown a 30 to 1 ratio
    int value_ms = old->valueint * 30; // i think this was the tick rate?
    cJSON* new = cJSON_AddNumberToObject(root, "clench_time_to_orgasm_ms", value_ms);

    if (new == NULL) return MIGRATION_ERR_BAD_DATA;
    cJSON_DeleteItemFromObject(root, "clench_threshold_2_orgasm");

    return MIGRATION_COMPLETE;
}

/**
 * Rename "use_post_orgasm" to "use_orgasm_modes"
 */
migration_result_t migrate_to_3(cJSON* root) {
    cJSON* old  = cJSON_GetObjectItemCaseSensitive(root, "use_post_orgasm");
    if (old == NULL) return MIGRATION_COMPLETE;

    // migrate old value
    if (cJSON_IsTrue(old) == 0) {
        cJSON* new = cJSON_AddBoolToObject(root,"use_orgasm_modes", true);
        if (new == NULL) return MIGRATION_ERR_BAD_DATA;
    } else {
        cJSON* new = cJSON_AddBoolToObject(root,"use_orgasm_modes", false);
        if (new == NULL) return MIGRATION_ERR_BAD_DATA;
    }

    cJSON_DeleteItemFromObject(root, "use_post_orgasm");
    cJSON_Delete(root);

    return MIGRATION_COMPLETE;
}

migration_result_t config_system_migrate(cJSON* root) {
    START_MIGRATION();

    // List all migration calls below, in numeric order:
    MIGRATE(1);
    MIGRATE(2);
    MIGRATE(3);

    END_MIGRATION();
}