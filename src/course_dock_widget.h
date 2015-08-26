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


#ifndef _OPENORIENTEERING_COURSE_DOCK_WIDGET_H_
#define _OPENORIENTEERING_COURSE_DOCK_WIDGET_H_

#define C_TEXT "703"
#define C_LINE "799"
#define CD_BLANK "18.1"
#define CD_CORN "720.0"

#include <QFileDialog>
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

	typedef QHash<QString,QString> c_point;
	typedef std::vector<Object*> courseVector;
	typedef std::vector<c_point*> cpVector;

	typedef struct
	{
		courseVector course;		// course objects (799, etc.)
		courseVector coursecp;		// map object pointers for controlnumbers (703)
		cpVector* control_points;	// controls info
		QString course_name;
		QString dist_to_finish;		// distance to finish from last control
	} onecourse;

	QString xml_names[6]={"cd_C","cd_D","cd_E","cd_F","cd_G","cd_H"};

	courseWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent = NULL);
	virtual ~courseWidget();
	void addRow();
	void updateRow(int row);
	void* getcontrolpoints(int i);
	void setcontrolpointstext(QString new_key, QString new_value, int course_index, int cp_index);
	void setcontrolpoints(cpVector* temp, int pos);
	inline QString getcoursename(int i) {return courses[i].course_name;}
	inline void setcoursename(QString c_name,int i) {courses[i].course_name=c_name;}
	inline Object* getcourse(int i) {if (i>=0) return courses[i].course.at(0); else return NULL;}
	inline const Object* getcourse(int i) const {if (i>=0) return courses[i].course.at(0); else return NULL;}
	inline int getNumcourses() const {return courses.size();}
	inline int getNumcoursecp(int i) const {return courses[i].coursecp.size();}
	inline void setdisttofinish(QString dtf, int cn) {courses[cn].dist_to_finish=dtf;}
	inline QString getdisttofinish(int cn) {return courses[cn].dist_to_finish;}

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
	
	void insertcp();
	void clearcp();
	void cpdescr();
	void coursecpchanged();
	void courseselchanged();
	void selcoursechanged();
	void opencourseClicked();
	void savecourseClicked();
	void coursedeleted();

private:

	QList<onecourse> courses;

	QTableWidget* course_table;
	QBoxLayout* all_courses_layout;

	QAction* edit_action;

	// Buttons
	QWidget* list_buttons_group;
	QToolButton* open_course_button;
	QToolButton* save_course_button;
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
	QToolButton* help_button;

	Map* map;
	MapView* main_view;
	MapEditorController* controller;
	bool react_to_changes;

	//Courses
	void courseadd(Object* temp);
	void addcoursefromfile(cpVector* temp, QString c_name, QString dtf);
	void removecourse(int pos);
	void setcourse(Object* temp, int pos);
	int findcourseIndex(const Object* temp) const;
	inline void setcoursecp(Object *temp,int i, int j) {courses[i].coursecp[j]=temp;};
	void clearcoursecp(int i);
	inline Object* getcoursecp(int i, int j) {if (i>=0) return courses[i].coursecp.at(j); else return NULL;}
	Symbol* getSymbolByTextNumber(QString ts);
	void selcourseedittext(int rn, int cn);
	void selcourseeditpoint(int rn);
	void selcourseeditnewpoint(int rn);
	void* getcoursedata(Object *temp);
	void coursereplace(int rn, Object* temp);
};

#endif
