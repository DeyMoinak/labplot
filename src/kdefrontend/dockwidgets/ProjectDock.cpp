/***************************************************************************
    File                 : ProjectDock.cpp
    Project              : LabPlot
    Description          : widget for project properties
    --------------------------------------------------------------------
    Copyright            : (C) 2012-2013 by Stefan Gerlach (stefan.gerlach@uni-konstanz.de)
	Copyright            : (C) 2013 Alexander Semke (alexander.semke@web.de)

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

#include "ProjectDock.h"
#include "backend/core/Project.h"
#include "kdefrontend/TemplateHandler.h"
#include <KConfigGroup>

/*!
  \class ProjectDock
  \brief Provides a widget for editing the properties of a project

  \ingroup kdefrontend
*/

ProjectDock::ProjectDock(QWidget *parent): QWidget(parent),	m_project(0), m_initializing(false) {
	ui.setupUi(this);

	// SLOTS
	connect(ui.leName, SIGNAL(textChanged(QString)), this, SLOT(titleChanged(QString)) );
	connect(ui.leAuthor, SIGNAL(textChanged(QString)), this, SLOT(authorChanged(QString)) );
	connect(ui.tbComment, SIGNAL(textChanged()), this, SLOT(commentChanged()) );

	TemplateHandler* templateHandler = new TemplateHandler(this, TemplateHandler::Worksheet);
	ui.verticalLayout->addWidget(templateHandler, 0, 0);
	templateHandler->show();
	connect( templateHandler, SIGNAL(loadConfigRequested(KConfig&)), this, SLOT(loadConfig(KConfig&)));
	connect( templateHandler, SIGNAL(saveConfigRequested(KConfig&)), this, SLOT(saveConfig(KConfig&)));

	this->retranslateUi();
}

void ProjectDock::setProject(Project *project) {
	m_initializing=true;
	m_project = project;
	ui.leFileName->setText(project->fileName());
	ui.lVersion->setText(project->version());
	ui.lCreated->setText(project->creationTime().toString());
	ui.lModified->setText(project->modificationTime().toString());

	//show default properties of the project
	KConfig config("", KConfig::SimpleConfig);
	loadConfig(config);

	connect(m_project, SIGNAL(aspectDescriptionChanged(const AbstractAspect*)),
			this, SLOT(projectDescriptionChanged(const AbstractAspect*)));

	m_initializing = false;
}

//************************************************************
//****************** SLOTS ********************************
//************************************************************
void ProjectDock::retranslateUi(){
}

void ProjectDock::titleChanged(const QString& title){
	if (m_initializing)
		return;

	m_project->setName(title);
}

void ProjectDock::authorChanged(const QString& author){
	if (m_initializing)
		return;

	m_project->setAuthor(author);
}

void ProjectDock::commentChanged(){
	if (m_initializing)
		return;

	m_project->setComment(ui.tbComment->toPlainText());
}

//*************************************************************
//******** SLOTs for changes triggered in Project   ***********
//*************************************************************
void ProjectDock::projectDescriptionChanged(const AbstractAspect* aspect) {
	if (m_project != aspect)
		return;

	m_initializing = true;
	if (aspect->name() != ui.leName->text()) {
			ui.leName->setText(aspect->name());
	} else if (aspect->comment() != ui.tbComment->toPlainText()) {
			ui.tbComment->setText(aspect->comment());
	}
	m_initializing = false;
}

//*************************************************************
//************************* Settings **************************
//*************************************************************
void ProjectDock::loadConfig(KConfig& config){
	KConfigGroup group = config.group( "Project" );

	ui.leName->setText( group.readEntry("Name", m_project->name()) );
	ui.leAuthor->setText( group.readEntry("Author", m_project->author()) );
	ui.tbComment->setText( group.readEntry("Comment", m_project->comment()) );
}

void ProjectDock::saveConfig(KConfig& config){
	KConfigGroup group = config.group( "Project" );

	group.writeEntry("Name", ui.leName->text());
	group.writeEntry("Author", ui.leAuthor->text());
	group.writeEntry("Comment", ui.tbComment->toPlainText());
}
