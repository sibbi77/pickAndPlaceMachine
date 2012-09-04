#ifndef CENTROIDDIALOG_H
#define CENTROIDDIALOG_H

#include "csvimporter.h"
#include <QDialog>

namespace Ui {
class CentroidDialog;
}

class CentroidDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit CentroidDialog(QWidget *parent = 0);
    ~CentroidDialog();
    void setCSV( QList<QStringList> csv, QHash<QString, int> *columnGuess = 0 );
    
private:
    Ui::CentroidDialog *ui;
};

#endif // CENTROIDDIALOG_H
