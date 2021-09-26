#include "eom_tscode_handler.h"
#include "Hardware.h"
#include "Page.h"
#include "UserInterface.h"

#include "tscode_capabilities.h"

void eom_tscode_install(void) {
    tscode_device_vendor_details_t dev_info = {
        .vendor = "Maus-Tec Electronics",
        .device = "Edge-o-Matic 3000",
        .version = VERSION,
    };

    tscode_register_vendor_details(&dev_info);
}

tscode_command_response_t eom_tscode_handler(tscode_command_t* cmd, char* response, size_t resp_len) {
    switch (cmd->type) {
        case TSCODE_VIBRATE_ON: {
            tscode_unit_t *speed = cmd->speed;
            if (speed != NULL) {
                uint8_t value = speed->value;
                if (speed->unit == TSCODE_UNIT_PERCENTAGE) {
                    value = speed->value * 255;
                }

                RunGraphPage.setMode("manual");
                Hardware::setMotorSpeed(value);
            }
            break;
        }


        case TSCODE_CONDITIONAL_STOP:
        case TSCODE_HALT_IMMEDIATE:
        case TSCODE_VIBRATE_OFF: {
            RunGraphPage.setMode("manual");
            Hardware::setMotorSpeed(0);
            break;
        }

        case TSCODE_DISPLAY_MESSAGE: {
            if (cmd->str != NULL) {
                UI.toastNow(cmd->str);
            }
            break;
        }

        default:
            return TSCODE_RESPONSE_FAULT;
    }

    return TSCODE_RESPONSE_OK;
}