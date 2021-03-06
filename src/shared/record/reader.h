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
#ifndef REC_READER_H
#define REC_READER_H

#include <QObject>

#include "../net/message.h"

class QFileDevice;

namespace recording {

enum Compatibility {
	//! Recording is fully compatible with current version
	COMPATIBLE,
	//! Minor changes: file will load but may appear different
	MINOR_INCOMPATIBILITY,
	//! Might be at least partially compatible. Unknown future version
	UNKNOWN_COMPATIBILITY,
	//! File is a recording, but known to be incompatible
	INCOMPATIBLE,
	//! File is not a recording at all
	NOT_DPREC,
	//! Couldn't read file due to IO error
	CANNOT_READ
};

struct MessageRecord {
	MessageRecord() : status(END_OF_RECORDING), message(0) {}

	enum { OK, INVALID, END_OF_RECORDING } status;
	union {
		protocol::Message *message;
		struct {
			int len;
			protocol::MessageType type;
		};
	};
};

class Reader : public QObject
{
	Q_OBJECT
public:
	/**
	 * @brief Reader
	 * @param filename
	 * @param parent
	 */
	Reader(const QString &filename, QObject *parent=0);

	/**
	 * @brief Reader
	 *
	 * @param file input file device
	 * @param autoclose if true, the Reader instance will take ownership of the file device
	 * @param parent
	 */
	Reader(QFileDevice *file, bool autoclose=false, QObject *parent=0);

	Reader(const Reader &) = delete;
	Reader &operator=(const Reader &) = delete;
	~Reader();

	QString filename() const;
	qint64 filesize() const;
	int current() const { return _current; }
	qint64 position() const;
	QString errorString() const;

	/**
	 * @brief Get the version number of the program that made the recording.
	 *
	 * The version number is available after opening the file.
	 * @return
	 */
	const QString writerVersion() const { return _writerversion; }

	/**
	 * @brief Open the file
	 * @return compatibility level of the opened file
	 */
	Compatibility open();

	//! Close the file
	void close();

	//! Rewind to the first message
	void rewind();

	/**
	 * @brief Read the next message to the given buffer
	 *
	 * The buffer will be resized, if necesasry, to hold the entire message.
	 *
	 * @param buffer
	 * @return false on error
	 */
	bool readNextToBuffer(QByteArray &buffer);

	/**
	 * @brief Read the next message
	 * @return
	 */
	MessageRecord readNext();

	//! Seek to given position in the recording
	void seekTo(int pos, qint64 offset);

private:
	QFileDevice *_file;
	QByteArray _msgbuf;
	bool _autoclose;
	QString _writerversion;
	int _current;
	qint64 _beginning;
};

}

#endif
