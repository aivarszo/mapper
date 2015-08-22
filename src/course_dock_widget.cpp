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


#include "course_dock_widget.h"
#include "course_edit_dock_widget.h"

#include "core/path_coord.h"
#include "gui/main_window.h"
#include "gui/widgets/segmented_button_layout.h"
#include "map.h"
#include "map_editor.h"
#include "map_widget.h"
#include "object_text.h"
#include "symbol.h"
#include "settings.h"
#include "util.h"

// ### courseWidget ###

courseWidget::courseWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent)
: QWidget(parent), 
  map(map), 
  main_view(main_view), 
  controller(controller)
{
	react_to_changes = true;
	
	this->setWhatsThis("<a href=\"courses.html#setup\">See more</a>");
	
	QStyleOption style_option(QStyleOption::Version, QStyleOption::SO_DockWidget);

	// Course table
	course_table = new QTableWidget(0, 4);
	course_table->installEventFilter(this);
	course_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	course_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	course_table->setSelectionMode(QAbstractItemView::SingleSelection);
	course_table->setHorizontalHeaderLabels(QStringList() << tr("Show") << tr("Course") << tr("Controls") << tr("Length"));
	course_table->verticalHeader()->setVisible(false);
	
	course_table->horizontalHeaderItem(0)->setData(Qt::ToolTipRole, tr("Show"));
	QCheckBox header_check;
	QSize header_check_size(header_check.sizeHint());
	if (header_check_size.isValid())
	{
		header_check.setChecked(true);
		header_check.setEnabled(false);
		QPixmap pixmap(header_check_size);
		header_check.render(&pixmap);
		course_table->horizontalHeaderItem(0)->setData(Qt::DecorationRole, pixmap);
	}
	
	QHeaderView* header_view = course_table->horizontalHeader();
	for (int i = 0; i < 3; ++i)
		header_view->setSectionResizeMode(i, QHeaderView::ResizeToContents);
	header_view->setSectionResizeMode(3, QHeaderView::Stretch);
	header_view->setSectionsClickable(false);
	
	SegmentedButtonLayout* button_layout2 = new SegmentedButtonLayout();
	open_course_button = newToolButton(QIcon(":/images/open.png"), tr("Open course"));
	save_course_button = newToolButton(QIcon(":/images/save.png"), tr("Save course"));
	button_layout2->addWidget(open_course_button);
	button_layout2->addWidget(save_course_button);
	QBoxLayout* list_buttons_layout1 = new QHBoxLayout();
	list_buttons_layout1->setContentsMargins(0,0,0,0);
	list_buttons_layout1->addLayout(button_layout2);

	all_courses_layout = new QVBoxLayout();
	all_courses_layout->setMargin(0);
	all_courses_layout->addWidget(course_table, 1);
	
	SegmentedButtonLayout* button_layout1 = new SegmentedButtonLayout();
	add_button = newToolButton(QIcon(":/images/plus.png"), tr("Add course"));
	delete_button = newToolButton(QIcon(":/images/minus.png"), tr("Delete course"));
	move_up_button = newToolButton(QIcon(":/images/arrow-up.png"), tr("Move Up"));
	move_up_button->setAutoRepeat(true);
	move_down_button = newToolButton(QIcon(":/images/arrow-down.png"), tr("Move Down"));
	move_down_button->setAutoRepeat(true);
	button_layout1->addWidget(add_button);
	button_layout1->addWidget(delete_button);
	button_layout1->addWidget(move_up_button);
	button_layout1->addWidget(move_down_button);

	SegmentedButtonLayout* button_layout = new SegmentedButtonLayout();
	insert_cp_on_map_button = newToolButton(QIcon(":/images/insert-cp-num.png"), tr("Show CP numbers"));
	insert_cp_on_map_button->setProperty("name",1);
	insert_cpc_on_map_button = newToolButton(QIcon(":/images/insert-cp-cod.png"), tr("Show CP codes"));
	insert_cpc_on_map_button->setProperty("name",2);
	insert_cpboth_on_map_button = newToolButton(QIcon(":/images/insert-cp-numcod.png"), tr("Show both"));
	insert_cpboth_on_map_button->setProperty("name",3);
	clear_cp_button = newToolButton(QIcon(":/images/clear-cp.png"), tr("Clear CPs"));
	cp_descr_button = newToolButton(QIcon(":/images/cp_descr.png"), tr("Show CP descriptions"));
	button_layout->addWidget(insert_cp_on_map_button);
	button_layout->addWidget(insert_cpc_on_map_button);
	button_layout->addWidget(insert_cpboth_on_map_button);
	button_layout->addWidget(clear_cp_button);
	button_layout->addWidget(cp_descr_button);

	QMenu* edit_menu = new QMenu(this);
	edit_action = edit_menu->addAction(tr("Edit CP"));

	edit_button = newToolButton(QIcon(":/images/settings.png"), QApplication::translate("MapEditorController", "&Edit").remove(QChar('&')));
	edit_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	edit_button->setPopupMode(QToolButton::InstantPopup);
	edit_button->setMenu(edit_menu);
	
	// The buttons row layout
	QBoxLayout* list_buttons_layout = new QHBoxLayout();
	list_buttons_layout->setContentsMargins(0,0,0,0);
	list_buttons_layout->addLayout(button_layout1);
	list_buttons_layout->addLayout(button_layout);
	list_buttons_layout->addWidget(edit_button);
	
	list_buttons_group = new QWidget();
	list_buttons_group->setLayout(list_buttons_layout);
	
	QToolButton* help_button = newToolButton(QIcon(":/images/help.png"), tr("Help"));
	help_button->setAutoRaise(true);
	
	QBoxLayout* all_buttons_layout = new QHBoxLayout();
	all_buttons_layout->setContentsMargins(
		style()->pixelMetric(QStyle::PM_LayoutLeftMargin, &style_option) / 2,
		0, // Covered by the main layout's spacing.
		style()->pixelMetric(QStyle::PM_LayoutRightMargin, &style_option) / 2,
		style()->pixelMetric(QStyle::PM_LayoutBottomMargin, &style_option) / 2
	);
	all_buttons_layout->addWidget(list_buttons_group);
	all_buttons_layout->addWidget(open_course_button);
	all_buttons_layout->addWidget(save_course_button);
	all_buttons_layout->addWidget(help_button);
	
	all_courses_layout->addLayout(all_buttons_layout);
	setLayout(all_courses_layout);
	
	updateButtons();
	
	// Connections
	courseWidget::connect(open_course_button,&QToolButton::clicked,this,&courseWidget::opencourseClicked);
	courseWidget::connect(save_course_button,&QToolButton::clicked,this,&courseWidget::savecourseClicked);
	courseWidget::connect(insert_cp_on_map_button, &QToolButton::clicked,this,&courseWidget::insertcp);
	courseWidget::connect(insert_cpc_on_map_button, &QToolButton::clicked,this,&courseWidget::insertcp);
	courseWidget::connect(insert_cpboth_on_map_button, &QToolButton::clicked,this,&courseWidget::insertcp);
	courseWidget::connect(clear_cp_button, &QToolButton::clicked,this,&courseWidget::clearcp);
	courseWidget::connect(cp_descr_button, &QToolButton::clicked,this,&courseWidget::cpdescr);

	courseWidget::connect(edit_action, &QAction::triggered,this,&courseWidget::editClicked);

	courseWidget::connect(course_table, &QTableWidget::cellClicked, this, &courseWidget::selcoursechanged);
	courseWidget::connect(course_table, &QTableWidget::cellChanged,this,&courseWidget::cellChange);
	courseWidget::connect(course_table->selectionModel(), &QItemSelectionModel::selectionChanged,this,&courseWidget::updateButtons);
	
	courseWidget::connect(add_button, &QToolButton::clicked,this,&courseWidget::addcourse);
	courseWidget::connect(delete_button, &QToolButton::clicked,this,&courseWidget::deletecourse);
	courseWidget::connect(move_up_button, &QToolButton::clicked,this,&courseWidget::movecourseUp);
	courseWidget::connect(move_down_button, &QToolButton::clicked,this,&courseWidget::movecourseDown);
	courseWidget::connect(help_button, &QToolButton::clicked, this, &courseWidget::showHelp);
	courseWidget::connect(map, &Map::selectedObjectEdited,this,&courseWidget::coursecpchanged);
	courseWidget::connect(map, &Map::objectSelectionChanged,this,&courseWidget::courseselchanged);
}

