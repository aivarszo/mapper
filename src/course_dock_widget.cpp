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


#include "course_dock_widget.h"
#include "course_edit_dock_widget.h"

#include "core/georeferencing.h"
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
	
//    for (int i = 0; i < map->getNumcourses(); i++)
//        addRow();
	
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
    insert_cpc_on_map_button = newToolButton(QIcon(":/images/insert-cp-cod.png"), tr("Show CP codes"));
    insert_cpboth_on_map_button = newToolButton(QIcon(":/images/insert-cp-numcod.png"), tr("Show both"));
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
    all_buttons_layout->addWidget(new QLabel("    "), 1);
	all_buttons_layout->addWidget(help_button);
	
    all_courses_layout->addLayout(all_buttons_layout);
    setLayout(all_courses_layout);
	
    updateButtons();
	
	// Connections
    connect(insert_cp_on_map_button, SIGNAL(clicked(bool)), this, SLOT(insertcponmap()));
    connect(insert_cpc_on_map_button, SIGNAL(clicked(bool)), this, SLOT(insertcpconmap()));
    connect(insert_cpboth_on_map_button, SIGNAL(clicked(bool)), this, SLOT(insertcpbothonmap()));
    connect(clear_cp_button, SIGNAL(clicked(bool)), this, SLOT(clearcp()));
    connect(cp_descr_button, SIGNAL(clicked(bool)), this, SLOT(cpdescr()));

    connect(edit_action, SIGNAL(triggered(bool)), this, SLOT(editClicked(bool)));

    connect(course_table, SIGNAL(cellChanged(int,int)), this, SLOT(cellChange(int,int)));
    connect(course_table->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateButtons()));
	
    connect(add_button, SIGNAL(clicked(bool)), this, SLOT(addcourse()));
    connect(delete_button, SIGNAL(clicked(bool)), this, SLOT(deletecourse()));
    connect(move_up_button, SIGNAL(clicked(bool)), this, SLOT(movecourseUp()));
    connect(move_down_button, SIGNAL(clicked(bool)), this, SLOT(movecourseDown()));
//	connect(help_button, SIGNAL(clicked(bool)), this, SLOT(showHelp()));
    connect(map, SIGNAL(selectedObjectEdited()), this, SLOT(coursecpchanged()));
    connect(map, SIGNAL(objectSelectionChanged()), this, SLOT(courseselchanged()));
}

courseWidget::~courseWidget()
{
	; // Nothing
}

void courseWidget::courseselchanged()
{
    for (int i=0; i<map->getNumcourses(); i++)
    {
        if (map->getFirstSelectedObject()==map->getcourse(i))
        {
            course_table->selectRow(i);
        }
    }
}

