/***************************************************************************
    File                 : CantorWorksheetView.cpp
    Project              : LabPlot
    Description          : View class for CantorWorksheet
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
#include "backend/cantorWorksheet/CantorWorksheet.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QToolBar>

#include <KToggleAction>
#include <KLocalizedString>
#include <KParts/ReadWritePart>

CantorWorksheetView::CantorWorksheetView(CantorWorksheet* worksheet) : QWidget(),
	m_worksheet(worksheet) {

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	part = worksheet->part();
	if (part) {
		layout->addWidget(part->widget());
		initActions();
		initMenus();
		connect(m_worksheet, SIGNAL(requestProjectContextMenu(QMenu*)), this, SLOT(createContextMenu(QMenu*)));
		connect(m_worksheet, SIGNAL(statusChanged(Cantor::Session::Status)), this, SLOT(statusChanged(Cantor::Session::Status)));
	} else {
		QLabel* label = new QLabel(i18n("Failed to initialize %1").arg(m_worksheet->backendName()));
		label->setAlignment(Qt::AlignHCenter);
		layout->addWidget(label);
	}
}

void CantorWorksheetView::initActions() {
	QActionGroup* cantorActionGroup = new QActionGroup(this);
	cantorActionGroup->setExclusive(false);

	m_restartBackendAction = new QAction(QIcon::fromTheme("system-reboot"), i18n("Restart Backend"), cantorActionGroup);
	m_restartBackendAction->setData("restart_backend");

	m_evaluateWorsheetAction = new QAction(QIcon::fromTheme("system-run"), i18n("Evaluate Worksheet"), cantorActionGroup);
	m_evaluateWorsheetAction->setData("evaluate_worksheet");

	m_evaluateEntryAction = new QAction(i18n("Evaluate Entry"), cantorActionGroup);
	m_evaluateEntryAction->setShortcut(Qt::SHIFT + Qt::Key_Return);
	m_evaluateEntryAction->setData("evaluate_current");

	m_insertCommandEntryAction = new QAction(i18n("Insert Command Entry"), cantorActionGroup);
	m_insertCommandEntryAction->setData("insert_command_entry");
	m_insertCommandEntryAction->setShortcut(Qt::CTRL + Qt::Key_Return);

	m_insertTextEntryAction = new QAction(i18n("Insert Text Entry"), cantorActionGroup);
	m_insertTextEntryAction->setData("insert_text_entry");

	m_insertLatexEntryAction = new QAction(i18n("Insert Latex Entry"), cantorActionGroup);
	m_insertLatexEntryAction->setData("insert_latex_entry");

	m_insertPageBreakAction = new QAction(i18n("Insert Page Break"), cantorActionGroup);
	m_insertPageBreakAction->setData("insert_page_break_entry");

	m_removeCurrentEntryAction = new QAction(i18n("Remove Current Entry"), cantorActionGroup);
	m_removeCurrentEntryAction->setData("remove_current");

	m_computeEigenvectorsAction = new QAction(i18n("Compute Eigenvectors"), cantorActionGroup);
	m_computeEigenvectorsAction->setData("eigenvectors_assistant");

	m_createMattrixAction = new QAction(i18n("Create Matrix"), cantorActionGroup);
	m_createMattrixAction->setData("creatematrix_assistant");

	m_computeEigenvaluesAction = new QAction(i18n("Compute Eigenvalues"), cantorActionGroup);
	m_computeEigenvaluesAction->setData("eigenvalues_assistant");

	m_invertMattrixAction = new QAction(i18n("Invert Matrix"), cantorActionGroup);
	m_invertMattrixAction->setData("invertmatrix_assistant");

	m_differentiationAction = new QAction(i18n("Differentiation"), cantorActionGroup);
	m_differentiationAction->setData("differentiate_assistant");

	m_integrationAction = new QAction(i18n("Integration"), cantorActionGroup);
	m_integrationAction->setData("integrate_assistant");

	m_solveEquationsAction = new QAction(i18n("Solve Equations"), cantorActionGroup);
	m_solveEquationsAction->setData("solve_assistant");

	m_zoomIn = new QAction(QIcon::fromTheme("zoom-in"), i18n("Zoom in"), cantorActionGroup);
	m_zoomIn->setData("view_zoom_in");
	m_zoomIn->setShortcut(Qt::CTRL+Qt::Key_Plus);

	m_zoomOut = new QAction(QIcon::fromTheme("zoom-out"), i18n("Zoom out"), cantorActionGroup);
	m_zoomOut->setData("view_zoom_out");
	m_zoomOut->setShortcut(Qt::CTRL+Qt::Key_Minus);

	m_find = new QAction(QIcon::fromTheme("edit-find"), i18n("Find"), cantorActionGroup);
	m_find->setData("edit_find");
	m_find->setShortcut(Qt::CTRL+Qt::Key_F);

	m_replace = new QAction(QIcon::fromTheme("edit-replace"), i18n("Replace"), cantorActionGroup);
	m_replace->setData("edit_replace");
	m_replace->setShortcut(Qt::CTRL+Qt::Key_R);

	m_lineNumbers = new KToggleAction(i18n("Line Numbers"), cantorActionGroup);
	m_lineNumbers->setChecked(false);
	m_lineNumbers->setData("enable_expression_numbers");

	m_animateWorksheet = new KToggleAction(i18n("Animate Worksheet"), cantorActionGroup);
	m_animateWorksheet->setChecked(true);
	m_animateWorksheet->setData("enable_animations");

	m_latexTypesetting = new KToggleAction(i18n("Latex Typesetting"), cantorActionGroup);
	m_latexTypesetting->setChecked(true);
	m_latexTypesetting->setData("enable_typesetting");

	m_showCompletion = new QAction(i18n("Syntax Completion"), cantorActionGroup);
	m_showCompletion->setShortcut(Qt::CTRL + Qt::Key_Space);
	m_showCompletion->setData("show_completion");

	connect(cantorActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(triggerCantorAction(QAction*)));
}

void CantorWorksheetView::initMenus() {
	m_worksheetMenu = new QMenu("Worksheet", part->widget());
	m_worksheetMenu->addAction(m_evaluateWorsheetAction);
	m_worksheetMenu->addAction(m_evaluateEntryAction);
	m_worksheetMenu->addAction(m_insertCommandEntryAction);
	m_worksheetMenu->addAction(m_insertTextEntryAction);
	m_worksheetMenu->addAction(m_insertLatexEntryAction);
	m_worksheetMenu->addAction(m_insertPageBreakAction);
	m_worksheetMenu->addAction(m_removeCurrentEntryAction);
	m_worksheetMenu->addAction(m_showCompletion);

	m_linearAlgebraMenu = new QMenu("Linear Algebra", part->widget());
	m_linearAlgebraMenu->addAction(m_invertMattrixAction);
	m_linearAlgebraMenu->addAction(m_createMattrixAction);
	m_linearAlgebraMenu->addAction(m_computeEigenvectorsAction);
	m_linearAlgebraMenu->addAction(m_computeEigenvaluesAction);

	m_calculateMenu = new QMenu("Calculate", part->widget());
	m_calculateMenu->addAction(m_solveEquationsAction);
	m_calculateMenu->addAction(m_integrationAction);
	m_calculateMenu->addAction(m_differentiationAction);

	m_settingsMenu = new QMenu("Settings", part->widget());
	m_settingsMenu->addAction(m_lineNumbers);
	m_settingsMenu->addAction(m_animateWorksheet);
	m_settingsMenu->addAction(m_latexTypesetting);
}

/*!
 * Populates the menu \c menu with the CantorWorksheet and CantorWorksheet view relevant actions.
 * The menu is used
 *   - as the context menu in CantorWorksheetView
 *   - as the "CantorWorksheet menu" in the main menu-bar (called form MainWin)
 *   - as a part of the CantorWorksheet context menu in project explorer
 */
