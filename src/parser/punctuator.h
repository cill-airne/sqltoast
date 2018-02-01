/*
 * Use and distribution licensed under the Apache license version 2.
 *
 * See the COPYING file in the root project directory for full text.
 */

#ifndef SQLTOAST_PARSER_PUNCTUATOR_H
#define SQLTOAST_PARSER_PUNCTUATOR_H

#include "parser/context.h"
#include "parser/symbol.h"
#include "parser/token.h"

namespace sqltoast {

const unsigned int NUM_PUNCTUATORS = 4;

static const char punctuator_char_map[4] = {
    ';', // PUNCTUATOR_SEMICOLON
    ',', // PUNCTUATOR_COMMA
    '(', // PUNCTUATOR_LPAREN
    ')', // PUNCTUATOR_RPAREN
};

static const symbol_t punctuator_symbol_map[4] = {
    SYMBOL_SEMICOLON,
    SYMBOL_COMMA,
    SYMBOL_LPAREN,
    SYMBOL_RPAREN
};

// Moves the supplied parse context's cursor to the next punctuator found in the
// context's input stream and sets the context's current symbol to the found
// punctuator symbol. Returns whether a punctuator was found.
tokenize_result_t token_punctuator(parse_context_t& ctx);

} // namespace sqltoast

#endif /* SQLTOAST_PARSER_PUNCTUATOR_H */
