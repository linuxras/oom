//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: filedialog.cpp,v 1.3.2.3 2005/06/19 06:32:07 lunar_shuttle Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <errno.h>

#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <QSplitter>
#include <QStringList>

#include "icons.h"
#include "filedialog.h"
#include "../globals.h"
#include "gconfig.h"

MFileDialog::ViewType MFileDialog::lastViewUsed = GLOBAL_VIEW;
QString MFileDialog::lastUserDir = "";
QString MFileDialog::lastGlobalDir = "";

//---------------------------------------------------------
//   createDir
//    return true if dir could not created
//---------------------------------------------------------

static bool createDir(const QString& s)
{
	QString sl("/");
	QStringList l = s.split(sl, QString::SkipEmptyParts);
	QString path(sl);
	QDir dir;
	for (QStringList::Iterator it = l.begin(); it != l.end(); ++it)
	{
		dir.setPath(path);
		if (!QDir(path + sl + *it).exists())
		{
			if (!dir.mkdir(*it))
			{
				printf("mkdir failed: %s %s\n",
						path.toLatin1().constData(), (*it).toLatin1().constData());
				return true;
			}
		}
		path += sl;
		path += *it;
	}
	return false;
}

//---------------------------------------------------------
//   testDirCreate
//    return true if dir does not exist
//---------------------------------------------------------