courseWidget::~courseWidget()
{
	; // Nothing
}

void courseWidget::courseselchanged()
{
	for (int i=0; i<getNumcourses(); i++)
	{
		if (map->getFirstSelectedObject()==getcourse(i))
		{
			course_table->selectRow(i);
		}
	}
}

void courseWidget::selcoursechanged()
{
	map->clearObjectSelection(true);
	map->addObjectToSelection(getcourse(course_table->currentRow()),true);
}

void courseWidget::selcourseedittext(int rn, int cn)
{
	Object *selsym=map->getFirstSelectedObject();
	setcontrolpointstext(QString("cp_x"),QString::number(selsym->getRawCoordinateVector().at(0).nativeX()),rn,cn);
	setcontrolpointstext(QString("cp_y"),QString::number(selsym->getRawCoordinateVector().at(0).nativeY()),rn,cn);
}

void courseWidget::selcourseeditpoint(int rn)
{
	Object *selsym=map->getFirstSelectedObject();
	for (unsigned int j=0; j<selsym->getRawCoordinateVector().size(); j++)
	{
		if(selsym->asPath()->getCoordinate(j).isDashPoint())
		{
			setcontrolpointstext(QString("cp_x"),QString::number(selsym->getRawCoordinateVector().at(j).nativeX()-4000),rn,j);
			setcontrolpointstext(QString("cp_y"),QString::number(selsym->getRawCoordinateVector().at(j).nativeY()-10000),rn,j);
			setcontrolpointstext(QString("cr_x"),QString::number(selsym->getRawCoordinateVector().at(j).nativeX()),rn,j);
			setcontrolpointstext(QString("cr_y"),QString::number(selsym->getRawCoordinateVector().at(j).nativeY()),rn,j);
			for(int k=0; k<getNumcourses();k++)
			{
				if (selsym!=getcourse(k))
				{
					for(unsigned int kk=0;kk<reinterpret_cast<cpVector*>(getcontrolpoints(k))->size();kk++)
					{
						if((abs(reinterpret_cast<cpVector*>(getcontrolpoints(k))->at(kk)->value("cr_x").toInt() - selsym->getRawCoordinateVector().at(j).nativeX())<2000) && \
								(abs(reinterpret_cast<cpVector*>(getcontrolpoints(k))->at(kk)->value("cr_y").toInt() - selsym->getRawCoordinateVector().at(j).nativeY())<2000))
						{
							courses[rn].control_points->erase(courses[rn].control_points->begin()+j);
							courses[rn].control_points->insert(courses[rn].control_points->begin()+j,courses[k].control_points->at(kk));
							MapCoord ccc=MapCoord();
							ccc.setNativeX(reinterpret_cast<cpVector*>(getcontrolpoints(k))->at(kk)->value("cr_x").toInt());
							ccc.setNativeY(reinterpret_cast<cpVector*>(getcontrolpoints(k))->at(kk)->value("cr_y").toInt());
							ccc.setDashPoint(true);
							selsym->asPath()->setCoordinate(j,ccc);
						}
					}
				}
			}
		}
	}
	updateRow(rn);
}

