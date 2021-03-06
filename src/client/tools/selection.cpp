/*
   DrawPile - a collaborative drawing program.

   Copyright (C) 2006-2014 Calle Laakkonen

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

#include "scene/selectionitem.h"
#include "scene/canvasscene.h"
#include "net/client.h"

#include "tools/selection.h"
#include "tools/utils.h"

namespace tools {
void Selection::begin(const paintcore::Point &point, bool right)
{
	// Right click to dismiss selection (and paste buffer)
	if(right) {
		scene().setSelectionItem(0);
		return;
	}

	if(scene().selectionItem())
		_handle = scene().selectionItem()->handleAt(point.toPoint());
	else
		_handle = drawingboard::SelectionItem::OUTSIDE;

	_start = point.toPoint();
	_p1 = _start;

	if(_handle == drawingboard::SelectionItem::OUTSIDE) {
		bool hasPaste = scene().selectionItem() && !scene().selectionItem()->pasteImage().isNull();
		if(hasPaste) {
			// Left click outside and paste buffer exists: merge image
			QImage image = scene().selectionItem()->pasteImage();
			const QRect rect = scene().selectionItem()->rect();
			if(image.size() != rect.size())
				image = image.scaled(rect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

			// Clip image to scene
			const QRect scenerect(0, 0, scene().width(), scene().height());
			QRect intersection = rect & scenerect;
			if(!intersection.isEmpty()) {
				int xoff=0, yoff=0;
				if(intersection != rect) {
					if(rect.x() < 0)
						xoff = -rect.x();
					if(rect.y() < 0)
						yoff = -rect.y();

					intersection.moveLeft(xoff);
					intersection.moveTop(yoff);
					image = image.copy(intersection);
				}

				// Merge image
				client().sendUndopoint();
				client().sendImage(layer(), rect.x() + xoff, rect.y() + yoff, image, true);
			}
			scene().setSelectionItem(0);
		} else {
			drawingboard::SelectionItem *sel = new drawingboard::SelectionItem();
			sel->setRect(QRectF(point, point).toRect());
			scene().setSelectionItem(sel);
		}
	}
}

void Selection::motion(const paintcore::Point &point, bool constrain, bool center)
{
	if(!scene().selectionItem())
		return;

	if(_handle==drawingboard::SelectionItem::OUTSIDE) {
		QPointF p;
		if(constrain)
			p = constraints::square(_start, point);
		else
			p = point;

		if(center)
			_p1 = _start - (p.toPoint() - _start);
		else
			_p1 = _start;

		scene().selectionItem()->setRect(QRectF(_p1, p).normalized().toRect());
	} else {
			// TODO constrain
		QPointF p = point - _start;
		scene().selectionItem()->adjustGeometry(_handle, p.toPoint());
		_start = point.toPoint();
	}
}

void Selection::end()
{
	if(!scene().selectionItem())
		return;

	// Remove tiny selections
	QRect sel = scene().selectionItem()->rect();
	if(sel.width() * sel.height() <= 2)
		scene().setSelectionItem(0);
}

}

