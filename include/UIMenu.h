#ifndef __UIMenu_h
#define __UIMenu_h

#include "config.h"
#include <functional>
#include <string>

#define TITLE_SIZE 20

/**
 * Time in ms to ignore scroll events.
 *
 * This prevents that absolutely infuriating thing my CR-10 does when I go to scroll one item and it
 * registers two clicks because I was *kinda sorta* between the next thing. However, this also makes
 * it super annoying to scroll through menus at light speed, so this has been reduced from 500ms to
 * 300ms. There is truly no winning, but having this debounce here seemed to make my life slightly
 * less full of fire and rage.
 *
 * If I was an actual dragon, I would have set the scroll wheel on fire. Do I need larger smoothing
 * capacitors? 100nF was too big, 10nF seemed just right, but I still have this absolutely annoying
 * bounce that sometimes happens when you're in-between detents on the scroll wheel. Is this an issue
 * with the PEL12T? The no-name (also a PEL12 variant) SparkFun encoder does this too. I think every
 * encoder in the universe does this (see CR-10).
 *
 * Maybe I just need to learn patience when scrolling.
 * That would explain why the scroll wheel on my mouse is wearing out.
 *
 * But look, that scroll wheel has seen the duality of heal staff and murder gun when I mained Mercy,
 * and that is a pretty tense environment when I'm furiously scrolling to grab my pistol and shoot
 * a Junkrat for--     what was I on about?
 *
 * Oh yeah, rotary encoders are of the devil.
 */
#define MENU_SCROLL_DEBOUNCE 50

class UIMenu;
class Page;

//typedef void(*MenuCallback)(UIMenu*);
typedef std::function<void(UIMenu*)> MenuCallback;
typedef std::function<void(UIMenu*, void*)> ParameterizedMenuCallback;
typedef std::function<void(UIMenu*, int)> IParameterizedMenuCallback;

enum AutocleanMethod {
  AUTOCLEAN_OFF,
  AUTOCLEAN_FREE,
  AUTOCLEAN_DELETE,
};

typedef struct UIMenuItem {
  char text[21];
  MenuCallback cb;
  ParameterizedMenuCallback pcb;
  IParameterizedMenuCallback ipcb;
  void *arg = nullptr;
  int iarg = NULL;
  UIMenuItem *next = nullptr;
  UIMenuItem *prev = nullptr;
} UIMenuItem;

class UIMenu {
public:
  // Construction
  UIMenu(const char *t, MenuCallback = nullptr);
  virtual ~UIMenu() {};
  void initialize(bool reinit = false);
  void rerender();

  // Item Manipulation
  void addItem(const char *text, ParameterizedMenuCallback pcb = nullptr, void *arg = nullptr);
  void addItem(std::string text, ParameterizedMenuCallback pcb = nullptr, void *arg = nullptr) { addItem(text.c_str(), pcb, arg); }
  void addItem(const char *text, MenuCallback cb = nullptr);
  void addItem(std::string text, MenuCallback cb = nullptr) { addItem(text.c_str(), cb); }
  void addItem(const char *text, Page *p);
  void addItem(UIMenu *submenu, void *arg = nullptr);
  void addItem(std::string text, UIMenu *submenu, void *arg = nullptr);
  void addItem(const char *text, IParameterizedMenuCallback pcb = nullptr, int arg = NULL);
  void addItem(std::string text, IParameterizedMenuCallback pcb = nullptr, int arg = NULL) { addItem(text.c_str(), pcb, arg); };
  void addItemAt(size_t index, const char *text, MenuCallback cb = nullptr);
  void removeItem(size_t index);
  void clearItems();
  int getIndexByArgument(void *arg);

  // Menu Manipulation
  void setTitle(const char *text);
  void setTitle(std::string text) { setTitle(text.c_str()); };

  // Render Lifecycle
  void open(UIMenu *previous = nullptr, bool save_history = true, void *arg = nullptr);
  void onOpen(MenuCallback cb = nullptr);
  void onClose(MenuCallback cb = nullptr);
  virtual void render();
  virtual void tick();
  virtual UIMenu *close();

  // Selection Handling
  virtual void selectNext();
  virtual void selectPrev();
  virtual void handleClick();
  virtual int getItemCount();
  virtual int getCurrentPosition();

  // Misc Queries
  UIMenuItem *firstItem() { return first_item; }
  bool isPreviousMenu(UIMenu *m) { return prev == m; }
  void *getCurrentArg() {
    log_i("Getting current arg: %x", this->current_arg);
    return this->current_arg;
  }

  void enableAutoCleanup(AutocleanMethod m) { autoclean = m; };

protected:
  char title[TITLE_SIZE + 1];

  UIMenu *prev = nullptr;
  void *current_arg = nullptr;

  UIMenuItem *getNthItem(int n);

private:
  UIMenuItem *first_item = nullptr;
  UIMenuItem *last_item = nullptr;
  UIMenuItem *current_item = nullptr;

  MenuCallback initializer = nullptr;
  MenuCallback on_open = nullptr;
  MenuCallback on_close = nullptr;
  long last_menu_change = 0;

  AutocleanMethod autoclean = AUTOCLEAN_OFF;
};

extern UIMenu MainMenu;
extern UIMenu GamesMenu;
extern UIMenu NetworkMenu;
extern UIMenu UISettingsMenu;
extern UIMenu EdgingSettingsMenu;
extern UIMenu EdgingOrgasmSettingsMenu;
extern UIMenu AccessoryPortMenu;
extern UIMenu UpdateMenu;
extern UIMenu BluetoothScanMenu;
extern UIMenu BluetoothDevicesMenu;
extern UIMenu WiFiScanMenu;

#endif