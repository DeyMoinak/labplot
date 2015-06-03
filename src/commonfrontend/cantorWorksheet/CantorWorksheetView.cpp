/***************************************************************************
    File                 : CantorWorksheetView.cpp
    Project              : LabPlot
    Description          : Aspect providing a Cantor Worksheets for Multiple backends
    --------------------------------------------------------------------
    Copyright            : (C) 2015 Garvit Khatri (garvitdelhi@gmail.com)

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

#include "CantorWorksheetView.h"

#include <QIcon>
#include <QAction>
#include <QDebug>
#include <QTableView>
#include <QHBoxLayout>

#include <KLocalizedString>
#include <KMessageBox>
#include <cantor/backend.h>

CantorWorksheetView::CantorWorksheetView(CantorWorksheet* worksheet) : QWidget(),
    m_worksheet(worksheet) {
	
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    KPluginFactory* factory = KPluginLoader(QLatin1String("libcantorpart")).factory();
    if (factory)
    {
        // now that the Part is loaded, we cast it to a Part to get
        // our hands on it
	char* backendName = "Maxima";
        KParts::ReadWritePart* part = factory->create<KParts::ReadWritePart>(worksheet, QVariantList()<<backendName);
// 
        if (part)
        {
//             connect(part, SIGNAL(setWindowCaption(const QString&)), this, SLOT(setTabCaption(const QString&)));
	    layout->addWidget(part->widget());
        }
        else
        {
            qDebug()<<"error creating part ";
        }

    }
    else
    {
        // if we couldn't find our Part, we exit since the Shell by
        // itself can't do anything useful
        KMessageBox::error(this, i18n("Could not find the Cantor Part."));
        qApp->quit();
        // we return here, cause qApp->quit() only means "exit the
        // next time we enter the event loop...
        return;
    }
}

void CantorWorksheetView::initActions() {
    m_restartBackendAction = new QAction(QIcon::fromTheme("system-reboot"), i18n("Restart Backend"), this);    
//     m_restartBackendMenu = new QMenu(i18n("Add new"));
//     m_restartBackendMenu->setIcon(QIcon::fromTheme("office-chart-line"));
//     m_restartBackendMenu->addAction(m_restartBackendAction);
//     connect(selectAllAction, SIGNAL(triggered()), SLOT(selectAllElements()));
}

void CantorWorksheetView::createContextMenu(QMenu* menu) const{
    Q_ASSERT(menu);
    
    #ifdef ACTIVATE_SCIDAVIS_SPECIFIC_CODE
	QAction* firstAction = menu->actions().first();
    #else
	QAction* firstAction = 0;
	// if we're populating the context menu for the project explorer, then
	//there're already actions available there. Skip the first title-action
	//and insert the action at the beginning of the menu.
	if (menu->actions().size()>1)
	    firstAction = menu->actions().at(1);
    #endif
    
    menu->insertAction(firstAction, m_restartBackendAction);
}

CantorWorksheetView::~CantorWorksheetView() {

}
