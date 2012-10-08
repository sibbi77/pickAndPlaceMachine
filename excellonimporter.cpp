#include "excellonimporter.h"
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
    m_digits1 = 3;
    m_digits2 = 3;
    m_unit = inch;
    m_zeros = leading;
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
            m_inHeader = false;
            continue;
        }
        if (line == "M71") {
            m_unit = metric;
            continue;
        }
        if (line == "M72") {
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
            QRegExp rx("T[0-9]{1,2}C[0-9.]+");
            int idx = rx.indexIn(line);
            if (idx != -1) {
                // found tool definition command
                int num = rx.cap(1).toInt();
                QString dia = rx.cap(2);
                if (m_tool.contains(num)) {
                    qDebug() << "Again found tool definition for tool number" << num << ". Replacing old definition.";
                }
                m_tool[num] = mpz_class(dia.toAscii().constData());
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
