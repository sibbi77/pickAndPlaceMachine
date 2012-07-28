#include "gerberimporter.h"

#include <QtCore>

GerberImporter::GerberImporter()
{
//    QList<QByteArray> l = QTextCodec::availableCodecs();
//    foreach (QByteArray name, l)
//        qDebug() << name;
    m_deprecated_parameters << "AS" << "MI" << "OF" << "SF" << "IP" << "IR" << "KO" << "SM";

    m_FS_integer = -1;
    m_FS_decimals = -1;
    m_FS_decimals10 = 0;
    m_FS_zero = omit_leading;
    m_MO = in;
}

bool GerberImporter::import( QString filename )
{
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
                processParameterBlock( block );
            continue;
        }
        if (idx1 != -1) {
            // start/end of parameter block found
            line.remove( 0, idx1+1 ); // remove garbage
            if (state == dataBlock)
                state = parameterBlock;
            else
                state = dataBlock;
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
    qDebug() << "dataBlock:" << dataBlock;

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
    else if (dataBlock.left(4) == "G54D")
        setDCode( dataBlock.mid(3) );
    else if ((dataBlock.left(1) == "X") || (dataBlock.left(1) == "Y"))
        draw( dataBlock );

    return true; // go on with processing
}

void GerberImporter::processParameterBlock( QString parameterBlock )
{
    parameterBlock = parameterBlock.trimmed();
    qDebug() << "parameterBlock:" << parameterBlock;

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
}

//! \brief Define the number format.
void GerberImporter::parameterFS( QString parameterBlock )
{
    if (m_FS_integer != -1) {
        // only one times allowed
        qDebug() << "FS specified multiple times";
        return;
    }
    if (parameterBlock.mid(2,1) == "L")
        m_FS_zero = omit_leading;
    else if (parameterBlock.mid(2,1) == "T")
        m_FS_zero = omit_trailing;
    else {
        qDebug() << "FS error";
        return;
    }
    if (parameterBlock.mid(3,1) == "A")
        ;
    else if (parameterBlock.mid(3,1) == "I") {
        qDebug() << "FS I unsupported";
        return;
    } else {
        qDebug() << "FS error";
        return;
    }
    int pos = 4;
    if (parameterBlock.mid(pos,1) == "N") {
        qDebug() << "FS N unsupported";
        pos += 3;
    }
    if (parameterBlock.mid(pos,1) == "G") {
        qDebug() << "FS G unsupported";
        pos += 3;
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

//! \brief set mode inches or millimeters
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

Layer& GerberImporter::newLayer()
{
    if (m_layers.isEmpty())
        m_layers << Layer();
    if (!m_layers.last().isEmpty())
        m_layers << Layer();

    return m_layers.last();
}

Layer& GerberImporter::currentLayer()
{
    if (m_layers.isEmpty())
        m_layers << Layer();

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
//    DrawMode drawMode = m_drawMode;
//    int aperture = m_currentAperture;
//    InterpolationMode interpolationMode = m_interpolationMode;

    mpq_class x = currentLayer().x();
    mpq_class y = currentLayer().y();

    int pos = 0;
    if (dataBlock.at(pos) == 'X') {
        QString x_str;
        pos++;
        while (dataBlock.at(pos).isDigit() || (dataBlock.at(pos) == '-') || (dataBlock.at(pos) == '+')) {
            x_str += dataBlock.at(pos++);
        }
        x = makeCoordinate( x_str );
    }
    if (dataBlock.at(pos) == 'Y') {
        QString y_str;
        pos++;
        while (dataBlock.at(pos).isDigit() || (dataBlock.at(pos) == '-') || (dataBlock.at(pos) == '+')) {
            y_str += dataBlock.at(pos++);
        }
        y = makeCoordinate( y_str );
    }

    if (dataBlock.at(pos) == 'D') {
        // parse D code
        setDCode( dataBlock.mid(pos) );
    }

    currentLayer().draw(x,y);
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














Layer::Layer()
{
    m_current_x = 0;
    m_current_y = 0;
    m_imagePolarity = positive;

    m_drawMode = off;
    m_aperture = 0;
    m_interpolationMode = linear;
}

Layer::~Layer()
{

}

void Layer::draw( mpq_class x, mpq_class y )
{
    qDebug() << "draw(): x=" << x.get_d() << " y=" << y.get_d();

    if (m_drawMode == on) {
        // assume linear interpolation with filling off
        Line* line = new Line( m_current_x, m_current_y, x, y, m_aperture );
        m_objects << line;
    }

    m_current_x = x;
    m_current_y = y;
}







Object::Object()
{
}

QGraphicsItem* Object::getGraphicsItem() const
{
    return 0;
}

Line::Line( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, int aperture )
{
    m_x1 = x1;
    m_y1 = y1;
    m_x2 = x2;
    m_y2 = y2;
    m_aperture = aperture;
}

QGraphicsItem* Line::getGraphicsItem() const
{
    qreal x1 = m_x1.get_d();
    qreal y1 = m_y1.get_d();
    qreal x2 = m_x2.get_d();
    qreal y2 = m_y2.get_d();
    QGraphicsLineItem* line = new QGraphicsLineItem(x1,y1,x2,y2);
    return line;
}
