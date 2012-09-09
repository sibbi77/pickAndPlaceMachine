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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
}

MainWindow::~MainWindow()
{
    qDeleteAll(m_Centroid);
    m_Centroid.clear();

    delete ui;
}

void MainWindow::on_actionImport_Gerber_triggered()
{
    QString filename = QFileDialog::getOpenFileName( this, "Select a file to import" );

    QFileInfo fi(filename);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText( 0, QDir::toNativeSeparators(fi.fileName()) );
    item->setToolTip( 0, QDir::toNativeSeparators(filename) );

    if (fi.suffix().toLower() == "csv") {
        // most likely Pick&Place file
        CSVImporter csvImporter;
        bool ok = csvImporter.import( filename );
        if (!ok) {
            QMessageBox::information( this, "open centroid (pick&place) file", "Cannot open csv file." );
            return;
        }
        Centroid* centroid = new Centroid;
        ok = centroid->analyze( csvImporter.csv() );
        if (!ok) {
            // automatic column determination failed; let the user assign the colunms
            QMessageBox::information( this, "open centroid (pick&place) file", "Manual column assignment." );
            CentroidDialog dlg;
            dlg.setCSV( centroid );
            dlg.exec();
            return;
        }

        m_Centroid << centroid;
        int id = m_Centroid.size() - 1;
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

    m_GerberImporter << importer;
    int id = m_GerberImporter.size() - 1;
    item->setData( 0, Qt::UserRole, id );
    item->setData( 0, Qt::UserRole+1, Type_GerberFile ); // Gerber file

    if (fi.suffix().toLower() == "outline") {
        m_layerOutline->addChild(item);
    } else if ((fi.suffix().toLower() == "top") || (fi.suffix().toLower() == "positop")) {
        m_layerTop->addChild(item);
    } else if ((fi.suffix().toLower() == "bot") || (fi.suffix().toLower() == "posibot")) {
        m_layerBottom->addChild(item);
    } else {
        m_layerUnknown->addChild(item);
    }

    //render( importer );
    updateView();
}

void MainWindow::updateView()
{
    m_scene->clear();
    ui->graphicsView->update();

    m_vtkRenderer->Clear();
    m_vtkRenderer->ResetCamera();
    ui->qvtkWidget->GetRenderWindow()->Render();


    double laminateHeight = 1.6e-3; // 1.6 mm
    double thickness = 35e-6; // metal thickness 35 um

    if (m_layerOutline->childCount() > 0) {
        if (m_layerOutline->childCount() > 1)
            qDebug() << "currently only one outline supported";

        GerberImporter& importer = m_GerberImporter[m_layerOutline->child(0)->data(0,Qt::UserRole).toInt()];

        QRectF dimensions = importer.getDimensionsF();

        // create the laminate
        vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
        double xmin = std::min( dimensions.left(), dimensions.right() );
        double xmax = std::max( dimensions.left(), dimensions.right() );
        double ymin = std::min( dimensions.bottom(), dimensions.top() );
        double ymax = std::max( dimensions.bottom(), dimensions.top() );
        cubeSource->SetBounds( xmin, xmax, ymin, ymax, -laminateHeight, 0 );
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(cubeSource->GetOutputPort());
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor( 0, 0.9, 0 );
        actor->GetProperty()->SetOpacity( 0.4 );
        m_vtkRenderer->AddViewProp( actor );
    }

    for (int i=0; i<m_layerTop->childCount(); i++) {
        GerberImporter& importer = m_GerberImporter[m_layerTop->child(i)->data(0,Qt::UserRole).toInt()];
        render( importer, 0, thickness );
    }

    for (int i=0; i<m_layerBottom->childCount(); i++) {
        GerberImporter& importer = m_GerberImporter[m_layerBottom->child(i)->data(0,Qt::UserRole).toInt()];
        render( importer, -laminateHeight - thickness, thickness );
    }

    for (int i=0; i<m_csv->childCount(); i++) {
        //Centroid* centroid = m_Centroid[m_csv->child(i)->data(0,Qt::UserRole).toInt()];
        render_Centroid( m_csv->child(i)->data(0,Qt::UserRole).toInt(), 0, -laminateHeight, thickness );
    }

    // rescale 2D view
    ui->graphicsView->fitInView( m_scene->sceneRect(), Qt::KeepAspectRatio );

    // rescale 3D view
    m_vtkRenderer->ResetCamera();
    ui->qvtkWidget->GetRenderWindow()->Render();
}

void MainWindow::render( GerberImporter& importer, double zpos, double thickness )
{
    //
    // draw 2D image
    //
    QList<Layer> layers = importer.getLayers();
    foreach (Layer layer, layers) {
        QList<Object*> objects = layer.getObjects();
        foreach (Object* object, objects) {
            m_scene->addItem( object->getGraphicsItem() );
//            ui->graphicsView->fitInView( scene->sceneRect(), Qt::KeepAspectRatio );
//            qApp->processEvents();
//            usleep( 100000 );
        }
    }

    //
    // draw 3D image
    //

    layers = importer.getLayers();
    foreach (Layer layer, layers) {
        vtkSmartPointer<vtkAssembly> assembly = vtkSmartPointer<vtkAssembly>::New();
        vtkSmartPointer<vtkProp3D> prop3D = layer.getVtkProp3D( thickness );
        assembly->AddPart( prop3D );
        assembly->AddPosition( 0, 0, zpos );
        m_vtkRenderer->AddViewProp( assembly );

//        QList<Object*> objects = layer.getObjects();
//        foreach (Object* object, objects) {
//            m_vtkRenderer->AddViewProp( object->getVtkActor() );
//        }
    }
}

void MainWindow::render_Centroid( int num, double zpos_top, double zpos_bottom, double thickness )
{
    if (num < 0 || num >= m_Centroid.size())
        return;

    Centroid* centroid = m_Centroid.at(num);
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
        r.setSize( QSizeF(1e-3,1e-3) );
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
    int id = item->data(0,Qt::UserRole).toInt();
    if (item->data(0,Qt::UserRole+1) == Type_PickPlaceFile) {
        // Pick&Place file
//        CSVImporter importer;
//        importer.import( item->data(0,Qt::UserRole+2).toString() );
        CentroidDialog dlg;
        dlg.setCSV( m_Centroid.at(id) );
        dlg.exec();
    }
}
