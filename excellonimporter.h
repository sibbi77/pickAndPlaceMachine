#ifndef EXCELLONIMPORTER_H
#define EXCELLONIMPORTER_H

#include <gmpxx.h>
#include <QtCore>

class Drill
{
public:
    Drill();
    Drill( mpq_class x, mpq_class y, mpq_class dia );
    void setDrill( mpq_class x, mpq_class y, mpq_class dia );
    mpq_class x() const {return m_x;}
    mpq_class y() const {return m_y;}
    mpq_class diameter() const {return m_diameter;}

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
    QList<Drill> m_drill; //!< a list of all drills; position in m
    QHash<int,mpq_class> m_tool; //!< stores the diameter in m for each defined tool number
    bool m_inHeader;
    int m_digits1; //!< digits in front of the decimal point
    int m_digits2; //!< digits after the decimal point
    bool m_numberFormatExplicitlySet;
    enum {metric,inch} m_unit;
    enum {leading,trailing} m_zeros; //!< specifies which zeros must always be included
    int m_currentTool;

    mpq_class convert_into_mpq( QString x );
};

#endif // EXCELLONIMPORTER_H
