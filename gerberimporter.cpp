#include "gerberimporter.h"

#include <QtCore>
#include <QtGui>
#include <cmath>

#include <vtkCubeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkProperty.h>
#include <vtkCylinderSource.h>
#include <vtkPropAssembly.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkTriangleFilter.h>
#include <vtkCellArray.h>
#include <vtkCleanPolyData.h>


GerberImporter::GerberImporter()
{
    m_deprecated_parameters << "AS" << "MI" << "OF" << "SF" << "IP" << "IR" << "KO" << "SM";

    m_FS_integer = -1;
    m_FS_decimals = -1;
    m_FS_decimals10 = 0;
    m_FS_zero = omit_leading;
    m_MO = in;
}

bool GerberImporter::import( QString filename )
{
    if (filename.isEmpty())
        return false;

    QFile file( filename );
    if (!file.open(QFile::ReadOnly))
        return false;

    QTextStream stream(&file);
    QString line;
    bool finished = false;
    enum {dataBlock,parameterBlock} state = dataBlock;
    while (!finished) {
        int idx1 = line.indexOf('%');
        int idx2 = line.indexOf('*');
        if ((idx2 != -1) && ((idx2 < idx1) || (idx1 == -1))) {
            // complete data block found
            QString block = line.left(idx2+1);
            line.remove( 0, idx2+1 );
            if (state == dataBlock) {
                if (processDataBlock( block ) == false)
                    return true;
            } else
                processParameterBlock( block, false );
            continue;
        }
        if (idx1 != -1) {
            // start/end of parameter block found
            line.remove( 0, idx1+1 ); // remove garbage
            if (state == dataBlock)
                state = parameterBlock;
            else {
                // end of parameter block
                processParameterBlock( QString(), true ); // notify processParameterBlock() of end of block
                state = dataBlock;
            }
            continue;
        }
        QString temp = stream.readLine();
        if (temp.isEmpty() && stream.atEnd())
            finished = true;
        line += temp;
    }

    return true;
}

bool GerberImporter::processDataBlock( QString dataBlock )
{
    dataBlock = dataBlock.trimmed();
//    qDebug() << "dataBlock:" << dataBlock;

    if (dataBlock.left(3) == "M02")
        return false; // end of file
    else if (dataBlock.left(3) == "G04")
        qDebug() << "Comment:" << dataBlock.mid(3,dataBlock.length()-4);
    else if (dataBlock.left(3) == "G01")
        drawG01( dataBlock );
    else if (dataBlock.left(3) == "G02")
        drawG02( dataBlock );
    else if (dataBlock.left(3) == "G03")
        drawG03( dataBlock );
    else if (dataBlock.left(1) == "D")
        setDCode( dataBlock );
    else if (dataBlock.left(3) == "G36")
        currentLayer().startOutlineFill();
    else if (dataBlock.left(3) == "G37")
        currentLayer().stopOutlineFill();
    else if (dataBlock.left(3) == "G54")
        setDCode( dataBlock.mid(3) );
    else if ((dataBlock.left(1) == "X") || (dataBlock.left(1) == "Y"))
        draw( dataBlock );

    return true; // go on with processing
}

//! \brief
void GerberImporter::processParameterBlock( QString parameterBlock, bool finished )
{
    static QString collect_parameter_AM;

    if (!collect_parameter_AM.isEmpty()) {
        // what a pitty!
        // the pieces of the AM parameter are delimited by '*'
        // this leads to multiple invokations of this (processParameterBlock) function
        // we need to collect all these pieces
        if (!finished) {
            collect_parameter_AM += parameterBlock;
            return;
        } else {
            // the parameter block is finished
            collect_parameter_AM += parameterBlock;
            parameterAM( collect_parameter_AM );
            collect_parameter_AM.clear();
            return;
        }
    }

    parameterBlock = parameterBlock.trimmed();
//    qDebug() << "parameterBlock:" << parameterBlock;

    //parameterBlock = parameterBlock.left(2).toUpper() + parameterBlock.remove(0,2);
    parameterBlock = parameterBlock.toUpper();

    if (m_deprecated_parameters.contains(parameterBlock.left(2))) {
        qDebug() << "deprecated Parameter. IGNORING.";
        return;
    }

    if (parameterBlock.left(2) == "FS")
        parameterFS( parameterBlock );
    else if (parameterBlock.left(2) == "MO")
        parameterMO( parameterBlock );
    else if (parameterBlock.left(2) == "LN")
        parameterLN( parameterBlock );
    else if (parameterBlock.left(3) == "ADD")
        parameterAD( parameterBlock );
    else if (parameterBlock.left(2) == "AM") {
        // what a pitty!
        // the pieces of the AM parameter are delimited by '*'
        // this leads to multiple invokations of this (processParameterBlock) function
        // we need to collect all these pieces
        collect_parameter_AM = parameterBlock;
    }
}

//! \brief Define the number format.
void GerberImporter::parameterFS( QString parameterBlock )
{
    if (m_FS_integer != -1) {
        // only one times allowed
        qDebug() << "FS specified multiple times";
        return;
    }
    int pos=2;
    if (parameterBlock.mid(pos,1) == "L") {
        m_FS_zero = omit_leading;
        pos++;
    } else if (parameterBlock.mid(pos,1) == "T") {
        m_FS_zero = omit_trailing;
        pos++;
    } else {
        qDebug() << "FS error; neither omit leading zeros nor omit trailing zeros specified. Defaulting to omit leading zeros.";
        m_FS_zero = omit_leading;
//        return;
    }
    if (parameterBlock.mid(pos,1) == "A")
        ;
    else if (parameterBlock.mid(pos,1) == "I") {
        qDebug() << "FS I unsupported";
        return;
    } else {
        qDebug() << "FS error";
        return;
    }
    pos++;
    if (parameterBlock.mid(pos,1) == "N") {
        qDebug() << "FS N unsupported";
        pos += 2;
    }
    if (parameterBlock.mid(pos,1) == "G") {
        qDebug() << "FS G unsupported";
        pos += 2;
    }
    if (parameterBlock.mid(pos,1) == "X") {
        bool ok;
        int temp = parameterBlock.mid(++pos,1).toInt(&ok);
        if (ok)
            m_FS_integer = temp;
        temp = parameterBlock.mid(++pos,1).toInt(&ok);
        if (ok) {
            m_FS_decimals = temp;
            m_FS_decimals10 = pow(10,m_FS_decimals); // speed up later calculation of coordinates
        }
        pos++;
    } else {
        qDebug() << "FS error";
        return;
    }
    if (parameterBlock.mid(pos,1) == "Y") {
        bool ok;
        int temp = parameterBlock.mid(++pos,1).toInt(&ok);
        if (!ok || (m_FS_integer != temp)) {
            qDebug() << "FS Y error";
            return;
        }
        temp = parameterBlock.mid(++pos,1).toInt(&ok);
        if (!ok || (m_FS_decimals != temp)) {
            qDebug() << "FS Y error";
            return;
        }
        pos++;
    } else {
        qDebug() << "FS error";
        return;
    }
}

