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

#include <QPainter>
#include <QTextDocument>
#include <QDataStream>

#include "annotation.h"

namespace paintcore {

Annotation::Annotation(int id)
	: _id(id), _bgcolor(Qt::transparent), _doc(0)
{
}

Annotation::Annotation(const Annotation &a)
	: _id(a._id), _rect(a._rect), _text(a._text), _bgcolor(a._bgcolor), _doc(0)
{
}

Annotation::~Annotation()
{
	delete _doc;
}

void Annotation::setText(const QString &text)
{
	_text = text;
	if(_doc)
		_doc->setHtml(text);
}

bool Annotation::isEmpty() const
{
	if(!_doc) {
		_doc = new QTextDocument;
		_doc->setHtml(_text);
	}
	return _doc->isEmpty();
}

void Annotation::paint(QPainter *painter) const
{
	paint(painter, _rect);
}

void Annotation::paint(QPainter *painter, const QRectF& rect) const
{
	painter->save();
	painter->translate(rect.topLeft());

	const QRectF rect0(QPointF(), rect.size());

	painter->fillRect(rect0, _bgcolor);

	if(!_doc) {
		_doc = new QTextDocument;
		_doc->setHtml(_text);
	}
	_doc->setTextWidth(rect0.width());
	_doc->drawContents(painter, rect0);

	painter->restore();
}

QImage Annotation::toImage() const
{
	QImage img(_rect.size(), QImage::Format_ARGB32);
	img.fill(0);
	QPainter painter(&img);
	paint(&painter, QRectF(QPointF(), _rect.size()));
	return img;
}

void Annotation::toDatastream(QDataStream &out) const
{
	// Write ID
	out << qint16(_id);

	// Write position and size
	out << _rect;

	// Write content
	out << _bgcolor;
	out << _text;
}

Annotation *Annotation::fromDatastream(QDataStream &in)
{
	qint16 id;
	in >> id;
	Annotation *a = new Annotation(id);
	in >> a->_rect;
	in >> a->_bgcolor;
	in >> a->_text;

	return a;
}

}
