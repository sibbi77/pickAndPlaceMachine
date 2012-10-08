#ifndef EXCELLONIMPORTER_H
#define EXCELLONIMPORTER_H

#include <gmpxx.h>
#include <QtCore>

class Drill
{
    Drill();
    Drill( mpq_class x, mpq_class y, mpq_class dia );
    void setDrill( mpq_class x, mpq_class y, mpq_class dia );

protected:
    mpq_class m_x, m_y;
    mpq_class m_diameter;
};

class ExcellonImporter
{
public:
    ExcellonImporter();

    bool import( QString filename );
    QList<Drill> drills() const { return m_drill; }

protected:
    QList<Drill> m_drill;
    QHash<int,mpq_class> m_tool;
    bool m_inHeader;
    int m_digits1; //!< digits in front of the decimal point
    int m_digits2; //!< digits after the decimal point
    enum {metric,inch} m_unit;
    enum {leading,trailing} m_zeros; //!< specifies which zeros must always be included
};

#endif // EXCELLONIMPORTER_H
