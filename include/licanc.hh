/*

This header file is responsible for external access and usage of the compiler.

*/

#pragma once

namespace licanc {
    // Used for external interaction with the compiler. Carries through basic command info and more.
    struct s_compile_info {

    };

    void compile(s_compile_info&& info);
}