#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>

#include <vtkRenderer.h>

#include "gerberimporter.h"
#include "centroid.h"

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

    void on_treeWidget_customContextMenuRequested(const QPoint &pos);

    void on_actionZoom_In_triggered();

    void on_actionZoom_Out_triggered();

    void on_actionZoom_Fit_triggered();

    void on_actionExport_to_Freecad_triggered();

    void on_actionRemove_File_triggered();

    void on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    Ui::MainWindow *ui;

protected:
    enum Type {Type_GerberFile, Type_PickPlaceFile};

    QGraphicsScene* m_scene;

    vtkRenderer *m_vtkRenderer;
    QHash<int,GerberImporter> m_GerberImporter; //!< int: unique id (used in treeWidget) to refer to a GerberImporter
    QHash<int,Centroid*> m_Centroid; //!< int: unique id (used in treeWidget) to refer to a Centroid file
    QTreeWidgetItem *m_layerOutline, *m_layerTop, *m_layerBottom, *m_csv, *m_layerUnknown;

    double m_laminateHeight, m_metalThickness;

    void updateView();
    void render( GerberImporter& importer, double zpos, double thickness );
    void render_2D( GerberImporter& importer );
    void render_3D( GerberImporter& importer, double zpos, double thickness );
    void render_Centroid( int num, double zpos_top, double zpos_bottom, double thickness );
    void render_Centroid_into_Freecad( QTextStream& stream, int num, double zpos_top, double zpos_bottom, double thickness );
    void render_Centroid_into_Freecad_importItem( QTextStream& stream, CentroidLine line, double zpos_top, double zpos_bottom, double thickness );
};

#endif // MAINWINDOW_H
