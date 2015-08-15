/*
 *    Copyright 2012, 2013 Thomas Schöps
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _OPENORIENTEERING_COURSE_DOCK_WIDGET_H_
#define _OPENORIENTEERING_COURSE_DOCK_WIDGET_H_

#define COURSE_ITEMS 11

#include <QtWidgets>
#include "object.h"
#include "symbol_text.h"

class Map;
class MapEditorController;
class MapView;

/**
 * Widget showing the list of templates, including the map layer.
 * Allows to load templates, set their view properties and reoder them,
 * and do various other actions like adjusting template positions.
 */
class courseWidget : public QWidget
{
Q_OBJECT
public:
    courseWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent = NULL);
    virtual ~courseWidget();
    static QString showOpencourseDialog(QWidget* dialog_parent, MapEditorController* controller);
    void addRow();
    void updateRow(int row);

protected:
    /**
	 * When key events for Qt::Key_Space are sent to the template_table,
	 * this will toggle the visibility of the current template.
	 */
	virtual bool eventFilter(QObject* watched, QEvent* event);
	
	/**
	 * Returns a new QToolButton with a unified appearance.
	 */
	QToolButton* newToolButton(const QIcon& icon, const QString& text);

protected slots:
    void addcourse();
    void deletecourse();
    void movecourseUp();
    void movecourseDown();
	void showHelp();
    void editClicked(bool checked);

	void cellChange(int row, int column);
	void updateButtons();
	
    void insertcponmap();
    void insertcpconmap();
    void insertcpbothonmap();
    void insertcp(int cc);
    void clearcp();
    void cpdescr();
    void coursecpchanged();
    void courseselchanged();

private:
    Symbol* getSymbolByTextNumber(QString ts);

    QTableWidget* course_table;
    QBoxLayout* all_courses_layout;

    QAction* edit_action;

	// Buttons
	QWidget* list_buttons_group;
    QToolButton* add_button;
    QToolButton* delete_button;
	QToolButton* move_up_button;
	QToolButton* move_down_button;
    QToolButton* insert_cp_on_map_button;
    QToolButton* insert_cpc_on_map_button;
    QToolButton* insert_cpboth_on_map_button;
    QToolButton* clear_cp_button;
    QToolButton* cp_descr_button;
    QToolButton* edit_button;

    Map* map;
	MapView* main_view;
	MapEditorController* controller;
	bool react_to_changes;
};

#endif
