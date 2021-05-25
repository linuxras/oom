//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: confmport.h,v 1.3 2004/01/25 11:20:31 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CONFMPORT_H__
#define __CONFMPORT_H__

#include <QWidget>
#include <QToolTip>

#include "ui_synthconfigbase.h"

class QTreeWidget;
class QTableWidget;
class QPoint;
class QMenu;
class Xml;

//---------------------------------------------------------
//   MPConfig
//    Midi Port Config
//---------------------------------------------------------

class MPConfig : public QFrame, Ui::SynthConfigBase
{
    QMenu* instrPopup;
    //QMenu* popup;
    int _showAliases; // -1: None. 0: First aliases. 1: Second aliases etc.
    void setWhatsThis(QTableWidgetItem *item, int col);
    void setToolTip(QTableWidgetItem *item, int col);
    void addItem(int row, int col, QTableWidgetItem *item, QTableWidget *table);

    Q_OBJECT

private slots:
    void rbClicked(QTableWidgetItem*);
    void mdevViewItemRenamed(QTableWidgetItem*);
    //void songChanged(int);
    void selectionChanged();
    void addInstanceClicked();
    void removeInstanceClicked();

public slots:
    void songChanged(int);

public:
    MPConfig(QWidget* parent = nullptr);
    ~MPConfig();
};

#endif
