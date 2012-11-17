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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "csvimporter.h"
#include "centroiddialog.h"

#include <QtCore>
#include <QtGui>

#include <vtkRenderWindow.h>
#include <vtkCubeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkProperty.h>
#include <vtkPolygon.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkCellArray.h>
#include <vtkTriangleFilter.h>
#include <vtkCleanPolyData.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // enable status bar updates
    ui->graphicsView->setStatusBar( ui->statusBar );

    // setup QGraphics view
    ui->graphicsView->scale(1,-1);
    m_scene = new QGraphicsScene;
    ui->graphicsView->setScene(m_scene);

    // setup vtk view
    m_vtkRenderer = vtkRenderer::New();
    ui->qvtkWidget->GetRenderWindow()->AddRenderer( m_vtkRenderer );
    ui->qvtkWidget->GetRenderWindow()->Render();

    // setup tree view
    ui->treeWidget->setHeaderLabels( QStringList("Layer") );
    m_layerOutline = new QTreeWidgetItem(QStringList("Outline"));
    ui->treeWidget->addTopLevelItem( m_layerOutline );
    m_layerTop = new QTreeWidgetItem(QStringList("Top"));
    ui->treeWidget->addTopLevelItem( m_layerTop );
    m_layerBottom = new QTreeWidgetItem(QStringList("Bottom"));
    ui->treeWidget->addTopLevelItem( m_layerBottom );
    m_csv = new QTreeWidgetItem(QStringList("Pick&Place"));
    ui->treeWidget->addTopLevelItem( m_csv );
    m_layerUnknown = new QTreeWidgetItem(QStringList("Unknown"));
    ui->treeWidget->addTopLevelItem( m_layerUnknown );

    // setup defaults
    m_laminateHeight = 1.6; // 1.6 mm
    m_metalThickness = 35e-3; // metal thickness 35 um

}

MainWindow::~MainWindow()
{
    qDeleteAll(m_Centroid);
    m_Centroid.clear();
    delete m_scene;
    delete ui;
}

void MainWindow::on_actionImport_Gerber_triggered()
{
    QString filename = QFileDialog::getOpenFileName( this, "Select a file to import" );
    if (filename.isEmpty())
        return;

    QFileInfo fi(filename);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText( 0, QDir::toNativeSeparators(fi.fileName()) );
    item->setToolTip( 0, QDir::toNativeSeparators(filename) );

    if ((fi.suffix().toLower() == "csv") || (fi.suffix().toLower() == "mnt")) {
        // most likely Pick&Place file
        Centroid* centroid = new Centroid;
        bool ok = centroid->analyze( filename );
        if (!ok) {
            // automatic column determination failed; let the user assign the colunms
            CentroidDialog dlg;
            dlg.setCSV( centroid );
            dlg.exec();
        }

        QList<int> keys = m_Centroid.keys();
        int id = 0;
        if (!keys.isEmpty()) {
            qSort(keys);
            id = keys.last()+1;
        }
        m_Centroid[id] = centroid;
        item->setData( 0, Qt::UserRole, id );
        item->setData( 0, Qt::UserRole+1, Type_PickPlaceFile ); // Pick&Place file
        item->setData( 0, Qt::UserRole+2, filename );

        m_csv->addChild(item);
        updateView();
        return;
    }

    GerberImporter importer;
    bool ok = importer.import( filename );
    if (!ok)
        return;

    QList<int> keys = m_GerberImporter.keys();
    int id = 0;
    if (!keys.isEmpty()) {
        qSort(keys);
        id = keys.last()+1;
    }
    m_GerberImporter[id] = importer;
    item->setData( 0, Qt::UserRole, id );
    item->setData( 0, Qt::UserRole+1, Type_GerberFile ); // Gerber file

    if (fi.suffix().toLower() == "outline") {
        m_layerOutline->addChild(item);
    } else if ((fi.suffix().toLower() == "top") || (fi.suffix().toLower() == "positop") || (fi.suffix().toLower() == "cmp")) {
        m_layerTop->addChild(item);
    } else if ((fi.suffix().toLower() == "bot") || (fi.suffix().toLower() == "posibot") || (fi.suffix().toLower() == "sol")) {
        m_layerBottom->addChild(item);
    } else {
        m_layerUnknown->addChild(item);
    }

    //render( importer );
    updateView();
}


