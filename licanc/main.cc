#include <string>
#include <iostream>

import build_system;

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
    

    std::string project_path;
    std::string start_subpath;

    if (argc == 1) {
        project_path = "C:/Users/jghig/Documents/TempEmpty/main";
        start_subpath = "main.li";
    }
    else if (argc == 3) {
        project_path = argv[1];
        start_subpath = argv[2];
    }

    build_system::LicancConfig config {
        .project_path = project_path,
        .start_path = project_path + '/' + start_subpath
    };

    build_system::compile(config);
    
    return 0;
}