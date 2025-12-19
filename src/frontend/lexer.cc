#include "frontend/fcore/fcore.hh"

#include <unordered_map>

std::unordered_map<u8, tok::e_token_type> single_character_map = {
    {'.', tok::e_token_type::DOT},
    {'+', tok::e_token_type::PLUS},
    {'-', tok::e_token_type::MINUS},
    {'*', tok::e_token_type::ASTERISK},
    {'^', tok::e_token_type::CARET},
    {'/', tok::e_token_type::SLASH},
};

std::unordered_map<u16, tok::e_token_type> double_character_map = {
    {base::mergel_16('.', '.'), tok::e_token_type::DOUBLE_DOT},
};

base::e_success fcore::lex(fcore::s_file_process_state& file_process_state) {
    fcore::s_spot current_spot;

    for (char c : file_process_state.source_code) {
        file_process_state.token_list.push_back(tok::s_token(current_spot, tok::e_token_type::NONE));
    }

    return base::e_success::SUCCESS;
}