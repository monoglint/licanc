/*

Holds top level declarations that are openly shared to any file that includes fcore.

*/

#pragma once

/*
lican demo program for telling the telling the time and displaying it with a contrived library

//

use "lib/winlib"
use "lib/renderlib"
use "core/os"
use "core/string"

main() {
    new_window = winlib::new("Clock", 600, 600)
    viewport = renderlib::new(600, 600)

    viewport.set_flush_target(new_window)

    while (true) {
        ; Example of explicit type declaration
        hour: u8 = os::clock::hour_of_day()
        minute: u8 = os::clock::minute_of_hour()    
        second: u8 = os::clock::second_of_minute()

        viewport::render_text(hour + ':' + minute + ':' + second)
        viewport::flush()
    }
}

*/

namespace tok {
    enum class e_token_type {
        NONE,

        IDENTIFIER,

        // literals
        INTEGER,
        CHARACTER,
        STRING,
    };

    struct s_token;
}