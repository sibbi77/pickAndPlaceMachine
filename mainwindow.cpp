#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gerberimporter.h"

#include <QtCore>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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
