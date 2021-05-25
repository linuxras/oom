//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: midisyncimpl.h,v 1.1.1.1.2.3 2009/05/03 04:14:01 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDISYNCIMPL_H__
#define __MIDISYNCIMPL_H__

#include "ui_midisync.h"
#include "sync.h"

class QCloseEvent;
class QShowEvent;
class QTreeWidgetItem;

//----------------------------------------------------------
//   MidiSyncLViewItem
//----------------------------------------------------------

class MidiSyncLViewItem : public QTreeWidgetItem
{
    int _port;

public:

    MidiSyncLViewItem(QTreeWidget* parent)
    : QTreeWidgetItem(parent)
    {
        _port = -1;
        _inDet = _curDet = _tickDet = false;
    }

    bool _inDet;
    bool _curDet;
    bool _curMTCDet;
    bool _tickDet;
    bool _MRTDet;
    bool _MMCDet;
    bool _MTCDet;
    int _recMTCtype;

    int _idOut;
    int _idIn;

    bool _sendMC;
    bool _sendMRT;
    bool _sendMMC;
    bool _sendMTC;
    bool _recMC;
    bool _recMRT;
    bool _recMMC;
    bool _recMTC;

    bool _recRewOnStart;

    int port() const
    {
        return _port;
    }
    void setPort(int port);

    void copyFromSyncInfo(const MidiSyncInfo &sp);
    void copyToSyncInfo(MidiSyncInfo &sp);
};

//---------------------------------------------------------
//   MSConfig
//---------------------------------------------------------

class MidiSyncConfig : public QFrame, public Ui::MidiSyncConfigBase
{
    Q_OBJECT

    bool inHeartBeat;
    bool _dirty;

    void updateSyncInfoLV();
    void closeEvent(QCloseEvent*);
    void setToolTips(QTreeWidgetItem *item);
    void setWhatsThis(QTreeWidgetItem *item);
    void addDevice(QTreeWidgetItem *item, QTreeWidget *tree);
	virtual void showEvent(QShowEvent*);

private slots:
    void heartBeat();
    void syncChanged();
    void extSyncChanged(bool v);
    void cancel();
    void apply();
    void dlvClicked(QTreeWidgetItem*, int);
    void dlvDoubleClicked(QTreeWidgetItem*, int);

public slots:
    void songChanged(int);

public:
    MidiSyncConfig(QWidget* parent = nullptr);
    ~MidiSyncConfig();
    void setDirty();
};

#endif