static vtkSmartPointer<vtkActor> AddClosedPoly( QList<QPointF> coords, double thickness )
{
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> poly = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkPolyData> profile = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkLinearExtrusionFilter> extrude = vtkSmartPointer<vtkLinearExtrusionFilter>::New();
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();

    for (int i=0; i<coords.size(); i++)
        points->InsertPoint( i, coords.at(i).x(), coords.at(i).y(), 0 );
    poly->InsertNextCell( coords.size() );
    for (int i=0; i<coords.size(); i++)
        poly->InsertCellPoint(i);
    profile->SetPoints( points );
    profile->SetPolys( poly );

    vtkSmartPointer<vtkCleanPolyData> clean = vtkSmartPointer<vtkCleanPolyData>::New();
    clean->SetInput( profile );

    vtkSmartPointer<vtkTriangleFilter> tf = vtkSmartPointer<vtkTriangleFilter>::New();
    tf->SetInputConnection( clean->GetOutputPort() );

    extrude->SetInputConnection( tf->GetOutputPort() );
    extrude->SetExtrusionTypeToVectorExtrusion();
    extrude->SetVector( 0,0,thickness );
    extrude->CappingOn();

    mapper->SetInputConnection( extrude->GetOutputPort() );
    actor->SetMapper( mapper );

    return actor;
}




void MainWindow::updateView()
{
    m_scene->clear();
    ui->graphicsView->update();

    // remove all actors from 3D scene
    m_vtkRenderer->RemoveAllViewProps();
    m_vtkRenderer->ResetCamera();
    ui->qvtkWidget->GetRenderWindow()->Render();

    if (m_layerOutline->childCount() > 0) {
        if (m_layerOutline->childCount() > 1)
            qDebug() << "currently only one outline supported";

        int id = m_layerOutline->child(0)->data(0,Qt::UserRole).toInt();
        GerberImporter& importer = m_GerberImporter[id];

        // create 2D outline
        render_2D( importer );

        // create 3D laminate

        QList<QPointF> outline;
        QPolygonF outline_poly = importer.getOutlineF();
        outline = outline_poly.toList();
        if (outline.size() >= 3) {
            vtkSmartPointer<vtkActor> actor2 = AddClosedPoly( outline, -m_laminateHeight );
            actor2->GetProperty()->SetColor( 0, 0.9, 0 );
            actor2->GetProperty()->SetOpacity( 0.4 );
            m_vtkRenderer->AddViewProp( actor2 );
        }
    }

    for (int i=0; i<m_layerTop->childCount(); i++) {
        int id = m_layerTop->child(i)->data(0,Qt::UserRole).toInt();
        GerberImporter& importer = m_GerberImporter[id];
        render( importer, 0, m_metalThickness );
    }

    for (int i=0; i<m_layerBottom->childCount(); i++) {
        int id = m_layerBottom->child(i)->data(0,Qt::UserRole).toInt();
        GerberImporter& importer = m_GerberImporter[id];
        render( importer, -m_laminateHeight - m_metalThickness, m_metalThickness );
    }

    for (int i=0; i<m_csv->childCount(); i++) {
        int id = m_csv->child(i)->data(0,Qt::UserRole).toInt();
        render_Centroid( id, 0, -m_laminateHeight, m_metalThickness );
    }

    // rescale 2D view
    ui->graphicsView->fitInView( m_scene->itemsBoundingRect(), Qt::KeepAspectRatio );

    // rescale 3D view
    m_vtkRenderer->ResetCamera();
    ui->qvtkWidget->GetRenderWindow()->Render();
}

void MainWindow::render( GerberImporter& importer, double zpos, double thickness )
{
    render_2D( importer );
    render_3D( importer, zpos, thickness );
}

