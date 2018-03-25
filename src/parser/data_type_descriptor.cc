/*
 * Use and distribution licensed under the Apache license version 2.
 *
 * See the COPYING file in the root project directory for full text.
 */

#include <stdlib.h>

#include <iostream>
#include <cctype>
#include <sstream>

#include "sqltoast.h"

#include "parser/parse.h"
#include "parser/error.h"
#include "parser/sequence.h"

namespace sqltoast {

//
// Parses the <data type> clause of a column definition, which follows this
// EBNF grammar for ANSI-92 SQL:
//
// <data type> ::=
//     <character string type> [ CHARACTER SET <character set specification> ]
//     | <national character string type>
//     | <bit string type>
//     | <numeric type>
//     | <datetime type>
//     | <interval type>
//

bool parse_data_type_descriptor(
        parse_context_t& ctx,
        token_t& cur_tok,
        column_definition_t& column_def) {
    symbol_t cur_sym = cur_tok.symbol;

    // BEGIN STATE MACHINE

    // We start here. The first component of the column definition is the
    // identifier that indicates the column name.
    switch (cur_sym) {
        case SYMBOL_CHAR:
        case SYMBOL_CHARACTER:
        case SYMBOL_VARCHAR:
            if (! parse_character_string(ctx, cur_tok, column_def))
                return false;
            goto optional_character_set;
        case SYMBOL_NCHAR:
        case SYMBOL_NATIONAL:
            return parse_character_string(ctx, cur_tok, column_def);
        case SYMBOL_BIT:
            return parse_bit_string(ctx, cur_tok, column_def);
        case SYMBOL_INT:
        case SYMBOL_INTEGER:
        case SYMBOL_SMALLINT:
        case SYMBOL_NUMERIC:
        case SYMBOL_DEC:
        case SYMBOL_DECIMAL:
            return parse_exact_numeric(ctx, cur_tok, column_def);
        case SYMBOL_FLOAT:
        case SYMBOL_REAL:
        case SYMBOL_DOUBLE:
            return parse_approximate_numeric(ctx, cur_tok, column_def);
        case SYMBOL_DATE:
        case SYMBOL_TIME:
        case SYMBOL_TIMESTAMP:
            return parse_datetime(ctx, cur_tok, column_def);
        case SYMBOL_INTERVAL:
            return parse_interval(ctx, cur_tok, column_def);
        default:
            goto err_expect_data_type;
    }
err_expect_data_type:
    {
        std::stringstream estr;
        estr << "Expected data type after <column name> but found " << cur_tok << std::endl;
        create_syntax_error_marker(ctx, estr);
        return false;
    }
optional_character_set:
    // We get here after processing the optional length specifier. After
    // that specifier, there may be an optional CHARACTER SET <character
    // set specification> clause
    cur_sym = ctx.lexer.current_token.symbol;
    if (cur_sym == SYMBOL_CHARACTER)
        goto process_character_set;
    return true;
process_character_set:
    {
        symbol_t exp_sym_seq[3] = {
            SYMBOL_CHARACTER,
            SYMBOL_SET,
            SYMBOL_IDENTIFIER
        };
        if (! expect_sequence(ctx, exp_sym_seq, 3))
            return false;
        // tack the character set onto the char_string_t data type descriptor
        lexer_t& lex = ctx.lexer;
        char_string_t* dtd = static_cast<char_string_t*>(column_def.data_type.get());
        dtd->charset = lex.current_token.lexeme;
        cur_tok = ctx.lexer.next();

        return true;
    }
}

// <character string type> ::=
//     CHARACTER [ <left paren> <length> <right paren> ]
//     | CHAR [ <left paren> <length> <right paren> ]
//     | CHARACTER VARYING [ <left paren> <length> <right paren> ]
//     | CHAR VARYING [ <left paren> <length> <right paren> ]
//     | VARCHAR [ <left paren> <length> <right paren> ]
bool parse_character_string(
        parse_context_t& ctx,
        token_t& cur_tok,
        column_definition_t& column_def) {
    lexer_t& lex = ctx.lexer;
    symbol_t cur_sym = cur_tok.symbol;
    data_type_t data_type = DATA_TYPE_CHAR;
    size_t char_len = 0;

    // BEGIN STATE MACHINE

    // We get here after the column name identifier has been found and
    // we've determined that either the CHAR, CHARACTER, or VARCHAR symbols
    // were next
    switch (cur_sym) {
        case SYMBOL_NATIONAL:
            data_type = DATA_TYPE_NCHAR;
            cur_tok = lex.next();
            goto expect_char;
        case SYMBOL_NCHAR:
            data_type = DATA_TYPE_NCHAR;
        case SYMBOL_CHAR:
        case SYMBOL_CHARACTER:
            cur_tok = lex.next();
            goto optional_varying;
        case SYMBOL_VARCHAR:
            data_type = DATA_TYPE_VARCHAR;
            cur_tok = lex.next();
            goto optional_length;
        default:
            return false;
    }
expect_char:
    cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_CHAR || cur_sym == SYMBOL_CHARACTER) {
        cur_tok = lex.next();
        goto optional_varying;
    }
    goto err_expect_char;
err_expect_char:
    expect_any_error(ctx, {SYMBOL_CHAR, SYMBOL_CHARACTER});
    return false;
optional_varying:
    // We get here if we got a CHAR or CHARACTER as the data type. This
    // might be followed by the VARYING symbol, in which case we will
    // process a VARCHAR. Otherwise, we'll process a CHAR type
    cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_VARYING) {
        if (data_type == DATA_TYPE_CHAR)
            data_type = DATA_TYPE_VARCHAR;
        else
            data_type = DATA_TYPE_NVARCHAR;
        cur_tok = lex.next();
    }
    goto optional_length;
