//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: synth.cpp,v 1.43.2.23 2009/12/15 03:39:58 terminator356 Exp $
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include "config.h"
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <vector>
#include <fcntl.h>
#include <dlfcn.h>

#include <QDir>
#include <QMenu>
#include <QStandardItem>
#include <QStandardItemModel>

#include "app.h"
#include "synth.h"
#include "xml.h"
#include "midi.h"
#include "midiport.h"
#include "mididev.h"
//#include "libsynti/mess.h"
#include "synti/libsynti/mess.h"   // p4.0.2
#include "song.h"
#include "audio.h"
#include "event.h"
#include "mpevent.h"
#include "audio.h"
#include "midiseq.h"
#include "midictrl.h"
//#include "stringparam.h"

std::vector<Synth*> synthis; // array of available synthis

extern void connectNodes(AudioTrack*, AudioTrack*);

/*
//---------------------------------------------------------
//   description
//---------------------------------------------------------

const char* MessSynth::description() const
	  {
	  return _descr ? _descr->description : "";
	  }

//---------------------------------------------------------
//   version
//---------------------------------------------------------

const char* MessSynth::version() const
	  {
	  return _descr ? _descr->version : "";
	  }
 */

bool MessSynthIF::guiVisible() const
{
	return _mess ? _mess->guiVisible() : false;
}

void MessSynthIF::showGui(bool v)
{
	if (v == guiVisible())
		return;
	if (_mess)
		_mess->showGui(v);
}

bool MessSynthIF::hasGui() const
{
	if (_mess)
		return _mess->hasGui();
	return false;
}

MidiPlayEvent MessSynthIF::receiveEvent()
{
	if (_mess)
		return _mess->receiveEvent();
	return MidiPlayEvent();
}

int MessSynthIF::eventsPending() const
{
	if (_mess)
		return _mess->eventsPending();
	return 0;
}

void MessSynthIF::getGeometry(int* x, int* y, int* w, int* h) const
{
	if (_mess)
		_mess->getGeometry(x, y, w, h);
}

void MessSynthIF::setGeometry(int x, int y, int w, int h)
{
	if (_mess)
		_mess->setGeometry(x, y, w, h);
}

//---------------------------------------------------------
//   findSynth
//    search for synthesizer base class
//---------------------------------------------------------

//static Synth* findSynth(const QString& sclass)

static Synth* findSynth(const QString& sclass, const QString& label)
{
	for (std::vector<Synth*>::iterator i = synthis.begin();
			i != synthis.end(); ++i)
	{
		//if ((*i)->baseName() == sclass)
		//if ((*i)->name() == sclass)
		if (((*i)->baseName() == sclass) && (label.isEmpty() || ((*i)->name() == label)))

			return *i;
	}
	printf("synthi class:%s label:%s not found\n", sclass.toLatin1().constData(), label.toLatin1().constData());
	return 0;
}

//---------------------------------------------------------
//   createSynthInstance
//    create a synthesizer instance of class "label"
//---------------------------------------------------------

//static SynthI* createSynthI(const QString& sclass)

static SynthI* createSynthInstance(const QString& sclass, const QString& label)
{
	//Synth* s = findSynth(sclass);
	Synth* s = findSynth(sclass, label);
	SynthI* si = 0;
	if (s)
	{
		si = new SynthI();
		QString n;
		n.setNum(s->instances());
		//QString instance_name = s->baseName() + "-" + n;
		QString instance_name = s->name() + "-" + n;

		if (si->initInstance(s, instance_name))
		{
			delete si;
			return 0;
		}
	}
	else
		printf("createSynthInstance: synthi class:%s label:%s not found\n", sclass.toLatin1().constData(), label.toLatin1().constData());
	return si;
}

//---------------------------------------------------------
//   Synth
//---------------------------------------------------------

//Synth::Synth(const QFileInfo& fi)
//   : info(fi)
//Synth::Synth(const QFileInfo& fi, QString label)
//   : info(fi), _name(label)

Synth::Synth(const QFileInfo& fi, QString label, QString descr, QString maker, QString ver)
: info(fi), _name(label), _description(descr), _maker(maker), _version(ver)
{
	_instances = 0;
}

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

//void* MessSynth::instantiate()