//! \brief Set mode inches or millimeters.
void GerberImporter::parameterMO( QString parameterBlock )
{
    if (parameterBlock.mid(2,2) == "MM")
        m_MO = mm;
    else if (parameterBlock.mid(2,2) == "IN")
        m_MO = in;
    else {
        qDebug() << "MO error";
        return;
    }
}

//! \brief Name a layer.
void GerberImporter::parameterLN( QString parameterBlock )
{
    newLayer().setName( parameterBlock.mid(2,parameterBlock.length()-3) );
    qDebug() << "The layer's name is:" << newLayer().name();
}

//! \brief Define an aperture.
void GerberImporter::parameterAD( QString parameterBlock )
{
    parameterBlock.remove(0,3); // remove "ADD"
    QString num_str;
    int pos = 0;
    while ((pos < parameterBlock.length()) && parameterBlock.at(pos).isDigit())
        num_str += parameterBlock.at(pos++);

    bool ok;
    int num = num_str.toInt(&ok);
    if (!ok || (num<10) || (num>999)) {
        qDebug() << "invalid aperture definition";
        return;
    }

    QString aperture_str = parameterBlock.mid(pos);
    aperture_str.chop(1); // remove '*'

    Aperture aperture( m_MO );
    QList<mpq_class> arguments;

    pos = aperture_str.indexOf(',');
    if (pos != -1) {
        // parse arguments
        pos++;
        while ((pos < aperture_str.length()) && (aperture_str.at(pos).isDigit() || aperture_str.at(pos) == '.' || aperture_str.at(pos) == '+' || aperture_str.at(pos) == '-')) {
            QString argument_str;
            while ((pos < aperture_str.length()) && (aperture_str.at(pos).isDigit() || aperture_str.at(pos) == '.' || aperture_str.at(pos) == '+' || aperture_str.at(pos) == '-')) {
                argument_str += aperture_str.at(pos++);
            }
            mpq_class temp = mpq_from_decimal_string( argument_str );
            arguments << temp;
//            qDebug() << "argument:" << temp.get_d();
            if (pos < aperture_str.length() && aperture_str.at(pos) == 'X')
                pos++; // next argument
        }
    }

    if (aperture_str.left(2) == "C,") {
        // circular aperture
        aperture.setCircle( arguments );
    } else if (aperture_str.left(2) == "R,") {
        // rectangular aperture
        aperture.setRectangle( arguments );
    } else if (aperture_str.left(2) == "O,") {
        // oval aperture
        aperture.setOval( arguments );
    } else if (aperture_str.left(2) == "P,") {
        // polygon aperture
        aperture.setPolygon( arguments );
    } else {
        // custom macro
        int idx = aperture_str.indexOf(',');
        if (idx == -1) {
            // no ',' found
            idx = aperture_str.length();
        }
        QString macroName = aperture_str.left(idx);
        if (!m_apertureMacros.contains(macroName)) {
            // cannot use this aperture; macro invalid
            qDebug() << "AD command: cannot use macro" << macroName;
            return;
        }
        aperture.setMacro( m_apertureMacros[macroName], arguments );
    }

    if (m_layers.isEmpty()) {
        // this is a global aperture
        m_apertures[num] = aperture;
    } else {
        // this is a layer local aperture
        currentLayer().defineAperture( num, aperture );
    }
}

void GerberImporter::parameterAM( QString collect_parameter_AM )
{
//    qDebug() << "macro:" << collect_parameter_AM;

    collect_parameter_AM = collect_parameter_AM.trimmed();

    // extract macro name
    int idx = collect_parameter_AM.indexOf('*');
    if (idx == -1) {
        qDebug() << "invalid macro";
        return;
    }
    QString macroName = collect_parameter_AM.mid(2,idx-2);
    collect_parameter_AM.remove(0,idx+1); // remove "AM" + macroName + "*"

    QStringList primitives;
    while (!collect_parameter_AM.isEmpty()) {
        // iterate over all primitives
        QString primitive;
        int pos = 0;
        while (pos < collect_parameter_AM.length() && collect_parameter_AM.at(pos) != '*')
            primitive += collect_parameter_AM.at(pos++);
        collect_parameter_AM.remove(0,pos+1);
        primitive = primitive.trimmed();
        if (!primitive.isEmpty())
            primitives << primitive;
    }

    ApertureMacro macro( primitives );
    m_apertureMacros[macroName] = macro;
}

Layer& GerberImporter::newLayer()
{
    if (m_layers.isEmpty())
        m_layers << Layer( m_apertures );
    if (!m_layers.last().isEmpty())
        m_layers << Layer( m_apertures );

    return m_layers.last();
}

Layer& GerberImporter::currentLayer()
{
    if (m_layers.isEmpty())
        m_layers << Layer( m_apertures );

    return m_layers.last();
}

//! \brief Draw linear interpolation.
void GerberImporter::drawG01( QString dataBlock )
{
    dataBlock.remove(0,3); // remove G01
    currentLayer().setInterpolationMode( Layer::linear );
    draw( dataBlock );
}

//! \brief Draw clockwise circular interpolation.
void GerberImporter::drawG02( QString dataBlock )
{
    dataBlock.remove(0,3); // remove G02
    currentLayer().setInterpolationMode( Layer::clockwise );
    draw( dataBlock );
}

//! \brief Draw counter clockwise circular interpolation.
void GerberImporter::drawG03( QString dataBlock )
{
    dataBlock.remove(0,3); // remove G03
    currentLayer().setInterpolationMode( Layer::counterclockwise );
    draw( dataBlock );
}

//! \brief Draw s.th..
void GerberImporter::draw( QString dataBlock )
{
    if (dataBlock.isEmpty())
        return;

    mpq_class x = currentLayer().x();
    mpq_class y = currentLayer().y();
    mpq_class i = 0;
    mpq_class j = 0;

    int pos = 0;
    if (pos < dataBlock.length() && dataBlock.at(pos) == 'X') {
        QString x_str;
        pos++;
        while (pos < dataBlock.length() && (dataBlock.at(pos).isDigit() || (dataBlock.at(pos) == '-') || (dataBlock.at(pos) == '+'))) {
            x_str += dataBlock.at(pos++);
        }
        x = makeCoordinate( x_str );
    }
    if (pos < dataBlock.length() && dataBlock.at(pos) == 'Y') {
        QString y_str;
        pos++;
        while (pos < dataBlock.length() && (dataBlock.at(pos).isDigit() || (dataBlock.at(pos) == '-') || (dataBlock.at(pos) == '+'))) {
            y_str += dataBlock.at(pos++);
        }
        y = makeCoordinate( y_str );
    }
    if (pos < dataBlock.length() && dataBlock.at(pos) == 'I') {
        QString i_str;
        pos++;
        while (pos < dataBlock.length() && (dataBlock.at(pos).isDigit() || (dataBlock.at(pos) == '-') || (dataBlock.at(pos) == '+'))) {
            i_str += dataBlock.at(pos++);
        }
        i = makeCoordinate( i_str );
    }
    if (pos < dataBlock.length() && dataBlock.at(pos) == 'J') {
        QString j_str;
        pos++;
        while (pos < dataBlock.length() && (dataBlock.at(pos).isDigit() || (dataBlock.at(pos) == '-') || (dataBlock.at(pos) == '+'))) {
            j_str += dataBlock.at(pos++);
        }
        j = makeCoordinate( j_str );
    }

    if (pos < dataBlock.length() && dataBlock.at(pos) == 'D') {
        // parse D code
        setDCode( dataBlock.mid(pos) );
    }

    currentLayer().draw(x,y,i,j,m_MO);
}

