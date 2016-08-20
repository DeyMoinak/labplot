/***************************************************************************
    File                 : ThemeHandler.cpp
    Project              : LabPlot
    Description          : Widget for handling saving and loading of themes
    --------------------------------------------------------------------
    Copyright            : (C) 2016 Prakriti Bhardwaj (p_bhardwaj14@informatik.uni-kl.de)
    Copyright            : (C) 2016 Alexander Semke (alexander.semke@web.de)

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

#include "ThemeHandler.h"
#include "widgets/ThemesWidget.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileInfo>
#include <QWidgetAction>

#include <KLocale>
#include <KStandardDirs>
#include <KLineEdit>
#include <KMenu>
#include <KConfig>
#include <KConfigGroup>

#include <KMessageBox>
#include <knewstuff3/uploaddialog.h>
//#include <kapplication.h>
#include <kdebug.h>
//#include <klocale.h>
//#include <kcmdlineargs.h>
//#include <kaboutdata.h>

/*!
  \class ThemeHandler
  \brief Provides a widget with buttons for loading of themes.

  Emits \c loadConfig() signal that have to be connected
  to the appropriate slots in the backend (plot widgets)

  \ingroup kdefrontend
*/

ThemeHandler::ThemeHandler(QWidget* parent) : QWidget(parent) {
	QHBoxLayout* horizontalLayout = new QHBoxLayout(this);
	horizontalLayout->setSpacing(0);

	pbLoadTheme = new QPushButton(this);
	horizontalLayout->addWidget(pbLoadTheme);
	pbLoadTheme->setText("Apply theme");

	pbSaveTheme = new QPushButton(this);
	horizontalLayout->addWidget(pbSaveTheme);
	pbSaveTheme->setText("Save theme");

	pbPublishTheme = new QPushButton(this);
	horizontalLayout->addWidget(pbPublishTheme);
	pbPublishTheme->setText("Publish theme");

	QSpacerItem* horizontalSpacer2 = new QSpacerItem(10, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);
	horizontalLayout->addItem(horizontalSpacer2);

	connect( pbLoadTheme, SIGNAL(clicked()), this, SLOT(showPanel()));
	connect( pbSaveTheme, SIGNAL(clicked()), this, SLOT(saveMenu()));
	connect( pbPublishTheme, SIGNAL(clicked()), this, SLOT(publishThemes()));

	m_themeList = KGlobal::dirs()->findAllResources("data", "labplot2/themes/theme_files/*");
	m_themeList.append(KGlobal::dirs()->findAllResources("appdata", "themes/local/*"));
	pbLoadTheme->setEnabled(!m_themeList.empty());
}

void ThemeHandler::loadSelected(QString str) {
	QString themeName;
	for(int i = 0; i < m_themeList.size();i++) {
		if((m_themeList.at(i).indexOf(str)!=-1) || (m_themeList.at(i).indexOf(str + ".txt")!=-1))
			themeName = m_themeList.at(i);
	}
	KConfig config(themeName, KConfig::SimpleConfig);
	emit (loadThemeRequested(config));

	emit info( i18n("Theme \"%1\" was loaded.", str) );
}

QStringList ThemeHandler::themes() {
	QStringList pathList = KGlobal::dirs()->findAllResources("data", "labplot2/themes/theme_files/*");
	pathList.append(KGlobal::dirs()->findAllResources("appdata", "themes/local/*"));
	QStringList themeList;
	for(int i = 0; i < pathList.size(); ++i) {
		QFileInfo fileinfo(pathList.at(i));
		themeList.append(fileinfo.fileName().split('.').at(0));
	}
	return themeList;
}

const QString ThemeHandler::themeConfigPath(const QString& name) {
	QStringList themes = KGlobal::dirs()->findAllResources("data", "labplot2/themes/theme_files/*");
	themes.append(KGlobal::dirs()->findAllResources("appdata", "themes/local/*"));
	for (int i=0; i<themes.size(); ++i) {
		if ((themes.at(i).indexOf(name + ".txt")!=-1) || (themes.at(i).indexOf(name)!=-1))
			return themes.at(i);
	}

	return QString();
}

void ThemeHandler::showPanel() {
	QMenu menu;
	ThemesWidget themeWidget(&menu);
	connect(&themeWidget, SIGNAL(themeSelected(QString)), this, SLOT(loadSelected(QString)));
	connect(&themeWidget, SIGNAL(themeSelected(QString)), &menu, SLOT(close()));
	connect(&themeWidget, SIGNAL(canceled()), &menu, SLOT(close()));

	QWidgetAction* widgetAction = new QWidgetAction(this);
	widgetAction->setDefaultWidget(&themeWidget);
	menu.addAction(widgetAction);

	QPoint pos(-menu.sizeHint().width()+pbLoadTheme->width(),-menu.sizeHint().height());
	menu.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	menu.exec(pbLoadTheme->mapToGlobal(pos));
}

void ThemeHandler::saveMenu() {
	KMenu menu;
	menu.addTitle(i18n("Save as"));

	// add editable action
	QWidgetAction* widgetAction = new QWidgetAction(this);
	QFrame* frame = new QFrame(this);
	QHBoxLayout* layout = new QHBoxLayout(frame);

	QLabel* label = new QLabel(i18n("Enter name:"), frame);
	layout->addWidget(label);

	KLineEdit* leFilename = new KLineEdit("", frame);
	layout->addWidget(leFilename);
	connect(leFilename, SIGNAL(returnPressed(QString)), this, SLOT(saveNewSelected(QString)));
	connect(leFilename, SIGNAL(returnPressed(QString)), &menu, SLOT(close()));

	widgetAction->setDefaultWidget(frame);
	menu.addAction(widgetAction);

	QPoint pos(-menu.sizeHint().width()+pbSaveTheme->width(),-menu.sizeHint().height());
	menu.exec(pbSaveTheme->mapToGlobal(pos));
	leFilename->setFocus();
}

void ThemeHandler::saveNewSelected(const QString& filename) {
	KConfig config(KGlobal::dirs()->locateLocal("appdata", "themes/local") + '/' + filename, KConfig::SimpleConfig);
	emit (saveThemeRequested(config));
	emit info( i18n("New theme \"%1\" was saved.", filename) );
}

void ThemeHandler::publishThemes() {
	QStringList localThemeFiles = KGlobal::dirs()->findAllResources("appdata", "themes/local/*");

	int ret = KMessageBox::questionYesNo(this,
					     i18n("Do you want to upload your themes to public web server?"),
					     i18n("Question - Labplot"));
	if (ret != KMessageBox::Yes) return;


	// creating upload dialog

	if(!localThemeFiles.empty()) {
		foreach(QString fileName, localThemeFiles) {
			KNS3::UploadDialog dialog("labplot2_themes.knsrc", this);
			qDebug()<<"uploading file "<<fileName;
			dialog.setUploadFile(fileName);
			dialog.exec();
		}
	}
	else {
		ret = KMessageBox::warningContinueCancel(this,
							 i18n("There are no locally saved themes to be uploaded. Please create new themes."),
							 i18n("Warning - Labplot"),  KStandardGuiItem::ok());
		if (ret != KMessageBox::Continue) return;
	}

}
