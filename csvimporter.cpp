/*
    This file is part of pickAndPlaceMachine.

    pickAndPlaceMachine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    pickAndPlaceMachine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pickAndPlaceMachine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "csvimporter.h"

#include <QtCore>
#include <QtGui>

CSVImporter::CSVImporter()
{
    m_sep = QChar(',');
}

bool CSVImporter::import( QString filename, QString separator )
{
    if (separator.size() >= 1)
        m_sep = separator.at(0);

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

