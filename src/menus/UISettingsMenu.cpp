#include "UIInput.h"
#include "UIMenu.h"
#include "config.h"
#include "eom-hal.h"

UIInput ScreenDim("Screen Dim (s)", [](UIMenu* ip) {
    UIInput* input = (UIInput*)ip;
    input->setMax(360);
    input->setStep(10);
    input->setValue(Config.screen_dim_seconds);
    input->onConfirm([](int value) {
        Config.screen_dim_seconds = value;
        config_enqueue_save(0);
    });
});

UIInput ScreenTimeout("Screen Timeout", [](UIMenu* ip) {
    UIInput* input = (UIInput*)ip;
    input->setMax(360);
    input->setStep(10);
    input->setValue(Config.screen_timeout_seconds);
    input->onConfirm([](int value) {
        Config.screen_timeout_seconds = value;
        config_enqueue_save(0);
    });
});

UIInput LEDBrightness("LED Brightness", [](UIMenu* ip) {
    UIInput* input = (UIInput*)ip;
    input->setMax(255);
    input->setValue(Config.led_brightness);
    input->onConfirm([](int value) {
        Config.led_brightness = value;
        config_enqueue_save(0);
    });
    input->onChange([](int value) { eom_hal_set_encoder_brightness(value); });
});

static void buildMenu(UIMenu* menu) {
    menu->addItem(&ScreenDim);
    menu->addItem(&ScreenTimeout);
    menu->addItem(&LEDBrightness);
}

UIMenu UISettingsMenu("UI Settings", &buildMenu);