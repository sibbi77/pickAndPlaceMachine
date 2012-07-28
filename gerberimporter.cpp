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
        if (ok)
            m_FS_decimals = temp;
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



















Layer::Layer()
{
    m_current_x = 0;
    m_current_y = 0;
    m_imagePolarity = positive;

}

Layer::~Layer()
{

}
