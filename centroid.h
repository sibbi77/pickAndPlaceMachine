#ifndef CENTROID_H
#define CENTROID_H

#include <QStringList>
#include <gmpxx.h>

class CentroidLine
{
public:
    // RefDes, Description, Value, X, Y, rotation, top/bottom
    QString RefDes, Description, Value;
    mpq_class x, y, rotation;
    QString side;
};

class Centroid
{
public:
    Centroid();
    bool analyze( QList<QStringList> csv, QHash<QString,int> *columnGuess = 0 );
    void assignColumns( QList<QStringList> csv, QHash<QString,int> columns );
    static QStringList columns() { return QStringList() << "RefDes" << "Description" << "Value" << "X" << "Y" << "Rotation" << "Side"; }
    QList<CentroidLine> lines() const { return m_lines; }

protected:
    QList<CentroidLine> m_lines;
};

#endif // CENTROID_H
