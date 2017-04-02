/***************************************************************************
    File                 : NotesDock.cpp
    Project              : LabPlot
    Description          : Dock for configuring notes
    --------------------------------------------------------------------
    Copyright            : (C) 2016 Garvit Khatri (garvitdelhi@gmail.com)

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#include "NoteDock.h"
#include "kdefrontend/TemplateHandler.h"

#include <QDir>
#include <KConfigGroup>
#include <KLocale>

NoteDock::NoteDock(QWidget *parent) : QWidget(parent), m_initializing(false), m_notes(0) {
	ui.setupUi(this);

	connect(ui.leName, SIGNAL(returnPressed(QString)), this, SLOT(nameChanged(QString)));
	connect(ui.leComment, SIGNAL(returnPressed(QString)), this, SLOT(commentChanged(QString)));
	
	connect(ui.kcbBgColor, SIGNAL(changed(QColor)), this, SLOT(backgroundColorChanged(QColor)));
	connect(ui.kcbTextColor, SIGNAL(changed(QColor)), this, SLOT(textColorChanged(QColor)));
	connect(ui.kfrTextFont, SIGNAL(fontSelected(QFont)), this, SLOT(textFontChanged(QFont)));

	TemplateHandler* templateHandler = new TemplateHandler(this, TemplateHandler::Worksheet);
	ui.gridLayout->addWidget(templateHandler, 8, 3);
	templateHandler->show();
	connect(templateHandler, SIGNAL(loadConfigRequested(KConfig&)), this, SLOT(loadConfigFromTemplate(KConfig&)));
	connect(templateHandler, SIGNAL(saveConfigRequested(KConfig&)), this, SLOT(saveConfigAsTemplate(KConfig&)));
}

void NoteDock::setNotesList(QList< Note* > list) {
	m_notesList = list;
	m_notes = list.first();

	m_initializing=true;
	ui.leName->setText(m_notes->name());
	ui.kcbBgColor->setColor(m_notes->backgroundColor());
	ui.kcbTextColor->setColor(m_notes->textColor());
	ui.kfrTextFont->setFont(m_notes->textFont());
	m_initializing=false;
}

//*************************************************************
//********** SLOTs for changes triggered in NoteDock **********
//*************************************************************
void NoteDock::nameChanged(QString name) {
	if (m_initializing)
		return;

	m_notes->setName(name);
}

void NoteDock::commentChanged(QString name) {
	if (m_initializing)
		return;

	m_notes->setComment(name);
}

void NoteDock::backgroundColorChanged(QColor color) {
	if (m_initializing)
		return;

	foreach(Note* note, m_notesList)
		note->setBackgroundColor(color);
}

void NoteDock::textColorChanged(QColor color) {
	if (m_initializing)
		return;

	foreach(Note* note, m_notesList)
		note->setTextColor(color);
}

void NoteDock::textFontChanged(QFont font) {
	if (m_initializing)
		return;

	foreach(Note* note, m_notesList)
		note->setTextFont(font);
}

//*************************************************************
//************************* Settings **************************
//*************************************************************
void NoteDock::loadConfigFromTemplate(KConfig& config) {
	QString name;
	int index = config.name().lastIndexOf(QDir::separator());
	if (index!=-1)
		name = config.name().right(config.name().size() - index - 1);
	else
		name = config.name();

	KConfigGroup group = config.group("Notes");
	ui.kcbBgColor->setColor(group.readEntry("BackgroundColor", m_notes->backgroundColor()));
	ui.kcbTextColor->setColor(group.readEntry("TextColor", m_notes->textColor()));
	ui.kfrTextFont->setFont(group.readEntry("TextColor", m_notes->textFont()));
}

void NoteDock::saveConfigAsTemplate(KConfig& config) {
	KConfigGroup group = config.group("Notes");

	group.writeEntry("BackgroundColor", ui.kcbBgColor->color());
	group.writeEntry("TextColor", ui.kcbTextColor->color());
	group.writeEntry("TextFont", ui.kfrTextFont->font());
}
