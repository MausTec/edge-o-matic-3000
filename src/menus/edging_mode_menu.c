#include "config.h"
#include "orgasm_control.h"
#include "ui/menu.h"
#include "ui/page.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"

extern void edging_stats_page_reset_denial_count(void);

static void
on_start_recording(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    orgasm_control_start_recording();
    ui_reenter_menu();
}

static void
on_stop_recording(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    orgasm_control_stop_recording();
    ui_reenter_menu();
}

static void
on_reset_denial_count(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    edging_stats_page_reset_denial_count();
    ui_toast("%s", _("Denial count reset."));
}

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_page(m, &EDGING_CHART_PAGE);

    if (orgasm_control_is_recording()) {
        ui_menu_add_item(m, _("Stop Recording"), on_stop_recording, NULL);
    } else {
        ui_menu_add_item(m, _("Start Recording"), on_start_recording, NULL);
    }

    ui_menu_add_item(m, _("Reset Denial Count"), on_reset_denial_count, NULL);

#ifdef EOM_BETA
    ui_menu_add_item(m, _("Set Time Limit"), NULL, NULL);
    ui_menu_add_item(m, _("Set Edge Goal"), NULL, NULL);
#endif
}

DYNAMIC_MENU(EDGING_MODE_MENU, "Edging Menu", on_open);