#include "graphicsview.h"
#include <QtCore>
#include <QtGui>

GraphicsView::GraphicsView(QWidget *parent) : QGraphicsView(parent)
{
    m_statusBar = 0;
    setMouseTracking( true );
}

void GraphicsView::setStatusBar( QStatusBar* statusBar )
{
    m_statusBar = statusBar;
}

void GraphicsView::mouseMoveEvent( QMouseEvent * event )
{
    if (m_statusBar) {
        QPointF p = mapToScene( event->pos() );
        m_statusBar->showMessage( QString("(%1,%2)").arg(p.x()).arg(p.y()) );
    }
}
