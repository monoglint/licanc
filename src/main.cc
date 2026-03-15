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

    import <string>;
    import <iostream>;
    import driver;

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
            project_path = "C:/Users/jghig/projects/cxx/licanc/hello_world";
            start_subpath = "main.li";
        }
        else if (argc == 3) {
            project_path = argv[1];
            start_subpath = argv[2];
        }

        licanc::LicancConfig config {
            .project_path = project_path,
            .start_path = project_path + '/' + start_subpath
        };

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