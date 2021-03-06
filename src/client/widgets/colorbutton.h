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
#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QToolButton>

#ifndef DESIGNER_PLUGIN
namespace widgets {
#define PLUGIN_EXPORT
#else
#include <QtDesigner/QDesignerExportWidget>
#define PLUGIN_EXPORT QDESIGNER_WIDGET_EXPORT
#endif

//! A button for selecting a color
class PLUGIN_EXPORT ColorButton : public QToolButton {
Q_OBJECT
Q_PROPERTY(QColor color READ color WRITE setColor)
Q_PROPERTY(bool setAlpha READ alpha WRITE setAlpha)
Q_PROPERTY(bool locked READ locked WRITE setLocked)
public:
	ColorButton(QWidget *parent=0,const QColor& color = Qt::black);
	~ColorButton() {}

	//! Get the selected color
	QColor color() const { return _color; }

	//! Allow setting of alpha value
	void setAlpha(bool use);

	//! Set alpha value?
	bool alpha() const { return _setAlpha; }

	//! Lock color changing
	void setLocked(bool locked) { _locked = locked; }

	bool locked() const { return _locked; }

public slots:
	//! Set color selection
	void setColor(const QColor& color);

signals:
	void colorChanged(const QColor& color);

private slots:
	void selectColor();

protected:
	void paintEvent(QPaintEvent *);
	void dragEnterEvent(QDragEnterEvent *);
	void dropEvent(QDropEvent *);

private:
	QColor _color;
	bool _setAlpha;
	bool _locked;
};

#ifndef DESIGNER_PLUGIN
}
#endif

#endif
