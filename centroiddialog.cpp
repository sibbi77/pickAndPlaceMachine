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
    Centroid::Unit unit = centroid->unit();
    if (unit == Centroid::UnitMm)
        ui->comboBox->setCurrentIndex(0);
    else if (unit == Centroid::UnitInch)
        ui->comboBox->setCurrentIndex(1);
    else
        ui->comboBox->setCurrentIndex(2);
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

void CentroidDialog::on_buttonBox_accepted()
{
    Centroid::Unit unit = Centroid::UnitMils;
    if (ui->comboBox->currentIndex() == 0)
        unit == Centroid::UnitMm;
    else if (ui->comboBox->currentIndex() == 1)
        unit == Centroid::UnitInch;

    m_model->setUnit( unit );
}
