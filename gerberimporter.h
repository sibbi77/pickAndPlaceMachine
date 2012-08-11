#ifndef GERBERIMPORTER_H
#define GERBERIMPORTER_H

#include <QString>
#include <QStringList>
#include <gmpxx.h>
#include <QGraphicsItem>

#include <vtkAssembly.h>
#include <vtkSmartPointer.h>

enum Unit {mm,in};

//! \internal
class ApertureMacro_internalRep
{
public:
    enum Operator {value,plus,minus,mul,div};

public:
    ApertureMacro_internalRep();
    ApertureMacro_internalRep( mpq_class value );
    ApertureMacro_internalRep( Operator op );

    mpq_class m_value;
    Operator m_operator;
};

class ApertureMacro
{
public:
    ApertureMacro();
    ApertureMacro( QStringList primitives );

    QList<QList<mpq_class> > calc( QList<mpq_class> arguments ) const;

protected:
    QStringList m_primitives;

    QList<ApertureMacro_internalRep> calc_convertIntoInternalRep( QString term, QList<mpq_class> arguments ) const;
    QList<ApertureMacro_internalRep> calc_canonicalize( QList<ApertureMacro_internalRep> temp ) const;
    mpq_class calc_processInternalRep( QList<ApertureMacro_internalRep> temp ) const;
    //mpq_class calc_intern1( QString term, QList<mpq_class> arguments ) const;
};






class Aperture
{
public:
    enum Type {invalid,circle,rectangle,oval,polygon,macro};
    enum HoleType {noHole,circularHole,rectangularHole};

public:
    Aperture();
    Aperture( Unit unit );
    void setCircle( mpq_class diameter, mpq_class x_hole_dimension = -1, mpq_class y_hole_dimension = -1 );
    void setCircle( QList<mpq_class> arguments );
    void setOval( mpq_class x_length, mpq_class y_length, mpq_class x_hole_dimension = -1, mpq_class y_hole_dimension = -1 );
    void setOval( QList<mpq_class> arguments );
    void setRectangle( QList<mpq_class> arguments );
    void setPolygon( QList<mpq_class> arguments );
    void setMacro( ApertureMacro macro, QList<mpq_class> arguments );
    Type type() const {return m_type;}
    mpq_class diameter() const {return m_arguments.value(0);}

    QGraphicsItem* getGraphicsItem() const;

protected:
    Type m_type;
    HoleType m_hole;
    QList<mpq_class> m_arguments;
    ApertureMacro m_macro;
    Unit m_unit;
    mpq_class m_unitFactor;
};

class Object
{
public:
    Object();
    virtual QGraphicsItem* getGraphicsItem() const;
    virtual vtkSmartPointer<vtkProp> getVtkProp(double thickness) const;
};

class Line : public Object
{
public:
    Line( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, Aperture aperture );
    virtual QGraphicsItem* getGraphicsItem() const;
    virtual vtkSmartPointer<vtkProp> getVtkProp( double thickness ) const;

protected:
    mpq_class m_x1, m_y1, m_x2, m_y2;
    Aperture m_aperture;
};

class Line_cw_ccw : public Object
{
public:
    Line_cw_ccw( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, mpq_class i, mpq_class j, Aperture aperture );
    virtual QGraphicsItem* getGraphicsItem( bool ccw ) const;

protected:
    mpq_class m_x1, m_y1, m_x2, m_y2, m_i, m_j;
    Aperture m_aperture;
};

class Line_cw : public Line_cw_ccw
{
public:
    Line_cw( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, mpq_class i, mpq_class j, Aperture aperture );
    virtual QGraphicsItem* getGraphicsItem() const;
};

class Line_ccw : public Line_cw_ccw
{
public:
    Line_ccw( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, mpq_class i, mpq_class j, Aperture aperture );
    virtual QGraphicsItem* getGraphicsItem() const;
};

class Flash : public Object
{
public:
    Flash( mpq_class x, mpq_class y, Aperture aperture );
    virtual QGraphicsItem* getGraphicsItem() const;

protected:
    mpq_class m_x, m_y;
    Aperture m_aperture;
};

class FilledOutline : public Object
{
public:
    FilledOutline( QList<QPair<mpq_class, mpq_class> > points );
    virtual QGraphicsItem* getGraphicsItem() const;
    virtual vtkSmartPointer<vtkProp> getVtkProp( double thickness ) const;

protected:
    QList< QPair<mpq_class,mpq_class> > m_points;
};

class Layer
{
public:
    enum ImagePolarity {positive, negative};
    enum DrawMode {on,off,flash};
    enum OutlineFillMode {fillOn,fillOff};
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
    void draw( mpq_class x, mpq_class y, mpq_class i, mpq_class j, Unit unit );
    void setX( mpq_class x ) {m_current_x = x;}
    mpq_class x() const {return m_current_x;}
    void setY( mpq_class y ) {m_current_y = y;}
    mpq_class y() const {return m_current_y;}
    void defineAperture( int num, Aperture aperture );
    void startOutlineFill();
    void stopOutlineFill();

    QList<Object*> getObjects() const {return m_objects;}
    vtkSmartPointer<vtkProp> getVtkProp( double thickness ) const;

protected:
    mpq_class m_current_x;
    mpq_class m_current_y;
    ImagePolarity m_imagePolarity;
    QString m_name;
    QList<Object*> m_objects;
    InterpolationMode m_interpolationMode;
    DrawMode m_drawMode;
    int m_aperture;

    OutlineFillMode m_outlineFillMode;
    QList< QPair<mpq_class,mpq_class> > m_outlineFillPoints;

    QHash<int,Aperture> m_apertures; //!< Layer specific apertures

    void drawFilledOutline( QList< QPair<mpq_class,mpq_class> > points );
};

class GerberImporter
{
public:
    GerberImporter();

    bool import( QString filename );
    QList<Layer>& getLayers() {return m_layers;}
    QRectF getDimensionsF() const;

    static mpq_class mpq_from_decimal_string( QString decimal_str, bool *conversion_ok = 0, int *pos_after_number = 0 );

protected:
    bool processDataBlock( QString dataBlock );
    void processParameterBlock( QString parameterBlock, bool finished );
    void parameterFS( QString parameterBlock );
    void parameterMO( QString parameterBlock );
    void parameterLN( QString parameterBlock );
    void parameterAD( QString parameterBlock );
    void parameterAM( QString collect_parameter_AM );
    void drawG01( QString dataBlock );
    void drawG02( QString dataBlock );
    void drawG03( QString dataBlock );
    void draw( QString dataBlock );
    void setDCode( QString dataBlock );
    void startOutlineFill();
    void stopOutlineFill();
    mpq_class makeCoordinate( QString str );

    Layer& newLayer();
    Layer& currentLayer();

    QStringList m_deprecated_parameters;

    QList<Layer> m_layers;
    QHash<int,Aperture> m_apertures; //!< global defined apertures; available to all layers
    QHash<QString,ApertureMacro> m_apertureMacros;

    // current graphics state
    int m_FS_integer, m_FS_decimals, m_FS_decimals10;
    enum {omit_leading, omit_trailing} m_FS_zero;
    Unit m_MO;

    int m_currentAperture; //!< 10-999: aperture
};

#endif // GERBERIMPORTER_H
