/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2015 Kai Pastor
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


#ifndef _OPENORIENTEERING_EDIT_LINE_TOOL_H_
#define _OPENORIENTEERING_EDIT_LINE_TOOL_H_

#include <QScopedPointer>
#include <QElapsedTimer>

#include "tool_edit.h"

class MapWidget;
class CombinedSymbol;
class PointObject;
class PathObject;
class Symbol;


/**
 * A tool to edit lines of PathObjects.
 */
class EditLineTool : public EditTool
{
Q_OBJECT
public:
	EditLineTool(MapEditorController* editor, QAction* tool_action);
	virtual ~EditLineTool();
	
	void mouseMove() override;
	void clickPress() override;
	void clickRelease() override;
	
	void dragStart() override;
	void dragMove() override;
	void dragFinish() override;
	
protected:
	bool keyPress(QKeyEvent* event) override;
	bool keyRelease(QKeyEvent* event) override;
	
	void initImpl() override;
	void objectSelectionChangedImpl() override;
	int updateDirtyRectImpl(QRectF& rect) override;
	void drawImpl(QPainter* painter, MapWidget* widget) override;
	
	/** In addition to the base class implementation, updates the status text. */
	void updatePreviewObjects() override;
	
	/** Deletes the highlight object if it exists and correctly removes its renderables. */
	void deleteHighlightObject();
	
	void updateStatusText();
	
	/** Recalculates hover_line. */
	void updateHoverState(MapCoordF cursor_pos);
	
	void toggleAngleHelper();
	
	bool hoveringOverFrame() const;
	
private:
	/** Measures the time a click takes to decide whether to do selection. */
	QElapsedTimer click_timer;
	
	/** Bounding box of the selection */
	QRectF selection_extent;
	
	
	/**
	 * Provides general information on what is hovered over.
	 */
	HoverState hover_state;
	
	/**
	 * Object which is hovered over (if any).
	 */
	PathObject* hover_object;
	
	/**
	 * Coordinate identifying the hover_object's line which is hovered over.
	 */
	MapCoordVector::size_type hover_line;
	
	/**
	 * An object created for the current hover_line.
	 */
	PathObject* highlight_object;
	
	
	/** Is a box selection in progress? */
	bool box_selection;
	
	bool no_more_effect_on_click;
	
	/**
	 * Offset from cursor position to drag handle of moved element.
	 * When snapping to another element, this offset must be corrected.
	 */
	MapCoordF handle_offset;
	
	QScopedPointer<ObjectMover> object_mover;
	QScopedPointer<MapRenderables> highlight_renderables;
};



// ### EditLineTool inline code ###

inline
bool EditLineTool::hoveringOverFrame() const
{
	return hover_state == OverFrame;
}

#endif
