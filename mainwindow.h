#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>

#include <vtkRenderer.h>

#include "gerberimporter.h"
#include "centroid.h"
#include "excellonimporter.h"

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
    void on_actionImport_triggered();

    void on_treeWidget_customContextMenuRequested(const QPoint &pos);

    void on_actionZoom_In_triggered();

    void on_actionZoom_Out_triggered();

    void on_actionZoom_Fit_triggered();

    void on_actionExport_to_Freecad_triggered();

    void on_actionRemove_File_triggered();

    void on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    void on_actionImportGerber_triggered();

    void on_actionImportCentroid_triggered();

    void on_actionImportExcellon_triggered();

private:
    Ui::MainWindow *ui;

protected:
    enum Type {Type_GerberFile, Type_PickPlaceFile, Type_ExcellonFile};

    QGraphicsScene* m_scene;

    vtkRenderer *m_vtkRenderer;
    QHash<int,GerberImporter> m_GerberImporter; //!< int: unique id (used in treeWidget) to refer to a GerberImporter
    QHash<int,Centroid*> m_Centroid; //!< int: unique id (used in treeWidget) to refer to a Centroid file
    QHash<int,ExcellonImporter*> m_Excellon; //!< int: unique id (used in treeWidget) to refer to an Excellon file
    QTreeWidgetItem *m_treeLayerOutline, *m_treeLayerTop, *m_treeLayerBottom, *m_treeCentroid, *m_treeExcellon, *m_treeUnknown;

    double m_laminateHeight, m_metalThickness;

    void updateView();
    void render( GerberImporter& importer, double zpos, double thickness );
    void render_2D( GerberImporter& importer );
    void render_3D( GerberImporter& importer, double zpos, double thickness );
    void render_Centroid( int num, double zpos_top, double zpos_bottom, double thickness );
    void render_Excellon( int num, double zpos_top, double zpos_bottom, double thickness );
    void render_Centroid_into_Freecad( QTextStream& stream, int num, double zpos_top, double zpos_bottom, double thickness );
    void render_Centroid_into_Freecad_importItem( QTextStream& stream, CentroidLine line, double zpos_top, double zpos_bottom, double thickness );
    bool import_Centroid( QString filename );
    bool import_Excellon( QString filename );
    bool import_Gerber( QString filename );
};

#endif // MAINWINDOW_H
