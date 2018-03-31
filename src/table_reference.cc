/*
 * Use and distribution licensed under the Apache license version 2.
 *
 * See the COPYING file in the root project directory for full text.
 */

#include "sqltoast.h"

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
        default:
            break;
    }
    if (tr.has_alias()) {
        out << " AS " << std::string(tr.alias.start, tr.alias.end);
    }
    return out;
}

std::ostream& operator<< (std::ostream& out, const table_t& t) {
    out << std::string(t.table_name.start, t.table_name.end);
    return out;
}

std::ostream& operator<< (std::ostream& out, const derived_table_t& dt) {
    out << "<derived table> AS " << dt.table_name;
    return out;
}

} // namespace sqltoast
