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
