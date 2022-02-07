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

UIInput ClenchOrgasmTimeThreshold("Clench time to Orgasm", [](UIMenu *ip) {
  UIInput *input = (UIInput*) ip;
  input->setMax(Config.max_clench_duration);
  input->setStep(1);
  input->setValue(Config.clench_threshold_2_orgasm);
  input->onChange([](int value) {
    Config.clench_threshold_2_orgasm = value;
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
  UI.toastNow("Edge Menu UnLock", 3000);
  Config.edge_menu_lock = false;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void onEdgeMenuLock(UIMenu* menu) {
  UI.toastNow("Edge Menu Lock", 3000);
  Config.edge_menu_lock = true;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void onPostOrgasmMenuUnLock(UIMenu* menu) {
  UI.toastNow(" Post orgasm\n Menu UnLock", 3000);
  Config.post_orgasm_menu_lock = false;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void onPostOrgasmMenuLock(UIMenu* menu) {
  UI.toastNow(" Post orgasm\n Menu Lock", 3000);
  Config.post_orgasm_menu_lock = true;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void onClenchDetectorOn(UIMenu* menu) {
  UI.toastNow(" Using Clench\n in Edging", 3000);
  Config.clench_detector_in_edging = true;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void onClenchDetectorOff(UIMenu* menu) {
  UI.toastNow(" Not Using Clench\n in Edging", 3000);
  Config.clench_detector_in_edging = false;
  saveConfigToSd(0);
  menu->initialize();
  menu->render();
}

static void buildMenu(UIMenu *menu) {
  menu->addItem(&EdgingDuration);            //   Auto Edging
  menu->addItem(&ClenchOrgasmTimeThreshold);       //  Clench Menu
  menu->addItem(&PostOrgasmDuration);        //   Post Orgasm
  if (Config.edge_menu_lock) {
    menu->addItem("MenuLock Edge Disable", &onEdgeMenuUnLock);
  } else {
    menu->addItem("MenuLock Edge Enable", &onEdgeMenuLock);
  }
  if (Config.post_orgasm_menu_lock) {
    menu->addItem("MenuLock Post Disable", &onPostOrgasmMenuUnLock);
  } else {
    menu->addItem("MenuLock Post Enable", &onPostOrgasmMenuLock);
  }
  if (Config.clench_detector_in_edging) {
    menu->addItem("Stop Clench in Edging", &onClenchDetectorOff);
  } else {
    menu->addItem("Use Clench in Edging", &onClenchDetectorOn);
  }
  menu->addItem(&ClenchPressureSensitivity); //  Clench Menu
}

UIMenu EdgingOrgasmSettingsMenu("Edging Orgasm Settings", &buildMenu);
