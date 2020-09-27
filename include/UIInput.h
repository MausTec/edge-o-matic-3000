#include "UIMenu.h"

class UIInput;

typedef std::function<void(int)> InputCallback;

class UIInput : public UIMenu {
public:
  UIInput(char *t, MenuCallback cb = nullptr) : UIMenu(t, cb) {};

  void render() override;
  void setMax(int);
  void setValue(int);
  void setStep(int);
  void onChange(InputCallback);
  void onConfirm(InputCallback);

  // Navigation (which now changes value)
  void selectNext() override;
  void selectPrev() override;
  void handleClick() override;
  int getItemCount() override;
  int getCurrentPosition() override;

  UIMenu *close() override;

private:
  int current_value = 0;
  int min_value = 0;
  int max_value = 255;
  int default_value = 0;
  int increment = 1;

  InputCallback on_change = nullptr;
  InputCallback on_confirm = nullptr;

  void set_current(int);
};