/*
 * Use and distribution licensed under the Apache license version 2.
 *
 * See the COPYING file in the root project directory for full text.
 */

#ifndef SQLTOAST_IDENTIFIER_H
#define SQLTOAST_IDENTIFIER_H

#include <ostream>
#include <string>

#include "sqltoast.h"

namespace sqltoast {

typedef struct identifier {
    const std::string name;
    identifier(lexeme_t& lexeme) : name(lexeme.start, lexeme.end)
    {}
} identifier_t;

std::ostream& operator<< (std::ostream& out, const identifier_t& id);

} // namespace sqltoast

#endif /* SQLTOAST_IDENTIFIER_H */
