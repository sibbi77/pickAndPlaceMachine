#ifndef GERBERIMPORTER_H
#define GERBERIMPORTER_H

#include <QString>

class GerberImporter
{
public:
    GerberImporter();

    bool import( QString filename );

protected:
    void processDataBlock( QString dataBlock );
    void processParameterBlock( QString parameterBlock );
};

#endif // GERBERIMPORTER_H
