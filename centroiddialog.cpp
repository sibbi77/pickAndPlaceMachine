#include "centroiddialog.h"
#include "ui_centroiddialog.h"

#include <QtCore>

CentroidDialog::CentroidDialog(QWidget *parent) : QDialog(parent), ui(new Ui::CentroidDialog)
{
    ui->setupUi(this);

    // enable assignment of column headers to columns
    ui->tableView->horizontalHeader()->setMovable( true );
    ui->tableView->horizontalHeader()->setDropIndicatorShown( true );
    connect( ui->tableView->horizontalHeader(), SIGNAL(sectionMoved(int,int,int)), SLOT(onSectionMoved(int,int,int)) );
}

CentroidDialog::~CentroidDialog()
{
    delete ui;
}

void CentroidDialog::setCSV( Centroid* centroid )
{
    m_model = centroid;
    ui->tableView->setModel( m_model );
//    m_model.setCSV( csv, columnGuess );
}

void CentroidDialog::onSectionMoved( int logicalIndex, int oldVisualIndex, int newVisualIndex )
{
    // the view already rearranged the column => revert the movement of the column and move the header alone
    static bool inside = false;

    if (inside)
        qDebug() << "inside" << logicalIndex << oldVisualIndex << newVisualIndex;
    else
        qDebug() << "outside" << logicalIndex << oldVisualIndex << newVisualIndex;

    if (!inside) {
        inside = true;
        ui->tableView->horizontalHeader()->moveSection( newVisualIndex, oldVisualIndex );
        inside = false;
        m_model->reassignColumn( oldVisualIndex, newVisualIndex );
    }
}
