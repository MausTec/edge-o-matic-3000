#include "UIMenu.h"
#include "UIInput.h"
#include "Hardware.h"
#include "orgasm_control.h"

#include "esp_log.h"

static const char *TAG = "EdgingSettingsMenu";

UIInput MotorMaxSpeedInput("Motor Max Speed", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(255);
  input->setValue(Config.motor_max_speed);
  input->setStep(1);

  input->onOpen([](UIMenu*) {
    orgasm_control_pauseControl();
  });

  input->onClose([](UIMenu*) {
    orgasm_control_resumeControl();
  });

  input->onChange([](int value) {
    Config.motor_max_speed = value;
    Hardware::setMotorSpeed(value);
  });

  input->onConfirm([](int value) {
    Config.motor_max_speed = value;
    config_enqueue_save(0);
  });
});

UIInput MotorStartSpeedInput("Motor Start Speed", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(255);
  input->setStep(1);
  input->setValue(Config.motor_start_speed);
  input->onChange([](int value) {
    Config.motor_start_speed = value;
  });
  input->onConfirm([](int) {
    config_enqueue_save(0);
  });
});

UIInput MotorRampTimeInput("Motor Ramp Time", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(180);
  input->setStep(5);
  input->setValue(Config.motor_ramp_time_s);
  input->onChange([](int value) {
    Config.motor_ramp_time_s = value;
  });
  input->onConfirm([](int) {
    config_enqueue_save(0);
  });
});

UIInput EdgeDelayInput("Edge Delay", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(60);
  input->setStep(1);
  input->setValue(Config.edge_delay / 1000);
  input->onChange([](int value) {
    Config.edge_delay = value * 1000;
  });
  input->onConfirm([](int) {
    config_enqueue_save(0);
  });
});

UIInput MaxAdditionalDelayInput("Maximum Additional Delay", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(60);
  input->setStep(1);
  input->setValue(Config.max_additional_delay / 1000);
  input->onChange([](int value) {
    Config.max_additional_delay = value * 1000;
  });
  input->onConfirm([](int) {
    config_enqueue_save(0);
  });
});

UIInput MinimumOnTimeInput("Minimum On Time", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(5000);
  input->setStep(50);
  input->setValue(Config.minimum_on_time);
  input->onChange([](int value) {
    Config.minimum_on_time = value;
  });
  input->onConfirm([](int) {
    config_enqueue_save(0);
  });
});

UIInput ArousalLimitInput("Arousal Limit", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(1023);
  input->setStep(8);
  input->setValue(Config.sensitivity_threshold);
  input->onChange([](int value) {
    Config.sensitivity_threshold = value;
  });
  input->onConfirm([](int) {
    config_enqueue_save(0);
  });
});

UIInput SensorSensitivityInput("Sensor Sensitivity", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(255);
  input->setStep(1);
  input->setValue(Config.sensor_sensitivity);
  input->setPollPeriod(1000 / 30);
  input->getSecondaryValue([](int value) {
    return Hardware::getPressure();
  });
  input->onChange([](int value) {
    Config.sensor_sensitivity = value;
    Hardware::setPressureSensitivity(value);
  });
  input->onConfirm([](int) {
    config_enqueue_save(0);
  });
});

static void setVibrateMode(UIMenu *menu, int m) {
  vibration_mode_t mode = (vibration_mode_t) m;

  ESP_LOGI(TAG, "Setting mode to: ");
  switch(mode) {
    case Depletion:
      ESP_LOGI(TAG, "Depletion");
      break;
    case Enhancement:
      ESP_LOGI(TAG, "Enhancement");
      break;
    case RampStop:
      ESP_LOGI(TAG, "RampStop");
      break;
    default:
      break;
  }

  Config.vibration_mode = mode;
  config_enqueue_save(0);
  menu->rerender();
}

static void add_vibration_item(UIMenu *menu, std::string label, vibration_mode_t mode) {
  std::string text = "";
  if (Config.vibration_mode == mode) {
    text += "X";
  } else {
    text += " ";
  }
  text += " " + label;

  menu->addItem(text, &setVibrateMode, (int) mode);
}

static void buildvibration_mode_tMenu(UIMenu *menu) {
  add_vibration_item(menu, "Depletion", Depletion);
  add_vibration_item(menu, "Enhancement", Enhancement);
  add_vibration_item(menu, "Ramp-Stop", RampStop);
}

UIMenu vibration_mode_tMenu("Vibration Mode", &buildvibration_mode_tMenu);

static void buildMenu(UIMenu *menu) {
  menu->addItem(&vibration_mode_tMenu);
  menu->addItem(&MotorMaxSpeedInput);
  menu->addItem(&MotorStartSpeedInput);
  menu->addItem(&MotorRampTimeInput);
  menu->addItem(&EdgeDelayInput);
  menu->addItem(&MaxAdditionalDelayInput);
  menu->addItem(&MinimumOnTimeInput);
  menu->addItem(&ArousalLimitInput);
  menu->addItem(&SensorSensitivityInput);
}

UIMenu EdgingSettingsMenu("Edging Settings", &buildMenu);