optional_length:
    // We get here after determining the exact type of the character
    // string. The type will be followed by an optional length specifier
    // clause, which if an unsigned integer enclosed by parentheses.
    if (! parse_length_specifier(ctx, cur_tok, &char_len))
        return false;
    goto push_descriptor;
push_descriptor:
    {
        if (ctx.opts.disable_statement_construction)
            return true;
        std::unique_ptr<data_type_descriptor_t> dtd_p;
        dtd_p = std::move(std::make_unique<char_string_t>(data_type, char_len));
        column_def.data_type = std::move(dtd_p);
        return true;
    }
}

// <bit string type> ::=
//     BIT [ <left paren> <length> <right paren> ]
//     | BIT VARYING [ <left paren> <length> <right paren> ]
bool parse_bit_string(
        parse_context_t& ctx,
        token_t& cur_tok,
        column_definition_t& column_def) {
    lexer_t& lex = ctx.lexer;
    data_type_t data_type = DATA_TYPE_BIT;
    size_t bit_len = 0;

    // BEGIN STATE MACHINE

    // We get here after the column name identifier has been found and
    // we've determined that the BIT symbol is the current symbol.
    // were next
    cur_tok = lex.next(); // consume the BIT symbol
    symbol_t cur_sym = cur_tok.symbol;
    goto optional_varying;
optional_varying:
    // We get here if we got a CHAR or CHARACTER as the data type. This
    // might be followed by the VARYING symbol, in which case we will
    // process a VARCHAR. Otherwise, we'll process a CHAR type
    cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_VARYING) {
        data_type = DATA_TYPE_VARBIT;
        cur_tok = lex.next();
    }
    goto optional_length;
optional_length:
    // We get here after determining the exact type of the bit string. The type
    // will be followed by an optional length specifier clause, which if an
    // unsigned integer enclosed by parentheses.
    if (! parse_length_specifier(ctx, cur_tok, &bit_len))
        return false;
    goto push_descriptor;
push_descriptor:
    {
        if (ctx.opts.disable_statement_construction)
            return true;
        std::unique_ptr<data_type_descriptor_t> dtd_p;
        dtd_p = std::move(std::make_unique<bit_string_t>(data_type, bit_len));
        column_def.data_type = std::move(dtd_p);
        return true;
    }
}

// <length> ::= <unsigned integer>
bool parse_length_specifier(
        parse_context_t& ctx,
        token_t& cur_tok,
        size_t* out) {
    lexer_t& lex = ctx.lexer;
    symbol_t cur_sym;
    *out = 0;

    // We get here if we've processed the opening parentheses of the
    // optional length modifier and now expect to find an unsigned integer
    // followed by a closing parentheses
    if (cur_tok.is_literal()) {
        // Make sure we can parse our literal token to an unsigned integer
        cur_sym = cur_tok.symbol;
        if (cur_sym != SYMBOL_LITERAL_UNSIGNED_INTEGER)
            goto err_expect_size_literal;
        const std::string len_str(cur_tok.lexeme.start, cur_tok.lexeme.end);
        *out = atoi(len_str.data());
        cur_tok = lex.next();
        return true;
    }
    goto err_expect_size_literal;
err_expect_size_literal:
    expect_error(ctx, SYMBOL_LITERAL_UNSIGNED_INTEGER);
    return false;
}