void* MessSynth::instantiate(const QString& instanceName)
{
	++_instances;

	//QString n;
	//n.setNum(_instances);
	//QString instanceName = baseName() + "-" + n;

	doSetuid();
	QByteArray ba = info.filePath().toLatin1();
	const char* path = ba.constData();

	// load Synti dll
	void* handle = dlopen(path, RTLD_NOW);
	if (handle == 0)
	{
		fprintf(stderr, "Synth::instantiate: dlopen(%s) failed: %s\n",
				path, dlerror());
		undoSetuid();
		return 0;
	}
	typedef const MESS * (*MESS_Function)();
	MESS_Function msynth = (MESS_Function) dlsym(handle, "mess_descriptor");

	if (!msynth)
	{
		const char *txt = dlerror();
		if (txt)
		{
			fprintf(stderr,
					"Unable to find msynth_descriptor() function in plugin "
					"library file \"%s\": %s.\n"
					"Are you sure this is a MESS plugin file?\n",
					info.filePath().toLatin1().constData(), txt);
			undoSetuid();
			return 0;
		}
	}
	_descr = msynth();
	if (_descr == 0)
	{
		fprintf(stderr, "Synth::instantiate: no MESS descr found\n");
		undoSetuid();
		return 0;
	}
	Mess* mess = _descr->instantiate(sampleRate, oom, &oomProject, instanceName.toLatin1().constData());
	undoSetuid();
	return mess;
}

//---------------------------------------------------------
//   SynthI
//---------------------------------------------------------

SynthI::SynthI()
: AudioTrack(AUDIO_SOFTSYNTH)
{
	synthesizer = 0;
	_sif = 0;
	_rwFlags = 1;
	_openFlags = 1;
	_readEnable = false;
	_writeEnable = false;

	_curBankH = 0;
	_curBankL = 0;
	_curProgram = 0;
	m_cachenrpn = false;

	setVolume(1.0, true);
	setPan(0.0, true);
}

//---------------------------------------------------------
//   open
//---------------------------------------------------------

QString SynthI::open()
{
	// Make it behave like a regular midi device.
	_readEnable = false;
	_writeEnable = (_openFlags & 0x01);

	return QString("OK");
}

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void SynthI::close()
{
	_readEnable = false;
	_writeEnable = false;
}

//---------------------------------------------------------
//   putMidiEvent
//---------------------------------------------------------

bool SynthI::putEvent(const MidiPlayEvent& ev)
//bool SynthI::putMidiEvent(const MidiPlayEvent& ev) 
{
	if (_writeEnable)
		return _sif->putEvent(ev);

	// Hmm, act as if the event went through?
	//return true;
	return false;
}

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void SynthI::setName(const QString& s)
{
	AudioTrack::setName(s);
	MidiDevice::setName(s);
}

//---------------------------------------------------------
//   currentProg
//---------------------------------------------------------

void SynthI::currentProg(unsigned long *prog, unsigned long *bankL, unsigned long *bankH)
{
	if (prog)
		*prog = _curProgram;
	if (bankL)
		*bankL = _curBankL;
	if (bankH)
		*bankH = _curBankH;
}

//---------------------------------------------------------
//   init
//---------------------------------------------------------

//bool MessSynthIF::init(Synth* s)

bool MessSynthIF::init(Synth* s, SynthI* si)
{
	//_mess = (Mess*)s->instantiate();
	_mess = (Mess*) ((MessSynth*) s)->instantiate(si->name());

	return (_mess == 0);
}

int MessSynthIF::channels() const
{
	return _mess->channels();
}

int MessSynthIF::totalOutChannels() const
{
	return _mess->channels();
}

int MessSynthIF::totalInChannels() const
{
	return 0;
}

//SynthIF* MessSynth::createSIF() const

SynthIF* MessSynth::createSIF(SynthI* si)
{
	//return new MessSynthIF(si);

	MessSynthIF* sif = new MessSynthIF(si);
	sif->init(this, si);
	return sif;
}

//---------------------------------------------------------
//   initInstance
//    returns false on success
//---------------------------------------------------------

