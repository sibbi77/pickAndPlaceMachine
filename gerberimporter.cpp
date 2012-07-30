#include "gerberimporter.h"

#include <QtCore>
#include <QtGui>

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
    else if (dataBlock.left(4) == "G54D")
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

    Aperture aperture;
    QList<mpq_class> arguments;

    pos = aperture_str.indexOf(',');
    if (pos != -1) {
        // parse arguments
        pos++;
        while ((pos < aperture_str.length()) && (aperture_str.at(pos).isDigit() || aperture_str.at(pos) == '.')) {
            QString argument_str;
            while ((pos < aperture_str.length()) && (aperture_str.at(pos).isDigit() || aperture_str.at(pos) == '.')) {
                argument_str += aperture_str.at(pos++);
            }
            mpq_class temp = mpq_from_decimal_string( argument_str );
            arguments << temp;
            qDebug() << "argument:" << temp.get_d();
            if (pos < aperture_str.length() && aperture_str.at(pos) == 'X')
                pos++; // next argument
        }
    }

    if (aperture_str.left(2) == "C,") {
        // circular aperture
        if (arguments.size() == 1)
            aperture.setCircle( arguments.at(0) );
        else if (arguments.size() == 2)
            aperture.setCircle( arguments.at(0), arguments.at(1) );
        else if (arguments.size() == 3)
            aperture.setCircle( arguments.at(0), arguments.at(1), arguments.at(2) );
    } else if (aperture_str.left(2) == "R,") {
        // rectangular aperture
    } else if (aperture_str.left(2) == "O,") {
        // oval aperture
    } else if (aperture_str.left(2) == "P,") {
        // polygon aperture
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
    qDebug() << "macro:" << collect_parameter_AM;

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

    if (pos < dataBlock.length() && dataBlock.at(pos) == 'D') {
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












Layer::Layer(QHash<int,Aperture> apertures)
{
    m_current_x = 0;
    m_current_y = 0;
    m_imagePolarity = positive;

    m_drawMode = off;
    m_aperture = 0;
    m_interpolationMode = linear;

    m_apertures = apertures;
}

Layer::~Layer()
{

}

void Layer::draw( mpq_class x, mpq_class y )
{
    qDebug() << "draw(): x=" << x.get_d() << " y=" << y.get_d() << " Aperture:" << m_aperture;

    if (m_drawMode == on) {
        // assume linear interpolation with filling off
        if (!m_apertures.contains(m_aperture)) {
            qDebug() << "cannot draw line; undefined aperture.";
            return;
        }
        Line* line = new Line( m_current_x, m_current_y, x, y, m_apertures.value(m_aperture) );
        m_objects << line;
    } else if (m_drawMode == flash) {
        Flash* flash = new Flash( x, y, m_apertures.value(m_aperture) );
        m_objects << flash;
    }

    m_current_x = x;
    m_current_y = y;
}

void Layer::defineAperture( int num, Aperture aperture )
{
    m_apertures[num] = aperture;
}






Object::Object()
{
}

QGraphicsItem* Object::getGraphicsItem() const
{
    return 0;
}

Line::Line( mpq_class x1, mpq_class y1, mpq_class x2, mpq_class y2, Aperture aperture )
{
    m_x1 = x1;
    m_y1 = y1;
    m_x2 = x2;
    m_y2 = y2;
    m_aperture = aperture;
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

Flash::Flash( mpq_class x, mpq_class y, Aperture aperture )
{
    m_x = x;
    m_y = y;
    m_aperture = aperture;
}

QGraphicsItem* Flash::getGraphicsItem() const
{
    qreal x = m_x.get_d();
    qreal y = m_y.get_d();
    QGraphicsItem* item = m_aperture.getGraphicsItem();
    item->moveBy( x, y );
    qDebug() << item->pos() << item->boundingRect();
    return item;
}






Aperture::Aperture()
{
    m_type = circle;
    m_hole = noHole;
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
        qreal dia = diameter().get_d();
        QGraphicsEllipseItem* circle = new QGraphicsEllipseItem(-dia/2.0,-dia/2.0,dia,dia);
        QBrush brush( circle->pen().color() );
        circle->setBrush( brush );
        return circle;
    }
    if (type() == macro) {
        qDebug() << "QGraphicsItem* Aperture::getGraphicsItem() const ## macro";
        QGraphicsItemGroup* group = new QGraphicsItemGroup;

        QList<QList<mpq_class> > primitives = m_macro.calc( m_arguments );
        foreach (QList<mpq_class> primitive, primitives) {
            qDebug() << "primitive:";
            for (int n=0; n<primitive.size(); n++)
                qDebug() << primitive.value(n).get_d();
            if (primitive.value(0) == 5) {
                // regular polygon

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
    qDebug() << "ApertureMacro:" << primitives;
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
        pos = pos_after_number;
    }

    return result;
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

////! \brief \internal expects \c term to be free of white space
//mpq_class ApertureMacro::calc_intern1( QString term, QList<mpq_class> arguments ) const
//{
//    if (term.isEmpty())
//        return 0;

//    qDebug() << "calc:" << term;

//    bool ok;
//    int pos_after_number;
//    mpq_class temp = GerberImporter::mpq_from_decimal_string( term, &ok, &pos_after_number );

//    if (ok && pos_after_number >= term.length())
//        return temp; // parser finished

//    // check for placeholder
//    if (!ok && term.at(0) == '$') {
//        temp = GerberImporter::mpq_from_decimal_string( term.mid(1), &ok, &pos_after_number );

//    }

//    // perform calculation
//    qDebug() << temp.get_d() << ok << pos_after_number;

//    return mpq_class();
//}







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
