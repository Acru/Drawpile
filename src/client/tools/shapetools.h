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
#ifndef TOOLS_SHAPETOOLS_H
#define TOOLS_SHAPETOOLS_H

#include "tool.h"

namespace tools {

/**
 * \brief Line tool
 *
 * The line tool draws straight lines.
 */
class Line : public Tool {
public:
	Line(ToolCollection &owner) : Tool(owner, LINE) {}

	void begin(const paintcore::Point& point, bool right);
	void motion(const paintcore::Point& point, bool constrain, bool center);
	void end();

private:
	QPointF _p1, _p2;
	bool _swap;
};

/**
 * \brief Base class for shape drawing tools that can be defined with a rectangle
 */
class RectangularTool : public Tool {
public:
	RectangularTool(ToolCollection &owner, Type type) : Tool(owner, type) {}

	void begin(const paintcore::Point& point, bool right);
	void motion(const paintcore::Point& point, bool constrain, bool center);
	void end();

protected:
	virtual QAbstractGraphicsShapeItem *createPreview(const paintcore::Point &p) = 0;
	virtual void updateToolPreview() = 0;
	virtual paintcore::PointVector pointVector() = 0;
	QRectF rect() const { return QRectF(_p1, _p2).normalized(); }

private:
	QPointF _start, _p1, _p2;
	bool _swap;
};

/**
 * \brief Rectangle drawing tool
 *
 * This tool is used for drawing squares and rectangles
 */
class Rectangle : public RectangularTool {
public:
	Rectangle(ToolCollection &owner) : RectangularTool(owner, RECTANGLE) {}

protected:
	virtual QAbstractGraphicsShapeItem *createPreview(const paintcore::Point &p);
	virtual void updateToolPreview();
	virtual paintcore::PointVector pointVector();
};

/**
 * \brief Ellipse drawing tool
 *
 * This tool is used for drawing circles and ellipses
 */
class Ellipse : public RectangularTool {
public:
	Ellipse(ToolCollection &owner) : RectangularTool(owner, ELLIPSE) {}

protected:
	virtual QAbstractGraphicsShapeItem *createPreview(const paintcore::Point &p);
	virtual void updateToolPreview();
	virtual paintcore::PointVector pointVector();
};

}

#endif

