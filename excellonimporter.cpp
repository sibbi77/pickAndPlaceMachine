#include "excellonimporter.h"
#include "utils.h"
#include <QtCore>

Drill::Drill()
{

}

Drill::Drill( mpq_class x, mpq_class y, mpq_class dia ) : m_x(x), m_y(y), m_diameter(dia)
{

}

void Drill::setDrill( mpq_class x, mpq_class y, mpq_class dia )
{
    m_x = x;
    m_y = y;
    m_diameter = dia;
}




ExcellonImporter::ExcellonImporter()
{
    m_inHeader = false;
    m_digits1 = 2;     // default according to Excellon specification
    m_digits2 = 4;     // default according to Excellon specification
    m_numberFormatExplicitlySet = false;
    m_unit = inch;     // default according to Excellon specification
    m_zeros = leading; // default according to Excellon specification
    m_zeros = trailing; // Eagle (Cadsoft) uses this default; TO BE VERIFIED WITH OTHER SOFTWARE!!!
    m_currentTool = -1;
}

bool ExcellonImporter::import( QString filename )
{
    if (filename.isEmpty())
        return false;

    QFile file( filename );
    if (!file.open(QFile::ReadOnly))
        return false;

    QTextStream stream(&file);
    QString line;
    while (true) {
        line = stream.readLine();
        line = line.trimmed().toUpper();
        if (line.isEmpty() && stream.atEnd())
            break;
        if (line.startsWith(";"))
            break;
        if (line == "M48") {
            m_inHeader = true;
            continue;
        }
        if (line == "M95") {
            if (!m_inHeader)
                qDebug() << "M95 command used at the wrong place.";
            m_inHeader = false;
            continue;
        }
        if (line == "M71") {
            m_unit = metric;
            if (!m_numberFormatExplicitlySet) {
                // the file did not specify the number format
                // use the default
                m_digits1 = 3;
                m_digits2 = 3;
            }
            continue;
        }
        if ((line == "M72") || (line == "M70")) {
            m_unit = inch;
            continue;
        }
        if (m_inHeader) {
            if (line == "%") {
                m_inHeader = false;
                continue;
            }
            if (line.startsWith("METRIC")) {
                m_unit = metric;
                line = line.mid(7); // jump over "METRIC,"
            }
            if (line.startsWith("INCH")) {
                m_unit = inch;
                line = line.mid(5); // jump over "INCH,"
            }
            if (line == "TZ") {
                m_zeros = trailing;
                continue;
            }
            if (line == "LZ") {
                m_zeros = leading;
                continue;
            }
            QRegExp rx("^T([0-9]{1,2})C([0-9.]+)");
            int idx = rx.indexIn(line);
            if (idx != -1) {
                // found tool definition command
                int num = rx.cap(1).toInt();
                QString dia = rx.cap(2);
                if (m_tool.contains(num)) {
                    qDebug() << "Again found tool definition for tool number" << num << ". Replacing old definition.";
                }
                m_tool[num] = mpq_from_decimal_string(dia);
                if (m_unit == metric)
                    m_tool[num] /= 1000; // convert from mm to m
                else {
                    m_tool[num] *= 254;
                    m_tool[num] /= 10000; // convert from inch to m
                }
                continue;
            }
        } else {
            // not inside header
            QRegExp rx("^T([0-9]{1,2})$");
            int idx = rx.indexIn(line);
            if (idx != -1) {
                // found tool change command
                int num = rx.cap(1).toInt();
                if (!m_tool.contains(num)) {
                    qDebug() << "Tool" << num << "not defined.";
                }
                m_currentTool = num;
                continue;
            }
            rx.setPattern("^X([0-9.]+)Y([0-9.]+)$");
            idx = rx.indexIn(line);
            if (idx != -1) {
                // drill at position
                QString x_str = rx.cap(1);
                QString y_str = rx.cap(2);
                if (m_currentTool == -1) {
                    qDebug() << "Cannot drill at" << x_str << y_str << ". No drill.";
                    continue;
                }
                mpq_class dia = m_tool.value(m_currentTool);
                mpq_class x = convert_into_mpq( x_str );
                mpq_class y = convert_into_mpq( y_str );
                if (m_unit == metric) {
                    x /= 1000; // convert from mm to m
                    y /= 1000; // convert from mm to m
                } else {
                    x *= 254;
                    x /= 10000; // convert from inch to m
                    y *= 254;
                    y /= 10000; // convert from inch to m
                }
                m_drill << Drill(x,y,dia);
                qDebug() << line << ": drilling at (in m) " << x.get_d() << y.get_d() << "dia:" << dia.get_d();
                continue;
            }
        }

        // command not handled
        if (m_inHeader)
            qDebug() << "unknown command in header:" << line;
        else
            qDebug() << "unknown command in body:" << line;
    }

    return true;
}

mpq_class ExcellonImporter::convert_into_mpq( QString x )
{
    if (x.contains('.'))
        return mpq_from_decimal_string(x);

    if (m_zeros == leading) {
        // leading zeros must be included
        while (x.size() < m_digits1) {
            // fill with '0'
            x.append( '0' );
        }
        x.insert( m_digits1, '.' );
    } else {
        // trailing zeros must be included
        while (x.size() < m_digits2) {
            // fill with '0'
            x.prepend('0');
        }
        x.insert( x.size() - m_digits2, '.' );
    }

    return mpq_from_decimal_string(x);
}
