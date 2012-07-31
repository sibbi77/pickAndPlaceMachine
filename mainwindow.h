#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vtkRenderer.h>

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
    vtkRenderer *m_vtkRenderer;
};

#endif // MAINWINDOW_H
