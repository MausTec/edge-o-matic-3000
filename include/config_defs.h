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
        }                                                                                          \
    }

bool atob(const char* a);
esp_err_t config_init(void);
void config_serialize(config_t* cfg, char* buf, size_t buflen);
void config_deserialize(config_t* cfg, const char* buf);
esp_err_t config_load_from_sd(const char* filename, config_t* cfg);
esp_err_t config_save_to_sd(const char* filename, config_t* cfg);
void config_load_default(config_t* cfg);
void config_to_json(cJSON* root, config_t* cfg);
void json_to_config(cJSON* root, config_t* cfg);
void json_to_config_merge(cJSON* root, config_t* cfg);

#ifdef __cplusplus
}
#endif

#endif
