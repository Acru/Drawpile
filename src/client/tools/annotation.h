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
#ifndef TOOLS_ANNOTATION_H
#define TOOLS_ANNOTATION_H

#include "scene/annotationitem.h"
#include "tool.h"

namespace drawingboard {
	class AnnotationItem;
}

namespace tools {

/**
 * @brief Annotation tool
 */
class Annotation : public Tool {
public:
	Annotation(ToolCollection &owner) : Tool(owner, ANNOTATION), _selected(0) { }

	void begin(const paintcore::Point& point, bool right);
	void motion(const paintcore::Point& point, bool constrain, bool center);
	void end();

private:
	QPointer<drawingboard::AnnotationItem> _selected;
	drawingboard::AnnotationItem::Handle _handle;
	QPointF _start, _p1, _p2;
	bool _wasselected;
};

}

#endif

