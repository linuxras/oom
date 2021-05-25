#ifndef _PROGRAMCHAMGETABLEMODEL_
#define _PROGRAMCHAMGETABLEMODEL_

#include <QStandardItemModel>
#include <QStringList>

class ProgramChangeTableModel : public QStandardItemModel
{
    Q_OBJECT
    virtual QStringList mimeTypes();

public:
    ProgramChangeTableModel(QObject *parent = nullptr);
    void emit_layoutChanged();
};
#endif
