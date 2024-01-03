#ifndef __config_defs_h
#define __config_defs_h

#include "cJSON.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum _config_def_operation {
    // Get a config entry into the target JSON object or String (single key)
    CFG_GET,

    // Set a config entry from target JSON object (with defaults) or String (single key)
    CFG_SET,

    // Merge the target JSON object into the config, leaving missing entries alone
    CFG_MERGE
};

typedef enum migration_result {
    MIGRATION_COMPLETE,
    MIGRATION_NOT_RUN,
    MIGRATION_CURRENT,
    MIGRATION_ERR_TOO_NEW,
    MIGRATION_ERR_BAD_DATA
} migration_result_t;

#define START_MIGRATION()                                                                          \
    cJSON* root_version = cJSON_GetObjectItem(root, "$version");                                   \
    int current_version = root_version == NULL ? 0 : root_version->valueint;                       \
    if (current_version == SYSTEM_CONFIG_FILE_VERSION) return MIGRATION_NOT_RUN;                   \
    if (current_version > SYSTEM_CONFIG_FILE_VERSION) return MIGRATION_ERR_TOO_NEW;

#define MIGRATE(version)                                                                           \
    if (current_version < version) {                                                               \
        migration_result_t res = migrate_to_##version(root);                                       \
        if (res != MIGRATION_COMPLETE) return res;                                                 \
        current_version = version;                                                                 \
        cJSON_SetIntValue(root_version, version);                                                  \
    }

#define END_MIGRATION()                                                                            \
    if (current_version == SYSTEM_CONFIG_FILE_VERSION)                                             \
        return MIGRATION_COMPLETE;                                                                 \
    else                                                                                           \
        return MIGRATION_ERR_BAD_DATA;

#define CONFIG_DEFS                                                                                \
    bool _config_defs(                                                                             \
        enum _config_def_operation operation,                                                      \
        cJSON* root,                                                                               \
        config_t* cfg,                                                                             \
        const char* key,                                                                           \
        const char* in_val,                                                                        \
        char* out_val,                                                                             \
        size_t len,                                                                                \
        bool* restart_required                                                                     \
    )