void courseWidget::coursecpchanged()
{
    Object *selsym=map->getFirstSelectedObject();
    if (selsym->getSymbol()->getNumberAsString()=="703")
    {
        for (int i=0; i<map->getNumcourses(); i++)
            for (int j=0; j<map->getNumcoursecp(i);j++)
            {
                if (selsym==map->getcoursecp(i,j))
                {
                    map->setcontrolpointstext(QString::number(map->getFirstSelectedObject()->getRawCoordinateVector().at(0).nativeX()),i,j*COURSE_ITEMS+2);
                    map->setcontrolpointstext(QString::number(map->getFirstSelectedObject()->getRawCoordinateVector().at(0).nativeY()),i,j*COURSE_ITEMS+3);
                }
            }
    }
    if (selsym->getSymbol()->getNumberAsString()=="799")
    {
        int rn;
        for (int i=0; i<map->getNumcourses(); i++)
            if (selsym==map->getcourse(i))
            {
                for (unsigned int j=0; j<selsym->getRawCoordinateVector().size(); j++)
                {
                    if(!selsym->asPath()->isCurveHandle(j))
                    {
                        map->setcontrolpointstext(QString::number(selsym->getRawCoordinateVector().at(j).nativeX()-4000),i,j*COURSE_ITEMS+2);
                        map->setcontrolpointstext(QString::number(selsym->getRawCoordinateVector().at(j).nativeY()-10000),i,j*COURSE_ITEMS+3);
                        map->setcontrolpointstext(QString::number(selsym->getRawCoordinateVector().at(j).nativeX()),i,j*COURSE_ITEMS+4);
                        map->setcontrolpointstext(QString::number(selsym->getRawCoordinateVector().at(j).nativeY()),i,j*COURSE_ITEMS+5);
                        for(int k=0; k<map->getNumcourses();k++)
                        {
                            if (selsym!=map->getcourse(k))
                            {
                                for(int kk=4;kk<map->getcontrolpoints(k)->size();kk+=COURSE_ITEMS)
                                {
                                    if(abs(map->getcontrolpoints(k)->at(kk).toInt()-selsym->getRawCoordinateVector().at(j).nativeX())<2000 && \
                                            abs(map->getcontrolpoints(k)->at(kk+1).toInt()-selsym->getRawCoordinateVector().at(j).nativeY())<2000)
                                    {
                                        map->setcontrolpointstext(map->getcontrolpoints(k)->at(kk),i,j*COURSE_ITEMS+4);
                                        map->setcontrolpointstext(map->getcontrolpoints(k)->at(kk+1),i,j*COURSE_ITEMS+5);
                                        //@todo set all cp data to edited dashpoint
                                        MapCoord ccc=MapCoord();
                                        ccc.setNativeX(map->getcontrolpoints(k)->at(kk).toInt());
                                        ccc.setNativeY(map->getcontrolpoints(k)->at(kk+1).toInt());
                                        ccc.setDashPoint(true);
                                        selsym->asPath()->setCoordinate(j,ccc);
                                    }
                                }
                            }
                        }
                    }
                }
                rn=i;
                break;
            }
        updateRow(rn);
    }
}
QString courseWidget::showOpencourseDialog(QWidget* dialog_parent, MapEditorController* controller)
{
    Q_UNUSED(controller);

    QSettings settings;
    QString course_directory = settings.value("courseFileDirectory", QDir::homePath()).toString();

    QString path = QFileDialog::getOpenFileName(dialog_parent, tr("Open course file"), course_directory, QString("%1 (*.xml);;%2 (*.*)").arg(tr("Course files")).arg(tr("All files")));
    path = QFileInfo(path).canonicalFilePath();
    if (path.isEmpty())
        return NULL;
    else
        return path;
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
            if (map->getCurrentPart()->getObject(i)->getSymbol()->getNumberAsString()=="720.0")
            {
                orig_x=map->getCurrentPart()->getObject(i)->asPoint()->getRawCoordinateVector().at(0).x();
                orig_y=map->getCurrentPart()->getObject(i)->asPoint()->getRawCoordinateVector().at(0).y();
                orig_rx=map->getCurrentPart()->getObject(i)->asPoint()->getRawCoordinateVector().at(0).nativeX();
                orig_ry=map->getCurrentPart()->getObject(i)->asPoint()->getRawCoordinateVector().at(0).nativeY();
            }
        }
        int cd_height=(map->getcontrolpoints(rownum)->size()-1)/COURSE_ITEMS;
        for (int i=0; i<map->getNumSymbols(); i++)
        {
            if (map->getSymbol(i)->getNumberAsString()=="16.2")
            {
                int jk=0;
                PathObject* line1=new PathObject(map->getSymbol(i));
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
            }
        }
        for (int i=0; i<map->getNumSymbols(); i++)
        {
            if (map->getSymbol(i)->getNumberAsString()=="16.1")
            {
                PathObject* line1=new PathObject(map->getSymbol(i));
                line1->addCoordinate(0,MapCoord(orig_x+18,orig_y+12));
                line1->addCoordinate(1,MapCoord(orig_x+18,orig_y+(cd_height+2)*6));
                map->addObject(line1);
                line1=new PathObject(map->getSymbol(i));
                line1->addCoordinate(0,MapCoord(orig_x+36,orig_y+12));
                line1->addCoordinate(1,MapCoord(orig_x+36,orig_y+(cd_height+2)*6));
                map->addObject(line1);
                line1=new PathObject(map->getSymbol(i));
                line1->addCoordinate(0,MapCoord(orig_x,orig_y));
                line1->addCoordinate(1,MapCoord(orig_x,orig_y+(cd_height+3)*6));
                line1->addCoordinate(2,MapCoord(orig_x+48,orig_y+(cd_height+3)*6));
                line1->addCoordinate(3,MapCoord(orig_x+48,orig_y));
                line1->addCoordinate(4,MapCoord(orig_x,orig_y));
                map->addObject(line1);
                for (int j=0; j<4; j++)
                {
                    line1=new PathObject(map->getSymbol(i));
                    line1->addCoordinate(0,MapCoord(orig_x,orig_y+6*j));
                    line1->addCoordinate(1,MapCoord(orig_x+48,orig_y+6*j));
                    map->addObject(line1);
                }
                for (int j=4; j<cd_height+3; j+=3)
                {
                    line1=new PathObject(map->getSymbol(i));
                    line1->addCoordinate(0,MapCoord(orig_x,orig_y+6*j));
                    line1->addCoordinate(1,MapCoord(orig_x+48,orig_y+6*j));
                    map->addObject(line1);
                }
                line1=new PathObject(map->getSymbol(i));
                line1->addCoordinate(0,MapCoord(orig_x,orig_y+(cd_height+2)*6));
                line1->addCoordinate(1,MapCoord(orig_x+48,orig_y+(cd_height+2)*6));
                map->addObject(line1);
            }
        }
        TextObject* to;
        Symbol* sym=map->getSymbolByTextNumber(QString("17.1"));
        for (int i=0; i<cd_height; i++)
        {
            to=new TextObject(sym);
            to->setText(QString::number(i));
            to->setBox(orig_rx+3*1000,i*6000+orig_ry+21*1000,6,6);
            map->addObject(to);
        }
        sym=map->getSymbolByTextNumber(QString("17.2"));
        PointObject* po;
        Symbol* sym1;
        for (int i=0; i<cd_height; i++)
        {
            to=new TextObject(sym);
            to->setText(map->getcontrolpoints(rownum)->at(i*COURSE_ITEMS+1));
            to->setBox(orig_rx+9*1000,i*6000+orig_ry+21*1000,6,6);
            map->addObject(to);
            for (int k=5; k<11; k++)
            {
                sym1=map->getSymbolByTextNumber(map->getcontrolpoints(rownum)->at(i*COURSE_ITEMS+k+1));
                if (sym1->getType()==Symbol::Point)
                {
                    po=new PointObject(sym1);
                    po->setPosition(orig_rx+((k-5)*6+15)*1000,i*6000+orig_ry+21*1000);
                    map->addObject(po);
                }
            }
        }
    }
}

