#include "frontend/fcore/fcore.hh"

base::e_success fcore::lex(fcore::s_file_process_state& file_process_state) {
    fcore::s_spot current_spot;

    for (char c : file_process_state.source_code) {
        file_process_state.token_list.push_back(tok::s_token(current_spot))
    }

    return base::e_success::SUCCESS;
}