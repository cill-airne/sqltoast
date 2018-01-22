/*
 * Use and distribution licensed under the Apache license version 2.
 *
 * See the COPYING file in the root project directory for full text.
 */

#include "symbol.h"

namespace sqltoast {

symbol_map::symbol_map_t  _init_symbol_map() {
    symbol_map::symbol_map_t m;

    // Special characters
    m[SYMBOL_SEMICOLON] = std::string("SEMICOLON");
    m[SYMBOL_COMMA] = std::string("COMMA");
    m[SYMBOL_LPAREN] = std::string("LPAREN");
    m[SYMBOL_RPAREN] = std::string("RPAREN");

    // Reserved keywords
    m[SYMBOL_AUTHORIZATION] = std::string("AUTHORIZATION");
    m[SYMBOL_CHARACTER] = std::string("CHARACTER");
    m[SYMBOL_CREATE] = std::string("CREATE");
    m[SYMBOL_DEFAULT] = std::string("DEFAULT");

    // Non-Reserved keywordsr
    m[SYMBOL_SCHEMA] = std::string("SCHEMA");
    m[SYMBOL_SET] = std::string("SET");

    // Other symbols
    m[SYMBOL_IDENTIFIER] = std::string("<< identifier >>");
    m[SYMBOL_COMMENT] = std::string("<< comment >>");

    return m;
}

symbol_map::symbol_map_t symbol_map::m = _init_symbol_map();

} // namespace sqltoast
