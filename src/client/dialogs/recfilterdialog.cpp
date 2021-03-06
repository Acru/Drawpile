/*
   DrawPile - a collaborative drawing program.

   Copyright (C) 2014 Calle Laakkonen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <QFileDialog>
#include <QMessageBox>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
#include <QSaveFile>
#else
#include <QFile>
#define QSaveFile QFile
#define NO_QSAVEFILE
#endif

#include "dialogs/recfilterdialog.h"
#include "recording/filter.h"

#include "ui_recfilter.h"

namespace dialogs {

FilterRecordingDialog::FilterRecordingDialog(QWidget *parent) :
	QDialog(parent)
{
	_ui = new Ui_FilterRecording;
	_ui->setupUi(this);
}

FilterRecordingDialog::~FilterRecordingDialog()
{
	delete _ui;
}

QString FilterRecordingDialog::filterRecording(const QString &recordingFile)
{
	// First, get output file name
	QString outfile = QFileDialog::getSaveFileName(this, tr("Save filtered recording"), QString(),
		tr("Drawpile recordings (%1)").arg("*.dprec")
	);

	if(outfile.isEmpty())
		return QString();

	if(!outfile.endsWith(".dprec", Qt::CaseInsensitive))
		outfile.append(".dprec");

	// Set filtering options
	recording::Filter filter;

	filter.setExpungeUndos(_ui->removeUndos->isChecked());
	filter.setRemoveChat(_ui->removeChat->isChecked());
	filter.setRemoveLookyloos(_ui->removeLookyloos->isChecked());
	filter.setRemoveDelays(_ui->removeDelays->isChecked());
	filter.setRemoveLasers(_ui->removeLasers->isChecked());
	filter.setRemoveMarkers(_ui->removeMarkers->isChecked());

	// Perform filtering
	QFile inputfile(recordingFile);
	if(!inputfile.open(QFile::ReadOnly)) {
		QMessageBox::warning(this, tr("Error"), inputfile.errorString());
		return QString();
	}

	QSaveFile outputfile(outfile);
	if(!outputfile.open(QSaveFile::WriteOnly)) {
		QMessageBox::warning(this, tr("Error"), outputfile.errorString());
		return QString();
	}

	if(!filter.filterRecording(&inputfile, &outputfile)) {
		QMessageBox::warning(this, tr("Error"), filter.errorString());
		return QString();
	}

#ifndef NO_QSAVEFILE // Qt 5.0 compatibility
	outputfile.commit();
#else
	outputfile.close();
#endif

	return outfile;
}

}