void MainWindow::render_2D( GerberImporter& importer )
{
    //
    // draw 2D image
    //
    QList<Layer> layers = importer.getLayers();
    foreach (Layer layer, layers) {
        QList<Object*> objects = layer.getObjects();
        foreach (Object* object, objects) {
            m_scene->addItem( object->getGraphicsItem() );
        }
    }
}

void MainWindow::render_3D( GerberImporter& importer, double zpos, double thickness )
{
    //
    // draw 3D image
    //

    QList<Layer> layers = importer.getLayers();
    foreach (Layer layer, layers) {
        vtkSmartPointer<vtkAssembly> assembly = vtkSmartPointer<vtkAssembly>::New();
        vtkSmartPointer<vtkProp3D> prop3D = layer.getVtkProp3D( thickness );
        assembly->AddPart( prop3D );
        assembly->AddPosition( 0, 0, zpos );
        m_vtkRenderer->AddViewProp( assembly );
    }
}

void MainWindow::render_Centroid( int num, double zpos_top, double zpos_bottom, double thickness )
{
    if (!m_Centroid.contains(num))
        return;

    Centroid* centroid = m_Centroid.value(num);
    QList<CentroidLine> lines = centroid->lines();

    //
    // draw 2D
    //

    QGraphicsItemGroup* group = new QGraphicsItemGroup;
    group->setData(0,Type_PickPlaceFile);
    group->setData(1,num);

    // delete previous rendering
    QList<QGraphicsItem*> items = m_scene->items();
    QMutableListIterator<QGraphicsItem*> it(items);
    while (it.hasNext()) {
        QGraphicsItem* item = it.next();
        if (item && !item->data(0).isNull() && (item->data(0).toInt() == Type_PickPlaceFile) && (item->data(1).toInt() == num)) {
            delete item;
            break;
        }
    }

    for (int i=0; i<lines.size(); i++) {
        CentroidLine line = lines.at(i);

        qDebug() << line.RefDes << line.Description << line.Value << line.x.get_d() << line.y.get_d() << line.rotation.get_d();

        QRectF r;
        r.setSize( QSizeF(1,1) );
        r.moveCenter( QPointF(line.x.get_d(),line.y.get_d()) );
        QGraphicsEllipseItem *item = new QGraphicsEllipseItem(r);
        item->setPen( QPen("red") );
        item->setBrush( QBrush("red") );
        group->addToGroup( item );
    }

    m_scene->addItem( group );
}

void MainWindow::on_treeWidget_customContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem* item = ui->treeWidget->itemAt(pos);
    if (!item)
        return;
    if (!item->parent())
        return; // clicked at top level item

    int id = item->data(0,Qt::UserRole).toInt();
    Type type = (Type)item->data(0,Qt::UserRole+1).toInt();

    QMenu menu;
    menu.addAction( ui->actionRemove_File );
    QAction* actionProperties = menu.addAction( tr("Properties") );
    actionProperties->setEnabled( false );

    if (type == Type_PickPlaceFile) {
        actionProperties->setEnabled( true );
    }

    QAction* selected = menu.exec( ui->treeWidget->mapToGlobal(pos) );

    if (selected == actionProperties) {
        if (type == Type_PickPlaceFile) {
            CentroidDialog dlg;
            dlg.setCSV( m_Centroid.value(id) );
            dlg.exec();
            updateView();
        }
    }
}

void MainWindow::on_actionZoom_In_triggered()
{
    if (ui->tabWidget->currentIndex() == 0) {
        // 3D view activated
    } else {
        // 2D view activated
        ui->graphicsView->scale( 2, 2 );
    }
}

void MainWindow::on_actionZoom_Out_triggered()
{
    if (ui->tabWidget->currentIndex() == 0) {
        // 3D view activated
        m_vtkRenderer->GetActiveCamera();
    } else {
        // 2D view activated
        ui->graphicsView->scale( 0.5, 0.5 );
    }
}

