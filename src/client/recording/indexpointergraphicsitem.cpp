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

#include "indexpointergraphicsitem.h"
#include "indexgraphicsitem.h"

IndexPointerGraphicsItem::IndexPointerGraphicsItem(int height, QGraphicsItem *parent)
	: QGraphicsItem(parent)
{
	const int Y = 5;
	_rect = QRectF(0, -Y, IndexGraphicsItem::STEP_WIDTH, height + 2*Y);
}

void IndexPointerGraphicsItem::setIndex(int i)
{
	setX(i * IndexGraphicsItem::STEP_WIDTH);
}

QRectF IndexPointerGraphicsItem::boundingRect() const
{
	return _rect;
}

void IndexPointerGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *)
{
	Q_UNUSED(options);
	qreal c = _rect.width() / 2;
	painter->drawLine(_rect.topLeft(), _rect.topRight());
	painter->drawLine(_rect.left() + c, _rect.top(), _rect.left() + c, _rect.bottom());
	painter->drawLine(_rect.bottomLeft(), _rect.bottomRight());
}