void courseWidget::insertcp(int cc)
{
    int i=course_table->currentRow();
    if (i<0) return;
    if (map->getcoursecp(i,0)==NULL)
    {
        Symbol* ns=map->getSymbolByTextNumber(QString("703"));
        int n_cp=0;
        for (unsigned int j=0; j<map->getcourse(i)->getRawCoordinateVector().size(); j++)
        {
            if(!map->getcourse(i)->asPath()->isCurveHandle(j))
            {
                TextObject *cp_symbol=new TextObject();
                cp_symbol->setSymbol(ns,false);
                switch (cc)
                {
                    case 1:
                        cp_symbol->setText(QString::number(n_cp));
                        cp_symbol->setBox(map->getcontrolpoints(i)->at((n_cp)*COURSE_ITEMS+2).toInt(),map->getcontrolpoints(i)->at((n_cp)*COURSE_ITEMS+3).toInt(),10,10);
                        break;
                    case 2:
//                        qint64 xx=map->getcourse(i)->getRawCoordinateVector()[j].rawX();
//                        qint64 yy=map->getcourse(i)->getRawCoordinateVector()[j].rawY();
                        cp_symbol->setText(map->getcontrolpoints(i)->at((n_cp)*COURSE_ITEMS+1));
                        cp_symbol->setBox(map->getcontrolpoints(i)->at((n_cp)*COURSE_ITEMS+2).toInt(),map->getcontrolpoints(i)->at((n_cp)*COURSE_ITEMS+3).toInt(),10,10);
//                        cp_symbol->setBox(xx-4000,yy-10000,10,10);
                        break;
                    case 3:
                        cp_symbol->setText(QString::number(n_cp)+"-"+map->getcontrolpoints(i)->at((n_cp)*COURSE_ITEMS+1));
                        cp_symbol->setBox(map->getcontrolpoints(i)->at((n_cp)*COURSE_ITEMS+2).toInt(),map->getcontrolpoints(i)->at((n_cp)*COURSE_ITEMS+3).toInt(),10,10);
                        break;
                }
                map->addObject(cp_symbol);
                map->setcoursecp(cp_symbol,i,n_cp);
                n_cp++;
            }
        }
        map->setObjectsDirty();
    }
}