void GerberImporter::setDCode( QString dataBlock )
{
    dataBlock.remove(0,1); // strip 'D'
    dataBlock.chop(1); // strip '*'
    bool ok;
    int temp = dataBlock.toInt(&ok);
    if (ok) {
        if (temp == 1)
            currentLayer().setDrawMode( Layer::on );
        else if (temp == 2)
            currentLayer().setDrawMode( Layer::off );
        else if (temp == 3)
            currentLayer().setDrawMode( Layer::flash );
        else if (temp < 10)
            qDebug() << "Invalid D Code.";
        else
            currentLayer().setAperture( temp );
    } else {
        qDebug() << "Error in D Code.";
    }
}

mpq_class GerberImporter::makeCoordinate( QString str )
{
    if (str.isEmpty()) {
        qDebug() << "makeCoordinate(): not a valid coordinate: <empty>";
        return 0;
    }

    int num = m_FS_integer + m_FS_decimals;

    int digit_length = str.length();
    if (str.at(0) == '-' || str.at(0) == '+')
        digit_length--;

    if (digit_length < num) {
        if (m_FS_zero == omit_trailing)
            str.append( QString(num - digit_length,'0') );
    } else if (digit_length > num) {
        qDebug() << "makeCoordinate(): not a valid coordinate:" << str;
        return 0;
    }

    bool ok;
    int temp = str.toInt(&ok);
    if (!ok) {
        qDebug() << "makeCoordinate(): not a valid coordinate:" << str;
        return 0;
    }

    mpq_class value(temp,m_FS_decimals10);
    value.canonicalize();
    return value;
}

mpq_class GerberImporter::mpq_from_decimal_string( QString decimal_str, bool* conversion_ok, int* pos_after_number )
{
    if (conversion_ok)
        *conversion_ok = false;

    decimal_str = decimal_str.trimmed();
    if (decimal_str.isEmpty())
        return 0;

    QString digits;
    if (decimal_str.at(0) == '-' || decimal_str.at(0) == '+') {
        digits = decimal_str.at(0);
        decimal_str.remove(0,1); // remove sign
    }

    int decimal_point = -1;
    int pos = 0;
    while (pos < decimal_str.length() && (decimal_str.at(pos).isDigit() || decimal_str.at(pos) == '.')) {
        if (decimal_str.at(pos) == '.') {
            decimal_point = pos++;
            continue;
        }
        digits += decimal_str.at(pos++);
    }

    bool ok;
    long value = digits.toInt(&ok);
    if (!ok) {
        qDebug() << "mpq_from_decimal_string(): invalid decimal string.";
        return 0;
    }

    // evaluate decimals
    int decimal10 = 1;
    if (decimal_point != -1) {
        decimal10 = pow( 10, pos - decimal_point - 1 );
    }

    if (conversion_ok)
        *conversion_ok  = true;
    if (pos_after_number)
        *pos_after_number = pos;

    mpq_class result( value, decimal10 );
    return result;
}

//! \brief Bounding rectangle.
//! \todo use mpq_class to get an accurate bounding rect.
QRectF GerberImporter::getDimensionsF() const
{
    QRectF rect;
    foreach( Layer layer, m_layers ) {
        QList<Object*> objects = layer.getObjects();
        foreach (Object* object, objects) {
            QRectF temp = object->getGraphicsItem()->boundingRect();
            rect = rect.united( temp );
        }
    }
    return rect;
}










Layer::Layer(QHash<int,Aperture> apertures)
{
    m_current_x = 0;
    m_current_y = 0;
    m_imagePolarity = positive;

    m_drawMode = off;
    m_aperture = 0;
    m_interpolationMode = linear;
    m_outlineFillMode = fillOff;

    m_apertures = apertures;
}

Layer::~Layer()
{

}

void Layer::draw( mpq_class x, mpq_class y, mpq_class i, mpq_class j, Unit unit )
{
//    qDebug() << "draw(): x=" << x.get_d() << " y=" << y.get_d() << " Aperture:" << m_aperture;

    mpq_class unit_factor = 0;
    if (unit == mm)
        unit_factor = 1;
    if (unit == in)
        unit_factor = mpq_class(10) / mpq_class(254); // 1/(25.4 mm)

    // this will fail, if the unit is changed within the layer (not allowed by specs)
    x *= unit_factor;
    y *= unit_factor;
    i *= unit_factor;
    j *= unit_factor;

    if (m_outlineFillMode == fillOff) {
        if (m_drawMode == on) {
            // linear or circular interpolation
            if (!m_apertures.contains(m_aperture)) {
                qDebug() << "cannot draw line; undefined aperture.";
                return;
            }
    //        qDebug() << "Layer::draw() m_aperture=" << m_aperture << "diameter=" << m_apertures.value(m_aperture).diameter().get_d();
            if (m_interpolationMode == linear) {
                Line* line = new Line( m_current_x, m_current_y, x, y, m_apertures.value(m_aperture) );
                m_objects << line;
            } else if (m_interpolationMode == clockwise) {
                Line_cw* line = new Line_cw( m_current_x, m_current_y, x, y, i, j, m_apertures.value(m_aperture) );
                m_objects << line;
            } else if (m_interpolationMode == counterclockwise) {
                Line_ccw* line = new Line_ccw( m_current_x, m_current_y, x, y, i, j, m_apertures.value(m_aperture) );
                m_objects << line;
            }
        } else if (m_drawMode == flash) {
            Flash* flash = new Flash( x, y, m_apertures.value(m_aperture) );
            m_objects << flash;
        }
    } else {
        // filled outline
        // assume linear interpolation
        if (m_drawMode == off) {
            // finish last outline
            drawFilledOutline( m_outlineFillPoints );
            m_outlineFillPoints.clear();
        } else if (m_drawMode == on) {
            if (m_outlineFillPoints.isEmpty() || m_outlineFillPoints.last() != QPair<mpq_class,mpq_class>(x,y))
                m_outlineFillPoints << QPair<mpq_class,mpq_class>(x,y);
        } else {
            qDebug() << "Flash (D03) invalid in filled outline mode.";
            return;
        }
    }


    m_current_x = x;
    m_current_y = y;
}

void Layer::drawFilledOutline( QList< QPair<mpq_class,mpq_class> > points )
{
    FilledOutline* filledOutline = new FilledOutline( points );
    m_objects << filledOutline;
}

void Layer::defineAperture( int num, Aperture aperture )
{
    m_apertures[num] = aperture;
}

void Layer::startOutlineFill()
{
    m_outlineFillMode = fillOn;
    m_outlineFillPoints.clear();
}

void Layer::stopOutlineFill()
{
    m_outlineFillMode = fillOff;
    drawFilledOutline( m_outlineFillPoints );
    m_outlineFillPoints.clear();
}