static bool testDirCreate(QWidget* parent, const QString& path)
{
	QDir dir(path);
	if (!dir.exists())
	{
		if (QMessageBox::information(parent,
				QWidget::tr("OOMidi: get file name"),
				QWidget::tr("The directory\n") + path
				+ QWidget::tr("\ndoes not exist.\nCreate it?"),
				QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) != QMessageBox::Ok)
			return true;

		if (createDir(path))
		{
			QMessageBox::critical(parent,
					QWidget::tr("OOMidi: create directory"),
					QWidget::tr("creating dir failed"));
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------
//   globalToggled
//---------------------------------------------------------

void MFileDialog::globalToggled(bool flag)
{
	if (flag)
	{
		buttons.userButton->setChecked(!flag);
		buttons.projectButton->setChecked(!flag);
		if (lastGlobalDir.isEmpty())
			lastGlobalDir = oomGlobalShare + QString("/") + baseDir; // Initialize if first time
		QString dir = lastGlobalDir;
		setDirectory(dir);
		lastViewUsed = GLOBAL_VIEW;
	}
}

//---------------------------------------------------------
//   userToggled
//---------------------------------------------------------

void MFileDialog::userToggled(bool flag)
{
	if (flag)
	{
		buttons.globalButton->setChecked(!flag);
		buttons.projectButton->setChecked(!flag);


		if (lastUserDir.isEmpty())
		{
			lastUserDir = oomUser + QString("/") + baseDir; // Initialize if first time
		}

		if (testDirCreate(this, lastUserDir))
			setDirectory(oomUser);
		else
			setDirectory(lastUserDir);

		lastViewUsed = USER_VIEW;
	}
}

//---------------------------------------------------------
//   projectToggled
//---------------------------------------------------------

void MFileDialog::projectToggled(bool flag)
{
	if (flag)
	{
		buttons.globalButton->setChecked(!flag);
		buttons.userButton->setChecked(!flag);

		QString s;
		if (oomProject == oomProjectInitPath)
		{
			// if project path is uninitialized, meaning it is still set to oomProjectInitPath.
			// then project path is set to current pwd instead.
			//s = QString(getcwd(0,0)) + QString("/");
			s = config.projectBaseFolder;
		}
		else
			s = oomProject + QString("/"); // + baseDir;

		if (testDirCreate(this, s))
			setDirectory(oomProject);
		else
			setDirectory(s);
		lastViewUsed = PROJECT_VIEW;
	}
}


//---------------------------------------------------------
//   MFileDialog
//---------------------------------------------------------

MFileDialog::MFileDialog(const QString& dir,
		const QString& filter, QWidget* parent, bool writeFlag)
: QFileDialog(parent, QString(), QString("."), filter)
{
	showButtons = false;
	if (dir.length() > 0 && dir[0] == QChar('/'))
	{
		setDirectory(dir);
	}
	else
	{
		// We replace the original sidebar widget with our 3-button widget
		QLayout* mainlayout = this->layout();
		QSplitter* spl = (QSplitter*) mainlayout->itemAt(2)->widget();
		QWidget* original_sidebarwidget = spl->widget(0);
		original_sidebarwidget->setVisible(false);

		baseDir = dir;
		showButtons = true;

		spl->insertWidget(0, &buttons);

		// Qt >= 4.6 allows us to select icons from the theme
#if QT_VERSION >= 0x040600
		buttons.globalButton->setIcon(*globalIcon);
		buttons.userButton->setIcon(*userIcon);
		buttons.projectButton->setIcon(*projectIcon);
#else
		buttons.globalButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
		buttons.userButton->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
		buttons.projectButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
#endif	    

		connect(buttons.globalButton, SIGNAL(toggled(bool)), this, SLOT(globalToggled(bool)));
		connect(buttons.userButton, SIGNAL(toggled(bool)), this, SLOT(userToggled(bool)));
		connect(buttons.projectButton, SIGNAL(toggled(bool)), this, SLOT(projectToggled(bool)));
		connect(this, SIGNAL(directoryEntered(const QString&)), SLOT(directoryChanged(const QString&)));

		if (writeFlag)
		{
			setAcceptMode(QFileDialog::AcceptSave);
			buttons.globalButton->setEnabled(false);
			switch (lastViewUsed)
			{
				case GLOBAL_VIEW:
				case PROJECT_VIEW:
					buttons.projectButton->setChecked(true);
					break;

				case USER_VIEW:
					buttons.userButton->setChecked(true);
					break;
			}
		}
		else
		{
			switch (lastViewUsed)
			{
				case GLOBAL_VIEW:
					buttons.globalButton->setChecked(true);
					break;

				case PROJECT_VIEW:
					buttons.projectButton->setChecked(true);
					break;

				case USER_VIEW:
					buttons.userButton->setChecked(true);
					break;
			}

		}
		buttons.loadAllGroup->setVisible(false);
	}
}

//---------------------------------------------------------
//   MFileDialog::directoryChanged
//---------------------------------------------------------

void MFileDialog::directoryChanged(const QString&)
{
	ViewType currentView = GLOBAL_VIEW;
	QDir ndir = directory();
	///QString newdir = ndir.absolutePath().toLatin1();
	QString newdir = ndir.absolutePath();
	if (buttons.projectButton->isChecked())
		currentView = PROJECT_VIEW;
	else if (buttons.userButton->isChecked())
		currentView = USER_VIEW;

	switch (currentView)
	{
		case GLOBAL_VIEW:
			lastGlobalDir = newdir;
			break;

		case USER_VIEW:
			lastUserDir = newdir;
			break;

		case PROJECT_VIEW: // Do nothing
		default:
			break;
	}
}


//---------------------------------------------------------
//   getFilterExtension
//---------------------------------------------------------

QString getFilterExtension(const QString &filter)
{
	//
	// Return the first extension found. Must contain at least one * character.
	//

	int pos = filter.indexOf('*');
	if (pos == -1)
		return QString();

	QString filt;
	int len = filter.length();
	++pos;
	for (; pos < len; ++pos)
	{
		QChar c = filter[pos];
		if ((c == ')') || (c == ';') || (c == ',') || (c == ' '))
			break;
		filt += filter[pos];
	}
	return filt;
}

//---------------------------------------------------------
//   getOpenFileName
//---------------------------------------------------------

QString getOpenFileName(const QString &startWith,
		const QStringList& filters, QWidget* parent, const QString& name, bool* all, MFileDialog::ViewType viewType)
{
	QString initialSelection; // FIXME Tim.
	MFileDialog *dlg = new MFileDialog(startWith, QString::null, parent, false);
	dlg->setNameFilters(filters);
	dlg->setWindowTitle(name);
	if (viewType == MFileDialog::GLOBAL_VIEW)
		dlg->globalToggled(true);
	else if (viewType == MFileDialog::PROJECT_VIEW)
		dlg->projectToggled(true);
	else if (viewType == MFileDialog::USER_VIEW)
		dlg->userToggled(true);
	if (all)
	{
		dlg->buttons.loadAllGroup->setVisible(true);
		//dlg->buttons.globalButton->setVisible(false);
	}
	if (!initialSelection.isEmpty())
		dlg->selectFile(initialSelection);
	dlg->setFileMode(QFileDialog::ExistingFile);
	QStringList files;
	QString result;
	if (dlg->exec() == QDialog::Accepted)
	{
		files = dlg->selectedFiles();
		if (!files.isEmpty())
			result = files[0];
		if (all)
		{
			*all = dlg->buttons.loadAllButton->isChecked();
		}
	}
	delete dlg;
	return result;
}

//---------------------------------------------------------
//   getSaveFileName
//---------------------------------------------------------

QString getSaveFileName(const QString &startWith,
		//const char** filters, QWidget* parent, const QString& name)
		const QStringList& filters, QWidget* parent, const QString& name)
{
	MFileDialog *dlg = new MFileDialog(startWith, QString::null, parent, true);
	dlg->setNameFilters(filters);
	dlg->setWindowTitle(name);
	dlg->setFileMode(QFileDialog::AnyFile);
	QStringList files;
	QString result;
	if (dlg->exec() == QDialog::Accepted)
	{
		files = dlg->selectedFiles();
		if (!files.isEmpty())
			result = files[0];
	}

	// Added by T356.
	if (!result.isEmpty())
	{
		QString filt = dlg->selectedNameFilter();
		filt = getFilterExtension(filt);
		// Do we have a valid extension?
		if (!filt.isEmpty())
		{
			// If the rightmost characters of the filename do not already contain
			//  the extension, add the extension to the filename.
			//if(result.right(filt.length()) != filt)
			if (!result.endsWith(filt))
				result += filt;
		}
		else
		{
			// No valid extension, or just * was given. Although it would be nice to allow no extension
			//  or any desired extension by commenting this section out, it's probably not a good idea to do so.
			//
			// NOTE: Most calls to this routine getSaveFileName() are followed by fileOpen(),
			//  which can tack on its own extension, but only if the *complete* extension is blank.
			// So there is some overlap going on. Enabling this actually stops that action,
			//  but only if there are no errors in the list of filters. fileOpen() will act as a 'catchall'.
			//
			// Force the filter list to the first one (the preferred one), and then get the filter.
			dlg->selectNameFilter(dlg->nameFilters().at(0));
			filt = dlg->selectedNameFilter();
			filt = getFilterExtension(filt);

			// Do we have a valid extension?
			if (!filt.isEmpty())
			{
				// If the rightmost characters of the filename do not already contain
				//  the extension, add the extension to the filename.
				//if(result.right(filt.length()) != filt)
				if (!result.endsWith(filt))
					result += filt;
			}
		}
	}

	delete dlg;
	return result;
}

//---------------------------------------------------------
//   getImageFileName
//---------------------------------------------------------

QString getImageFileName(const QString& startWith,
		//const char** filters, QWidget* parent, const QString& name)
		const QStringList& filters, QWidget* parent, const QString& name)
{
	QString initialSelection;
	QString* workingDirectory = new QString(QDir::currentPath());
	if (!startWith.isEmpty())
	{
		QFileInfo fi(startWith);
		if (fi.exists() && fi.isDir())
		{
			*workingDirectory = startWith;
		}
		else if (fi.exists() && fi.isFile())
		{
			*workingDirectory = fi.absolutePath();
			initialSelection = fi.absoluteFilePath();
		}
	}
	MFileDialog *dlg = new MFileDialog(*workingDirectory, QString::null,
			parent);

	/* ORCAN - disable preview for now. It is not available in qt4. We will
			   need to implement it ourselves.
	dlg->setContentsPreviewEnabled(true);
	ContentsPreview* preview = new ContentsPreview(dlg);
	dlg->setContentsPreview(preview, preview);
	dlg->setPreviewMode(QFileDialog::Contents);
	 */
	dlg->setWindowTitle(name);
	dlg->setNameFilters(filters);
	dlg->setFileMode(QFileDialog::ExistingFile);
	QStringList files;
	QString result;
	if (!initialSelection.isEmpty())
		dlg->selectFile(initialSelection);
	if (dlg->exec() == QDialog::Accepted)
	{
		files = dlg->selectedFiles();
		if (!files.isEmpty())
			result = files[0];
	}
	delete dlg;
	return result;
}

//---------------------------------------------------------
//   fileOpen
//    opens file "name" with extension "ext" in mode "mode"
//    handles "name.ext.bz2" and "name.ext.gz"
//
//    mode = "r" or "w"
//    popenFlag   set to true on return if file was opened
//                with popen() (and therefore must be closed
//                with pclose())
//    noError     show no error if file was not found in "r"
//                mode. Has no effect in "w" mode
//    overwriteWarning
//                warn in "w" mode, if file exists
//---------------------------------------------------------

FILE* fileOpen(QWidget* parent, QString name, const QString& ext,
		const char* mode, bool& popenFlag, bool noError,
		bool overwriteWarning)
{
	QFileInfo info(name);
	QString zip;

	popenFlag = false;
	if (info.completeSuffix() == "")
	{
		name += ext;
		info.setFile(name);
	}
	else if (info.suffix() == "gz")
	{
		popenFlag = true;
		zip = QString("gzip");
	}
	else if (info.suffix() == "bz2")
	{
		popenFlag = true;
		zip = QString("bzip2");
	}

	if (strcmp(mode, "w") == 0 && overwriteWarning && info.exists())
	{
		QString s(QWidget::tr("File\n") + name + QWidget::tr("\nexists. Overwrite?"));
		/*
		int rv = QMessageBox::warning(parent,
		   QWidget::tr("OOMidi: write"),
		   s,
		   QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Save);
		switch(rv) {
			  case 0:  // overwrite
					break;
			  case 1:  // quit
					return 0;
			  }
		 */
		if (QMessageBox::warning(parent,
				QWidget::tr("OOMidi: write"), s,
				QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Save)
				!= QMessageBox::Save)
			return 0;

	}
	FILE* fp = 0;
	if (popenFlag)
	{
		if (strcmp(mode, "r") == 0)
			zip += QString(" -d < ");
		else
			zip += QString(" > ");
		zip += name;
		fp = popen(zip.toLatin1().data(), mode);
	}
	else
	{
		fp = fopen(name.toLatin1().data(), mode);
	}
	if (fp == 0 && !noError)
	{
		QString s(QWidget::tr("Open File\n") + name + QWidget::tr("\nfailed: ")
				+ QString(strerror(errno)));
		QMessageBox::critical(parent, QWidget::tr("OOMidi: Open File"), s);
		return 0;
	}
	return fp;
}

//---------------------------------------------------------
//   MFile
//---------------------------------------------------------

MFile::MFile(const QString& _path, const QString& _ext)
: path(_path), ext(_ext)
{
	f = 0;
	isPopen = false;
}

MFile::~MFile()
{
	if (f)
	{
		if (isPopen)
			pclose(f);
		else
			fclose(f);
	}
}

//---------------------------------------------------------
//   open
//---------------------------------------------------------

//FILE* MFile::open(const char* mode, const char** pattern,

FILE* MFile::open(const char* mode, const QStringList& pattern,
		QWidget* parent, bool noError, bool warnIfOverwrite, const QString& caption)
{
	QString name;
	if (strcmp(mode, "r") == 0)
		name = getOpenFileName(path, pattern, parent, caption, 0);
	else
		name = getSaveFileName(path, pattern, parent, caption);
	if (name.isEmpty())
		return 0;
	f = fileOpen(parent, name, ext, mode, isPopen, noError,
			warnIfOverwrite);
	return f;
}