void courseWidget::selcourseeditnewpoint(int rn)
{
	coursereplace(rn,getcourse(rn));
	updateRow(rn);
}

void courseWidget::coursecpchanged()
{
	Object *selsym=map->getFirstSelectedObject();
	if (selsym->getSymbol()->getNumberAsString()==C_TEXT)
	{
		for (int i=0; i<getNumcourses(); i++)
			for (int j=0; j<getNumcoursecp(i);j++)
			{
				if (selsym==getcoursecp(i,j))
				{
					selcourseedittext(i,j);
				}
			}
	}
	if (selsym->getSymbol()->getNumberAsString()==C_LINE)
	{
		for (int i=0; i<getNumcourses(); i++)
			if (selsym==getcourse(i))
			{
				if (selsym->getRawCoordinateVector().size()!=getNumcoursecp(i))
				{
					selcourseeditnewpoint(i);
					break;
				}
				if (selsym->getRawCoordinateVector().size()==getNumcoursecp(i))
				{
					selcourseeditpoint(i);
					break;
				}
			}
	}
}

QToolButton* courseWidget::newToolButton(const QIcon& icon, const QString& text)
{
	QToolButton* button = new QToolButton();
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button->setToolTip(text);
	button->setIcon(icon);
	button->setText(text);
	button->setWhatsThis("<a href=\"courses.html#setup\">See more</a>");
	return button;
}