void CantorWorksheetView::createContextMenu(QMenu* menu) const{
	Q_ASSERT(menu);
	if (!part)
		return;

	QAction* firstAction = 0;
	// if we're populating the context menu for the project explorer, then
	//there're already actions available there. Skip the first title-action
	//and insert the action at the beginning of the menu.
	if (menu->actions().size()>1)
		firstAction = menu->actions().at(1);

	menu->insertMenu(firstAction, m_worksheetMenu);
	menu->insertMenu(firstAction, m_linearAlgebraMenu);
	menu->insertMenu(firstAction, m_calculateMenu);
	menu->insertSeparator(firstAction);
	menu->insertAction(firstAction, m_zoomIn);
	menu->insertAction(firstAction, m_zoomOut);
	menu->insertSeparator(firstAction);
	menu->insertAction(firstAction, m_find);
	menu->insertAction(firstAction, m_replace);
	menu->insertSeparator(firstAction);
	menu->insertMenu(firstAction, m_settingsMenu);
	menu->insertSeparator(firstAction);
	menu->insertAction(firstAction, m_restartBackendAction);
	menu->insertSeparator(firstAction);
}

void CantorWorksheetView::fillToolBar(QToolBar* toolbar) {
	if (!part)
		return;
	toolbar->addAction(m_restartBackendAction);
	toolbar->addAction(m_evaluateWorsheetAction);
}

/*!
 * Slot for actions triggered
 */
void CantorWorksheetView::triggerCantorAction(QAction* action) {
	QString actionName = action->data().toString();
	if(!actionName.isEmpty()) part->action(actionName.toStdString().c_str())->trigger();
}

CantorWorksheetView::~CantorWorksheetView() {
	if (part)
		part->widget()->setParent(0);
}

void CantorWorksheetView::statusChanged(Cantor::Session::Status status) {
	if(status==Cantor::Session::Running) {
		m_evaluateWorsheetAction->setText(i18n("Interrupt"));
		m_evaluateWorsheetAction->setIcon(QIcon::fromTheme(QLatin1String("dialog-close")));
		emit m_worksheet->statusInfo(i18n("Calculating..."));
	} else {
		m_evaluateWorsheetAction->setText(i18n("Evaluate Worksheet"));
		m_evaluateWorsheetAction->setIcon(QIcon::fromTheme(QLatin1String("system-run")));
		emit m_worksheet->statusInfo(i18n("Ready"));
	}
}
