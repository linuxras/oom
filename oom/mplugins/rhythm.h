//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: rhythm.h,v 1.1.1.1 2003/10/27 18:52:44 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//
//  This code is an adaption of the random rhythm generator taken
//  from "The JAZZ++ Midi Sequencer"
//  Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all
//  rights reserved.
//  Distributed under the GNU General Public License
//=========================================================

#ifndef __RHYTHM_H__
#define __RHYTHM_H__

#include "ui_rhythmbase.h"

#include <QMainWindow>

class QCloseEvent;

class tTrack;
class tEventWin;
class tSong;
class tBarInfo;

#define MAX_GROUPS  5
#define MAX_KEYS   20

class Xml;

#if 0
//---------------------------------------------------------
//   tRhyGroup
//---------------------------------------------------------

struct tRhyGroup
{
    int contrib;
    int listen;

    tRhyGroup()
    {
        listen = 0;
        contrib = 0;
    }
    //      void write(int, Xml&);
    //      void read(Xml&);
};

//---------------------------------------------------------
//   tRhyGroups
//---------------------------------------------------------

struct tRhyGroups
{
    tRhyGroup g[MAX_GROUPS];

    tRhyGroup & operator [] (int i)
    {
        return g[i];
    }

    //      void write(int, Xml&);
    //      void read(Xml&);
};

//---------------------------------------------------------
//   tRhythm
//---------------------------------------------------------

class tRhythm
{
    friend class tRhythmWin;

    char* label;

    //      tRndArray rhythm;
    //      tRndArray length;
    //      tRndArray veloc;

    int steps_per_count;
    int count_per_bar;
    int n_bars;
    int keys[MAX_KEYS];
    int n_keys;
    int mode;
    int parm;

    int randomize;
    tRhyGroups groups;
    //      tRndArray history;

    // set by GenInit()
    long start_clock;
    long next_clock;

    //      void GenGroup(tRndArray& out, int grp, tBarInfo &bi, tRhythm *rhy[], int n_rhy);
    int Clock2i(long clock, tBarInfo &bi) const;
    int ClocksPerStep(tBarInfo &bi) const;

public:
    tRhythm(int key);
    tRhythm(const tRhythm &o);
    tRhythm & operator=(const tRhythm &o);
    virtual ~tRhythm();

    char const * GetLabel()
    {
        return label;
    }
    void SetLabel(char const *);

    void Generate(tTrack *track, long fr_clock, long to_clock, long ticks_per_bar);
    void Generate(tTrack *track, tBarInfo &bi, tRhythm *rhy[], int n_rhy);
    void GenInit(long start_clock);
    void GenerateEvent(tTrack *track, long clock, short vel, short len);

    void write(int, Xml&);
    void read(Xml&);
};
#endif

//---------------------------------------------------------
//   RhythmGen
//---------------------------------------------------------

class RhythmGen : public QMainWindow, public Ui::RhythmBase
{
    Q_OBJECT
#if 0
    wxPanel *inst_panel;
    wxText *label;
    wxSlider *steps_per_count;
    wxSlider *count_per_bar;
    wxSlider *n_bars;
    wxListBox *instrument_list;
    wxCheckBox *rand_checkbox;

    wxPanel *group_panel;
    wxListBox *group_list;
    wxSlider *group_contrib;
    wxSlider *group_listen;
    int act_group;

    tArrayEdit *length_edit;
    tArrayEdit *veloc_edit;
    tRhyArrayEdit *rhythm_edit;

    enum
    {
        MAX_INSTRUMENTS = 20
    };
    tRhythm *instruments[MAX_INSTRUMENTS];
    int n_instruments;
    int act_instrument; // -1 if none

    // this one is edited and copied from/to instruments[i]
    tRhythm edit;

    // ignore Updates while creating the window (motif)
    Bool in_create;

    // callbacks
    static void ItemCallback(wxItem& item, wxCommandEvent& event);
    static void SelectInstr(wxListBox& list, wxCommandEvent& event);
    static void SelectGroup(wxListBox& list, wxCommandEvent& event);
    static void Add(wxButton &but, wxCommandEvent& event);
    static void Del(wxButton &but, wxCommandEvent& event);
    static void Generate(wxButton &but, wxCommandEvent& event);
    static void Help();

    void Instrument2Win(int i = -1); // instrument[act_instrument] -> win
    void Win2Instrument(int i = -1); // win -> instrument[act_instrument]
    void AddInstrumentDlg();
    void AddInstrument(tRhythm *r);
    void DelInstrument();

    tEventWin *event_win;
    tSong *song;

    void RndEnable();

    char *default_filename;
    int has_changed;
    wxToolBar *tool_bar;
    float tb_width, tb_height;

    void UpInstrument();
    void DownInstrument();
    void InitInstrumentList();
#endif
    virtual void closeEvent(QCloseEvent*);

signals:
    void hideWindow();

public:
    //      virtual void OnMenuCommand(int id);
    //      virtual void OnSize(int w, int h);
    RhythmGen(QWidget* parent = nullptr, Qt::WindowFlags fo = Qt::Window);
    virtual ~RhythmGen();
    //      void OnPaint();
    //      void GenRhythm();
    //      bool OnClose();
};

#endif