void courseWidget::cpdescr()
{
	int rownum=course_table->currentRow();
	if (rownum>=0)
	{
		double orig_x=0,orig_y=0;
		qint64 orig_rx=0,orig_ry=0;
		for (int i=0; i<map->getNumObjects(); i++)
		{
			if (map->getCurrentPart()->getObject(i)->getSymbol()->getNumberAsString()==CD_CORN)
			{
				orig_x=map->getCurrentPart()->getObject(i)->asPoint()->getRawCoordinateVector().at(0).x();
				orig_y=map->getCurrentPart()->getObject(i)->asPoint()->getRawCoordinateVector().at(0).y();
				orig_rx=map->getCurrentPart()->getObject(i)->asPoint()->getRawCoordinateVector().at(0).nativeX();
				orig_ry=map->getCurrentPart()->getObject(i)->asPoint()->getRawCoordinateVector().at(0).nativeY();
			}
		}
		int cd_height=courses[rownum].control_points->size();

		PathObject* line1=new PathObject(getSymbolByTextNumber("16.2"));
		int jk=0;
		int dd=0;
		for (int j=1; j<=cd_height+2; j++)
		{
			line1->addCoordinate(jk++,MapCoord(orig_x+dd,orig_y+6*j));
			dd=48-dd;
			line1->addCoordinate(jk++,MapCoord(orig_x+dd,orig_y+6*j));
		}
		dd=(cd_height+2)*6;
		for (int j=1; j<=7; j++)
		{
			line1->addCoordinate(jk++,MapCoord(orig_x+6*j,orig_y+dd));
			if (dd==18)
			{
				dd=(cd_height+2)*6;
			}
			else
			{
				dd=18;
			}
			line1->addCoordinate(jk++,MapCoord(orig_x+6*j,orig_y+dd));
		}
		map->addObject(line1);

		Symbol* sym=getSymbolByTextNumber("16.1");
		line1=new PathObject(sym);
		line1->addCoordinate(0,MapCoord(orig_x+18,orig_y+12));
		line1->addCoordinate(1,MapCoord(orig_x+18,orig_y+(cd_height+2)*6));
		map->addObject(line1);
		line1=new PathObject(sym);
		line1->addCoordinate(0,MapCoord(orig_x+36,orig_y+12));
		line1->addCoordinate(1,MapCoord(orig_x+36,orig_y+(cd_height+2)*6));
		map->addObject(line1);
		line1=new PathObject(sym);
		line1->addCoordinate(0,MapCoord(orig_x,orig_y));
		line1->addCoordinate(1,MapCoord(orig_x,orig_y+(cd_height+3)*6));
		line1->addCoordinate(2,MapCoord(orig_x+48,orig_y+(cd_height+3)*6));
		line1->addCoordinate(3,MapCoord(orig_x+48,orig_y));
		line1->addCoordinate(4,MapCoord(orig_x,orig_y));
		map->addObject(line1);
		for (int j=0; j<4; j++)
		{
			line1=new PathObject(sym);
			line1->addCoordinate(0,MapCoord(orig_x,orig_y+6*j));
			line1->addCoordinate(1,MapCoord(orig_x+48,orig_y+6*j));
			map->addObject(line1);
		}
		for (int j=4; j<cd_height+3; j+=3)
		{
			line1=new PathObject(sym);
			line1->addCoordinate(0,MapCoord(orig_x,orig_y+6*j));
			line1->addCoordinate(1,MapCoord(orig_x+48,orig_y+6*j));
			map->addObject(line1);
		}
		line1=new PathObject(sym);
		line1->addCoordinate(0,MapCoord(orig_x,orig_y+(cd_height+2)*6));
		line1->addCoordinate(1,MapCoord(orig_x+48,orig_y+(cd_height+2)*6));
		map->addObject(line1);
		TextObject* to;
		sym=getSymbolByTextNumber("17.1");
		for (int i=1; i<cd_height-1; i++)
		{
			to=new TextObject(sym);
			to->setText(QString::number(i));
			to->setBox(orig_rx+3*1000,i*6000+orig_ry+21*1000,6,6);
			map->addObject(to);
		}
		sym=getSymbolByTextNumber("17.2");
		PointObject* po;
		Symbol* sym1;
		sym1=getSymbolByTextNumber("15.1");
		po=new PointObject(sym1);
		po->setPosition(orig_rx+3*1000,orig_ry+21*1000);
		map->addObject(po);
		for (int i=1; i<cd_height-1; i++)
		{
			to=new TextObject(sym);
			to->setText(courses[rownum].control_points->at(i)->value("cp_cod"));
			to->setBox(orig_rx+9*1000,i*6000+orig_ry+21*1000,6,6);
			map->addObject(to);
			for (int k=0; k<6; k++)
			{
				sym1=getSymbolByTextNumber(courses[rownum].control_points->at(i)->value(xml_names[k]));
				if (sym1->getType()==Symbol::Point)
				{
					po=new PointObject(sym1);
					po->setPosition(orig_rx+(k*6+15)*1000,i*6000+orig_ry+21*1000);
					map->addObject(po);
				}
			}
		}
	}
}

