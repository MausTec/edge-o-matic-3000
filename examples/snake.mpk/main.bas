REM This program is an example of MY-BASIC
REM For more information, see https://github.com/paladin-t/my_basic/

import "@ui"
import "@graphics"
import "@system"

print "Program starting..."
toast("Program starting...")

let run_game = 1

while run_game
    for i = 0 to 128 step 2
        for j = 0 to 64 step 2
            draw_pixel(i, j, 1)
        next
    next

    display_buffer
    print "Snake tick.";
    delay(1000)
wend

print "Snake Ended"