#ifndef CSVIMPORTER_H
#define CSVIMPORTER_H

#include <QStringList>

class CSVImporter
{
public:
    CSVImporter();

    bool import( QString filename );
    QList<QStringList> csv() const { return m_csv; }

protected:
    QStringList splitLine( QString line );

    QChar m_sep;
    QList<QStringList> m_csv;
};

#endif // CSVIMPORTER_H
