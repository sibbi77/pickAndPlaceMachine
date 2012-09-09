#ifndef CENTROID_H
#define CENTROID_H

#include <QStringList>
#include <QAbstractTableModel>
#include <gmpxx.h>

class CentroidLine
{
public:
    // RefDes, Description, Value, X, Y, rotation, top/bottom
    QString RefDes, Description, Value;
    mpq_class x, y, rotation;
    QString side;
};

class Centroid : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Unit {UnitInch,UnitMm,UnitMils};
    Centroid();
    bool analyze( QString filename );
    void assignColumns( QList<QStringList> csv, QHash<QString,int> columns );
    static QStringList columns() { return QStringList() << "RefDes" << "Description" << "Value" << "X" << "Y" << "Rotation" << "Side"; }
    QList<CentroidLine> lines() const;

    // QAbstractTableModel
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    void setCSV( QList<QStringList> csv, QHash<QString, int> *columnGuess = 0 );
    void reassignColumn( int oldVisualIndex, int newVisualIndex );
    void setUnit( Unit unit );
    Unit unit() const;
    QString filename() const {return m_filename;}
    void setSeparator( QString separator ) {m_csvSeparator = separator;}
    QString separator() const {return m_csvSeparator;}

protected:
    QString m_filename; //!< original filename of csv file
    QString m_csvSeparator; //!< column separator in csv file (default: ",")

    QList<QStringList> m_data; //!< interpreted data from csv file
    QStringList m_headers; //!< column names and meaning
    int m_rowCount, m_columnCount; //!< row and column count of m_data (stored for speed up)
    mpq_class m_unit; //!< factor to calculate the position in m from the values in the csv

    bool analyze( QList<QStringList> csv );
};


#endif // CENTROID_H
