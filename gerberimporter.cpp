#include "gerberimporter.h"

#include <gmpxx.h>
#include <QtCore>

GerberImporter::GerberImporter()
{
//    QList<QByteArray> l = QTextCodec::availableCodecs();
//    foreach (QByteArray name, l)
//        qDebug() << name;
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
            if (state == dataBlock)
                processDataBlock( block );
            else
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

void GerberImporter::processDataBlock( QString dataBlock )
{
    static mpq_class current_x(0);
    static mpq_class current_y(0);

    dataBlock = dataBlock.trimmed();
    qDebug() << "dataBlock:" << dataBlock;
}

void GerberImporter::processParameterBlock( QString parameterBlock )
{
    parameterBlock = parameterBlock.trimmed();
    qDebug() << "parameterBlock:" << parameterBlock;

}