// <exact numeric type> ::=
//     NUMERIC [ <left paren> <precision> [ <comma> <scale> ] <right paren> ]
//     | DECIMAL [ <left paren> <precision> [ <comma> <scale> ] <right paren> ]
//     | DEC [ <left paren> <precision> [ <comma> <scale> ] <right paren> ]
//     | INTEGER
//     | INT
//     | SMALLINT
bool parse_exact_numeric(
        parse_context_t& ctx,
        token_t& cur_tok,
        column_definition_t& column_def) {
    lexer_t& lex = ctx.lexer;
    symbol_t cur_sym = cur_tok.symbol;
    data_type_t data_type = DATA_TYPE_INT;
    size_t prec = 0;
    size_t scale = 0;

    // BEGIN STATE MACHINE

    // We get here after the column name identifier has been found and
    // we've determined that either the CHAR, CHARACTER, or VARCHAR symbols
    // were next
    switch (cur_sym) {
        case SYMBOL_INT:
        case SYMBOL_INTEGER:
            cur_tok = lex.next();
            goto push_descriptor;
        case SYMBOL_SMALLINT:
            data_type = DATA_TYPE_SMALLINT;
            cur_tok = lex.next();
            goto push_descriptor;
        case SYMBOL_NUMERIC:
        case SYMBOL_DEC:
        case SYMBOL_DECIMAL:
            data_type = DATA_TYPE_NUMERIC;
            cur_tok = lex.next();
            goto optional_precision_scale;
        default:
            return false;
    }
optional_precision_scale:
    // We get here after determining the exact type of the character
    // string. The type will be followed by an optional length specifier
    // clause, which if an unsigned integer enclosed by parentheses.
    if (! parse_precision_scale(ctx, cur_tok, &prec, &scale))
        return false;
    goto push_descriptor;
push_descriptor:
    {
        if (ctx.opts.disable_statement_construction)
            return true;
        std::unique_ptr<data_type_descriptor_t> dtd_p;
        dtd_p = std::move(std::make_unique<exact_numeric_t>(data_type, prec, scale));
        column_def.data_type = std::move(dtd_p);
        return true;
    }
}

// [ <left paren> <precision> [ <comma> <scale> ] <right paren> ]
// <precision> ::= <unsigned integer>
//
// <scale> ::= <unsigned integer>
bool parse_precision_scale(
        parse_context_t& ctx,
        token_t& cur_tok,
        size_t* out_precision,
        size_t* out_scale) {
    lexer_t& lex = ctx.lexer;
    *out_precision = 0;
    *out_scale = 0;

    // BEGIN STATE MACHINE

    symbol_t cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_LPAREN) {
        cur_tok = lex.next();
        goto process_precision;
    }
    return true;
process_precision:
    // We get here if we've processed the opening parentheses of the
    // optional length modifier and now expect to find an unsigned integer
    // followed by a closing parentheses
    if (cur_tok.is_literal()) {
        // Make sure we can parse our literal token to an unsigned integer
        cur_sym = cur_tok.symbol;
        if (cur_sym != SYMBOL_LITERAL_UNSIGNED_INTEGER)
            goto err_expect_uint_literal;
        const std::string prec_str(cur_tok.lexeme.start, cur_tok.lexeme.end);
        *out_precision = atoi(prec_str.data());
        cur_tok = lex.next();
        goto optional_scale;
    }
    goto err_expect_uint_literal;
err_expect_uint_literal:
    expect_error(ctx, SYMBOL_LITERAL_UNSIGNED_INTEGER);
    return false;
optional_scale:
    cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_COMMA) {
        cur_tok = lex.next();
        goto process_scale;
    }
    goto precision_close;
process_scale:
    // We get here if we've processed the comma that delimits the optional
    // scale specifier and now expect to find an unsigned integer followed by a
    // closing parentheses
    if (cur_tok.is_literal()) {
        // Make sure we can parse our literal token to an unsigned integer
        cur_sym = cur_tok.symbol;
        if (cur_sym != SYMBOL_LITERAL_UNSIGNED_INTEGER)
            goto err_expect_uint_literal;
        const std::string scale_str(cur_tok.lexeme.start, cur_tok.lexeme.end);
        *out_scale = atoi(scale_str.data());
        cur_tok = lex.next();
        goto precision_close;
    }
    goto err_expect_uint_literal;
precision_close:
    // We get here if we've processed the opening parentheses of the precision
    // specifier and the unsigned integer precision and now expect a closing
    // parentheses
    cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_RPAREN) {
        cur_tok = lex.next();
        return true;
    }
    goto err_expect_rparen;