vtkSmartPointer<vtkProp3D> Layer::getVtkProp3D( double thickness ) const
{
    vtkSmartPointer<vtkAssembly> assembly = vtkSmartPointer<vtkAssembly>::New();
    foreach (Object* object, m_objects) {
        vtkSmartPointer<vtkProp3D> prop3D = object->getVtkProp3D(thickness);
        if (prop3D)
            assembly->AddPart( prop3D );
    }
    return assembly;
}






Object::Object()
{
}

QGraphicsItem* Object::getGraphicsItem() const
{
    return 0;
}

vtkSmartPointer<vtkProp3D> Object::getVtkProp3D( double thickness ) const
{
    return 0;
}

Line::Line( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, Aperture aperture ) : m_aperture(aperture)
{
    m_x1 = x1;
    m_y1 = y1;
    m_x2 = x2;
    m_y2 = y2;
}

QGraphicsItem* Line::getGraphicsItem() const
{
    QPen pen;
    if (m_aperture.type() == Aperture::circle) {
        pen.setWidthF( m_aperture.diameter().get_d() );
        pen.setCapStyle( Qt::RoundCap );
    }
    qreal x1 = m_x1.get_d();
    qreal y1 = m_y1.get_d();
    qreal x2 = m_x2.get_d();
    qreal y2 = m_y2.get_d();
    QGraphicsLineItem* line = new QGraphicsLineItem(x1,y1,x2,y2);
    line->setPen( pen );
    return line;
}

