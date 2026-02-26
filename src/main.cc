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

    int main(int argc, char* argv[]) {
        // this is the first line of statement code i am pretty sure was written in this project. DO NOT DELETE

        
        // DO NOT DELETE ---------v
        std::cout << "Hello world, this is Lican!\n";// <-------------------_______
        // DO NOT DELETE          ^ DO NOT DELETE         \    \__                 \__________ 
        // DO NOT DELETE          |      ^               /        \                           \
        // DO NOT DELETE          \      |       DO NOT DELETE     `                           \
        // DO NOT DELETE           \     |                        DO NOT DELETE                 \
        // DO NOT DELETE            |   DO NOT DELETE                                           |
        //                         DO NOT DELETE            DO NOT DELETE       DO NOT DELETE   |
        //                                                      \                   |          /
        //                                                       `----------------------------`
        
        licanc::t_lican_config config;

        if (argc == 1) {
            config.project_path = "C:/Users/jghig/projects/c/lican_loader/hello_world";
            config.start_subpath = "main.li";
        }
        else if (argc == 3) {
            config.project_path = argv[1];
            config.start_subpath = argv[2];
        }        

        licanc::compile(std::move(config));
        
        return 0;
    }

#elif MODE == 1
constexpr int test = 0;
#elif MODE == 2
constexpr int test = 0;
#elif MODE == 3
constexpr int test = 0;

#endif