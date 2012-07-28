#ifndef GERBERIMPORTER_H
#define GERBERIMPORTER_H

#include <QString>
#include <QStringList>
#include <gmpxx.h>

class Layer
{
public:
    enum ImagePolarity {positive, negative};
public:
    Layer();
    ~Layer();

    void setImagePolarity( ImagePolarity p ) {m_imagePolarity = p;}
    ImagePolarity imagePolarity() const {return m_imagePolarity;}
    void setName( QString name ) {m_name = name;}
    QString name() const {return m_name;}
    bool isEmpty() const {return m_objects.isEmpty();}

protected:
    mpq_class m_current_x;
    mpq_class m_current_y;
    ImagePolarity m_imagePolarity;
    QString m_name;
    QList<bool> m_objects;
};

class GerberImporter
{
public:
    GerberImporter();

    bool import( QString filename );

protected:
    bool processDataBlock( QString dataBlock );
    void processParameterBlock( QString parameterBlock );
    void parameterFS( QString parameterBlock );
    void parameterMO( QString parameterBlock );
    void parameterLN( QString parameterBlock );

    Layer& newLayer();
    Layer& currentLayer();

    QStringList m_deprecated_parameters;

    QList<Layer> m_layers;

    // current graphics state
    int m_FS_integer, m_FS_decimals;
    enum {omit_leading, omit_trailing} m_FS_zero;
    enum {mm,in} m_MO;
};

#endif // GERBERIMPORTER_H
