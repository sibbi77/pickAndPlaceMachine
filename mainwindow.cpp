#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gerberimporter.h"

#include <QtCore>
#include <QtGui>

#include <vtkRenderWindow.h>
#include <vtkCubeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // setup vtk view
    m_vtkRenderer = vtkRenderer::New();
    ui->qvtkWidget->GetRenderWindow()->AddRenderer( m_vtkRenderer );
    ui->qvtkWidget->GetRenderWindow()->Render();

    vtkCubeSource* cube = vtkCubeSource::New();
    vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
    mapper->SetInputConnection( cube->GetOutputPort() );
    vtkActor* actor = vtkActor::New();
    actor->SetMapper( mapper );
    m_vtkRenderer->AddViewProp( actor );
    m_vtkRenderer->ResetCamera();
    ui->qvtkWidget->GetRenderWindow()->Render();
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

    QGraphicsScene* scene = new QGraphicsScene;

    ui->tabWidget->setCurrentIndex(1);
    QList<Layer> layers = importer.getLayers();
    foreach (Layer layer, layers) {
        QList<Object*> objects = layer.getObjects();
        foreach (Object* object, objects) {
            scene->addItem( object->getGraphicsItem() );
        }
    }

    ui->graphicsView->setScene(scene);
    ui->graphicsView->fitInView( scene->sceneRect(), Qt::KeepAspectRatio );
}
