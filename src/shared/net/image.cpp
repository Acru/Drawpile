/*
   DrawPile - a collaborative drawing program.

   Copyright (C) 2013 Calle Laakkonen

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

#include <QtEndian>
#include "image.h"

namespace protocol {

PutImage *PutImage::deserialize(const uchar *data, uint len)
{
	if(len < 11)
		return 0;

	return new PutImage(
		*(data+0),
		*(data+1),
		*(data+2),
		qFromBigEndian<quint16>(data+3),
		qFromBigEndian<quint16>(data+5),
		qFromBigEndian<quint16>(data+7),
		qFromBigEndian<quint16>(data+9),
		QByteArray((const char*)data+11, len-11)
	);
}

int PutImage::payloadLength() const
{
	return 1 + 2 + 4*2 + _image.size();
}

int PutImage::serializePayload(uchar *data) const
{
	uchar *ptr = data;
	*(ptr++) = contextId();
	*(ptr++) = _layer;
	*(ptr++) = _flags;
	qToBigEndian(_x, ptr); ptr += 2;
	qToBigEndian(_y, ptr); ptr += 2;
	qToBigEndian(_w, ptr); ptr += 2;
	qToBigEndian(_h, ptr); ptr += 2;
	memcpy(ptr, _image.constData(), _image.length());
	ptr += _image.length();
	return ptr-data;
}

FillRect *FillRect::deserialize(const uchar *data, uint len)
{
	if(len != 15)
		return 0;

	return new FillRect(
		*(data+0),
		*(data+1),
		*(data+2),
		qFromBigEndian<quint16>(data+3),
		qFromBigEndian<quint16>(data+5),
		qFromBigEndian<quint16>(data+7),
		qFromBigEndian<quint16>(data+9),
		qFromBigEndian<quint32>(data+11)
	);
}

int FillRect::payloadLength() const
{
	return 1 + 2 + 2*4 + 4;
}

int FillRect::serializePayload(uchar *data) const
{
	uchar *ptr = data;
	*(ptr++) = contextId();
	*(ptr++) = _layer;
	*(ptr++) = _blend;
	qToBigEndian(_x, ptr); ptr += 2;
	qToBigEndian(_y, ptr); ptr += 2;
	qToBigEndian(_w, ptr); ptr += 2;
	qToBigEndian(_h, ptr); ptr += 2;
	qToBigEndian(_color, ptr); ptr += 4;

	return ptr-data;
}


}