void courseWidget::insertcp()
{
	int cc=sender()->property("name").toInt();
	int i=course_table->currentRow();
	if (i<0) return;
	if (getcoursecp(i,0)==NULL)
	{
		Symbol* ns=getSymbolByTextNumber(QString(C_TEXT));
		int n_cp=0;
		for (unsigned int j=0; j<getcourse(i)->getRawCoordinateVector().size(); j++)
		{
			if(!getcourse(i)->asPath()->isCurveHandle(j))
			{
				TextObject *cp_symbol=new TextObject();
				cp_symbol->setSymbol(ns,false);
				switch (cc)
				{
					case 1:
						cp_symbol->setText(QString::number(n_cp));
						cp_symbol->setBox(courses[i].control_points->at(n_cp)->value("cp_x").toInt(),
										  courses[i].control_points->at(n_cp)->value("cp_y").toInt(),
										  10,10);
						break;
					case 2:
						cp_symbol->setText(courses[i].control_points->at(n_cp)->value("cp_cod"));
						cp_symbol->setBox(courses[i].control_points->at(n_cp)->value("cp_x").toInt(),
										  courses[i].control_points->at(n_cp)->value("cp_y").toInt(),
										  10,10);
						break;
					case 3:
						cp_symbol->setText(QString::number(n_cp)+"-"+courses[i].control_points->at(n_cp)->value("cp_cod"));
						cp_symbol->setBox(courses[i].control_points->at(n_cp)->value("cp_x").toInt(),
										  courses[i].control_points->at(n_cp)->value("cp_y").toInt(),
										  10,10);
						break;
				}
				map->addObject(cp_symbol);
				setcoursecp(cp_symbol,i,n_cp);
				n_cp++;
			}
		}
		map->setObjectsDirty();
	}
}

void courseWidget::clearcp()
{
	int r=course_table->currentRow();
	if (getcoursecp(r,0)!=NULL)
	{
		for (int i=0; i<getNumcourses(); i++)
			clearcoursecp(i);
		map->setObjectsDirty();
	}
}

bool courseWidget::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == course_table)
	{
		if ( event->type() == QEvent::KeyPress &&
			 static_cast<QKeyEvent*>(event)->key() == Qt::Key_Space )
		{
			int row = course_table->currentRow();
			if (row >= 0 && course_table->item(row, 1)->flags().testFlag(Qt::ItemIsEnabled))
			{
				bool is_checked = course_table->item(row, 0)->checkState() != Qt::Unchecked;
				course_table->item(row, 0)->setCheckState(is_checked ? Qt::Unchecked : Qt::Checked);
			}
			return true;
		}
		
		if ( event->type() == QEvent::KeyRelease &&
			 static_cast<QKeyEvent*>(event)->key() == Qt::Key_Space )
		{
			return true;
		}
	}
	
	return false;
}

void courseWidget::addcourse()
{
	Object* new_course;
	if (map->getNumSelectedObjects()==0) return;
	QString zz=map->getFirstSelectedObject()->getSymbol()->getNumberAsString();
	if (zz==C_LINE)
	{
		if (findcourseIndex(map->getFirstSelectedObject())>=0)
		{
			new_course = map->getFirstSelectedObject()->duplicate();
		}
		else
		{
			new_course = map->getFirstSelectedObject();
		}
	}
	else
	{
		return;
	}

	courseadd(new_course);
	addRow();
	updateButtons();
}

