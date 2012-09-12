#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>
#include <QStatusBar>

class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit GraphicsView(QWidget *parent = 0);
    void setStatusBar( QStatusBar* statusBar );
    
signals:
    
public slots:
    
protected:
    virtual void mouseMoveEvent( QMouseEvent * event );
    QStatusBar* m_statusBar;
};

#endif // GRAPHICSVIEW_H