void courseWidget::insertcponmap()
{
    insertcp(1);
}

void courseWidget::insertcpconmap()
{
    insertcp(2);
}

void courseWidget::insertcpbothonmap()
{
    insertcp(3);
}

void courseWidget::clearcp()
{
    int r=course_table->currentRow();
    if (map->getcoursecp(r,0)!=NULL)
    {
        for (int i=0; i<map->getNumcourses(); i++)
            map->clearcoursecp(i);
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
    if (zz=="799")
    {
        if (map->findcourseIndex(map->getFirstSelectedObject())>=0)
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

    map->addcourse(new_course);
    addRow();
    map->setcoursesDirty();
    updateButtons();
}

void courseWidget::deletecourse()
{
    if (course_table->rowCount()>0)
    {
        int rownum = course_table->currentRow();

        map->clearObjectSelection(false);
        Object *dc=map->getcourse(rownum);

        map->removecourse(rownum);
        map->deleteObject(dc,false);
        course_table->removeRow(rownum);
        course_table->clearSelection();

        map->setcoursesDirty();
        map->setcourseAreaDirty(rownum);
        updateButtons();
    }
}

void courseWidget::movecourseUp()
{
    int row = course_table->currentRow();
	Q_ASSERT(row >= 1);
    if (!(row >= 1)) return;
	
    map->setcourseAreaDirty(row);
    map->setcourseAreaDirty(row-1);
	
    Object* above_course = map->getcourse(row-1);
    Object* cur_course = map->getcourse(row);
    map->setcourse(cur_course, row-1);
    map->setcourse(above_course, row);
	
    map->setcourseAreaDirty(row);
    map->setcourseAreaDirty(row-1);
	updateRow(row - 1);
	updateRow(row);
	
	updateButtons();
    map->setcoursesDirty();
}

void courseWidget::movecourseDown()
{
    int row = course_table->currentRow();
    if (!(row < course_table->rowCount() - 1)) return;
	
    // Exchanging two courses
    Object* below_course = map->getcourse(row+1);
    Object* cur_course = map->getcourse(row);
    map->setcourse(cur_course, row+1);
    map->setcourse(below_course, row);
	
    map->setcourseAreaDirty(row);
    map->setcourseAreaDirty(row+1);
	updateRow(row + 1);
	updateRow(row);
	
	updateButtons();
    map->setcoursesDirty();
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
        Object* temp = map->getcourse(row);
		if (!temp)
			return;
		
		react_to_changes = false;
		
		if (column == 0)
		{
            if (course_table->item(row, 0)->checkState() != Qt::Checked)
            {
                PathObject *the_new_course=map->getcourse(row)->duplicate()->asPath();
                map->clearObjectSelection(false);
                map->deleteObject(map->getcourse(row),false);
                map->setcourse(the_new_course,row);
            }
            else
            {
                map->addObject(map->getcourse(row));
            }
            map->setcourseAreaDirty(row);
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
        name = map->getcontrolpoints(row)->at(0);
        int n_cp=(map->getcontrolpoints(row)->size()-1)/COURSE_ITEMS;
        cp_num=QString::number(n_cp-2);

        Object* object = map->getcourse(row);
        const PathPartVector& parts = static_cast<PathObject*>(object)->parts();
        double mm_to_meters  = 0.001 * map->getScaleDenominator();
        double length_mm     = parts.front().length();
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
