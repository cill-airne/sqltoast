/*
 * Use and distribution licensed under the Apache license version 2.
 *
 * See the COPYING file in the root project directory for full text.
 */

#include "sqltoast/sqltoast.h"

namespace sqltoast {

std::ostream& operator<< (std::ostream& out, const table_reference_t& tr) {
    switch (tr.type) {
        case TABLE_REFERENCE_TYPE_TABLE:
            {
                const table_t& t = static_cast<const table_t&>(tr);
                out << t;
            }
            break;
        case TABLE_REFERENCE_TYPE_DERIVED_TABLE:
            {
                const derived_table_t& dt =
                    static_cast<const derived_table_t&>(tr);
                out << dt;
            }
            break;
        case TABLE_REFERENCE_TYPE_JOINED_TABLE:
            {
                const joined_table_t& jt =
                    static_cast<const joined_table_t&>(tr);
                out << jt;
            }
            break;
        default:
            break;
    }
    return out;
}

std::ostream& operator<< (std::ostream& out, const table_t& t) {
    out << std::string(t.table_name.start, t.table_name.end);
    if (t.has_alias()) {
        out << " AS " << std::string(t.alias.start, t.alias.end);
    }
    return out;
}

std::ostream& operator<< (std::ostream& out, const derived_table_t& dt) {
    out << "<derived table> AS " << dt.table_name;
    return out;
}

std::ostream& operator<< (std::ostream& out, const joined_table_t& jt) {
    switch (jt.join_type) {
        case JOIN_TYPE_INNER:
            out << "inner-join[";
            break;
        case JOIN_TYPE_LEFT:
            out << "left-join[";
            break;
        case JOIN_TYPE_RIGHT:
            out << "right-join[";
            break;
        case JOIN_TYPE_FULL:
            out << "full-join[";
            break;
        case JOIN_TYPE_CROSS:
            out << "cross-join[";
            break;
        case JOIN_TYPE_NATURAL:
            out << "natural-join[";
            break;
        default:
            break;
    }
    out << *jt.left << ',' << *jt.right;
    if (jt.spec)
        out << *jt.spec;
    out << ']';
    return out;
}

std::ostream& operator<< (std::ostream& out, const join_specification_t& js) {
    if (js.condition)
        out << ',' << *js.condition;
    else if (! js.named_columns.empty()) {
        out << ",using[";
        size_t x = 0;
        for (const lexeme_t& col : js.named_columns) {
            if (x++ > 0)
                out << ',';
            out << col;
        }
        out << ']';
    }
    return out;
}

} // namespace sqltoast