err_expect_rparen:
    expect_error(ctx, SYMBOL_RPAREN);
    return false;
}

// <approximate numeric type> ::=
//     FLOAT [ <left paren> <precision> <right paren> ]
//     | REAL
//     | DOUBLE PRECISION
bool parse_approximate_numeric(
        parse_context_t& ctx,
        token_t& cur_tok,
        column_definition_t& column_def) {
    lexer_t& lex = ctx.lexer;
    symbol_t cur_sym = cur_tok.symbol;
    data_type_t data_type = DATA_TYPE_FLOAT;
    size_t prec = 0;

    // BEGIN STATE MACHINE

    // We get here after the column name identifier has been found and
    // we've determined that either the FLOAT, REAL, or DOUBLE symbols
    // were next
    switch (cur_sym) {
        case SYMBOL_FLOAT:
            cur_tok = lex.next();
            goto optional_precision;
        case SYMBOL_REAL:
            // REAL is a synonym for FLOAT(24)
            prec = 24;
            cur_tok = lex.next();
            goto push_descriptor;
        case SYMBOL_DOUBLE:
            data_type = DATA_TYPE_DOUBLE;
            cur_tok = lex.next();
            goto expect_precision_sym;
        default:
            return false;
    }
optional_precision:
    // We get here after getting a FLOAT symbol. This can be followed by an
    // optional LPAREN <precision> RPAREN. Since the length specifier is an
    // identical structure, we use that...
    cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_LPAREN) {
        cur_tok = lex.next();
        goto process_length;
    }
    goto push_descriptor;
process_length:
    if (! parse_length_specifier(ctx, cur_tok, &prec))
        return false;
    cur_sym = cur_tok.symbol;
    if (cur_sym != SYMBOL_RPAREN)
        goto err_expect_rparen;
    goto push_descriptor;
err_expect_rparen:
    expect_error(ctx, SYMBOL_RPAREN);
    return false;
expect_precision_sym:
    // We get here if we got the DOUBLE symbol, which according to ANSI-92 SQL
    // must be followed by the keyword "PRECISION"
    cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_PRECISION) {
        cur_tok = lex.next();
        goto push_descriptor;
    }
    goto err_expect_precision_sym;
err_expect_precision_sym:
    expect_error(ctx, SYMBOL_PRECISION);
    return false;
push_descriptor:
    {
        if (ctx.opts.disable_statement_construction)
            return true;
        std::unique_ptr<data_type_descriptor_t> dtd_p;
        dtd_p = std::move(std::make_unique<approximate_numeric_t>(data_type, prec));
        column_def.data_type = std::move(dtd_p);
        return true;
    }
}

// <datetime type> ::=
//     DATE
//     | TIME [ <left paren> <time precision> <right paren> ] [ WITH TIME ZONE ]
//     | TIMESTAMP [ <left paren> <timestamp precision> <right paren> ] [ WITH TIME ZONE ]
//
// <time precision> ::= <time fractional seconds precision>
//
// <time fractional seconds precision> ::= <unsigned integer>
//
// <timestamp precision> ::= <time fractional seconds precision>
bool parse_datetime(
        parse_context_t& ctx,
        token_t& cur_tok,
        column_definition_t& column_def) {
    lexer_t& lex = ctx.lexer;
    symbol_t cur_sym = cur_tok.symbol;
    data_type_t data_type = DATA_TYPE_DATE;
    bool with_tz = false;
    size_t prec = 0;

    // BEGIN STATE MACHINE

    // We get here after the column name identifier has been found and
    // we've determined that either the FLOAT, REAL, or DOUBLE symbols
    // were next
    switch (cur_sym) {
        case SYMBOL_DATE:
            cur_tok = lex.next();
            goto push_descriptor;
        case SYMBOL_TIME:
            data_type = DATA_TYPE_TIME;
            cur_tok = lex.next();
            goto optional_precision;
        case SYMBOL_TIMESTAMP:
            data_type = DATA_TYPE_TIMESTAMP;
            cur_tok = lex.next();
            goto optional_precision;
        default:
            return false;
    }
optional_precision:
    // We get here after getting a FLOAT symbol. This can be followed by an
    // optional LPAREN <precision> RPAREN. Since the length specifier is an
    // identical structure, we use that...
    cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_LPAREN) {
        cur_tok = lex.next();
        goto process_length;
    }
    goto optional_with_tz;