void courseWidget::deletecourse()
{
	if (course_table->rowCount()>0)
	{
		int rownum = course_table->currentRow();

		map->clearObjectSelection(false);
		Object *dc=getcourse(rownum);

		removecourse(rownum);
		map->deleteObject(dc,false);
		course_table->removeRow(rownum);
		course_table->clearSelection();

		updateButtons();
	}
}

void courseWidget::movecourseUp()
{
	int row = course_table->currentRow();
	Q_ASSERT(row >= 1);
	if (!(row >= 1)) return;
	
	Object* above_course = getcourse(row-1);
	Object* cur_course = getcourse(row);
	setcourse(cur_course, row-1);
	setcourse(above_course, row);
	
	updateRow(row - 1);
	updateRow(row);
	
	updateButtons();
}

void courseWidget::movecourseDown()
{
	int row = course_table->currentRow();
	if (!(row < course_table->rowCount() - 1)) return;
	
	// Exchanging two courses
	Object* below_course = getcourse(row+1);
	Object* cur_course = getcourse(row);
	setcourse(cur_course, row+1);
	setcourse(below_course, row);
	
	updateRow(row + 1);
	updateRow(row);
	
	updateButtons();
}

void courseWidget::showHelp()
{
	Util::showHelp(controller->getWindow(), "courses.html", "setup");
}

void courseWidget::cellChange(int row, int column)
{
	if (!react_to_changes)
		return;
	
	if (row >= 0)
	{
		Object* temp = getcourse(row);
		if (!temp)
			return;
		
		react_to_changes = false;
		
		if (column == 0)
		{
			if (course_table->item(row, 0)->checkState() != Qt::Checked)
			{
				PathObject *the_new_course=getcourse(row)->duplicate()->asPath();
				map->clearObjectSelection(false);
				map->deleteObject(getcourse(row),false);
				setcourse(the_new_course,row);
			}
			else
			{
				map->addObject(getcourse(row));
			}
			updateRow(row);
			updateButtons();
		}
		
		react_to_changes = true;
		}
}

void courseWidget::updateButtons()
{
	if (!react_to_changes)
		return;

	bool first_row_selected = false;
	bool last_row_selected = false;
	int num_rows_selected = 0;
	num_rows_selected=course_table->selectedItems().size();
	bool single_row_selected = ((num_rows_selected <5)&&(num_rows_selected>0));
	if (single_row_selected)
	{
		const int row = course_table->currentRow();
		if (row == 0)
			first_row_selected = true;
		if (row == course_table->rowCount() - 1)
			last_row_selected = true;
	}
	insert_cp_on_map_button->setEnabled(true);
	insert_cpc_on_map_button->setEnabled(true);
	insert_cpboth_on_map_button->setEnabled(true);
	clear_cp_button->setEnabled(true);
	delete_button->setEnabled(single_row_selected);
	move_up_button->setEnabled(single_row_selected && !first_row_selected);
	move_down_button->setEnabled(single_row_selected && !last_row_selected);
	edit_button->setEnabled(single_row_selected);
}

void courseWidget::addRow()
{
	react_to_changes = false;

	course_table->insertRow(course_table->rowCount());
	int nr=course_table->rowCount()-1;
	for (int i = 0; i < 4; ++i)
	{
		QTableWidgetItem* item = new QTableWidgetItem();
		course_table->setItem(nr, i, item);
	}
	course_table->item(nr, 0)->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	course_table->item(nr, 0)->setCheckState(Qt::Checked);
	updateRow(nr);
	course_table->selectRow(nr);

	react_to_changes = true;
}

