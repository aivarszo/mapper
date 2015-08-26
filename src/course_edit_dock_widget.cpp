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

#include "course_edit_dock_widget.h"

#include <QtWidgets>

#include "gui/main_window.h"
#include "map_editor.h"
#include "course_dock_widget.h"
#include "map.h"
#include "symbol.h"

courseEditDockWidget::courseEditDockWidget(int rn, courseWidget* temp, MapEditorController* controller, QWidget* parent)
 : QDockWidget(tr("Course edit"), parent), controller(controller)
{
	react_to_changes = true;
	rownum=rn;
	cw=temp;
	QFontMetrics metrics(QApplication::font());
	courseWidget::cpVector *ss=reinterpret_cast<courseWidget::cpVector*>(temp->getcontrolpoints(rownum));

	QWidget* child_widget = new QWidget();
	QBoxLayout* cp_edit_field = new QVBoxLayout();
	QGridLayout* layout = new QGridLayout();
	QGridLayout* mlayout = new QGridLayout();
	x_label.push_back(new QLabel(tr("course")));
	x_edit.push_back(new QLineEdit(cw->getcoursename(rownum)));
	mlayout->addWidget(x_label[0], 0, 0);
	mlayout->addWidget(x_edit[0], 0, 1);
	cp_edit_field->addLayout(mlayout);
	int ccp=0;
	int nextcol=0;
	for (unsigned int i=0; i<ss->size(); i++)
	{
		if(cw->getcourse(rownum)->asPath()->getCoordinate(i).isDashPoint())
		{
			QString cps=QString::number((nextcol/3*15+(nextcol/3))+ccp);
			QLabel *nl=new QLabel(cps);
			QLineEdit *ne=new QLineEdit(ss->at(i)->value("cp_cod"));
			ne->setFixedSize(metrics.width("8888"),25);
			x_label.push_back(nl);
			x_edit.push_back(ne);
			layout->addWidget(nl, ccp, 0+nextcol);
			layout->addWidget(ne, ccp, 1+nextcol);
			SegmentedButtonLayout* button_layout = new SegmentedButtonLayout();
			for(int j=0;j<6;j++)
			{
				QToolButton* cd_button = newToolButton(i,j,QIcon(":/images/blank_cd.png"), cw->xml_names[j]+cps);
				connect(cd_button, &QToolButton::clicked, this, &courseEditDockWidget::set_cd_icon);
				button_layout->addWidget(cd_button);
			}
			QBoxLayout* list_buttons_layout = new QHBoxLayout();
			list_buttons_layout->setContentsMargins(0,0,0,0);
			list_buttons_layout->addLayout(button_layout);
			QWidget* list_buttons_group = new QWidget();
			list_buttons_group->setLayout(list_buttons_layout);
			layout->addWidget(list_buttons_group,ccp,2+nextcol);
			ccp++;
			if (ccp>15)
			{
				nextcol+=3;
				ccp=0;
			}
		}
	}
	cp_edit_field->addLayout(layout);

	QGridLayout* flayout = new QGridLayout();
	lb_to_finish=new QLabel(tr("Last control to finish:"));
	ed_to_finish=new QLineEdit(cw->getdisttofinish(rownum));
	flayout->addWidget(lb_to_finish, 0, 0);
	flayout->addWidget(ed_to_finish, 1, 0);
	cp_edit_field->addLayout(flayout);

	x_button = new QPushButton(tr("Save"));
	cp_edit_field->addWidget(x_button);
	QPushButton* x_button1 = new QPushButton(tr("Close"));
	cp_edit_field->addWidget(x_button1);
	child_widget->setLayout(cp_edit_field);
	setWidget(child_widget);
	

	connect(x_button, &QPushButton::clicked, this, &courseEditDockWidget::savecourse);
	connect(x_button1, &QPushButton::clicked, this, &courseEditDockWidget::close);
}

void courseEditDockWidget::set_cd_icon()
{
	int cp_row=reinterpret_cast<QToolButton*>(sender())->toolTip().mid(4).toInt();
	int cd_n=sender()->property("name").toInt();
	QString cd_k=cw->xml_names[cd_n];
	reinterpret_cast<QToolButton*>(sender())->setIcon(QPixmap::fromImage(controller->activeSymbol()->getIcon(controller->getMap(),false).copy()));
	QString cd_v=controller->activeSymbol()->getNumberAsString();
	for(int i=0; i<cw->getNumcourses(); i++)
	{
		for(int j=0; j<cw->getNumcoursecp(i); j++)
		{
			if(reinterpret_cast<courseWidget::cpVector*>(cw->getcontrolpoints(i))->at(j)->value("cp_cod")==x_edit[cp_row+1]->text() \
			   && (x_edit[cp_row+1]->text()!=QString("0")))
			{
				cw->setcontrolpointstext(cd_k,cd_v,i,j);
			}
		}
	}
}

bool courseEditDockWidget::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride && controller->getWindow()->areShortcutsDisabled())
		event->accept();
	return QDockWidget::event(event);
}

void courseEditDockWidget::closeEvent(QCloseEvent* event)
{
	Q_UNUSED(event);
	delete this;
}

QToolButton* courseEditDockWidget::newToolButton(int cp_num, int cr_num, const QIcon& icon, const QString& text)
{
	Q_UNUSED(icon);
	QToolButton* button = new QToolButton();
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button->setToolTip(text);
	button->setProperty("name",cr_num);

	Symbol* symstr;
	for (int i=0; i<controller->getMap()->getNumSymbols(); i++)
	{
		symstr=controller->getMap()->getSymbol(i);
		if (symstr->getNumberAsString()==reinterpret_cast<courseWidget::cpVector*>(cw->getcontrolpoints(rownum))->at(cp_num)->value(cw->xml_names[cr_num]))
		{
			button->setIcon(QPixmap::fromImage(symstr->getIcon(controller->getMap(),false).copy()));
			break;
		}
	}
	button->setText(text);
	button->setWhatsThis("<a href=\"courses.html#setup\">See more</a>");
	return button;
}


void courseEditDockWidget::savecourse()
{
	courseWidget::cpVector* ss=reinterpret_cast<courseWidget::cpVector*>(cw->getcontrolpoints(rownum));
	cw->setcoursename(QString(x_edit[0]->text()),rownum);
	cw->setdisttofinish(QString(ed_to_finish->text()),rownum);
	int ccp=1;
	for (unsigned int i=1; i<ss->size(); i++)
	{
		if(cw->getcourse(rownum)->asPath()->getCoordinate(i-1).isDashPoint())
		{
			cw->setcontrolpointstext("cp_cod",QString(x_edit[ccp]->text()),rownum,i-1);
			ccp++;
		}
	}
	cw->updateRow(rownum);
}
