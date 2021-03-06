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


#ifndef _OPENORIENTEERING_MEASURE_WIDGET_H_
#define _OPENORIENTEERING_MEASURE_WIDGET_H_

#include <QSize>
#include <QWidget>

class QLabel;
class QStackedWidget;

class Map;

/**
 * The widget which is shown in a dock widget when the measure tool is active.
 * Displays information about the currently selected objects.
 */
class MeasureWidget : public QWidget
{
Q_OBJECT
public:
	/** Creates a new MeasureWidget for a given map. */
	MeasureWidget(Map* map, QWidget* parent = NULL);

	/** Destroys the MeasureWidget. */
	virtual ~MeasureWidget();
	
	/** Returns the preferred size for this widget. */
	virtual QSize sizeHint() const;
	
protected slots:
	/**
	 * Is called when the object selection in the map changes.
	 * Updates the widget content.
	 */
	void objectSelectionChanged();
	
private:
	Map* map;
	
	QSize preferred_size;
	
	QLabel* headline_label;
	QStackedWidget* length_stack;
	QLabel* paper_length_label;
	QLabel* real_length_label;
	QStackedWidget* area_stack;
	QLabel* paper_area_label;
	QLabel* real_area_label;
	QLabel* warning_label;
};

#endif
