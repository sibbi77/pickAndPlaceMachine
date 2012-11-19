#include "utils.h"
#include <QtCore>

mpq_class mpq_from_decimal_string( QString decimal_str, bool* conversion_ok, int* pos_after_number )
{
    if (conversion_ok)
        *conversion_ok = false;

    decimal_str = decimal_str.trimmed();
    if (decimal_str.isEmpty())
        return 0;

    QString digits;
    if (decimal_str.at(0) == '-' || decimal_str.at(0) == '+') {
        digits = decimal_str.at(0);
        decimal_str.remove(0,1); // remove sign
    }

    int decimal_point = -1;
    int pos = 0;
    while (pos < decimal_str.length() && (decimal_str.at(pos).isDigit() || decimal_str.at(pos) == '.')) {
        if (decimal_str.at(pos) == '.') {
            decimal_point = pos++;
            continue;
        }
        digits += decimal_str.at(pos++);
    }

    bool ok;
    long value = digits.toInt(&ok);
    if (!ok) {
        qDebug() << "mpq_from_decimal_string(): invalid decimal string.";
        return 0;
    }

    // evaluate decimals
    int decimal10 = 1;
    if (decimal_point != -1) {
        decimal10 = pow( 10, pos - decimal_point - 1 );
    }

    if (conversion_ok)
        *conversion_ok  = true;
    if (pos_after_number)
        *pos_after_number = pos;

    mpq_class result( value, decimal10 );
    return result;
}

