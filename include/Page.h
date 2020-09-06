#ifndef __PAGE_h
#define __PAGE_h

#include "Arduino.h"
#include "UserInterface.h"

#define HISTORY_LENGTH 5

class Page {
public:
  // Navigation
  static void Go(Page* page, bool saveHistory = true);
  static void GoBack();
  static void DoLoop();
  static void Rerender();
  static void AttachButtonHandlers();
  static Page* CurrentPage();

  // Page Lifecycle
  virtual void Render();
  virtual void Loop();
  virtual void Enter();
  virtual void Exit();

  // Event Hooks
  virtual void onBtnPress(byte i);
  virtual void onEncoderChange(byte value);

private:
  static Page* currentPage;
  static Page* previousPage;
  static Page* previousPages[HISTORY_LENGTH];
  static size_t historyIndex;

  static void pushHistory(Page* page);
  static Page* popHistory();
};

#include "../src/pages/pDebug.h"
extern pDebug DebugPage;

#endif