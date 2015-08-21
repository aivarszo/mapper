/*
 *	Copyright 2012, 2013 Thomas Schöps
 *
 *	This file is part of OpenOrienteering.
 *
 *	OpenOrienteering is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	OpenOrienteering is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _OPENORIENTEERING_COURSE_EDIT_DOCK_WIDGET_H_
#define _OPENORIENTEERING_COURSE_EDIT_DOCK_WIDGET_H_

#include <QtWidgets>
#include "object.h"
#include "course_dock_widget.h"
#include "gui/widgets/segmented_button_layout.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QLabel;
QT_END_NAMESPACE

class MapEditorController;
class courseWidget;

/** Dock widget allowing to enter template positioning properties numerically. */
class courseEditDockWidget : public QDockWidget
{
Q_OBJECT
public:
	courseEditDockWidget(int rn, courseWidget* temp, MapEditorController* controller, QWidget* parent = NULL);
	
//	void updateValues();
	
	virtual bool event(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);
	QToolButton* newToolButton(int cp_num, int cr_num, const QIcon& icon, const QString& text);

public slots:
//	void courseChanged(int index, const course* temp);
//	void courseDeleted(int index, const course* temp);
//	void valueChanged();
	void savecourse();
	void set_cd_icon();

private:

	typedef std::vector<QLabel*> cp_label;
	typedef std::vector<QLineEdit*> cp_edit;
	int rownum;
	courseWidget *cw;

	cp_label x_label;
	cp_edit x_edit;
	bool react_to_changes;
	QPushButton* x_button;
	Object* temp;

	MapEditorController* controller;
};

#endif
