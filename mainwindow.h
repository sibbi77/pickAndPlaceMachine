#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>

#include <vtkRenderer.h>
#include "gerberimporter.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_actionImport_Gerber_triggered();

private:
    Ui::MainWindow *ui;

protected:
    QGraphicsScene* m_scene;

    vtkRenderer *m_vtkRenderer;
    QList<GerberImporter> m_GerberImporter;
    QTreeWidgetItem *m_layerOutline, *m_layerTop, *m_layerBottom, *m_layerUnknown;

    void updateView();
    void render( GerberImporter& importer, double zpos, double thickness );
};

#endif // MAINWINDOW_H