void courseWidget::updateRow(int row)
{
	react_to_changes = false;
	
	QString name, cp_num, length;
	if (row >= 0)
	{
		name = courses[row].course_name;
		int n_cp=courses[row].control_points->size();
		cp_num=QString::number(n_cp-2);

		Object* object = getcourse(row);
		const PathPartVector& parts = static_cast<PathObject*>(object)->parts();
		double mm_to_meters  = 0.001 * map->getScaleDenominator();
		double length_mm	 = parts.front().length();
		double length_meters = length_mm * mm_to_meters;
		length = locale().toString(length_meters, 'f', 0) % " " % tr("m", "meters");
		course_table->item(row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		course_table->item(row, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		course_table->item(row, 3)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		course_table->item(row, 1)->setText(name);
		course_table->item(row, 2)->setText(cp_num);
		course_table->item(row, 3)->setText(length);
	}
	
	react_to_changes = true;
}

void courseWidget::editClicked(bool checked)
{
	Q_UNUSED(checked);
	int rn=course_table->currentRow();
	controller->addcourseEditDockWidget(rn, this);
}

void courseWidget::opencourseClicked()
{
	QSettings settings;
	QString course_directory = settings.value("courseFileDirectory", QDir::homePath()).toString();

	QString path = QFileDialog::getOpenFileName(this, tr("Open course file"), course_directory, QString("%1 (*.xml);;%2 (*.*)").arg(tr("Course files")).arg(tr("All files")));
	path = QFileInfo(path).canonicalFilePath();
	if (path.isEmpty())
		return;

	QFile inputFile(path);

	if (!inputFile.open(QIODevice::ReadOnly))
	{
		return;
	}

	QXmlStreamReader reader(&inputFile);
	int n_cp=0;
	cpVector* t1=new cpVector();
	c_point* t2=new c_point();
	QString course_name;
	while (!reader.atEnd())
	{
		reader.readNext();
		if (reader.isEndElement()) continue;
		if (reader.name() == "cp")
		{
			while (!reader.atEnd())
			{
				reader.readNext();
				if ((reader.name()=="cp") && reader.isEndElement())
				{
					t1->push_back(t2);
					t2=new c_point();
					break;
				}
				if (reader.isEndElement()) continue;
				if(reader.name().toString()!="")
				{
					t2->insert(reader.name().toString(),reader.readElementText());
				}
			}
		}
		else if((reader.name() == "course") && !reader.isEndElement())
		{
			if (t1->size()>0)
			{
				addcoursefromfile(t1,course_name);
				t1=new cpVector();
				n_cp++;
			}
			course_name=reader.attributes().value("name").toString();
		}
	}
	reader.clear();
	inputFile.close();
	addcoursefromfile(t1,course_name);
}

void courseWidget::savecourseClicked()
{
	if (getNumcourses()>0)
	{
		QFileInfo current(controller->getWindow()->currentPath());
		QString save_directory = current.canonicalPath();
		if (save_directory.isEmpty())
		{
			// revert to least recently used directory or home directory.
			QSettings settings;
			save_directory = settings.value("openFileDirectory", QDir::homePath()).toString();
		}

		QString filters=QString("*.xml");

		QString filter = NULL;
		QString outputfilepath = QFileDialog::getSaveFileName(this, tr("Save courses"), save_directory, filters, &filter);

		if (outputfilepath.isEmpty())
			return;

		if (!outputfilepath.endsWith(QString(".xml"), Qt::CaseInsensitive))
			outputfilepath.append(".xml");

		QFile outputFile(outputfilepath);

		if (!outputFile.open(QIODevice::WriteOnly))
		{
			return;
		}

		QXmlStreamWriter writer(&outputFile);
		writer.setAutoFormatting(true);
		writer.writeStartDocument();
		writer.writeStartElement("courses");
		for (int i=0;i<getNumcourses();i++)
		{
			writer.writeStartElement("course");
			writer.writeAttribute(QString("name"), courses[i].course_name);
			for (unsigned int j=0;j<reinterpret_cast<cpVector*>(getcontrolpoints(i))->size();j++)
			{
				QHashIterator<QString,QString> hi(*reinterpret_cast<cpVector*>(getcontrolpoints(i))->at(j));
				writer.writeStartElement("cp");
				while (hi.hasNext())
				{
					hi.next();
					writer.writeTextElement(hi.key(), hi.value());
				}
				writer.writeEndElement();
			}
			writer.writeEndElement();
		}
		writer.writeEndElement();
		writer.writeEndDocument();
		outputFile.close();
	}
}

void courseWidget::setcourse(Object* temp, int row)
{
	courses[row].course = temp;
//	emit(courseChanged(row, courses[row].course));
}

void *courseWidget::getcontrolpoints(int i)
{
	if (i>=0)
	{
		return courses[i].control_points;
	}
	else
	{
		return NULL;
	}
}

void courseWidget::setcontrolpointstext(QString new_key, QString new_value, int course_index, int cp_index)
{
	courses[course_index].control_points->at(cp_index)->remove(new_key);
	courses[course_index].control_points->at(cp_index)->insert(new_key,new_value);
}

void courseWidget::setcontrolpoints(cpVector* temp, int row)
{
	courses[row].control_points = temp;
//	emit(courseChanged(row, courses[row]));
}

void courseWidget::clearcoursecp(int i)
{
	for (unsigned int j=0;j<courses[i].coursecp.size();j++)
	{
		map->deleteObject(courses[i].coursecp.at(j),false);
		courses[i].coursecp.at(j)=NULL;
	}
}

void courseWidget::removecourse(int pos)
{
	courses.removeAt(pos);
//	emit(courseDeleted(pos, temp));
}

int courseWidget::findcourseIndex(const Object* temp) const
{
	int size = (int)courses.size();
	for (int i = 0; i < size; ++i)
	{
		if (courses[i].course == temp)
			return i;
	}
	return -1;
}

Symbol* courseWidget::getSymbolByTextNumber(QString ts)
{
	Symbol *sym;
	for (int k=0; k<map->getNumSymbols(); k++)
	{
		if (map->getSymbol(k)->getNumberAsString()==ts)
			sym=map->getSymbol(k);
	}
	return sym;
}

void courseWidget::addcoursefromfile(cpVector* temp,QString c_name)
{
	MapCoord mc;
	onecourse new_course;
	new_course.control_points=temp;
	new_course.course_name=c_name;
	Symbol* csym=getSymbolByTextNumber(C_LINE);
	PathObject* new_course_obj=new PathObject();
	new_course_obj->setSymbol(csym,true);
	for (unsigned int i=0; i<temp->size(); i++)
	{
		mc.setNativeX(temp->at(i)->value("cp_x").toInt());
		mc.setNativeY(temp->at(i)->value("cp_y").toInt());
		mc.setDashPoint(true);
		new_course_obj->addCoordinate(i,mc);
		new_course.coursecp.push_back(NULL);
	}
	new_course_obj->recalculateParts();
	map->addObject(new_course_obj);
	new_course.course=new_course_obj;
	courses.push_back(new_course);
	addRow();
}

void* courseWidget::getcoursedata(Object* temp)
{
	onecourse* new_course=new onecourse;
	cpVector* ss=new cpVector();
	c_point* s1;
	new_course->course_name=QString("- new -");
	for(unsigned int i=0; i < (temp->asPath()->getCoordinateCount()); i++)
	{
		s1=new c_point();
		s1->insert(QString("cp_cod"),QString("0"));
		s1->insert(QString("cp_x"),QString::number(temp->asPath()->getRawCoordinateVector()[i].nativeX()-4000));
		s1->insert(QString("cp_y"),QString::number(temp->asPath()->getRawCoordinateVector()[i].nativeY()-10000));
		s1->insert(QString("cr_x"),QString::number(temp->asPath()->getRawCoordinateVector()[i].nativeX()));
		s1->insert(QString("cr_y"),QString::number(temp->asPath()->getRawCoordinateVector()[i].nativeY()));
		for(int j=0;j<6;j++)
		{
			s1->insert(xml_names[j],QString("18.1"));
		}
		ss->push_back(s1);
		new_course->coursecp.push_back(NULL);
	}
	new_course->control_points=ss;
	new_course->course=temp;

	return new_course;
}

void courseWidget::courseadd(Object* temp)
{
	onecourse* new_course;
	new_course=reinterpret_cast<onecourse*>(getcoursedata(temp));
	courses.push_back(*new_course);
}

void courseWidget::coursereplace(int rn, Object* temp)
{
	onecourse* new_course=new onecourse;
	onecourse old_course=courses[rn];
	new_course=reinterpret_cast<onecourse*>(getcoursedata(temp));
	new_course->course_name=old_course.course_name;
	int cpn=0;
	unsigned int i=0;
	while (i<old_course.coursecp.size())
	{
		if((old_course.control_points->at(i)->value("cr_x").toInt()==reinterpret_cast<PathObject*>(temp)->getCoordinate(cpn).nativeX()) \
		   &&(old_course.control_points->at(i)->value("cr_y").toInt()==reinterpret_cast<PathObject*>(temp)->getCoordinate(cpn).nativeY()))
		{
			new_course->control_points->at(cpn)=old_course.control_points->at(i);
			cpn++;
			i++;
		}
		else
		{
			if(new_course->control_points->size()>old_course.control_points->size())
			{
				cpn++;
			}
			else
			{
				i++;
			}
		}
	}
	courses.replace(rn, *new_course);
}
