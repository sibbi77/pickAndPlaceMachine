#include "centroid.h"
#include "centroiddialog.h"
#include "ui_centroiddialog.h"

CentroidDialog::CentroidDialog(QWidget *parent) : QDialog(parent), ui(new Ui::CentroidDialog)
{
    ui->setupUi(this);
}

CentroidDialog::~CentroidDialog()
{
    delete ui;
}

void CentroidDialog::setCSV( QList<QStringList> csv, QHash<QString, int> *columnGuess )
{
    int columnCount = 0;
    int rowCount = csv.size();
    for (int r=0; r<rowCount; r++)
        columnCount = std::max(columnCount,csv.at(r).size());
    ui->tableWidget->setColumnCount(columnCount);
    ui->tableWidget->setRowCount(rowCount);
    ui->tableWidget->setHorizontalHeaderLabels( Centroid::columns() );

    QStringList verticalHeaders;
    for (int r=0; r<rowCount; r++) {
        verticalHeaders << QString::number(r);

        for (int c=0; c<columnCount; c++) {
            QTableWidgetItem *item = new QTableWidgetItem;
            item->setText( csv.at(r).value(c) );
            ui->tableWidget->setItem( r, c, item );
        }
    }
    ui->tableWidget->setVerticalHeaderLabels( verticalHeaders );
}
