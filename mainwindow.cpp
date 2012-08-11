#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    m_layerUnknown = new QTreeWidgetItem(QStringList("Unknown"));
    ui->treeWidget->addTopLevelItem( m_layerUnknown );

//    vtkCubeSource* cube = vtkCubeSource::New();
//    vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
//    mapper->SetInputConnection( cube->GetOutputPort() );
//    vtkActor* actor = vtkActor::New();
//    actor->SetMapper( mapper );
//    m_vtkRenderer->AddViewProp( actor );
//    m_vtkRenderer->ResetCamera();
//    ui->qvtkWidget->GetRenderWindow()->Render();






//    // TEST
//    QGraphicsScene* scene = new QGraphicsScene;
//    QPainterPath path;
//    path.addEllipse( QPointF(0,0), 10, 10 );
//    path.addEllipse( QPointF(1,1), 5, 5 );
//    QGraphicsPathItem* item = new QGraphicsPathItem(path);
//    item->setBrush( item->pen().color() );
//    scene->addItem( item );

//    ui->graphicsView->setScene(scene);
//    ui->graphicsView->fitInView( scene->sceneRect(), Qt::KeepAspectRatio );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionImport_Gerber_triggered()
{
    GerberImporter importer;
    QString filename = QFileDialog::getOpenFileName( this, "Select gerber file to import" );

    bool ok = importer.import( filename );
    if (!ok)
        return;

    m_GerberImporter << importer;
    int id = m_GerberImporter.size() - 1;

    QFileInfo fi(filename);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText( 0, QDir::toNativeSeparators(fi.fileName()) );
    item->setToolTip( 0, QDir::toNativeSeparators(filename) );
    item->setData( 0, Qt::UserRole, id );

    if (fi.suffix().toLower() == "outline") {
        m_layerOutline->addChild(item);
    } else if (fi.suffix().toLower() == "top") {
        m_layerTop->addChild(item);
    } else if (fi.suffix().toLower() == "bot") {
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

    ui->graphicsView->fitInView( m_scene->sceneRect(), Qt::KeepAspectRatio );

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

    m_vtkRenderer->ResetCamera();
    ui->qvtkWidget->GetRenderWindow()->Render();
}
