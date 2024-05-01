#include "modules/post_orgasm_milk_o_matic.h"
#include "modulES/post_orgasm_default.h"

static const char* TAG = "Post_orgasm_control";


void post_orgasm_init(void) {
    post_orgasm_milk_o_matic_init();
    post_orgasm_default_init();
}