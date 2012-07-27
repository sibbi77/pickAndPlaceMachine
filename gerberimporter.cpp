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
    enum {readLine,process,parameterBlock} state = readLine;
    while (!finished) {
        switch (state) {
        case readLine:
        {
            QString temp = stream.readLine();
            if (temp.isEmpty())
                finished = true;
            line += temp;
            state = process;
            break;
        }
        case process:
        {
            int idx1 = line.indexOf('%');
            int idx2 = line.indexOf('*');
            if ((idx2 != -1) && ((idx2 < idx1) || (idx1 == -1))) {
                // complete data block found
                QString dataBlock = line.left(idx2+1);
                line.remove( 0, idx2+1 );
                processDataBlock( dataBlock );
                state = process;
                continue;
            }
            if (idx1 != -1) {
                // start of parameter block found
                line.remove( 0, idx1+1 );
                state = parameterBlock;
                continue;
            }
            state = readLine;
            break;
        }
        case parameterBlock:
        {
            int idx1 = line.indexOf('%');
            if (idx1 != -1) {
                // end of parameter block found
                QString parameterBlock = line.left(idx1);
                line.remove( 0, idx1+1 );
                processParameterBlock( parameterBlock );
                state = process;
                continue;
            }
            QString temp = stream.readLine();
            if (temp.isEmpty())
                finished = true;
            line += temp;
            break;
        }
        } // switch
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
