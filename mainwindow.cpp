#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gerberimporter.h"

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

    // setup vtk view
    m_vtkRenderer = vtkRenderer::New();
    ui->qvtkWidget->GetRenderWindow()->AddRenderer( m_vtkRenderer );
    ui->qvtkWidget->GetRenderWindow()->Render();

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

    // gerber import finished

    //
    // draw 2D image
    //
    QGraphicsScene* scene = new QGraphicsScene;
    ui->graphicsView->setScene(scene);

    ui->tabWidget->setCurrentIndex(1);
    QList<Layer> layers = importer.getLayers();
    foreach (Layer layer, layers) {
        QList<Object*> objects = layer.getObjects();
        foreach (Object* object, objects) {
            scene->addItem( object->getGraphicsItem() );
//            ui->graphicsView->fitInView( scene->sceneRect(), Qt::KeepAspectRatio );
//            qApp->processEvents();
//            usleep( 100000 );
        }
    }

    ui->graphicsView->fitInView( scene->sceneRect(), Qt::KeepAspectRatio );

    //
    // draw 3D image
    //
    QRectF dimensions = importer.getDimensionsF();

    // create the laminate
    vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
    double height = 1.6e-3; // 1.6 mm
    double xmin = std::min( dimensions.left(), dimensions.right() );
    double xmax = std::max( dimensions.left(), dimensions.right() );
    double ymin = std::min( dimensions.bottom(), dimensions.top() );
    double ymax = std::max( dimensions.bottom(), dimensions.top() );
    qDebug() << xmin << xmax << ymin << ymax << -height << 0;
    cubeSource->SetBounds( xmin, xmax, ymin, ymax, -height, 0 );
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(cubeSource->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor( 0, 0.9, 0 );
    m_vtkRenderer->AddViewProp( actor );

    layers = importer.getLayers();
    foreach (Layer layer, layers) {
        m_vtkRenderer->AddViewProp( layer.getVtkProp( 35e-6 ) );
//        QList<Object*> objects = layer.getObjects();
//        foreach (Object* object, objects) {
//            m_vtkRenderer->AddViewProp( object->getVtkActor() );
//        }
    }

    m_vtkRenderer->ResetCamera();
    ui->qvtkWidget->GetRenderWindow()->Render();
}
