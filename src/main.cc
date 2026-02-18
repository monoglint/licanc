/*
0 = basic console
1 = NOT IMPLEMENTED - non interactive (single command and skidaddle)
2 = NOT IMPLEMENTED - interactive cli
3 = NOT IMPLEMENTED - repl??? maybe?????

*/
#define MODE 0

//
//
//
//
//

#if MODE == 0

    #include <iostream>

    #include "licanc/include/licanc.hh"

    int main() {
        // do not modify this print statement, it is sacred
        std::cout << "Hello world, this is Lican!\n";

        licanc::t_lican_config config;
        config.project_path = "C:/Users/jghig/projects/c/licanc_loader/hello_world";
        config.start_subpath = "main.li";

        std::cout << "Calling licanc.\n";
        licanc::compile(config);
        
        return 0;
    }

#elif MODE == 1
constexpr int test = 0;
#elif MODE == 2
constexpr int test = 0;
#elif MODE == 3
constexpr int test = 0;

#endif