#include "centroid.h"

#include <QtCore>

Centroid::Centroid()
{
}

//! \brief Analyze the \c csv and try to determine the meaning of the columns.
bool Centroid::analyze( QList<QStringList> csv, QHash<QString, int> *columnGuess )
{
    QHash<QString,int> temp;
    if (columnGuess == 0)
        columnGuess = &temp;

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



    QStringList cols = columns();
    (*columnGuess)[cols[0]] = col_RefDes;
    (*columnGuess)[cols[1]] = col_Desc;
    (*columnGuess)[cols[2]] = col_Value;
    (*columnGuess)[cols[3]] = col_x;
    (*columnGuess)[cols[4]] = col_y;
    (*columnGuess)[cols[5]] = col_rotation;
    (*columnGuess)[cols[6]] = col_Side;

    // assign the determined columns
    assignColumns( csv, *columnGuess );

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