#define CFG_STRING(name, default)                                                                  \
    {                                                                                              \
        if (root != NULL) {                                                                        \
            if (operation == CFG_SET || operation == CFG_MERGE) {                                  \
                cJSON* item = cJSON_GetObjectItem(root, #name);                                    \
                if (item != NULL || operation == CFG_SET)                                          \
                    strncpy(                                                                       \
                        cfg->name, item == NULL ? default : item->valuestring, sizeof(cfg->name)   \
                    );                                                                             \
            } else {                                                                               \
                cJSON_AddStringToObject(root, #name, cfg->name);                                   \
            }                                                                                      \
        } else if (key != NULL && !strcasecmp(key, #name)) {                                       \
            if (operation == CFG_GET) {                                                            \
                if (out_val != NULL) strlcpy(out_val, cfg->name, len);                             \
                return true;                                                                       \
            } else {                                                                               \
                if (in_val != NULL) strlcpy(cfg->name, in_val, sizeof(cfg->name));                 \
                return true;                                                                       \
            }                                                                                      \
        } else {                                                                                   \
            if (operation == CFG_SET) {                                                            \
                strlcpy(cfg->name, default, sizeof(cfg->name));                                    \
            }                                                                                      \
        }                                                                                          \
    }

#define CFG_STRING_PTR(name, default)                                                              \
    {                                                                                              \
        if (root != NULL) {                                                                        \
            if (operation == CFG_SET || operation == CFG_MERGE) {                                  \
                cJSON* item = cJSON_GetObjectItem(root, #name);                                    \
                if (item != NULL || operation == CFG_SET) {                                        \
                    /* intentional cstring pointer comparison */                                   \
                    if (cfg->name != default) free(cfg->name);                                     \
                    if (item == NULL)                                                              \
                        cfg->name = default;                                                       \
                    else                                                                           \
                        asiprintf(&cfg->name, "%s", item->valuestring);                            \
                }                                                                                  \
            } else {                                                                               \
                cJSON_AddStringToObject(root, #name, cfg->name);                                   \
            }                                                                                      \
        } else if (key != NULL && !strcasecmp(key, #name)) {                                       \
            if (operation == CFG_GET) {                                                            \
                if (out_val != NULL) strlcpy(out_val, cfg->name, len);                             \
                return true;                                                                       \
            } else {                                                                               \
                if (in_val != NULL) {                                                              \
                    /* intentional cstring pointer comparison */                                   \
                    if (cfg->name != default) free(cfg->name);                                     \
                    asiprintf(&cfg->name, "%s", in_val);                                           \
                }                                                                                  \
                return true;                                                                       \
            }                                                                                      \
        } else {                                                                                   \
            if (operation == CFG_SET) {                                                            \
                if (cfg->name != default) free(cfg->name);                                         \
                cfg->name = default;                                                               \
            }                                                                                      \
        }                                                                                          \
    }

#define CFG_NUMBER(name, default)                                                                  \
    {                                                                                              \
        if (root != NULL) {                                                                        \
            if (operation == CFG_SET || operation == CFG_MERGE) {                                  \
                cJSON* item = cJSON_GetObjectItem(root, #name);                                    \
                if (item != NULL || operation == CFG_SET) {                                        \
                    cfg->name = item == NULL ? default : item->valueint;                           \
                }                                                                                  \
            } else {                                                                               \
                cJSON_AddNumberToObject(root, #name, cfg->name);                                   \
            }                                                                                      \
        } else if (key != NULL && !strcasecmp(key, #name)) {                                       \
            if (operation == CFG_GET) {                                                            \
                if (out_val != NULL) snprintf(out_val, len, "%d", cfg->name);                      \
                return true;                                                                       \
            } else {                                                                               \
                if (in_val != NULL) cfg->name = atoi(in_val);                                      \
                return true;                                                                       \
            }                                                                                      \
        } else {                                                                                   \
            if (operation == CFG_SET) {                                                            \
                cfg->name = default;                                                               \
            }                                                                                      \
        }                                                                                          \
    }

#define CFG_ENUM(name, type, default)                                                              \
    {                                                                                              \
        if (root != NULL) {                                                                        \
            if (operation == CFG_SET || operation == CFG_MERGE) {                                  \
                cJSON* item = cJSON_GetObjectItem(root, #name);                                    \
                if (item != NULL || operation == CFG_SET)                                          \
                    cfg->name = item == NULL ? default : (type)item->valueint;                     \
            } else {                                                                               \
                cJSON_AddNumberToObject(root, #name, (int)cfg->name);                              \
            }                                                                                      \
        } else if (key != NULL && !strcasecmp(key, #name)) {                                       \
            if (operation == CFG_GET) {                                                            \
                if (out_val != NULL) snprintf(out_val, len, "%d", (int)cfg->name);                 \
                return true;                                                                       \
            } else {                                                                               \
                if (in_val != NULL) cfg->name = (type)atoi(in_val);                                \
                return true;                                                                       \
            }                                                                                      \
        } else {                                                                                   \
            if (operation == CFG_SET) {                                                            \
                cfg->name = default;                                                               \
            }                                                                                      \
        }                                                                                          \
    }

#define CFG_BOOL(name, default)                                                                    \
    {                                                                                              \
        if (root != NULL) {                                                                        \
            if (operation == CFG_SET || operation == CFG_MERGE) {                                  \
                cJSON* item = cJSON_GetObjectItem(root, #name);                                    \
                if (item != NULL || operation == CFG_SET)                                          \
                    cfg->name = item == NULL ? default : item->valueint;                           \
            } else {                                                                               \
                cJSON_AddBoolToObject(root, #name, cfg->name);                                     \
            }                                                                                      \
        } else if (key != NULL && !strcasecmp(key, #name)) {                                       \
            if (operation == CFG_GET) {                                                            \
                if (out_val != NULL) strlcat(out_val, cfg->name ? "true" : "false", len);          \
                return true;                                                                       \
            } else {                                                                               \
                if (in_val != NULL) cfg->name = atob(in_val);                                      \
                return true;                                                                       \
            }                                                                                      \
        } else {                                                                                   \
            if (operation == CFG_SET) {                                                            \
                cfg->name = default;                                                               \
            }                                                                                      \
        }                                                                                          \
    }

bool atob(const char* a);
esp_err_t config_init(void);
void config_serialize(config_t* cfg, char* buf, size_t buflen);
esp_err_t config_deserialize(config_t* cfg, const char* buf);
esp_err_t config_load_from_sd(const char* filename, config_t* cfg);
esp_err_t config_save_to_sd(const char* filename, config_t* cfg);
void config_load_default(config_t* cfg);
void config_to_json(cJSON* root, config_t* cfg);
void json_to_config(cJSON* root, config_t* cfg);
void json_to_config_merge(cJSON* root, config_t* cfg);
migration_result_t config_system_migrate(cJSON* root);

#ifdef __cplusplus
}
#endif

#endif
