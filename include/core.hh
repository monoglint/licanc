/*

====================================================

Contains all of the needed structures and data for the internal functionalities of the compiler.
Used by all stages of the compiler.
Holds the declarations for all of the stages of the compiler.

====================================================

*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <any>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "licanapi.hh"

namespace core {
    using t_file_id = uint16_t;
    using t_identifier_id = uint16_t;

    // Honestly just for some reading clarification. I don't want to use size_t where ever I go.
    using t_pos = size_t;

    constexpr t_file_id MAX_FILES = UINT16_MAX;
    constexpr t_pos MAX_POS = UINT32_MAX;

    struct liprocess;

    struct lisel {
        lisel(const t_file_id file_id, const t_pos start, const t_pos end)
             : file_id(file_id), start(start), end(end) {}

        lisel(const t_file_id file_id,const t_pos position)
            : file_id(file_id), start(position), end(position) {}

        lisel(const lisel& other0, const lisel& other1)
            : file_id(other0.file_id), start(other0.start), end(other1.end) {}

        t_file_id file_id;
        t_pos start;
        t_pos end;
        
        inline lisel operator-(t_pos amount) const { return lisel(start - amount, end - amount); }
        inline lisel operator+(t_pos amount) const { return lisel(start + amount, end + amount); }

        inline lisel& operator++() {
            start++;
            end++;

            return *this;
        }

        inline t_pos length() const {
            return end - start;
        }

        std::string pretty_debug(const liprocess& process) const;
    };

    struct lilog {
        enum class log_level : uint8_t {
            LOG,
            WARNING,
            ERROR,
            COMPILER_ERROR,
        };

        lilog(const log_level level, const lisel& selection, const std::string& message)
            : level(level), selection(selection), message(message) {}

        const log_level level;
        const lisel selection;
        const std::string message;

        std::string pretty_debug(const liprocess& process) const;
    };

    struct identifier_lookup {
        inline t_identifier_id insert(const std::string& str) {
            if (reverse.find(str) != reverse.end())
                return reverse.at(str);

            const t_identifier_id new_id = forward.size();
            
            forward.push_back(str);
            reverse.insert({str, new_id});

            return new_id;
        }

        // UB warning
        inline const std::string& get(const t_identifier_id id) const {
            return forward[id];
        }

        // UB warning
        inline t_identifier_id get_id(const std::string& identifier) {
            return reverse[identifier];
        }

    private:
        std::vector<std::string> forward;
        std::unordered_map<std::string, t_identifier_id> reverse;
    };

    struct liprocess {
        struct lifile {
            lifile(const std::string& path, const std::string& source_code)
                : path(path), source_code(source_code) {}

            const std::string path;
            const std::string source_code;

            std::vector<t_pos> line_marker_list; // Used to get the current line and column.

            // Data dump - avoids additional header dependencies. Decast as needed
            std::any dump_token_list;                   // std::vector<token>
            std::any dump_ast_arena;                    // ast::ast_arena

            // 0-indexed
            t_pos get_line_of_position(const t_pos position) const;
            
            // 0-indexed
            t_pos get_column_of_position(const t_pos position) const;
        };

        liprocess(const licanapi::liconfig_init& config_init)
            : config(config_init) {}

        const licanapi::liconfig config;
        
        std::vector<lilog> log_list;
        std::vector<lifile> file_list;

        identifier_lookup identifier_lookup;

        bool add_file(const std::string& path);

        inline void add_log(const lilog::log_level level, const lisel& selection, const std::string& message) {
            log_list.emplace_back(level, selection, message);
        }

        inline std::string sub_source_code(const lisel& selection) const {
            return file_list[selection.file_id].source_code.substr(selection.start, selection.end - selection.start + 1);
        }
    };

    namespace frontend {
        bool init(liprocess& process);
        bool lex(liprocess& process, const t_file_id file_id);
        bool parse(liprocess& process, const t_file_id file_id);
        bool semantic_analyze(liprocess& process, const t_file_id file_id);
    }

    namespace backend {
        
    }
}