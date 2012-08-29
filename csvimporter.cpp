#include "csvimporter.h"

#include <QtCore>
#include <QtGui>

CSVImporter::CSVImporter()
{
    m_sep = QChar(',');
}

bool CSVImporter::import( QString filename )
{
    QFile file( filename );
    if (!file.open(QFile::ReadOnly))
        return false;

    QTextStream stream(&file);
    QString line;
    bool finished = false;
    while (!finished) {
        line = line.trimmed();
        if (!line.startsWith("#") && !line.isEmpty()) {
            QStringList fields = splitLine( line );
            m_csv << fields;
        }

        line = stream.readLine();
        if (line.isEmpty() && stream.atEnd())
            finished = true;
    }

    return true;
}

QStringList CSVImporter::splitLine( QString line )
{
    bool insideQuotes = false;
    QStringList fields;
    QString field;
    int n = -1;
    while (++n < line.size()) {
        if ((line.at(n) == '"') && !insideQuotes) {
            insideQuotes = true;
            continue;
        }
        if ((line.at(n) == '"') && insideQuotes) {
            if ((n+1 < line.size()) && (line.at(n+1) == '"')) {
                // escaped double quote
            } else {
                insideQuotes = false;
                continue;
            }
        }
        if (!insideQuotes && (line.at(n) == m_sep)) {
            fields << field;
            field.clear();
        } else {
            field += line.at(n);
        }
    }
    fields << field;

    return fields;
}