bool SynthI::initInstance(Synth* s, const QString& instanceName)
{
	synthesizer = s;
	//sif         = s->createSIF();
	//_sif        = s->createSIF(this);

	//sif->init(s);

	setName(instanceName); // set midi device name
	setIName(instanceName); // set instrument name
	_sif = s->createSIF(this);

	// p3.3.38
	//AudioTrack::setChannels(_sif->channels());
	AudioTrack::setTotalOutChannels(_sif->totalOutChannels());
	AudioTrack::setTotalInChannels(_sif->totalInChannels());

	//---------------------------------------------------
	//  read available controller from synti
	//---------------------------------------------------

	int id = 0;
	MidiControllerList* cl = MidiInstrument::controller();
	for (;;)
	{
		const char* name;
		int ctrl;
		int min;
		int max;
		int initval = CTRL_VAL_UNKNOWN;
		id = _sif->getControllerInfo(id, &name, &ctrl, &min, &max, &initval);
		//            printf("looking for params\n");
		if (id == 0)
			break;
		//             printf("got parameter:: %s\n", name);


		// Added by T356. Override existing program controller.
		iMidiController i = cl->end();
		if (ctrl == CTRL_PROGRAM)
		{
			for (i = cl->begin(); i != cl->end(); ++i)
			{
				if (i->second->num() == CTRL_PROGRAM)
				{
					delete i->second;
					cl->erase(i);

					break;
				}
			}
		}

		MidiController* c = new MidiController(QString(name), ctrl, min, max, initval);
		cl->add(c);
	}

	EventList* iel = midiState();
	if (!iel->empty())
	{
		for (iEvent i = iel->begin(); i != iel->end(); ++i)
		{
			Event ev = i->second;
			MidiPlayEvent pev(0, 0, 0, ev);
			if (_sif->putEvent(pev))
				break; // try later
		}
		iel->clear();
	}

	unsigned long idx = 0;
	for (std::vector<float>::iterator i = initParams.begin(); i != initParams.end(); ++i, ++idx)
		_sif->setParameter(idx, *i);

	// p3.3.40 Since we are done with the (sometimes huge) initial parameters list, clear it.
	// TODO: Decide: Maybe keep them around for a 'reset to previously loaded values' (revert) command? ...
	initParams.clear();

	return false;
}

//---------------------------------------------------------
//   getControllerInfo
//---------------------------------------------------------

int MessSynthIF::getControllerInfo(int id, const char** name, int* ctrl, int* min, int* max, int* initval)
{
	return _mess->getControllerInfo(id, name, ctrl, min, max, initval);
}

//---------------------------------------------------------
//   SynthI::deactivate
//---------------------------------------------------------

void SynthI::deactivate2()
{
	removeMidiInstrument(this);
	midiDevices.remove(this);
	if (midiPort() != -1)
	{
		// synthi is attached
		midiPorts[midiPort()].setMidiDevice(0);
	}
}
//---------------------------------------------------------
//   deactivate3
//---------------------------------------------------------

void SynthI::deactivate3()
{
	_sif->deactivate3();
	// Moved below by Tim. p3.3.14
	//synthesizer->incInstances(-1);

	if (debugMsg)
		printf("SynthI::deactivate3 deleting _sif...\n");

	delete _sif;
	_sif = 0;

	if (debugMsg)
		printf("SynthI::deactivate3 decrementing synth instances...\n");

	synthesizer->incInstances(-1);
}

void MessSynthIF::deactivate3()
{
	if (_mess)
	{
		delete _mess;
		_mess = 0;
	}
}

//---------------------------------------------------------
//   ~SynthI
//---------------------------------------------------------

SynthI::~SynthI()
{
	deactivate2();
	deactivate3();
}

//---------------------------------------------------------
//   initMidiSynth
//    search for software synthis and advertise
//---------------------------------------------------------

