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
    bool analyze( QList<QStringList> csv );
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

//protected:
//    QList<CentroidLine> m_lines;

protected:
    QList<QStringList> m_data;
    QStringList m_headers;
    int m_rowCount, m_columnCount;
    mpq_class m_unit;
};


#endif // CENTROID_H
