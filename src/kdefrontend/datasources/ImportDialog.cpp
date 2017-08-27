/***************************************************************************
    File                 : ImportDialog.cc
    Project              : LabPlot
    Description          : import file data dialog
    --------------------------------------------------------------------
    Copyright            : (C) 2008-2017 Alexander Semke (alexander.semke@web.de)
    Copyright            : (C) 2008-2015 by Stefan Gerlach (stefan.gerlach@uni.kn)

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

#include "ImportDialog.h"
#include "backend/core/AspectTreeModel.h"
#include "backend/core/Project.h"
#include "backend/spreadsheet/Spreadsheet.h"
#include "backend/matrix/Matrix.h"
#include "backend/core/Workbook.h"
#include "backend/lib/macros.h"
#include "commonfrontend/widgets/TreeViewComboBox.h"
#include "kdefrontend/MainWin.h"

#include <QDir>
#include <QGroupBox>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QProgressBar>
#include <QStatusBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <KInputDialog>
#include <KLocale>
#include <KMessageBox>

/*!
	\class ImportDialog
	\brief Base class for other import dialogs. Provides the "Import to" section of those dialogs.

	\ingroup kdefrontend
 */

ImportDialog::ImportDialog(MainWin* parent) : KDialog(parent),
	cbAddTo(0), cbPosition(0), m_mainWin(parent), m_newDataContainerMenu(0),
	m_aspectTreeModel(new AspectTreeModel(parent->project()) ) {

	QWidget* mainWidget = new QWidget(this);
	vLayout = new QVBoxLayout(mainWidget);
	vLayout->setSpacing(0);
	vLayout->setContentsMargins(0,0,0,0);
	setMainWidget(mainWidget);

	//menu for new data container
	m_newDataContainerMenu = new QMenu(this);
	m_newDataContainerMenu->addAction( QIcon::fromTheme("labplot-workbook-new"), i18n("new Workbook") );
	m_newDataContainerMenu->addAction( QIcon::fromTheme("labplot-spreadsheet-new"), i18n("new Spreadsheet") );
	m_newDataContainerMenu->addAction( QIcon::fromTheme("labplot-matrix-new"), i18n("new Matrix") );
	connect(m_newDataContainerMenu, SIGNAL(triggered(QAction*)), this, SLOT(newDataContainer(QAction*)));

	//ok is only available if a valid container was selected
	enableButtonOk(false);
}

ImportDialog::~ImportDialog() {
	if (m_aspectTreeModel)
		delete m_aspectTreeModel;
}

/*!
	creates widgets for the frame "Import-To" and sets the current model in the "Add to"-combobox.
 */
void ImportDialog::setModel() {
	//Frame for the "Import To"-Stuff
	frameAddTo = new QGroupBox(this);
	frameAddTo->setTitle(i18n("Import To"));
	QGridLayout *grid = new QGridLayout(frameAddTo);
	grid->addWidget(new QLabel(i18n("Data container"), frameAddTo), 0, 0);

	cbAddTo = new TreeViewComboBox(frameAddTo);
	cbAddTo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	grid->addWidget(cbAddTo, 0, 1);

	QList<const char*> list;
	list << "Folder" << "Spreadsheet" << "Matrix"  << "Workbook";
	cbAddTo->setTopLevelClasses(list);

	list.clear();
	list << "Spreadsheet" << "Matrix" << "Workbook";
	m_aspectTreeModel->setSelectableAspects(list);

	cbAddTo->setModel(m_aspectTreeModel);

	tbNewDataContainer = new QToolButton(frameAddTo);
	tbNewDataContainer->setIcon(QIcon::fromTheme("list-add"));
	tbNewDataContainer->setToolTip(i18n("Add new data container"));
	grid->addWidget( tbNewDataContainer, 0, 2);

	lPosition = new QLabel(i18n("Position"), frameAddTo);
	lPosition->setEnabled(false);
	grid->addWidget(lPosition, 1, 0);

	cbPosition = new QComboBox(frameAddTo);
	cbPosition->setEnabled(false);
	cbPosition->addItem(i18n("Append"));
	cbPosition->addItem(i18n("Prepend"));
	cbPosition->addItem(i18n("Replace"));
	KConfigGroup conf(KSharedConfig::openConfig(), "ImportDialog");
	cbPosition->setCurrentIndex(conf.readEntry("Position", 0));

	cbPosition->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	grid->addWidget(cbPosition, 1, 1);

	vLayout->addWidget(frameAddTo);

	connect(tbNewDataContainer, SIGNAL(clicked(bool)), this, SLOT(newDataContainerMenu()));
    connect(cbAddTo, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(checkOkButton()));
}

void ImportDialog::setCurrentIndex(const QModelIndex& index) {
	DEBUG("ImportFileDialog::setCurrentIndex()");
	QDEBUG(" index =" << index);
	cbAddTo->setCurrentModelIndex(index);
	QDEBUG("cbAddTo->currentModelIndex() =" << cbAddTo->currentModelIndex());
	checkOkButton();
}

void ImportDialog::newDataContainer(QAction* action) {
	DEBUG("ImportDialog::newDataContainer()");
	QString name = selectedObject();
	QString type = action->iconText().split(' ')[1];
	if (name.isEmpty())
		name = action->iconText();

	bool ok;
	// child widgets can't have own icons
	QInputDialog* dlg = new QInputDialog(this);
	name = dlg->getText(this, i18n("Add %1", action->iconText()), i18n("%1 name:", type), QLineEdit::Normal, name, &ok);
	if (ok) {
		AbstractAspect* aspect;
		int actionIndex = m_newDataContainerMenu->actions().indexOf(action);
		if (actionIndex == 0)
			aspect = new Workbook(0, name);
		else if (actionIndex == 1)
			aspect = new Spreadsheet(0, name);
		else
			aspect = new Matrix(0, name);

		m_mainWin->addAspectToProject(aspect);
		QDEBUG("cbAddTo->setCurrentModelIndex() to " << m_mainWin->model()->modelIndexOfAspect(aspect));
		cbAddTo->setCurrentModelIndex(m_mainWin->model()->modelIndexOfAspect(aspect));
		checkOkButton();
	}

	delete dlg;
}

void ImportDialog::newDataContainerMenu() {
	m_newDataContainerMenu->exec( tbNewDataContainer->mapToGlobal(tbNewDataContainer->rect().bottomLeft()));
}