void initMidiSynth()
{
	QString s = oomGlobalLib + "/synthi";

	QDir pluginDir(s, QString("*.so")); // ddskrjo
	if (debugMsg)
		printf("searching for software synthesizer in <%s>\n", s.toLatin1().constData());
	if (pluginDir.exists())
	{
		QFileInfoList list = pluginDir.entryInfoList();
		QFileInfoList::iterator it = list.begin();
		QFileInfo* fi;
		while (it != list.end())
		{
			fi = &*it;

			//doSetuid();
			QByteArray ba = fi->filePath().toLatin1();
			const char* path = ba.constData();

			// load Synti dll
			//printf("initMidiSynth: dlopen file:%s name:%s desc:%s\n", fi->filePath().toLatin1().constData(), QString(descr->name), QString(descr->description), QString(""), QString(descr->version)));
			void* handle = dlopen(path, RTLD_NOW);
			if (handle == 0)
			{
				fprintf(stderr, "initMidiSynth: MESS dlopen(%s) failed: %s\n", path, dlerror());
				//undoSetuid();
				//return 0;
				++it;
				continue;
			}
			typedef const MESS * (*MESS_Function)();
			MESS_Function msynth = (MESS_Function) dlsym(handle, "mess_descriptor");

			if (!msynth)
			{
#if 1
				const char *txt = dlerror();
				if (txt)
				{
					fprintf(stderr,
							"Unable to find msynth_descriptor() function in plugin "
							"library file \"%s\": %s.\n"
							"Are you sure this is a MESS plugin file?\n",
							path, txt);
					//undoSetuid();
					//return 0;
				}
#endif      
				dlclose(handle);
				++it;
				continue;
			}
			const MESS* descr = msynth();
			if (descr == 0)
			{
				fprintf(stderr, "initMidiSynth: no MESS descr found in %s\n", path);
				//undoSetuid();
				//return 0;
				dlclose(handle);
				++it;
				continue;
			}
			//Mess* mess = descr->instantiate(sampleRate, oom, &oomProject, instanceName.toLatin1().constData());
			//undoSetuid();




			//synthis.push_back(new MessSynth(*fi));
			synthis.push_back(new MessSynth(*fi, QString(descr->name), QString(descr->description), QString(""), QString(descr->version)));

			dlclose(handle);
			++it;
		}
		if (debugMsg)
			printf("%zd soft synth found\n", synthis.size());
	}
}

//---------------------------------------------------------
//   createSynthI
//    create a synthesizer instance of class "label"
//---------------------------------------------------------

//SynthI* Song::createSynthI(const QString& sclass)