vtkSmartPointer<vtkProp3D> Line::getVtkProp3D( double thickness ) const
{
    double x1 = m_x1.get_d();
    double y1 = m_y1.get_d();
    double x2 = m_x2.get_d();
    double y2 = m_y2.get_d();
    double diameter = m_aperture.diameter().get_d();
    double length = sqrt( pow(x2-x1,2.0) + pow(y2-y1,2.0) );
    double angle = atan2( y2-y1, x2-x1 );

    vtkSmartPointer<vtkAssembly> assembly = vtkSmartPointer<vtkAssembly>::New();

    vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
    cubeSource->SetBounds( 0, length, -diameter/2.0, diameter/2.0, 0, thickness );
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(cubeSource->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor( 0.7, 0.7, 0.7 );
    assembly->AddPart( actor );

    vtkSmartPointer<vtkCylinderSource> cylinderSource = vtkSmartPointer<vtkCylinderSource>::New();
    cylinderSource->SetHeight( thickness );
    cylinderSource->SetRadius( diameter/2.0 );
    cylinderSource->SetResolution(20); // FIXME cylinders in vtk are ugly!!!
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(cylinderSource->GetOutputPort());
    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor( 0.7, 0.7, 0.7 );
    actor->RotateX( 90 ); // let the axis of the cylinder point towards +z
    actor->SetPosition( 0, 0, thickness/2.0 );
    assembly->AddPart( actor );

    cylinderSource = vtkSmartPointer<vtkCylinderSource>::New();
    cylinderSource->SetHeight( thickness );
    cylinderSource->SetRadius( diameter/2.0 );
    cylinderSource->SetResolution(20); // FIXME cylinders in vtk are ugly!!!
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(cylinderSource->GetOutputPort());
    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor( 0.7, 0.7, 0.7 );
    actor->RotateX( 90 ); // let the axis of the cylinder point towards +z
    actor->SetPosition( length, 0, thickness/2.0 );
    assembly->AddPart( actor );

    assembly->RotateZ( angle / M_PI * 180.0 );
    assembly->SetPosition( x1, y1, 0 );

    return assembly;
}

Line_cw_ccw::Line_cw_ccw( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, mpq_class i, mpq_class j, Aperture aperture ) : m_aperture(aperture)
{
    m_x1 = x1;
    m_y1 = y1;
    m_x2 = x2;
    m_y2 = y2;
    m_i = i;
    m_j = j;
}

QGraphicsItem* Line_cw_ccw::getGraphicsItem( bool ccw ) const
{
    QPen pen;
    if (m_aperture.type() == Aperture::circle) {
        pen.setWidthF( m_aperture.diameter().get_d() );
        pen.setCapStyle( Qt::RoundCap );
    } else {
        qDebug() << "Line_cw_ccw::getGraphicsItem(): unsupported aperture";
    }
    qreal x1 = m_x1.get_d();
    qreal y1 = m_y1.get_d();
    qreal x2 = m_x2.get_d();
    qreal y2 = m_y2.get_d();
    qreal i = m_i.get_d(); // attention: absolute value in single quadrant mode!
    qreal j = m_j.get_d(); // attention: absolute value in single quadrant mode!
    qreal center_x = x1+i; // FIXME only valid in multi quadrant mode!
    qreal center_y = y1+j; // FIXME only valid in multi quadrant mode!
    qreal radius1 = sqrt( pow(center_x-x1,2.0) + pow(center_y-y1,2.0) );
    qreal radius2 = sqrt( pow(center_x-x2,2.0) + pow(center_y-y2,2.0) );
    QPainterPath path;
    qreal startAngle = atan2(y1-center_y,x1-center_x)/M_PI*180.0;
    qreal stopAngle = atan2(y2-center_y,x2-center_x)/M_PI*180.0;
    qreal spanAngle = abs(stopAngle - startAngle);
    if (ccw)
        spanAngle = -spanAngle;
    path.arcMoveTo( center_x-radius1, center_y-radius1, radius1*2.0, radius1*2.0, -startAngle );
    path.arcTo( center_x-radius1, center_y-radius1, radius1*2.0, radius1*2.0, -startAngle, spanAngle );
    QGraphicsPathItem* pathItem = new QGraphicsPathItem(path);
    pathItem->setPen( pen );

//    // DEBUG
//    QGraphicsItemGroup* group = new QGraphicsItemGroup;
//    group->addToGroup( pathItem );
//    QGraphicsEllipseItem* e1 = new QGraphicsEllipseItem( x1-m_aperture.diameter().get_d()/2.0, y1-m_aperture.diameter().get_d()/2.0, m_aperture.diameter().get_d(), m_aperture.diameter().get_d() );
//    e1->setPen( QColor("red") );
//    group->addToGroup( e1 );
//    QGraphicsEllipseItem* e2 = new QGraphicsEllipseItem( x2-m_aperture.diameter().get_d()/2.0, y2-m_aperture.diameter().get_d()/2.0, m_aperture.diameter().get_d(), m_aperture.diameter().get_d() );
//    e2->setPen( QColor("green") );
//    group->addToGroup( e2 );
//    QGraphicsEllipseItem* e3 = new QGraphicsEllipseItem( center_x-m_aperture.diameter().get_d()/2.0, center_y-m_aperture.diameter().get_d()/2.0, m_aperture.diameter().get_d(), m_aperture.diameter().get_d() );
//    e3->setPen( QColor("blue") );
//    group->addToGroup( e3 );
//    return group;

    return pathItem;
}


Line_cw::Line_cw( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, mpq_class i, mpq_class j, Aperture aperture ) : Line_cw_ccw(x1,y1,x2,y2,i,j,aperture)
{
}

QGraphicsItem* Line_cw::getGraphicsItem() const
{
    return Line_cw_ccw::getGraphicsItem( false );
}

Line_ccw::Line_ccw( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, mpq_class i, mpq_class j, Aperture aperture ) : Line_cw_ccw(x1,y1,x2,y2,i,j,aperture)
{
}

QGraphicsItem* Line_ccw::getGraphicsItem() const
{
    return Line_cw_ccw::getGraphicsItem( true );
}

Flash::Flash( mpq_class x, mpq_class y, Aperture aperture ) : m_aperture(aperture)
{
    m_x = x;
    m_y = y;
}

QGraphicsItem* Flash::getGraphicsItem() const
{
    qreal x = m_x.get_d();
    qreal y = m_y.get_d();
    QGraphicsItem* item = m_aperture.getGraphicsItem();
    if (item) {
        item->moveBy( x, y );
        qDebug() << "Flash::getGraphicsItem():" << item->pos() << item->boundingRect();
    } else {
        qDebug() << "Flash::getGraphicsItem(): cannot get aperture";
    }
    return item;
}

FilledOutline::FilledOutline( QList< QPair<mpq_class,mpq_class> > points )
{
    if (!points.isEmpty() && points.first() == points.last())
        points.removeLast();
    m_points = points;
}

//! \todo currently no support for arcs
QGraphicsItem* FilledOutline::getGraphicsItem() const
{
    QPolygonF polygon;
    typedef QPair<mpq_class,mpq_class> Pair;
    foreach (Pair point, m_points) {
        polygon << QPointF( point.first.get_d(), point.second.get_d() );
    }

    QGraphicsPolygonItem* polygonItem = new QGraphicsPolygonItem(polygon);
    polygonItem->setBrush( polygonItem->pen().color() );
    return polygonItem;
}

vtkSmartPointer<vtkProp3D> FilledOutline::getVtkProp3D( double thickness ) const
{
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> poly = vtkSmartPointer<vtkCellArray>::New();

    int numPoints = 0;
    poly->InsertNextCell(0);
    typedef QPair<mpq_class,mpq_class> Pair;
    foreach (Pair point, m_points) {
        int id = points->InsertNextPoint( point.first.get_d(), point.second.get_d(), 0 );
        poly->InsertCellPoint(id);
        numPoints++;
    }
    if (numPoints < 3)
        return 0;
    poly->InsertCellPoint(0); // close the polygon
    poly->UpdateCellCount(++numPoints);

    vtkSmartPointer<vtkPolyData> profile = vtkSmartPointer<vtkPolyData>::New();
    profile->SetPoints(points);
    profile->SetPolys(poly);

    vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
    cleaner->SetInput( profile );

    vtkSmartPointer<vtkTriangleFilter> tf = vtkSmartPointer<vtkTriangleFilter>::New();
    tf->SetInputConnection( cleaner->GetOutputPort() );

    vtkSmartPointer<vtkLinearExtrusionFilter> extrude = vtkSmartPointer<vtkLinearExtrusionFilter>::New();
    extrude->SetInputConnection( tf->GetOutputPort() );
    extrude->SetExtrusionTypeToVectorExtrusion();
    extrude->SetVector( 0, 0, thickness );
//    extrude->CappingOn();

    vtkSmartPointer<vtkPolyDataMapper> Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    Mapper->SetInputConnection( extrude->GetOutputPort() );

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(Mapper);
    actor->GetProperty()->SetColor( 0.7, 0.7, 0.7 );

    return actor;
}




Aperture::Aperture()
{
    m_type = circle;
    m_hole = noHole;
    m_unit = in;
    m_unitFactor = mpq_class(10) / mpq_class(254);
}

Aperture::Aperture(Unit unit)
{
    m_type = circle;
    m_hole = noHole;
    m_unit = unit;
    m_unitFactor = 0;
    if (m_unit == mm)
        m_unitFactor = 1;
    if (m_unit == in)
        m_unitFactor = mpq_class(10) / mpq_class(254);
}

void Aperture::setCircle( QList<mpq_class> arguments )
{
    m_arguments = arguments;
    m_type = circle;
    if (arguments.size() == 1)
        m_hole = noHole;
    else if (arguments.size() == 2)
        m_hole = circularHole;
    else if (arguments.size() == 3)
        m_hole = rectangularHole;
    else {
        qDebug() << "Aperture::setCircle(): invalid number of arguments.";
        m_type = invalid;
        m_arguments.clear();
    }
    for (int i=0; i<m_arguments.size(); i++)
        m_arguments[i] = m_arguments[i] * m_unitFactor;
}

void Aperture::setCircle( mpq_class diameter, mpq_class x_hole_dimension, mpq_class y_hole_dimension )
{
    qDebug() << "Aperture::setCircle()" << diameter.get_d() << x_hole_dimension.get_d() << y_hole_dimension.get_d();

    m_type = circle;
    m_hole = noHole;
    m_arguments.clear();
    m_arguments << diameter;

    if (x_hole_dimension != -1) {
        // enable circular hole
        m_hole = circularHole;
        m_arguments << x_hole_dimension;
        if (y_hole_dimension != -1) {
            // enable rectangular hole
            m_hole = rectangularHole;
            m_arguments << y_hole_dimension;
        }
    }

    for (int i=0; i<m_arguments.size(); i++)
        m_arguments[i] = m_arguments[i] * m_unitFactor;
}

void Aperture::setOval( QList<mpq_class> arguments )
{
    m_arguments = arguments;
    m_type = oval;
    if (arguments.size() == 2)
        m_hole = noHole;
    else if (arguments.size() == 3)
        m_hole = circularHole;
    else if (arguments.size() == 4)
        m_hole = rectangularHole;
    else {
        qDebug() << "Aperture::setOval(): invalid number of arguments.";
        m_type = invalid;
        m_arguments.clear();
    }

    for (int i=0; i<m_arguments.size(); i++)
        m_arguments[i] = m_arguments[i] * m_unitFactor;
}

void Aperture::setOval( mpq_class x_length, mpq_class y_length, mpq_class x_hole_dimension, mpq_class y_hole_dimension )
{
    qDebug() << "Aperture::setOval()" << x_length.get_d() << y_length.get_d() << x_hole_dimension.get_d() << y_hole_dimension.get_d();

    m_type = oval;
    m_hole = noHole;
    m_arguments.clear();
    m_arguments << x_length << y_length;

    if (x_hole_dimension != -1) {
        // enable circular hole
        m_hole = circularHole;
        m_arguments << x_hole_dimension;
        if (y_hole_dimension != -1) {
            // enable rectangular hole
            m_hole = rectangularHole;
            m_arguments << y_hole_dimension;
        }
    }

    for (int i=0; i<m_arguments.size(); i++)
        m_arguments[i] = m_arguments[i] * m_unitFactor;
}

void Aperture::setRectangle( QList<mpq_class> arguments )
{
    m_arguments = arguments;
    m_type = rectangle;
    if (arguments.size() == 2)
        m_hole = noHole;
    else if (arguments.size() == 3)
        m_hole = circularHole;
    else if (arguments.size() == 4)
        m_hole = rectangularHole;
    else {
        qDebug() << "Aperture::setRectangle(): invalid number of arguments.";
        m_type = invalid;
        m_arguments.clear();
    }

    for (int i=0; i<m_arguments.size(); i++)
        m_arguments[i] = m_arguments[i] * m_unitFactor;
}

void Aperture::setPolygon( QList<mpq_class> arguments )
{
    m_arguments = arguments;
    m_type = polygon;
    if (arguments.size() == 2 || arguments.size() == 3)
        m_hole = noHole;
    else if (arguments.size() == 4)
        m_hole = circularHole;
    else if (arguments.size() == 5)
        m_hole = rectangularHole;
    else {
        qDebug() << "Aperture::setPolygon(): invalid number of arguments.";
        m_type = invalid;
        m_arguments.clear();
        return;
    }

    m_arguments[0] = m_arguments[0] * m_unitFactor;
    for (int i=3; i<m_arguments.size(); i++)
        m_arguments[i] = m_arguments[i] * m_unitFactor;
}

void Aperture::setMacro( ApertureMacro apertureMacro, QList<mpq_class> arguments )
{
    m_type = macro;
    m_macro = apertureMacro;
    m_arguments = arguments;
}

QGraphicsItem* Aperture::getGraphicsItem() const
{
    if (type() == circle) {
        if (m_arguments.isEmpty()) {
            qDebug() << "invalid circular aperture definition (too few arguments)";
            return 0;
        }
        QPainterPath path;
        qreal radius = mpq_class(diameter()/2).get_d();
        path.addEllipse( QPointF(0,0), radius, radius );

        if (m_arguments.size() == 2) {
            // circular shape with circular hole
            qreal dia2 = m_arguments.at(1).get_d();
            path.addEllipse( QPointF(0,0), dia2/2.0, dia2/2.0 );
        } else if (m_arguments.size() == 3) {
            // circular shape with rectangular hole
            qreal width  = m_arguments.at(1).get_d();
            qreal height = m_arguments.at(2).get_d();
            path.addRect( -width/2.0, -height/2.0, width, height );
        } else if (m_arguments.size() > 3) {
            qDebug() << "invalid circular aperture definition (too many arguments)";
            return 0;
        }

        QGraphicsPathItem* circle = new QGraphicsPathItem(path);
        circle->setBrush( circle->pen().color() );

        return circle;
    }
    if (type() == rectangle) {
        if (m_arguments.size() < 2) {
            qDebug() << "invalid rectangular aperture definition (too few arguments)";
            return 0;
        }
        QPainterPath path;
        qreal width  = m_arguments.at(0).get_d();
        qreal height = m_arguments.at(1).get_d();
        path.addRect( -width/2.0, -height/2.0, width, height );

        if (m_arguments.size() == 3) {
            // circular hole
            qreal radius = mpq_class(m_arguments.at(2)/2).get_d();
            path.addEllipse( QPointF(0,0), radius, radius );
        } else if (m_arguments.size() == 4) {
            // rectangular hole
            qreal width  = m_arguments.at(2).get_d();
            qreal height = m_arguments.at(3).get_d();
            path.addRect( -width/2.0, -height/2.0, width, height );
        } else if (m_arguments.size() > 4) {
            qDebug() << "invalid circular aperture definition (too many arguments)";
            return 0;
        }

        QGraphicsPathItem* rect = new QGraphicsPathItem(path);
        rect->setBrush( rect->pen().color() );

        return rect;
    }
    if (type() == oval) {
        // obround (oval) aperture
        if (m_arguments.size() < 2) {
            qDebug() << "invalid oval aperture definition (too few arguments)";
            return 0;
        }
        mpq_class x = m_arguments.at(0);
        mpq_class y = m_arguments.at(1);
        if (x == y) {
            qDebug() << "invalid oval aperture definition (x==y)";
            return 0;
        }
        x = abs(x);
        y = abs(y);
        QPainterPath path;
        if (x < y) {
            // vertical
            mpq_class radius = x/2;
            mpq_class length = y-x;
            mpq_class length2 = length/2;
            path.moveTo( radius.get_d(),  length2.get_d() );
            path.lineTo( radius.get_d(), -length2.get_d() );
            path.arcTo( -radius.get_d(), mpq_class(-length2-radius).get_d(), x.get_d(), x.get_d(), 0.0, 180.0 );
            path.lineTo( -radius.get_d(), length2.get_d() );
            path.arcTo( -radius.get_d(), mpq_class(length2-radius).get_d(), x.get_d(), x.get_d(), 180.0, 180.0 );
            path.closeSubpath();
        } else {
            // horizontal
            mpq_class radius = y/2;
            mpq_class length = x-y;
            mpq_class length2 = length/2;
            path.moveTo(  length2.get_d(),  radius.get_d() );
            path.lineTo( -length2.get_d(),  radius.get_d() );
            path.arcTo( mpq_class(-length2-radius).get_d(), -radius.get_d(), y.get_d(), y.get_d(), -90.0, -180.0 );
            path.lineTo(  length2.get_d(), -radius.get_d() );
            path.arcTo( mpq_class(length2-radius).get_d(), -radius.get_d(), y.get_d(), y.get_d(), 90.0, -180.0 );
            path.closeSubpath();
        }

        if (m_arguments.size() == 3) {
            // circular hole
            qreal radius = mpq_class(m_arguments.at(2) / 2).get_d();
            path.addEllipse( QPointF(0,0), radius, radius );
        } else if (m_arguments.size() == 4) {
            // rectangular hole
            qreal width  = m_arguments.at(2).get_d();
            qreal height = m_arguments.at(3).get_d();
            path.addRect( -width/2.0, -height/2.0, width, height );
        } else if (m_arguments.size() > 4) {
            qDebug() << "invalid oval aperture definition (too many arguments)";
            return 0;
        }

        QGraphicsPathItem* pathItem = new QGraphicsPathItem(path);
        pathItem->setBrush( pathItem->pen().color() );

        return pathItem;
    }
    if (type() == polygon) {
        // regular polygon
        if (m_arguments.size() < 2) {
            qDebug() << "invalid polygon definition (too few arguments)";
            return 0;
        }
        if (m_arguments.size() > 5) {
            qDebug() << "invalid polygon definition (too many arguments)";
            return 0;
        }
        mpq_class radius = m_arguments.at(0) / 2;
        int numSides = m_arguments.at(1).get_d();
        if (numSides < 2) {
            qDebug() << "invalid polygon.";
            return 0;
        }
        mpq_class rotation = 0;
        if (m_arguments.size() >= 3)
            rotation = m_arguments.at(2);

        QPainterPath path;
        for (int n=0; n<numSides; n++) {
            mpq_class temp1 = rotation / mpq_class(180);
            mpq_class radian = temp1 * M_PI;
            mpq_class x = radius * cos(radian.get_d());
            mpq_class y = radius * sin(radian.get_d());
            if (n == 0)
                path.moveTo( x.get_d(),y.get_d() );
            else
                path.lineTo( x.get_d(),y.get_d() );
            rotation += mpq_class(360) / numSides;
        }

        if (m_arguments.size() == 4) {
            // circular hole
            qreal radius = mpq_class(m_arguments.at(3)/2).get_d();
            path.addEllipse( QPointF(0,0), radius, radius );
        } else if (m_arguments.size() == 5) {
            // rectangular hole
            qreal width  = m_arguments.at(3).get_d();
            qreal height = m_arguments.at(4).get_d();
            path.addRect( -width/2.0, -height/2.0, width, height );
        }

        QGraphicsPathItem* item = new QGraphicsPathItem(path);
        item->setBrush( QBrush(item->pen().color()) );

        return item;
    }
    if (type() == macro) {
        // FIXME the macro should be precalculated at setMacro()
        // FIXME and the unit handling should be moved there, too
//        qDebug() << "QGraphicsItem* Aperture::getGraphicsItem() const ## macro";
        QGraphicsItemGroup* group = new QGraphicsItemGroup;

        QList<QList<mpq_class> > primitives = m_macro.calc( m_arguments );
        foreach (QList<mpq_class> primitive, primitives) {
//            qDebug() << "primitive:";
//            for (int n=0; n<primitive.size(); n++)
//                qDebug() << primitive.value(n).get_d();
            if (primitive.value(0) == 1) {
                // circle
                if (primitive.size() != 5) {
                    qDebug() << "macro definition circle invalid.";
                    return group;
                }
                int exposure = primitive.value(1).get_d(); // FIXME
                mpq_class diameter = primitive.value(2) * m_unitFactor;
                mpq_class center_x = primitive.value(3) * m_unitFactor;
                mpq_class center_y = primitive.value(4) * m_unitFactor;

                mpq_class lower_x = center_x - diameter/mpq_class(2);
                mpq_class lower_y = center_y - diameter/mpq_class(2);
                QRectF rect( lower_x.get_d(), lower_y.get_d(), diameter.get_d(), diameter.get_d() );
                QGraphicsEllipseItem* item = new QGraphicsEllipseItem(rect,group);
                item->setBrush( QBrush(item->pen().color()) );

            } else if (primitive.value(0) == 4) {
                // outline
                if (primitive.size() < 6) {
                    qDebug() << "macro definition outline invalid.";
                    return group;
                }
                int exposure = primitive.value(1).get_d(); // FIXME
                int num = primitive.value(2).get_d();
                if (primitive.size() != 1+5+2*num) {
                    qDebug() << "macro definition outline invalid. Expected number of elements" << 1+5+2*num << "actual number of elements" << primitive.size();
                    return group;
                }

                QPolygonF poly;
                for (int n=0; n<num; n++) {
                    mpq_class x = primitive.value(3+2*n) * m_unitFactor;
                    mpq_class y = primitive.value(4+2*n) * m_unitFactor;
                    poly << QPointF(x.get_d(),y.get_d());
                }
                QGraphicsPolygonItem* item = new QGraphicsPolygonItem(poly,group);
                item->setBrush( QBrush(item->pen().color()) );
                mpq_class rotation = primitive.value(5+2*num);
                item->setRotation( rotation.get_d() );

            } else if (primitive.value(0) == 5) {
                // regular polygon
                if (primitive.size() != 6 && primitive.size() != 7) {
                    qDebug() << "macro definition regular polygon invalid.";
                    return group;
                }
                int exposure = primitive.value(1).get_d(); // FIXME
                int numSides = primitive.value(2).get_d();
                mpq_class center_x = primitive.value(3) * m_unitFactor;
                mpq_class center_y = primitive.value(4) * m_unitFactor;
                mpq_class radius = primitive.value(5) / mpq_class(2) * m_unitFactor;
                mpq_class rotation = primitive.value(6);

                if (numSides < 2) {
                    qDebug() << "invalid polygon.";
                    return group; // invalid
                }

                QPolygonF poly;
                for (int n=0; n<numSides; n++) {
                    mpq_class temp1 = rotation / mpq_class(180);
                    mpq_class temp2 = temp1 * M_PI;
                    double radian = temp2.get_d();
                    mpq_class x = center_x + radius * cos(radian);
                    mpq_class y = center_y + radius * sin(radian);
                    poly << QPointF(x.get_d(),y.get_d());
                    rotation += mpq_class(360) / numSides;
                }
                QGraphicsPolygonItem* item = new QGraphicsPolygonItem(poly,group);
                item->setBrush( QBrush(item->pen().color()) );

            } else if (primitive.value(0) == 21) {
                // line (center)
                if (primitive.size() != 7) {
                    qDebug() << "macro definition line (center) invalid.";
                    return group;
                }
                int exposure = primitive.value(1).get_d(); // FIXME
                mpq_class width = primitive.value(2) * m_unitFactor;
                mpq_class height = primitive.value(3) * m_unitFactor;
                mpq_class center_x = primitive.value(4) * m_unitFactor;
                mpq_class center_y = primitive.value(5) * m_unitFactor;
                mpq_class rotation = primitive.value(6);

                mpq_class lower_x = center_x-width/mpq_class(2);
                mpq_class lower_y = center_y-height/mpq_class(2);
                QRectF rect( lower_x.get_d(), lower_y.get_d(), width.get_d(), height.get_d() );
                QGraphicsRectItem* item = new QGraphicsRectItem(rect,group);
                item->setRotation( rotation.get_d() );
                item->setBrush( QBrush(item->pen().color()) );

            } else {
                qDebug() << "unsupported macro primitive" << (int)primitive.value(0).get_d();
                return group;
            }
        }
        return group;
    }

    return 0;
}




ApertureMacro::ApertureMacro()
{
}

ApertureMacro::ApertureMacro( QStringList primitives )
{
//    qDebug() << "ApertureMacro:" << primitives;
    m_primitives = primitives;
}

//! \brief substitute actual values for placeholders.
QList<QList<mpq_class> > ApertureMacro::calc( QList<mpq_class> arguments ) const
{
    QList<QList<mpq_class> > entire_result;
    foreach (QString primitive, m_primitives) {
        QList<mpq_class> result;
        primitive = primitive.trimmed();
        if (primitive.startsWith('0'))
            continue; // comment

        QStringList args;
        while (!primitive.isEmpty()) {
            int idx = primitive.indexOf(',');
            if (idx == -1) {
                // last argument
                args << primitive;
                primitive.clear();
            } else {
                args << primitive.left(idx);
                primitive.remove(0,idx+1);
            }
        }

        for (int n=0; n<args.size(); n++) {
            QString term = args.at(n).simplified();
            while (term.contains(' '))
                term.remove( term.indexOf(' '), 1 ); // remove all spaces
            //result << calc_intern1( term, arguments );
            QList<ApertureMacro_internalRep> temp = calc_convertIntoInternalRep( term, arguments );
            temp = calc_canonicalize( temp );
            result << calc_processInternalRep( temp );
        }

        entire_result << result;
    }
    return entire_result;
}

//! \brief \internal expects \c term to be free of white space.
//! Converts the \c term into an internal representation.
//! \todo does not support the '=' operator
QList<ApertureMacro_internalRep> ApertureMacro::calc_convertIntoInternalRep( QString term, QList<mpq_class> arguments ) const
{
    if (term.isEmpty()) {
        return QList<ApertureMacro_internalRep>();
    }

    QList<ApertureMacro_internalRep> result;

    bool ok;
    int pos_after_number;
    int pos = 0;
    while (pos < term.length()) {
        // check for placeholder
        if (term.at(pos) == '$') {
            mpq_class temp = GerberImporter::mpq_from_decimal_string( term.mid(pos+1), &ok, &pos_after_number );
            if (!ok) {
                // syntax error
                return QList<ApertureMacro_internalRep>();
            }
            int num = (int)temp.get_d() - 1; // '$2' refers arguments.at(1)
            if (arguments.size() <= num) {
                // invalid placeholder
                return QList<ApertureMacro_internalRep>();
            }
            result << ApertureMacro_internalRep( arguments.at(num) );
            pos = pos_after_number+pos+1;
            continue;
        }
        if (term.at(pos) == '+') {
            result << ApertureMacro_internalRep( ApertureMacro_internalRep::plus );
            pos++;
            continue;
        }
        if (term.at(pos) == '-') {
            result << ApertureMacro_internalRep( ApertureMacro_internalRep::minus );
            pos++;
            continue;
        }
        if (term.at(pos) == 'X') {
            result << ApertureMacro_internalRep( ApertureMacro_internalRep::mul );
            pos++;
            continue;
        }
        if (term.at(pos) == '/') {
            result << ApertureMacro_internalRep( ApertureMacro_internalRep::div );
            pos++;
            continue;
        }
        if (term.at(pos) == '=') {
            qDebug() << "equate '=' unsupported";
            return QList<ApertureMacro_internalRep>();
        }

        mpq_class temp = GerberImporter::mpq_from_decimal_string( term.mid(pos), &ok, &pos_after_number );
        if (!ok) {
            qDebug() << "syntax error";
            return QList<ApertureMacro_internalRep>();
        }
        result << ApertureMacro_internalRep( temp );
        pos = pos + pos_after_number;
    }

    return result;
}

//! \brief combine unary minus or plus into following (positive) value.
//! The function calc_convertIntoInternalRep() parses a minus or plus sign into an operator.
//! This is not neccessarily correct. E.g. "-1" is parsed into "operator-" "1".
QList<ApertureMacro_internalRep> ApertureMacro::calc_canonicalize( QList<ApertureMacro_internalRep> temp ) const
{
    if (temp.isEmpty() || temp.size() == 1)
        return temp;

    // simple cases
    if (temp.at(0).m_operator == ApertureMacro_internalRep::plus) {
        temp.removeFirst();
    } else if (temp.at(0).m_operator == ApertureMacro_internalRep::minus) {
        if (temp.at(1).m_operator == ApertureMacro_internalRep::value) {
            temp[1].m_value *= -1;
            temp.removeFirst();
        } else {
            qDebug() << "invalid macro syntax";
            return QList<ApertureMacro_internalRep>();
        }
    }

    if (temp.last().m_operator != ApertureMacro_internalRep::value || temp.first().m_operator != ApertureMacro_internalRep::value) {
        qDebug() << "invalid macro syntax (operator without value)";
        return QList<ApertureMacro_internalRep>();
    }

    for (int n=1; n<temp.size(); n++) {
        if (temp.at(n-1).m_operator != ApertureMacro_internalRep::value && temp.at(n).m_operator != ApertureMacro_internalRep::value) {
            // two operators directly behind each other
            if ((temp.at(n-1).m_operator == ApertureMacro_internalRep::mul || temp.at(n-1).m_operator == ApertureMacro_internalRep::div) && temp.at(n).m_operator == ApertureMacro_internalRep::plus) {
                temp.removeAt(n); // remove "operator+"
                n = 0; // start over
                continue;
            }
            if ((temp.at(n-1).m_operator == ApertureMacro_internalRep::mul || temp.at(n-1).m_operator == ApertureMacro_internalRep::div) && temp.at(n).m_operator == ApertureMacro_internalRep::minus) {
                if (n+1 < temp.size() && temp.at(n+1).m_operator == ApertureMacro_internalRep::value) {
                    temp[n+1].m_value *= -1;
                    temp.removeAt(n); // remove "operator-"
                    n = 0; // start over
                    continue;
                } else {
                    qDebug() << "invalid macro syntax (operator without value)";
                    return QList<ApertureMacro_internalRep>();
                }
            }
        }
    }

    return temp;
}

//! \brief reduce the internal representation to one value
mpq_class ApertureMacro::calc_processInternalRep( QList<ApertureMacro_internalRep> temp ) const
{
    if (temp.isEmpty())
        return mpq_class();

    if (temp.size() == 1) {
        if (temp.at(0).m_operator == ApertureMacro_internalRep::value)
            return temp.at(0).m_value;
        else {
            qDebug() << "syntax error";
            return mpq_class();
        }
    }

    if ((temp.size()-1) % 2) {
        qDebug() << "wrong number of elements";
        return mpq_class();
    }

    // now reduce the list according to mathematical rules of priority

    // '*', '/'
    int pos = 1;
    do {
        if (temp.at(pos).m_operator == ApertureMacro_internalRep::mul || temp.at(pos).m_operator == ApertureMacro_internalRep::div) {
            if (temp.at(pos-1).m_operator != ApertureMacro_internalRep::value || temp.at(pos+1).m_operator != ApertureMacro_internalRep::value) {
                qDebug() << "invalid math operation.";
                return mpq_class();
            }
            if (temp.at(pos).m_operator == ApertureMacro_internalRep::mul)
                temp[pos-1].m_value = temp.at(pos-1).m_value * temp.at(pos+1).m_value;
            else
                temp[pos-1].m_value = temp.at(pos-1).m_value / temp.at(pos+1).m_value;
            temp.removeAt(pos); // remove operator
            temp.removeAt(pos); // remove 2nd argument
            continue;
        }
        pos++;
    } while (pos < temp.size());

    // '+', '-'
    pos = 0;
    do {
        if (temp.at(pos).m_operator == ApertureMacro_internalRep::plus || temp.at(pos).m_operator == ApertureMacro_internalRep::minus) {
            if (temp.at(pos-1).m_operator != ApertureMacro_internalRep::value || temp.at(pos+1).m_operator != ApertureMacro_internalRep::value) {
                qDebug() << "invalid math operation.";
                return mpq_class();
            }
            if (temp.at(pos).m_operator == ApertureMacro_internalRep::plus)
                temp[pos-1].m_value = temp.at(pos-1).m_value + temp.at(pos+1).m_value;
            else
                temp[pos-1].m_value = temp.at(pos-1).m_value - temp.at(pos+1).m_value;
            temp.removeAt(pos); // remove operator
            temp.removeAt(pos); // remove 2nd argument
            continue;
        }
        pos++;
    } while (pos < temp.size());

    return temp.at(0).m_value;
}








ApertureMacro_internalRep::ApertureMacro_internalRep()
{
    m_operator = value;
    m_value = 0;
}

ApertureMacro_internalRep::ApertureMacro_internalRep( mpq_class val )
{
    m_operator = value;
    m_value = val;
}

ApertureMacro_internalRep::ApertureMacro_internalRep( Operator op )
{
    m_operator = op;
}
