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
	QStringList *ss=temp->getcontrolpoints(rownum);

	QWidget* child_widget = new QWidget();
	QBoxLayout* cp_edit_field = new QVBoxLayout();
	QGridLayout* layout = new QGridLayout();
	QGridLayout* mlayout = new QGridLayout();
	x_label.push_back(new QLabel(tr("course")));
	x_edit.push_back(new QLineEdit(ss->at(0)));
	mlayout->addWidget(x_label[0], 0, 0);
	mlayout->addWidget(x_edit[0], 0, 1);
	cp_edit_field->addLayout(mlayout);
	int ccp=0;
	int nextcol=0;
	for (int i=1; i<ss->size(); i+=COURSE_ITEMS)
	{
		QString cps=QString::number((nextcol/3*15+(nextcol/3))+ccp);
		QLabel *nl=new QLabel(cps);
		QLineEdit *ne=new QLineEdit(ss->at(i));
		ne->setFixedSize(metrics.width("8888"),25);
		x_label.push_back(nl);
		x_edit.push_back(ne);
		layout->addWidget(nl, ccp, 0+nextcol);
		layout->addWidget(ne, ccp, 1+nextcol);
		SegmentedButtonLayout* button_layout = new SegmentedButtonLayout();
		QToolButton* cd_C_button = newToolButton(i+5,QIcon(":/images/blank_cd.png"), tr("C")+cps);
		connect(cd_C_button, SIGNAL(clicked()), this, SLOT(set_cd_icon()));
		QToolButton* cd_D_button = newToolButton(i+6,QIcon(":/images/blank_cd.png"), tr("D")+cps);
		connect(cd_D_button, SIGNAL(clicked()), this, SLOT(set_cd_icon()));
		QToolButton* cd_E_button = newToolButton(i+7,QIcon(":/images/blank_cd.png"), tr("E")+cps);
		connect(cd_E_button, SIGNAL(clicked()), this, SLOT(set_cd_icon()));
		QToolButton* cd_F_button = newToolButton(i+8,QIcon(":/images/blank_cd.png"), tr("F")+cps);
		connect(cd_F_button, SIGNAL(clicked()), this, SLOT(set_cd_icon()));
		QToolButton* cd_G_button = newToolButton(i+9,QIcon(":/images/blank_cd.png"), tr("G")+cps);
		connect(cd_G_button, SIGNAL(clicked()), this, SLOT(set_cd_icon()));
		QToolButton* cd_H_button = newToolButton(i+10,QIcon(":/images/blank_cd.png"), tr("H")+cps);
		connect(cd_H_button, SIGNAL(clicked()), this, SLOT(set_cd_icon()));
		button_layout->addWidget(cd_C_button);
		button_layout->addWidget(cd_D_button);
		button_layout->addWidget(cd_E_button);
		button_layout->addWidget(cd_F_button);
		button_layout->addWidget(cd_G_button);
		button_layout->addWidget(cd_H_button);
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
	cp_edit_field->addLayout(layout);
	
	x_button = new QPushButton(tr("Save"));
	cp_edit_field->addWidget(x_button);
	QPushButton* x_button1 = new QPushButton(tr("Close"));
	cp_edit_field->addWidget(x_button1);
	child_widget->setLayout(cp_edit_field);
	setWidget(child_widget);
	

	connect(x_button, SIGNAL(clicked()), this, SLOT(savecourse()));
	connect(x_button1, SIGNAL(clicked()), this, SLOT(close()));
}

void courseEditDockWidget::set_cd_icon()
{
	qobject_cast<QToolButton*>(QObject::sender())->setIcon(QPixmap::fromImage(controller->activeSymbol()->getIcon(controller->getMap(),false).copy()));
	QString cdn=controller->activeSymbol()->getNumberAsString();
	cw->setcontrolpointstext(cdn,rownum,qobject_cast<QToolButton*>(QObject::sender())->text().mid(1).toInt()*COURSE_ITEMS+6+(qobject_cast<QToolButton*>(QObject::sender())->text().mid(0,1).toStdString().c_str()[0]-'C'));
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

QToolButton* courseEditDockWidget::newToolButton(int k, const QIcon& icon, const QString& text)
{
	QToolButton* button = new QToolButton();
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button->setToolTip(text);

	Symbol* symstr;
	for (int i=0; i<controller->getMap()->getNumSymbols(); i++)
	{
		symstr=controller->getMap()->getSymbol(i);
		if (symstr->getNumberAsString()==cw->getcontrolpoints(rownum)->at(k))
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
	QStringList *ss=cw->getcontrolpoints(rownum);
	ss->replace(0,QString(x_edit[0]->text()));
	int ccp=1;
	for (int i=1; i<ss->size(); i+=COURSE_ITEMS)
	{
		ss->replace(i,QString(x_edit[ccp]->text()));
		ccp++;
	}
	cw->setcontrolpoints(ss,rownum);
	cw->updateRow(rownum);
}
