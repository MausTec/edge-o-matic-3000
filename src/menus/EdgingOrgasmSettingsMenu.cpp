#include "UIMenu.h"
#include "UIInput.h"
#include "Hardware.h"
#include "OrgasmControl.h"

UIInput ClenchPressureSensitivity("Clench Pressure Sensitivity", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(300);
  input->setStep(1);
  input->setValue(Config.clench_pressure_sensitivity);
  input->onChange([](int value) {
    Config.clench_pressure_sensitivity = value;
  });
  input->onConfirm([](int) {
    saveConfigToSd(0);
  });
});

UIInput ClenchTimeThreshold("Clench Time Threshold", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(300);
  input->setStep(1);
  input->setValue(Config.clench_duration_threshold);
  input->onChange([](int value) {
    Config.clench_duration_threshold = value;
  });
  input->onConfirm([](int) {
    saveConfigToSd(0);
  });
});

UIInput EdgingDuration("Edge Duration Minutes", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(300);
  input->setStep(1);
  input->setValue(Config.auto_edging_duration_minutes);
  input->onChange([](int value) {
    Config.auto_edging_duration_minutes = value;
  });
  input->onConfirm([](int) {
    saveConfigToSd(0);
  });
});

UIInput PostOrgasmDuration("Post Orgasm Seconds", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(300);
  input->setStep(1);
  input->setValue(Config.post_orgasm_duration_seconds);
  input->onChange([](int value) {
    Config.post_orgasm_duration_seconds = value;
  });
  input->onConfirm([](int) {
    saveConfigToSd(0);
  });
});

static void onEdgeMenuUnLock(UIMenu* menu) {
  UI.toastNow("Edge UnLock", 3000);
  Config.edge_menu_lock = false;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void onEdgeMenuLock(UIMenu* menu) {
  UI.toastNow("Edge Lock", 3000);
  Config.edge_menu_lock = true;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void onPostOrgasmMenuUnLock(UIMenu* menu) {
  UI.toastNow("Post orgasm UnLock", 3000);
  Config.post_orgasm_menu_lock = false;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void onPostOrgasmMenuLock(UIMenu* menu) {
  UI.toastNow("Post orgasm Lock", 3000);
  Config.post_orgasm_menu_lock = true;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void onClenchDetectorOn(UIMenu* menu) {
  UI.toastNow("Edging clench : ON", 3000);
  Config.clench_detector_in_edging = true;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void onClenchDetectorOff(UIMenu* menu) {
  UI.toastNow("Edging clench : OFF", 3000);
  Config.clench_detector_in_edging = false;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void buildMenu(UIMenu *menu) {
  menu->addItem(&EdgingDuration);            //   Auto Edging
  menu->addItem(&PostOrgasmDuration);        //   Post Orgasm
  if (Config.edge_menu_lock) {
    menu->addItem("Edge+Orgasm UnLock", &onEdgeMenuUnLock);
  } else {
    menu->addItem("Edge+Orgasm Lock", &onEdgeMenuLock);
  }
  if (Config.post_orgasm_menu_lock) {
    menu->addItem("Post Orgasm UnLock", &onPostOrgasmMenuUnLock);
  } else {
    menu->addItem("Post Orgasm Lock", &onPostOrgasmMenuLock);
  }
  if (Config.clench_detector_in_edging) {
    menu->addItem("Edge Clench:Turn Off", &onClenchDetectorOff);
  } else {
    menu->addItem("Edge Clench:Turn On", &onClenchDetectorOn);
  }
  menu->addItem(&ClenchPressureSensitivity); //  Clench Menu
  menu->addItem(&ClenchTimeThreshold);       //  Clench Menu
}

UIMenu EdgingOrgasmSettingsMenu("Edging Orgasm Settings", &buildMenu);
