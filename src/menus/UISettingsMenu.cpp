#include "UIMenu.h"
#include "UIInput.h"
#include "config.h"

UIInput ScreenDim("Screen Dim (s)", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(360);
  input->setStep(10);
  input->setValue(Config.screen_dim_seconds);
  input->onConfirm([](int value) {
    Config.screen_dim_seconds = value;
    saveConfigToSd(0);
  });
});

UIInput ScreenTimeout("Screen Timeout", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(360);
  input->setStep(10);
  input->setValue(Config.screen_timeout_seconds);
  input->onConfirm([](int value) {
    Config.screen_timeout_seconds = value;
    saveConfigToSd(0);
  });
});

static void buildMenu(UIMenu *menu) {
  menu->addItem(&ScreenDim);
  menu->addItem(&ScreenTimeout);
}

UIMenu UISettingsMenu("UI Settings", &buildMenu);