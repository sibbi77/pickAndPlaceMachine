#include "centroid.h"

#include <QtCore>

Centroid::Centroid()
{
    m_rowCount = 0;
    m_columnCount = 0;
    m_unit = UnitInch;
}

//! \brief Analyze the \c csv and try to determine the meaning of the columns.
//! The csv content is stored in this object.
bool Centroid::analyze( QList<QStringList> csv )
{
    m_data = csv;
    m_rowCount = csv.size();
    m_columnCount = 0;
    for (int r=0; r<m_rowCount; r++)
        m_columnCount = std::max( m_columnCount, csv.at(r).size() );

    QHash<QString,int> temp;

    int col_RefDes = -1;
    int col_Desc = -1;
    int col_Value = -1;
    int col_x = -1;
    int col_y = -1;
    int col_rotation = -1;
    int col_Side = -1;

    // determine number of columns in csv
    int max_columns = 0;
    for (int r=0; r<csv.size(); r++)
        max_columns = std::max( max_columns, csv.at(r).size() );

    QList<int> remaining_columns;
    for (int c=0; c<max_columns; c++)
        remaining_columns << c;

    // determine column for side
    for (int c=0; c<max_columns; c++) {
        for (int r=0; r<csv.size(); r++) {
            if ((csv.at(r).value(c).toLower() == "top") || (csv.at(r).value(c).toLower() == "bottom")) {
                col_Side = c;
                break;
            }
        }
        if (col_Side != -1)
            break;
    }
    remaining_columns.removeOne( col_Side );

    // determine columns for x, y, rotation
    // expect x, y, rotation in this sequence
    QList<int> idx;
    for (int c=0; c<max_columns-2; c++) {
        if (!remaining_columns.contains(c) || !remaining_columns.contains(c+1) || !remaining_columns.contains(c+2))
            break;
        idx << c; // assuming column c, c+1 and c+2 obey the rules
        for (int r=0; r<csv.size(); r++) {
            // evaluate all rows
            bool ok1, ok2, ok3;
            csv.at(r).value(c).toDouble( &ok1 );
            csv.at(r).value(c+1).toDouble( &ok2 );
            csv.at(r).value(c+2).toDouble( &ok3 );
            if (!ok1 || !ok2 || !ok3) {
                // at least one rule was broken
                idx.removeLast();
                break;
            }
        }
    }
    if (idx.size() != 1) {
        // no columns meet all rules or too many colums match all rules
        // manually assign the columns via assignColumns()
        return false;
    }
    col_x = idx.first();
    col_y = col_x+1;
    col_rotation = col_x+2;

    remaining_columns.removeOne( col_x );
    remaining_columns.removeOne( col_y );
    remaining_columns.removeOne( col_rotation );

    qDebug() << remaining_columns;


    // fill detected columns
    QStringList unassigned;
    m_headers.clear();
    for (int i=0; i<columns().size(); i++)
        m_headers << QString();
    if (col_RefDes != -1)
        m_headers[col_RefDes] = columns().at(0);
    else
        unassigned << columns().at(0);
    if (col_Desc != -1)
        m_headers[col_Desc] = columns().at(1);
    else
        unassigned << columns().at(1);
    if (col_Value != -1)
        m_headers[col_Value] = columns().at(2);
    else
        unassigned << columns().at(2);
    if (col_x != -1)
        m_headers[col_x] = columns().at(3);
    else
        unassigned << columns().at(3);
    if (col_y != -1)
        m_headers[col_y] = columns().at(4);
    else
        unassigned << columns().at(4);
    if (col_rotation != -1)
        m_headers[col_rotation] = columns().at(5);
    else
        unassigned << columns().at(5);
    if (col_Side != -1)
        m_headers[col_Side] = columns().at(6);
    else
        unassigned << columns().at(6);

    for (int i=0; i<m_headers.size(); i++) {
        if (m_headers.at(i).isEmpty() && !unassigned.isEmpty())
            m_headers[i] = unassigned.takeFirst();
    }

    emit dataChanged( index(0,0), index(m_rowCount-1,m_columnCount-1) );
    emit headerDataChanged( Qt::Horizontal, 0, m_columnCount );
    emit headerDataChanged( Qt::Vertical, 0, m_rowCount );

    return true;
}

void Centroid::assignColumns( QList<QStringList> csv, QHash<QString,int> columns )
{
    QStringList colnames =this->columns();

    for (int r=0; r<csv.size(); r++) {
        CentroidLine line;

        int idx = columns.value(colnames[0],-1);
        if (idx >= 0)
            line.RefDes = csv.at(r).value(idx);

        idx = columns.value(colnames[1],-1);
        if (idx >= 0)
            line.Description = csv.at(r).value(idx);

        idx = columns.value(colnames[2],-1);
        if (idx >= 0)
            line.Value = csv.at(r).value(idx);

        idx = columns.value(colnames[3],-1);
        if (idx >= 0)
            line.x = csv.at(r).value(idx).toDouble();

        idx = columns.value(colnames[4],-1);
        if (idx >= 0)
            line.y = csv.at(r).value(idx).toDouble();

        idx = columns.value(colnames[5],-1);
        if (idx >= 0)
            line.rotation = csv.at(r).value(idx).toDouble();

        idx = columns.value(colnames[6],-1);
        if (idx >= 0)
            line.side = csv.at(r).value(idx);
    }
}



//
// QAbstractTableModel interface
//

int Centroid::rowCount(const QModelIndex &parent) const
{
    return m_rowCount;
}

int Centroid::columnCount(const QModelIndex &parent) const
{
    return m_columnCount;
}

QVariant Centroid::data( const QModelIndex& index, int role ) const
{
    int row = index.row();
    int col = index.column();

    if (!index.isValid() || row>=m_rowCount || col>=m_columnCount)
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    return m_data.at(row).value(col);
}

QVariant Centroid::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Vertical) {
        return section+1;
    } else {
        return m_headers.value(section);
    }
}






void Centroid::reassignColumn( int oldVisualIndex, int newVisualIndex )
{
    if (oldVisualIndex < 0 || newVisualIndex < 0)
        return;
    if (oldVisualIndex >= m_headers.size() || newVisualIndex >= m_headers.size())
        return;

    QStringList temp = m_headers;
    m_headers[newVisualIndex] = temp.value(oldVisualIndex);
    m_headers[oldVisualIndex] = temp.value(newVisualIndex);

    emit headerDataChanged( Qt::Horizontal, 0, m_columnCount );
}

QList<CentroidLine> Centroid::lines() const
{
    QList<CentroidLine> lines;

    int idx_RefDes = m_headers.indexOf( columns().value(0) );
    int idx_Desc   = m_headers.indexOf( columns().value(1) );
    int idx_Value  = m_headers.indexOf( columns().value(2) );
    int idx_x      = m_headers.indexOf( columns().value(3) );
    int idx_y      = m_headers.indexOf( columns().value(4) );
    int idx_rotation = m_headers.indexOf( columns().value(5) );
    int idx_Side   = m_headers.indexOf( columns().value(6) );

    foreach (QStringList data, m_data) {
        CentroidLine line;
        line.RefDes      = data.value( idx_RefDes );
        line.Description = data.value( idx_Desc );
        line.Value       = data.value( idx_Value );
        line.x           = data.value( idx_x ).toDouble(); // FIXME convert into mpq_class!
        line.y           = data.value( idx_y ).toDouble(); // FIXME convert into mpq_class!
        line.rotation    = data.value( idx_rotation ).toDouble(); // FIXME convert into mpq_class!
        line.side        = data.value( idx_Side );
        lines << line;
    }

    return lines;
}