SynthI* Song::createSynthI(const QString& sclass, const QString& label)
{
	//printf("Song::createSynthI calling ::createSynthI class:%s\n", sclass.toLatin1().constData());

	//SynthI* si = ::createSynthI(sclass);
	//SynthI* si = ::createSynthI(sclass, label);
	SynthI* si = createSynthInstance(sclass, label);
	if (!si)
		return 0;
	//printf("Song::createSynthI created SynthI. Before insertTrack1...\n");

	insertTrack1(si, -1);
	//printf("Song::createSynthI after insertTrack1. Before msgInsertTrack...\n");

	msgInsertTrack(si, -1, true); // add to instance list
	//printf("Song::createSynthI after msgInsertTrack. Before insertTrack3...\n");

	OutputList* ol = song->outputs();
	// add default route to master (first audio output)
	if (!ol->empty())
	{
		AudioOutput* ao = ol->front();
		// p3.3.38
		//audio->msgAddRoute(Route(si, -1), Route(ao, -1));
		//audio->msgAddRoute(Route((AudioTrack*)si, -1), Route(ao, -1));
		// Make sure the route channel and channels are valid.
		audio->msgAddRoute(Route((AudioTrack*) si, 0, ((AudioTrack*) si)->channels()), Route(ao, 0, ((AudioTrack*) si)->channels()));

		audio->msgUpdateSoloStates();
	}

	// Now that the track has been added to the lists in insertTrack2(),
	//  if it's a dssi synth, OSC can find the synth, and initialize (and show) its native gui.
	// No, initializing OSC without actually showing the gui doesn't work, at least for
	//  dssi-vst plugins - without showing the gui they exit after ten seconds.
	//si->initGui();

	return si;
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void SynthI::write(int level, Xml& xml) const
{
	xml.tag(level++, "SynthI");
	AudioTrack::writeProperties(level, xml);
	xml.strTag(level, "class", synth()->baseName());

	// To support plugins like dssi-vst where all the baseNames are the same 'dssi-vst' and the label is the name of the dll file.
	// Added by Tim. p3.3.16
	xml.strTag(level, "label", synth()->name());

	//---------------------------------------------
	// if soft synth is attached to a midi port,
	// write out port number
	//---------------------------------------------

	if (midiPort() != -1)
		xml.intTag(level, "port", midiPort());

	if (hasGui())
	{
		xml.intTag(level, "guiVisible", guiVisible());
		int x, y, w, h;
		w = 0;
		h = 0;
		getGeometry(&x, &y, &w, &h);
		if (h || w)
			xml.qrectTag(level, "geometry", QRect(x, y, w, h));
	}

	_stringParamMap.write(level, xml, "stringParam");

	xml.tag(level, "curProgram bankH=\"%ld\" bankL=\"%ld\" prog=\"%ld\"/", _curBankH, _curBankL, _curProgram);

	_sif->write(level, xml);
	xml.etag(level, "SynthI");
}

void MessSynthIF::write(int level, Xml& xml) const
{
	//---------------------------------------------
	// dump current state of synth
	//---------------------------------------------

	int len = 0;
	const unsigned char* p;
	_mess->getInitData(&len, &p);
	if (len)
	{
		xml.tag(level++, "midistate");
		xml.nput(level++, "<event type=\"%d\"", Sysex);
		xml.nput(" datalen=\"%d\">\n", len);
		xml.nput(level, "");
		for (int i = 0; i < len; ++i)
		{
			if (i && ((i % 16) == 0))
			{
				xml.nput("\n");
				xml.nput(level, "");
			}
			xml.nput("%02x ", p[i] & 0xff);
		}
		xml.nput("\n");
        xml.tag(--level, "/event");
        xml.etag(--level, "midistate");
	}
}

//---------------------------------------------------------
//   SynthI::readProgram
//---------------------------------------------------------

void SynthI::readProgram(Xml& xml, const QString& name)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				xml.unknown(name.toLatin1().constData());
				break;
			case Xml::Attribut:
				if (tag == "bankH")
					_curBankH = xml.s2().toUInt();
				else
					if (tag == "bankL")
					_curBankL = xml.s2().toUInt();
				else
					if (tag == "prog")
					_curProgram = xml.s2().toUInt();
				else
					xml.unknown(name.toLatin1().constData());
				break;
			case Xml::TagEnd:
				if (tag == name)
					return;
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   SynthI::read
//---------------------------------------------------------

void SynthI::read(Xml& xml)
{
	QString sclass;
	QString label;

	int port = -1;
	bool startgui = false;
	QRect r;

	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "class")
					sclass = xml.parse1();
				else if (tag == "label")
					label = xml.parse1();
				else if (tag == "port")
					port = xml.parseInt();
				else if (tag == "guiVisible")
					startgui = xml.parseInt();
				else if (tag == "midistate")
					readMidiState(xml);
				else if (tag == "param")
				{
					float val = xml.parseFloat();
					initParams.push_back(val);
				}
				else if (tag == "stringParam")
					_stringParamMap.read(xml, tag);
				else if (tag == "curProgram")
					readProgram(xml, tag);
				else if (tag == "geometry")
					r = readGeometry(xml, tag);
				else if (AudioTrack::readProperties(xml, tag))
					xml.unknown("softSynth");
				break;
			case Xml::TagEnd:
				if (tag == "SynthI")
				{
					//Synth* s = findSynth(sclass);
					Synth* s = findSynth(sclass, label);
					if (s == 0)
						return;
					if (initInstance(s, name()))
						return;
					song->insertTrack(this, -1);
					if (port != -1 && port < MIDI_PORTS)
						midiPorts[port].setMidiDevice(this);

					// Now that the track has been added to the lists in insertTrack2(),
					//  if it's a dssi synth, OSC can find the synth, and initialize (and show) its native gui.
					// No, initializing OSC without actually showing the gui doesn't work, at least for
					//  dssi-vst plugins - without showing the gui they exit after ten seconds.
					//initGui();
					showGui(startgui);
					setGeometry(r.x(), r.y(), r.width(), r.height());

					mapRackPluginsToControllers();

					// Now that the track has been added to the lists in insertTrack2(), if it's a dssi synth
					//  OSC can find the track and its plugins, and start their native guis if required...
					showPendingPluginNativeGuis();

					return;
				}
			default:
				break;
		}
	}
	AudioTrack::mapRackPluginsToControllers();
}

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

const char* MessSynthIF::getPatchName(int channel, int prog, MType type, bool drum)
{
	if (_mess)
	{
		//return _mess->getPatchName(channel, prog, type, drum);
		const char* s = _mess->getPatchName(channel, prog, type, drum);
		if (s)
			return s;
	}
	return "";
}

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void MessSynthIF::populatePatchPopup(QMenu* menu, int ch, MType, bool)
{
	menu->clear();
	const MidiPatch* mp = _mess->getPatchInfo(ch, 0);
	while (mp)
	{
		int id = ((mp->hbank & 0xff) << 16)
				+ ((mp->lbank & 0xff) << 8) + mp->prog;
		/*
		int pgid = ((mp->hbank & 0xff) << 8) | (mp->lbank & 0xff) | 0x40000000;
		int itemnum = menu->indexOf(pgid);
		if(itemnum == -1)
		{
		  QPopupMenu* submenu = new QPopupMenu(menu);
		  itemnum =
		}
		 */
		QAction *act = menu->addAction(QString(mp->name));
		act->setData(id);
		mp = _mess->getPatchInfo(ch, mp);
	}
}

