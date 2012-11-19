#ifndef UTILS_H
#define UTILS_H

#include <gmpxx.h>
#include <QString>

mpq_class mpq_from_decimal_string( QString decimal_str, bool *conversion_ok = 0, int *pos_after_number = 0 );

#endif // UTILS_H
