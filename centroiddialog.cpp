/*
    This file is part of pickAndPlaceMachine.

    pickAndPlaceMachine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    pickAndPlaceMachine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pickAndPlaceMachine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "centroiddialog.h"
#include "ui_centroiddialog.h"

#include <QtCore>

CentroidDialog::CentroidDialog(QWidget *parent) : QDialog(parent), ui(new Ui::CentroidDialog)
{
    ui->setupUi(this);

    m_model = 0;

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
    if (!centroid)
        return;

    m_model = centroid;
    ui->tableView->setModel( m_model );
    Centroid::Unit unit = centroid->unit();
    if (unit == Centroid::UnitMm)
        ui->comboBox->setCurrentIndex(0);
    else if (unit == Centroid::UnitInch)
        ui->comboBox->setCurrentIndex(1);
    else
        ui->comboBox->setCurrentIndex(2);

    ui->combo_separator->blockSignals(true);
    ui->combo_separator->setEditText( m_model->separator() );
    ui->combo_separator->blockSignals(false);
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
        unit = Centroid::UnitMm;
    else if (ui->comboBox->currentIndex() == 1)
        unit = Centroid::UnitInch;

    m_model->setUnit( unit );

    accept();
}

void CentroidDialog::on_combo_separator_currentIndexChanged(const QString &arg1)
{
    if (arg1.isEmpty() || (arg1.size() > 1))
        return;

    m_model->setSeparator( arg1 );
    m_model->analyze( m_model->filename() );
    setCSV( m_model );
}
