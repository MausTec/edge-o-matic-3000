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

migration_result_t migrate_to_2(cJSON* root) {
    // noop (original migration removed during contributor audit)
    return MIGRATION_COMPLETE;
}

/**
 * Remove clench detector and post-orgasm torture config keys (features removed in v1.3.0).
 */
migration_result_t migrate_to_3(cJSON* root) {
    // Clench detector keys
    cJSON_DeleteItemFromObject(root, "clench_pressure_sensitivity");
    cJSON_DeleteItemFromObject(root, "max_clench_duration_ms");
    cJSON_DeleteItemFromObject(root, "clench_time_to_orgasm_ms");
    cJSON_DeleteItemFromObject(root, "clench_time_threshold_ms");
    cJSON_DeleteItemFromObject(root, "clench_detector_in_edging");
    cJSON_DeleteItemFromObject(root, "clench_threshold_2_orgasm");

    // Post-orgasm torture keys
    cJSON_DeleteItemFromObject(root, "use_post_orgasm");
    cJSON_DeleteItemFromObject(root, "auto_edging_duration_minutes");
    cJSON_DeleteItemFromObject(root, "post_orgasm_duration_seconds");
    cJSON_DeleteItemFromObject(root, "edge_menu_lock");
    cJSON_DeleteItemFromObject(root, "post_orgasm_menu_lock");

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