process_length:
    if (! parse_length_specifier(ctx, cur_tok, &prec))
        return false;
    cur_sym = cur_tok.symbol;
    if (cur_sym != SYMBOL_RPAREN)
        goto err_expect_rparen;
    cur_tok = lex.next();
    goto optional_with_tz;
err_expect_rparen:
    expect_error(ctx, SYMBOL_RPAREN);
    return false;
optional_with_tz:
    cur_tok = lex.current_token;
    cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_WITH) {
        symbol_t exp_sym_seq[3] = {
            SYMBOL_WITH,
            SYMBOL_TIME,
            SYMBOL_ZONE
        };
        if (! expect_sequence(ctx, exp_sym_seq, 3))
            return false;
        with_tz = true;
        cur_tok = ctx.lexer.next();
        goto push_descriptor;
    }
push_descriptor:
    {
        if (ctx.opts.disable_statement_construction)
            return true;
        std::unique_ptr<data_type_descriptor_t> dtd_p;
        dtd_p = std::move(std::make_unique<datetime_t>(data_type, prec, with_tz));
        column_def.data_type = std::move(dtd_p);
        return true;
    }
}

// <interval type> ::= INTERVAL <interval qualifier>
//
// <interval qualifier> ::=
//     <start field> TO <end field>
//     | <single datetime field>
//
// <start field> ::=
//     <non-second datetime field> [ <left paren> <interval leading field precision> <right paren> ]
//
// <non-second datetime field> ::= YEAR | MONTH | DAY | HOUR | MINUTE
//
// <interval leading field precision> ::= <unsigned integer>
//
// <end field> ::=
//     <non-second datetime field>
//     | SECOND [ <left paren> <interval fractional seconds precision> <right paren> ]
//
// <interval fractional seconds precision> ::= <unsigned integer>
//
// <single datetime field> ::=
//     <non-second datetime field> [ <left paren> <interval leading field precision> <right paren> ]
//     | SECOND [ <left paren> <interval leading field precision> [ <comma> <left paren> <interval fractional seconds precision> ] <right paren> ]
bool parse_interval(
        parse_context_t& ctx,
        token_t& cur_tok,
        column_definition_t& column_def) {
    lexer_t& lex = ctx.lexer;
    interval_unit_t unit = INTERVAL_UNIT_YEAR;
    size_t prec = 0;

    cur_tok = lex.next(); // consume the INTERVAL token
    symbol_t cur_sym = cur_tok.symbol;

    // BEGIN STATE MACHINE

    switch (cur_sym) {
        case SYMBOL_YEAR:
            unit = INTERVAL_UNIT_YEAR;
            cur_tok = lex.next();
            goto push_descriptor;
        case SYMBOL_MONTH:
            unit = INTERVAL_UNIT_MONTH;
            cur_tok = lex.next();
            goto push_descriptor;
        case SYMBOL_DAY:
            unit = INTERVAL_UNIT_DAY;
            cur_tok = lex.next();
            goto push_descriptor;
        case SYMBOL_HOUR:
            unit = INTERVAL_UNIT_HOUR;
            cur_tok = lex.next();
            goto push_descriptor;
        case SYMBOL_MINUTE:
            unit = INTERVAL_UNIT_MINUTE;
            cur_tok = lex.next();
            goto push_descriptor;
        case SYMBOL_SECOND:
            unit = INTERVAL_UNIT_SECOND;
            cur_tok = lex.next();
            goto optional_precision;
        default:
            return false;
    }
optional_precision:
    // We get here after getting a SECOND symbol, which is the only interval
    // unit that can have a precision to it. This can be followed by an
    // optional LPAREN <precision> RPAREN. Since the length specifier is an
    // identical structure, we use that...
    cur_sym = cur_tok.symbol;
    if (cur_sym == SYMBOL_LPAREN) {
        cur_tok = lex.next();
        goto process_length;
    }
    goto push_descriptor;
process_length:
    if (! parse_length_specifier(ctx, cur_tok, &prec))
        return false;
    cur_sym = cur_tok.symbol;
    if (cur_sym != SYMBOL_RPAREN)
        goto err_expect_rparen;
    goto push_descriptor;
err_expect_rparen:
    expect_error(ctx, SYMBOL_RPAREN);
    return false;
push_descriptor:
    {
        if (ctx.opts.disable_statement_construction)
            return true;
        std::unique_ptr<data_type_descriptor_t> dtd_p;
        dtd_p = std::move(std::make_unique<interval_t>(unit, prec));
        column_def.data_type = std::move(dtd_p);
        return true;
    }
}

} // namespace sqltoast
