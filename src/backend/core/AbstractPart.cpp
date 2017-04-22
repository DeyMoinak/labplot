/***************************************************************************
    File                 : AbstractPart.cpp
    Project              : LabPlot
    Description          : Base class of Aspects with MDI windows as views.
    --------------------------------------------------------------------
    Copyright            : (C) 2008 Knut Franke (knut.franke@gmx.de)
	Copyright            : (C) 2012 Alexander Semke (alexander.semke@web.de)

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

#include "backend/core/AbstractPart.h"
#include "commonfrontend/core/PartMdiView.h"
#include "backend/core/Workbook.h"
#include "backend/datapicker/Datapicker.h"
#include "backend/spreadsheet/Spreadsheet.h"
#include "backend/datapicker/DatapickerCurve.h"
#include <QMenu>
#include <QStyle>

#include <KLocale>

/**
 * \class AbstractPart
 * \brief Base class of Aspects with MDI windows as views (AspectParts).
 */
AbstractPart::AbstractPart(const QString &name) : AbstractAspect(name),
	m_mdiWindow(0), m_view(0) {
}

AbstractPart::~AbstractPart() {
	if (m_view)
		delete m_view;

	if (m_mdiWindow)
		delete m_mdiWindow;
}

/**
 * \fn QWidget *AbstractPart::view() const
 * \brief Construct a primary view on me.
 *
 * The caller receives ownership of the view.
 *
 * This method may be called multiple times during the life time of a Part, or it might not get
 * called at all. Parts must not depend on the existence of a view for their operation.
 */

/**
 * \brief Wrap the view() into a PartMdiView.
 *
 * A new view is only created the first time this method is called;
 * after that, a pointer to the pre-existing view is returned.
 */
PartMdiView* AbstractPart::mdiSubWindow() const {
	if (!m_mdiWindow)
		m_mdiWindow = new PartMdiView(const_cast<AbstractPart*>(this));

	return m_mdiWindow;
}

bool AbstractPart::hasMdiSubWindow() const {
	return m_mdiWindow;
}

/*!
 * this function is called in MainWin when an aspect is removed from the project.
 * deletes the view and it's mdi-subwindow-wrapper
 */
void AbstractPart::deleteMdiSubWindow() {
	deleteView();
	delete m_mdiWindow;
	m_mdiWindow = 0;
}

/*!
 * this function is called when PartMdiView, the mdi-subwindow-wrapper of the actual view,
 * is closed (=deleted) in MainWindow. Makes sure that the view also gets deleted.
 */
void AbstractPart::deleteView() const {
    //if the parent is a Workbook or Datapicker, the actual view was already deleted when QTabWidget was deleted.
	//here just set the pointer to 0.
    if (dynamic_cast<const Workbook*>(parentAspect()) || dynamic_cast<const Datapicker*>(parentAspect())
            || dynamic_cast<const Datapicker*>(parentAspect()->parentAspect())) {
		m_view = 0;
		return;
	}

	if (m_view) {
		delete m_view;
		m_view = 0;
		m_mdiWindow = 0;
	}
}

/**
 * \brief Return AbstractAspect::createContextMenu() plus operations on the primary view.
 */
QMenu* AbstractPart::createContextMenu(){
	QMenu * menu = AbstractAspect::createContextMenu();
	Q_ASSERT(menu);
	menu->addSeparator();

	if (m_mdiWindow) {
		menu->addAction(QIcon::fromTheme("document-export-database"), i18n("Export"), this, SIGNAL(exportRequested()));
		menu->addAction(QIcon::fromTheme("document-print"), i18n("Print"), this, SIGNAL(printRequested()));
		menu->addAction(QIcon::fromTheme("document-print-preview"), i18n("Print Preview"), this, SIGNAL(printPreviewRequested()));
		menu->addSeparator();

		const QStyle *widget_style = m_mdiWindow->style();
		QAction *action_temp;
		if(m_mdiWindow->windowState() & (Qt::WindowMinimized | Qt::WindowMaximized))	{
			action_temp = menu->addAction(i18n("&Restore"), m_mdiWindow, SLOT(showNormal()));
			action_temp->setIcon(widget_style->standardIcon(QStyle::SP_TitleBarNormalButton));
		}

		if(!(m_mdiWindow->windowState() & Qt::WindowMinimized))	{
			action_temp = menu->addAction(i18n("Mi&nimize"), m_mdiWindow, SLOT(showMinimized()));
			action_temp->setIcon(widget_style->standardIcon(QStyle::SP_TitleBarMinButton));
		}

		if(!(m_mdiWindow->windowState() & Qt::WindowMaximized))	{
			action_temp = menu->addAction(i18n("Ma&ximize"), m_mdiWindow, SLOT(showMaximized()));
			action_temp->setIcon(widget_style->standardIcon(QStyle::SP_TitleBarMaxButton));
		}
	} else {
		//data spreadsheets in the datapicker curves cannot be hidden/minimized, don't show this menu entry
        if ( !(dynamic_cast<const Spreadsheet*>(this) && dynamic_cast<const DatapickerCurve*>(this->parentAspect())) )
			menu->addAction(i18n("Show"), this, SIGNAL(showRequested()));
	}

	return menu;
}
