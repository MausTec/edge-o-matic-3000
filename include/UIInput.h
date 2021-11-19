#include "UIMenu.h"

class UIInput;

typedef std::function<void(int)> InputCallback;
typedef std::function<int(int)> GetValueCallback;
typedef std::function<void(const char*)> TextInputCallback;

class UIInput : public UIMenu {
public:
    UIInput(char* t, MenuCallback cb = nullptr) : UIMenu(t, cb) {};

    void render() override;
    void setMax(int);
    void setValue(int);
    void setStep(int);
    void setPollPeriod(int);
    void onChange(InputCallback);
    void onConfirm(InputCallback);
    void getSecondaryValue(GetValueCallback);

    // Navigation (which now changes value)
    void selectNext() override;
    void selectPrev() override;
    void handleClick() override;
    int getItemCount() override;
    int getCurrentPosition() override;

    UIMenu* close() override;
    void tick() override;

private:
    int current_value = 0;
    int min_value = 0;
    int max_value = 255;
    int default_value = 0;
    int increment = 1;
    int poll_period = 0;

    InputCallback on_change = nullptr;
    InputCallback on_confirm = nullptr;
    GetValueCallback get_secondary_value = nullptr;

    void set_current(int);
};