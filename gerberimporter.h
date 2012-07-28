#ifndef GERBERIMPORTER_H
#define GERBERIMPORTER_H

#include <QString>
#include <QStringList>
#include <gmpxx.h>
#include <QGraphicsItem>

class Aperture
{
public:
    Aperture();
};

class Object
{
public:
    Object();
    virtual QGraphicsItem* getGraphicsItem() const;
};

class Line : public Object
{
public:
    Line( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, int aperture );
    virtual QGraphicsItem* getGraphicsItem() const;

protected:
    mpq_class m_x1, m_y1, m_x2, m_y2;
    int m_aperture;
};

class Layer
{
public:
    enum ImagePolarity {positive, negative};
    enum DrawMode {on,off,flash};
    enum InterpolationMode {linear,clockwise,counterclockwise};

public:
    Layer( QHash<int,Aperture> apertures );
    ~Layer();

    void setImagePolarity( ImagePolarity p ) {m_imagePolarity = p;}
    ImagePolarity imagePolarity() const {return m_imagePolarity;}
    void setName( QString name ) {m_name = name;}
    QString name() const {return m_name;}
    bool isEmpty() const {return m_objects.isEmpty();}
    void setInterpolationMode( InterpolationMode interpolationMode ) {m_interpolationMode = interpolationMode;}
    InterpolationMode interpolationMode() const {return m_interpolationMode;}
    void setDrawMode( DrawMode drawMode ) {m_drawMode = drawMode;}
    DrawMode drawMode() const {return m_drawMode;}
    void setAperture( int aperture ) {m_aperture = aperture;}
    int aperture() const {return m_aperture;}
    void draw( mpq_class x, mpq_class y );
    void setX( mpq_class x ) {m_current_x = x;}
    mpq_class x() const {return m_current_x;}
    void setY( mpq_class y ) {m_current_y = y;}
    mpq_class y() const {return m_current_y;}
    void defineAperture( int num, Aperture aperture );

    QList<Object*> getObjects() const {return m_objects;}

protected:
    mpq_class m_current_x;
    mpq_class m_current_y;
    ImagePolarity m_imagePolarity;
    QString m_name;
    QList<Object*> m_objects;
    InterpolationMode m_interpolationMode;
    DrawMode m_drawMode;
    int m_aperture;

    QHash<int,Aperture> m_apertures; //!< Layer specific apertures
};

class GerberImporter
{
public:
    GerberImporter();

    bool import( QString filename );
    QList<Layer>& getLayers() {return m_layers;}

protected:
    bool processDataBlock( QString dataBlock );
    void processParameterBlock( QString parameterBlock );
    void parameterFS( QString parameterBlock );
    void parameterMO( QString parameterBlock );
    void parameterLN( QString parameterBlock );
    void parameterAD( QString parameterBlock );
    void drawG01( QString dataBlock );
    void drawG02( QString dataBlock );
    void drawG03( QString dataBlock );
    void draw( QString dataBlock );
    void setDCode( QString dataBlock );
    mpq_class makeCoordinate( QString str );

    Layer& newLayer();
    Layer& currentLayer();

    QStringList m_deprecated_parameters;

    QList<Layer> m_layers;
    QHash<int,Aperture> m_apertures; //!< global defined apertures; available to all layers

    // current graphics state
    int m_FS_integer, m_FS_decimals, m_FS_decimals10;
    enum {omit_leading, omit_trailing} m_FS_zero;
    enum {mm,in} m_MO;

    int m_currentAperture; //!< 10-999: aperture
};

#endif // GERBERIMPORTER_H