//---------------------------------------------------------
//   populatePatchModel
//---------------------------------------------------------

void MessSynthIF::populatePatchModel(QStandardItemModel* model, int ch, MType, bool)
{
	model->clear();
	const MidiPatch* mp = _mess->getPatchInfo(ch, 0);
	QStandardItem* root = model->invisibleRootItem();
	while (mp)
	{
		int id = ((mp->hbank & 0xff) << 16) + ((mp->lbank & 0xff) << 8) + mp->prog;

		QList<QStandardItem*> row;
		QString strId = QString::number(id);
		QStandardItem* idItem = new QStandardItem(strId);
		QStandardItem* nItem = new QStandardItem(QString(mp->name));
		nItem->setToolTip(QString(mp->name));
		row.append(nItem);
		row.append(idItem);
		root->appendRow(row);
		//QAction *act = menu->addAction(QString(mp->name));
		//act->setData(id);
		mp = _mess->getPatchInfo(ch, mp);
	}
}

//---------------------------------------------------------
//   preProcessAlways
//---------------------------------------------------------

void SynthI::preProcessAlways()
{
	if (_sif)
		_sif->preProcessAlways();
	_processed = false;
	if(off())
	{
	    // Clear any accumulated play events.
	    playEvents()->clear();
	    // Eat up any fifo events.
	    while(!eventFifo.isEmpty()) 
	      eventFifo.get();  
	}
}

void MessSynthIF::preProcessAlways()
{
	if (_mess)
		_mess->processMessages();
}

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool SynthI::getData(unsigned pos, int ports, unsigned n, float** buffer)
{
	for (int k = 0; k < ports; ++k)
		memset(buffer[k], 0, n * sizeof (float));

	int p = midiPort();
	MidiPort* mp = (p != -1) ? &midiPorts[p] : 0;
	MPEventList* el = playEvents();

	iMPEvent ie = el->begin(); //nextPlayEvent();

	ie = _sif->getData(mp, el, ie, pos, ports, n, buffer);

	//setNextPlayEvent(ie);
	el->erase(el->begin(), ie);
	return true;
}

iMPEvent MessSynthIF::getData(MidiPort* mp, MPEventList* el, iMPEvent i, unsigned pos, int /*ports*/, unsigned n, float** buffer)
{
	//prevent compiler warning: comparison of signed/unsigned
	unsigned int curPos = pos;
	unsigned int endPos = pos + n;
	int off = pos;
	int frameOffset = audio->getFrameOffset();

	for (; i != el->end(); ++i)
	{
		int evTime = i->time();
		if (evTime == 0)
		{
			//      printf("MessSynthIF::getData - time is 0!\n");
			//      continue;
			evTime = frameOffset; // will cause frame to be zero, problem?
		}
		unsigned int frame = evTime - frameOffset;

		//TODO           if (frame > 0) // robert: ugly fix, don't really know what is going on here
		//                          // makes PPC work much better.

		if (frame >= endPos)
		{
			printf("frame > endPos!! frame = %d >= endPos %d, i->time() %d, frameOffset %d curPos=%d\n", frame, endPos, i->time(), frameOffset, curPos);
			continue;
		}

		if (frame > curPos)
		{
			if (frame < pos)
				printf("should not happen: missed event %d\n", pos - frame);
			else
			{
				if (!_mess)
					printf("should not happen - no _mess\n");
				else
				{
					_mess->process(buffer, curPos - pos, frame - curPos);
				}
			}
			curPos = frame;
		}
		if (mp)
			mp->sendEvent(*i);
		else
		{
			if (putEvent(*i))
				break;
		}
	}
	if (endPos - curPos)
	{
		if (!_mess)
			printf("should not happen - no _mess\n");
		else
		{
			_mess->process(buffer, curPos - off, endPos - curPos);
		}
	}
	return i;
}

//---------------------------------------------------------
//   putEvent
//    return true on error (busy)
//---------------------------------------------------------

bool MessSynthIF::putEvent(const MidiPlayEvent& ev)
{
	if (midiOutputTrace)
		ev.dump();
	if (_mess)
		return _mess->processEvent(ev);
	return true;
}