void MainWindow::on_actionZoom_Fit_triggered()
{
    if (ui->tabWidget->currentIndex() == 0) {
        // 3D view activated
        m_vtkRenderer->ResetCamera();
        ui->qvtkWidget->GetRenderWindow()->Render();
    } else {
        // 2D view activated
        ui->graphicsView->fitInView( m_scene->itemsBoundingRect(), Qt::KeepAspectRatio );
    }
}

void MainWindow::on_actionExport_to_Freecad_triggered()
{
    QString filename = QFileDialog::getSaveFileName( this, "Export to Freecad Python script", QString(), "Python scripts (*.py)" );
    if (filename.isEmpty())
        return;

    QFileInfo fi(filename);
    QFile file(fi.absoluteFilePath());
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::information( this, "Export file", "opening file failed." );
        return;
    }
    QTextStream stream(&file);

    // write header
    stream << "# -*- coding: UTF-8 -*-" << endl;
    stream << endl;
    stream << "# start from freecad python console with:" << endl;
    stream << "# execfile( '" + fi.absoluteFilePath() + "' )" << endl;
    stream << "import Part, FreeCAD, math" << endl;
    stream << "from FreeCAD import Base" << endl;
    stream << endl;

    for (int i=0; i<m_csv->childCount(); i++) {
        render_Centroid_into_Freecad( stream, m_csv->child(i)->data(0,Qt::UserRole).toInt(), 0, -m_laminateHeight, m_metalThickness );
    }

}

void MainWindow::render_Centroid_into_Freecad( QTextStream& stream, int num, double zpos_top, double zpos_bottom, double thickness )
{
    if (!m_Centroid.contains(num))
        return;

    Centroid* centroid = m_Centroid.value(num);
    QList<CentroidLine> lines = centroid->lines();

    for (int i=0; i<lines.size(); i++) {
        CentroidLine line = lines.at(i);
        render_Centroid_into_Freecad_importItem( stream, line, zpos_top, zpos_bottom, thickness );
    }
}

//! \brief The heart of 3D component placement.
//! This function searches for the correct component (based on the information in Pick&Place data \c line).
//! It creates a stream of Freecad Python commands to import the component at the correct position and orientation.
void MainWindow::render_Centroid_into_Freecad_importItem( QTextStream& stream, CentroidLine line, double zpos_top, double zpos_bottom, double thickness )
{
    QString Description = line.Description; // this identified the component

    //stream << "Part.insert(\"/home/sebastian/src/pickAndPlaceMachine/rs-online.com/STEP AP214/17433.stp\",\"X1\")" << endl;

//    stream << "s = Part.Shape()" << endl;
//    stream << "s.read(\"/home/sebastian/src/pickAndPlaceMachine/rs-online.com/STEP AP214/17433.stp\")" << endl;
//    stream << "Part.show(s)" << endl;

    stream << "shapeobj = FreeCAD.activeDocument().addObject(\"Part::Feature\",\"X1\")" << endl;
    stream << "s = Part.Shape()" << endl;
    stream << "s.read(\"/home/sebastian/src/pickAndPlaceMachine/rs-online.com/STEP AP214/17433.stp\")" << endl;
    stream << "shapeobj.Shape = s" << endl;
    stream << "FreeCAD.activeDocument().recompute()" << endl;
}

void MainWindow::on_actionRemove_File_triggered()
{
    QTreeWidgetItem* item = ui->treeWidget->currentItem();
    if (!item || !item->parent())
        return;

    int id = item->data(0,Qt::UserRole).toInt();
    Type type = (Type)item->data(0,Qt::UserRole+1).toInt();

    switch (type) {
    case Type_PickPlaceFile:
    {
        item->parent()->removeChild(item);
        m_Centroid.remove(id);
        break;
    }
    case Type_GerberFile:
    {
        item->parent()->removeChild(item);
        m_GerberImporter.remove(id);
        break;
    }
    } // switch

    updateView();
}

void MainWindow::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (!current || !current->parent())
        ui->actionRemove_File->setEnabled( false );
    else
        ui->actionRemove_File->setEnabled( true );
}
