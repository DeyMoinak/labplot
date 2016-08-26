/***************************************************************************
    File                 : TreeViewComboBox.cpp
    Project              : LabPlot
    Description          : Provides a QTreeView in a QComboBox
    --------------------------------------------------------------------
    Copyright            : (C) 2008-2016 by Alexander Semke (alexander.semke@web.de)
    Copyright            : (C) 2008 Tilman Benkert (thzs@gmx.net)

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

#include "commonfrontend/widgets/TreeViewComboBox.h"
#include "backend/core/AbstractAspect.h"
#include "backend/core/AspectTreeModel.h"

#include <QEvent>
#include <QHeaderView>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTreeView>

#include <KLocalizedString>
#include <KLineEdit>

/*!
    \class TreeViewComboBox
    \brief Provides a QTreeView in a QComboBox.

    \ingroup backend/widgets
*/
TreeViewComboBox::TreeViewComboBox(QWidget* parent) : QComboBox(parent) {

	QVBoxLayout* layout = new QVBoxLayout;
	m_treeView = new QTreeView;
	m_lineEdit = new KLineEdit;
	m_groupBox = new QGroupBox;
	
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	layout->addWidget(m_lineEdit);
	layout->addWidget(m_treeView);
	
	m_groupBox->setLayout(layout);
	m_groupBox->setParent(parent, Qt::Popup);
	m_groupBox->hide();
	m_groupBox->installEventFilter(this);
	
	m_treeView->header()->hide();
	m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_treeView->setUniformRowHeights(true);
	
	m_lineEdit->setPlaceholderText(i18n("Search/Filter text"));
	qobject_cast<KLineEdit*>(m_lineEdit)->setClearButtonShown(true);
	m_lineEdit->setFocus();

	addItem("");
	setCurrentIndex(0);

	connect(m_treeView, SIGNAL(activated(QModelIndex)), this, SLOT(treeViewIndexActivated(QModelIndex)) );
	connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
}

void TreeViewComboBox::setTopLevelClasses(QList<const char*> list) {
	m_topLevelClasses=list;
}

void TreeViewComboBox::setSelectableClasses(QList<const char*> list) {
	m_selectableClasses=list;
}

/*!
	Sets the \a model for the view to present.
*/
void TreeViewComboBox::setModel(QAbstractItemModel* model) {
	m_treeView->setModel(model);

	//show only the first column in the combo box
	for (int i=1; i<model->columnCount(); i++)
	  m_treeView->hideColumn(i);

	//Expand the complete tree in order to see everything in the first popup.
	m_treeView->expandAll();
}

/*!
	Sets the current item to be the item at \a index and selects it.
	\sa currentIndex()
*/
void TreeViewComboBox::setCurrentModelIndex(const QModelIndex& index) {
	m_treeView->setCurrentIndex(index);
	QComboBox::setItemText(0, index.data().toString());
}

/*!
	Returns the model index of the current item.
	\sa setCurrentModelIndex()
*/
QModelIndex TreeViewComboBox::currentModelIndex() const {
	return m_treeView->currentIndex();
}

/*!
	Displays the tree view of items in the combobox.
	Triggers showTopLevelOnly() to show toplevel items only.
*/
void TreeViewComboBox::showPopup() {
	if (!m_treeView->model() || !m_treeView->model()->hasChildren())
		return;

	QModelIndex root = m_treeView->model()->index(0,0);
	showTopLevelOnly(root);
	
	m_lineEdit->setText("");
	m_groupBox->show();
	m_groupBox->resize(this->width(), 250);
	m_groupBox->move(mapToGlobal( this->rect().topLeft() ));
}

void TreeViewComboBox::hidePopup() {
	m_groupBox->hide();
}

/*!
	Hides the non-toplevel items of the model used in the tree view.
*/
void TreeViewComboBox::showTopLevelOnly(const QModelIndex & index) {
	int rows = index.model()->rowCount(index);
	bool isTopLevel;
	for (int i=0; i<rows; i++) {
		QModelIndex child = index.child(i, 0);
		showTopLevelOnly(child);
		AbstractAspect* aspect =  static_cast<AbstractAspect*>(child.internalPointer());
		isTopLevel = false;
		foreach(const char* classString, m_topLevelClasses)
			if (aspect->inherits(classString)) {
				if ( strcmp(classString, "Spreadsheet")==0 ) {
					if (aspect->inherits("FileDataSource"))
						isTopLevel = false;
					else
						isTopLevel = true;
				} else {
					isTopLevel = true;
				}
			}

		m_treeView->setRowHidden(i, index, !isTopLevel);
	}
}

/*!
	catches the MouseButtonPress-event and hides the tree view on mouse clicking.
*/
bool TreeViewComboBox::eventFilter(QObject* object, QEvent* event) {
	if ( (object==m_groupBox) && event->type()==QEvent::MouseButtonPress ) {
		m_groupBox->hide();
		this->setFocus();
		return true;
	}
	return false;
}

//SLOTs

void TreeViewComboBox::treeViewIndexActivated(const QModelIndex& index) {
	if (index.internalPointer()) {
		AbstractAspect* aspect =  static_cast<AbstractAspect*>(index.internalPointer());
		const char* currentClassName = aspect->metaObject()->className();
		foreach(const char* className, m_selectableClasses) {
			if ( strcmp(currentClassName, className)==0 ) {
				QComboBox::setCurrentIndex(0);
				QComboBox::setItemText(0, index.data().toString());
				emit currentModelIndexChanged(index);
				m_groupBox->hide();
				return;
			}
		}
	}

	m_treeView->setCurrentIndex(QModelIndex());
	setCurrentIndex(0);
	QComboBox::setItemText(0, "");
	emit currentModelIndexChanged(QModelIndex());
	m_groupBox->hide();
}

void TreeViewComboBox::filterChanged(const QString& text) {
	QModelIndex root = m_treeView->model()->index(0,0);
	filter(root, text);
}

bool TreeViewComboBox::filter(const QModelIndex& index, const QString& text) {
	bool childVisible = false;
	const int rows = index.model()->rowCount(index);
	for (int i=0; i<rows; i++) {
		QModelIndex child = index.child(i, 0);
		AbstractAspect* aspect =  static_cast<AbstractAspect*>(child.internalPointer());
		bool topLevel = isTopLevel(aspect);
		if (!topLevel)
			continue;

		bool visible = aspect->name().contains(text, Qt::CaseInsensitive);

		if (visible) {
			//current item is visible -> make all its children (allowed top level types only) visible without applying the filter
			for (int j=0; j<child.model()->rowCount(child); ++j) {
				AbstractAspect* aspect =  static_cast<AbstractAspect*>(child.child(j,0).internalPointer());
				m_treeView->setRowHidden(j, child, !isTopLevel(aspect));
			}

			childVisible = true;
		} else {
			//check children items. if one of the children is visible, make the parent (current) item visible too.
			visible = filter(child, text);
			if (visible)
				childVisible = true;
		}

		m_treeView->setRowHidden(i, index, !visible);
	}

	return childVisible;
}

/*!
	checks whether \c aspect is one of the allowed top level types
*/
bool TreeViewComboBox::isTopLevel(const AbstractAspect* aspect) const {
	foreach(const char* classString, m_topLevelClasses) {
		if (aspect->inherits(classString)) {
			if ( strcmp(classString, "Spreadsheet")==0 ) {
				if (aspect->inherits("FileDataSource"))
					return false;
				else
					return true;
			} else {
				return true;
			}
		}
	}
	return false;
}
