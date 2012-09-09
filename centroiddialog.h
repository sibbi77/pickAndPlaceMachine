#ifndef CENTROIDDIALOG_H
#define CENTROIDDIALOG_H

#include "centroid.h"
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
    void setCSV( Centroid *centroid );
    
private:
    Ui::CentroidDialog *ui;

protected:
    Centroid* m_model;

protected slots:
    void onSectionMoved(  int logicalIndex, int oldVisualIndex, int newVisualIndex );
private slots:
    void on_buttonBox_accepted();
};

#endif // CENTROIDDIALOG_H
