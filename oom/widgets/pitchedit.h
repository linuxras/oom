//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: pitchedit.h,v 1.2 2004/01/09 17:12:54 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PITCHEDIT_H__
#define __PITCHEDIT_H__

#include <QSpinBox>

//---------------------------------------------------------
//   PitchEdit
//---------------------------------------------------------

class PitchEdit : public QSpinBox
{
    Q_OBJECT

    bool deltaMode;

protected:
    virtual QString mapValueToText(int v);
    virtual int mapTextToValue(bool* ok);

public:
    PitchEdit(QWidget* parent = nullptr);
    void setDeltaMode(bool);
};

extern QString pitch2string(int v);

#endif
