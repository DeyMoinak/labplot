/***************************************************************************
File                 : FITSHeaderEditWidget.cpp
Project              : LabPlot
Description          : Widget for listing/editing FITS header keywords
--------------------------------------------------------------------
Copyright            : (C) 2016 by Fabian Kristof (fkristofszabolcs@gmail.com)
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

#include "FITSHeaderEditWidget.h"
#include "backend/datasources/filters/FITSFilter.h"
#include "backend/matrix/Matrix.h"
#include "backend/spreadsheet/Spreadsheet.h"
#include "FITSHeaderEditNewKeywordDialog.h"
#include "FITSHeaderEditAddUnitDialog.h"
#include <QMenu>
#include <QTableWidget>
#include <QFileDialog>
#include <QContextMenuEvent>
#include <QDebug>

/*! \class FITSHeaderEditWidget
 * \brief Widget for listing/editing FITS header keywords
 * \since 2.2.0
 * \ingroup kdefrontend/widgets
 */
FITSHeaderEditWidget::FITSHeaderEditWidget(QWidget *parent) :
    QWidget(parent), m_initializingTable(false) {
    ui.setupUi(this);
    initActions();
    connectActions();
    initContextMenus();
    m_fitsFilter = new FITSFilter();
    ui.twKeywordsTable->setColumnCount(3);
    ui.twExtensions->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.twExtensions->headerItem()->setText(0, i18n("Extensions"));
    ui.twKeywordsTable->setHorizontalHeaderItem(0, new QTableWidgetItem(i18n("Key")));
    ui.twKeywordsTable->setHorizontalHeaderItem(1, new QTableWidgetItem(i18n("Value")));
    ui.twKeywordsTable->setHorizontalHeaderItem(2, new QTableWidgetItem(i18n("Comment")));
    ui.twKeywordsTable->installEventFilter(this);
    ui.twExtensions->installEventFilter(this);

    connect(ui.pbOpenFile, SIGNAL(clicked()), this, SLOT(openFile()));
    connect(ui.twKeywordsTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(updateKeyword(QTableWidgetItem*)));
    connect(ui.twExtensions, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(fillTable(QTreeWidgetItem*, int)));
}

/*!
 * \brief Destructor
 */
FITSHeaderEditWidget::~FITSHeaderEditWidget() {
    delete m_fitsFilter;
}

/*!
 * \brief Fills the keywords tablewidget.
 * If the selected extension was not yet selected before, then the keywords are read from the file
 * and then the table is filled, otherwise the table is filled using the already existing keywords.
 */
void FITSHeaderEditWidget::fillTable() {
    m_initializingTable = true;
    if (!m_extensionDatas.contains(m_seletedExtension)) {
        m_extensionDatas[m_seletedExtension].keywords = m_fitsFilter->chduKeywords(m_seletedExtension);
        m_extensionDatas[m_seletedExtension].updates.updatedKeywords.reserve(m_extensionDatas[m_seletedExtension].keywords.size());
        m_extensionDatas[m_seletedExtension].updates.updatedKeywords.resize(m_extensionDatas[m_seletedExtension].keywords.size());

        m_fitsFilter->parseHeader(m_seletedExtension, ui.twKeywordsTable);
    } else {
        QList<FITSFilter::Keyword> keywords = m_extensionDatas[m_seletedExtension].keywords;
        for (int i = 0; i < m_extensionDatas[m_seletedExtension].updates.updatedKeywords.size(); ++i) {
            FITSFilter::Keyword keyword = m_extensionDatas[m_seletedExtension].updates.updatedKeywords.at(i);
            if (!keyword.key.isEmpty()) {
                keywords.operator [](i).key = keyword.key;
            }
            if (!keyword.value.isEmpty()) {
                keywords.operator [](i).value = keyword.value;
            }
            if (!keyword.comment.isEmpty()) {
                keywords.operator [](i).comment = keyword.comment;
            }
        }
        foreach (const FITSFilter::Keyword& key, m_extensionDatas[m_seletedExtension].updates.newKeywords) {
            keywords.append(key);
        }
        m_fitsFilter->parseHeader(QString(), ui.twKeywordsTable, false, keywords);
    }
    m_initializingTable = false;
}

/*!
 * \brief Fills the tablewidget with the keywords of extension \a item
 * \param item the extension selected
 * \param col the column of the selected item
 */
void FITSHeaderEditWidget::fillTable(QTreeWidgetItem *item, int col) {
    WAIT_CURSOR;
    const QString& itemText = item->text(col);
    QString selectedExtension;
    int extType = 0;
    if (itemText.contains(QLatin1String("IMAGE #")) ||
            itemText.contains(QLatin1String("ASCII_TBL #")) ||
            itemText.contains(QLatin1String("BINARY_TBL #"))){
        extType = 1;
    } else if (!itemText.compare(QLatin1String("Primary header"))) {
        extType = 2;
    }
    if (extType == 0) {
        if (item->parent() != 0) {
            if (item->parent()->parent() != 0)
                selectedExtension = item->parent()->parent()->text(0) +"["+ item->text(col)+"]";
        }
    } else if (extType == 1) {
        if (item->parent() != 0) {
            if (item->parent()->parent() != 0) {
                bool ok;
                int hduNum = itemText.right(1).toInt(&ok);
                selectedExtension = item->parent()->parent()->text(0) +"["+ QString::number(hduNum-1) +"]";
            }
        }
    } else {
        if (item->parent()->parent() != 0) {
            selectedExtension = item->parent()->parent()->text(col);
        }
    }

    if (!selectedExtension.isEmpty()) {
        if (!(m_seletedExtension == selectedExtension)) {
            m_seletedExtension = selectedExtension;
            fillTable();
        }
    }
    RESET_CURSOR;
}

/*!
 * \brief Shows a dialog for opening a FITS file
 * If the returned file name is not empty (so a FITS file was selected) and it's not opened yet
 * then the file is parsed, so the treeview for the extensions is built and the table is filled.
 */
void FITSHeaderEditWidget::openFile() {

    KConfigGroup conf(KSharedConfig::openConfig(), "FITSHeaderEditWidget");
    QString dir = conf.readEntry("LastDir", "");
    QString fileName = QFileDialog::getOpenFileName(this, i18n("Open FITS file"), dir,
                                                    i18n("FITS files (*.fits)"));
    if (fileName.isEmpty())
        return;

    int pos = fileName.lastIndexOf(QDir::separator());
    if (pos!=-1) {
        QString newDir = fileName.left(pos);
        if (newDir!=dir)
            conf.writeEntry("LastDir", newDir);
    }

    WAIT_CURSOR;
    QTreeWidgetItem* root = ui.twExtensions->invisibleRootItem();
    const int childCount = root->childCount();
    bool opened = false;
    for (int i = 0; i < childCount; ++i) {
        if(root->child(i)->text(0) == fileName) {
            opened = true;
            break;
        }
    }
    if (!opened) {
        foreach (QTreeWidgetItem* item, ui.twExtensions->selectedItems()) {
            item->setSelected(false);
        }
        m_fitsFilter->parseExtensions(fileName, ui.twExtensions);
        ui.twExtensions->resizeColumnToContents(0);
        if (ui.twExtensions->selectedItems().size() > 0) {
            fillTable(ui.twExtensions->selectedItems().at(0), 0);
        }
    } else {
        KMessageBox::information(this, i18n("Cannot open file, file already opened!"),
                                 i18n("File already opened!"));
    }
    RESET_CURSOR;
}

/*!
 * \brief Triggered when clicking the Save button
 * Saves the modifications (new keywords, new keyword units, keyword modifications,
 * deleted keywords, deleted extensions) to the FITS files.
 * \return \c true if there was something saved, otherwise false
 */
bool FITSHeaderEditWidget::save() {
    bool saved = false;
    foreach (const QString& fileName, m_extensionDatas.keys()) {
        if (m_extensionDatas[fileName].updates.newKeywords.size() > 0) {
            m_fitsFilter->addNewKeyword(fileName,m_extensionDatas[fileName].updates.newKeywords);
            if (!saved) {
                saved = true;
            }
        }
        if (m_extensionDatas[fileName].updates.removedKeywords.size() > 0) {
            m_fitsFilter->deleteKeyword(fileName, m_extensionDatas[fileName].updates.removedKeywords);
            if (!saved) {
                saved = true;
            }
        }
        if (!saved) {
            foreach (const FITSFilter::Keyword& key, m_extensionDatas[fileName].updates.updatedKeywords) {
                if (!key.isEmpty()) {
                    saved = true;
                    break;
                }
            }
        }

        m_fitsFilter->updateKeywords(fileName, m_extensionDatas[fileName].keywords, m_extensionDatas[fileName].updates.updatedKeywords);
        m_fitsFilter->addKeywordUnit(fileName, m_extensionDatas[fileName].keywords);
        m_fitsFilter->addKeywordUnit(fileName, m_extensionDatas[fileName].updates.newKeywords);
    }

    if (m_removedExtensions.size() > 0) {
        m_fitsFilter->removeExtensions(m_removedExtensions);
        if (!saved) {
            saved = true;
        }
    }
    return saved;
}

/*!
 * \brief Initializes the context menu's actions.
 */
void FITSHeaderEditWidget::initActions() {
    action_add_keyword = new QAction(i18n("Add new keyword"), this);
    action_remove_keyword = new QAction(i18n("Remove keyword"), this);
    action_remove_extension = new QAction(i18n("Delete"), this);
    action_addmodify_unit = new QAction(i18n("Add unit"), this);
}

/*!
 * \brief Connects signals of the actions to the appropriate slots.
 */
void FITSHeaderEditWidget::connectActions() {
    connect(action_add_keyword, SIGNAL(triggered()), this, SLOT(addKeyword()));
    connect(action_remove_keyword, SIGNAL(triggered()), this, SLOT(removeKeyword()));
    connect(action_remove_extension, SIGNAL(triggered()), this, SLOT(removeExtension()));
    connect(action_addmodify_unit, SIGNAL(triggered()), this, SLOT(addModifyKeywordUnit()));
}

/*!
 * \brief Initializes the context menus.
 */
void FITSHeaderEditWidget::initContextMenus() {
    m_KeywordActionsMenu = new QMenu(this);
    m_KeywordActionsMenu->addAction(action_add_keyword);
    m_KeywordActionsMenu->addAction(action_remove_keyword);
    m_KeywordActionsMenu->addAction(action_addmodify_unit);

    m_ExtensionActionsMenu = new QMenu(this);
    m_ExtensionActionsMenu->addAction(action_remove_extension);
}

/*!
 * \brief Shows a FITSHeaderEditNewKeywordDialog and decides whether the new keyword provided in the dialog
 * can be added to the new keywords or not. Updates the tablewidget if it's needed.
 */
void FITSHeaderEditWidget::addKeyword() {
    FITSHeaderEditNewKeywordDialog* newKeywordDialog = new FITSHeaderEditNewKeywordDialog;
    m_initializingTable = true;
    if (newKeywordDialog->exec() == KDialog::Accepted) {
        FITSFilter::Keyword newKeyWord = newKeywordDialog->newKeyword();
        QList<FITSFilter::Keyword> currentKeywords = m_extensionDatas[m_seletedExtension].keywords;

        foreach (const FITSFilter::Keyword& keyword, currentKeywords) {
            if (keyword.operator==(newKeyWord)) {
                KMessageBox::information(this, i18n("Cannot add keyword, keyword already added"), i18n("Cannot add keyword"));
                return;
            }
        }

        foreach (const FITSFilter::Keyword& keyword, m_extensionDatas[m_seletedExtension].updates.newKeywords) {
            if (keyword.operator==(newKeyWord)) {
                KMessageBox::information(this, i18n("Cannot add keyword, keyword already added"), i18n("Cannot add keyword"));
                return;
            }
        }

        foreach (const QString& keyword, mandatoryKeywords()) {
            if (!keyword.compare(newKeyWord.key)) {
                KMessageBox::information(this, i18n("Cannot add mandatory keyword, they are already present"),
                                         i18n("Cannot add keyword"));
                return;
            }
        }

        /*
     - Column related keyword (TFIELDS, TTYPEn,TFORMn, etc.) in an image
     - SIMPLE, EXTEND, or BLOCKED keyword in any extension
     - BSCALE, BZERO, BUNIT, BLANK, DATAMAX, DATAMIN keywords in a table
     - Keyword name contains illegal character
     */

        m_extensionDatas[m_seletedExtension].updates.newKeywords.append(newKeyWord);

        const int lastRow = ui.twKeywordsTable->rowCount();
        ui.twKeywordsTable->setRowCount(lastRow + 1);
        QTableWidgetItem* newKeyWordItem = new QTableWidgetItem(newKeyWord.key);
        newKeyWordItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui.twKeywordsTable->setItem(lastRow, 0, newKeyWordItem);

        newKeyWordItem = new QTableWidgetItem(newKeyWord.value);
        newKeyWordItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui.twKeywordsTable->setItem(lastRow, 1, newKeyWordItem);

        newKeyWordItem = new QTableWidgetItem(newKeyWord.comment);
        newKeyWordItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui.twKeywordsTable->setItem(lastRow, 2, newKeyWordItem);
    }
    m_initializingTable = false;
    delete newKeywordDialog;
}

/*!
 * \brief Shows a messagebox whether we want to remove the keyword or not.
 * Mandatory keywords cannot be deleted.
 */
void FITSHeaderEditWidget::removeKeyword() {
    const int removeKeyWordMb = KMessageBox::questionYesNo(this,"Are you sure you want to delete this keyword?",
                                                     "Confirm deletion");
    if (removeKeyWordMb == KMessageBox::Yes) {
        const int row = ui.twKeywordsTable->currentRow();
        QString key = ui.twKeywordsTable->item(row, 0)->text();

        bool remove = true;
        foreach (const QString& k, mandatoryKeywords()) {
            if (!k.compare(key)) {
                remove = false;
                break;
            }
        }

        if (remove) {
            FITSFilter::Keyword toRemove = FITSFilter::Keyword(key,
                                                               ui.twKeywordsTable->item(row, 1)->text(),
                                                               ui.twKeywordsTable->item(row, 2)->text());
            ui.twKeywordsTable->removeRow(row);

            m_extensionDatas[m_seletedExtension].keywords.removeAt(row);
            m_extensionDatas[m_seletedExtension].updates.removedKeywords.append(toRemove);
        } else {
            KMessageBox::information(this, i18n("Cannot remove mandatory keyword!"), i18n("Removing keyword"));
        }
    }
}

/*!
 * \brief Trigggered when an item was updated by the user in the tablewidget
 * \param item the item which was updated
 */
void FITSHeaderEditWidget::updateKeyword(QTableWidgetItem *item) {
    if (!m_initializingTable) {
        const int row = item->row();
        int idx;
        bool fromNewKeyword = false;
        if (row > m_extensionDatas[m_seletedExtension].keywords.size()-1) {
            idx = row - m_extensionDatas[m_seletedExtension].keywords.size();
            fromNewKeyword = true;
        } else {
            idx = row;
        }

        if (item->column() == 0) {
            if (!fromNewKeyword) {
                m_extensionDatas[m_seletedExtension].updates.updatedKeywords.operator [](idx).key = item->text();
                m_extensionDatas[m_seletedExtension].keywords.operator [](idx).updates.keyUpdated = true;
            } else {
                m_extensionDatas[m_seletedExtension].updates.newKeywords.operator [](idx).key = item->text();
                m_extensionDatas[m_seletedExtension].updates.newKeywords.operator [](idx).updates.keyUpdated = true;
            }

        } else if (item->column() == 1) {
            if (!fromNewKeyword) {
                m_extensionDatas[m_seletedExtension].updates.updatedKeywords.operator [](idx).value = item->text();
                m_extensionDatas[m_seletedExtension].keywords.operator [](idx).updates.valueUpdated = true;
            } else {
                m_extensionDatas[m_seletedExtension].updates.newKeywords.operator [](idx).value = item->text();
                m_extensionDatas[m_seletedExtension].updates.newKeywords.operator [](idx).updates.valueUpdated = true;
            }
        } else {
            if (!fromNewKeyword) {
                m_extensionDatas[m_seletedExtension].updates.updatedKeywords.operator [](idx).comment = item->text();
                m_extensionDatas[m_seletedExtension].keywords.operator [](idx).updates.commentUpdated = true;
            } else {
                m_extensionDatas[m_seletedExtension].updates.newKeywords.operator [](idx).comment = item->text();
                m_extensionDatas[m_seletedExtension].updates.newKeywords.operator [](idx).updates.commentUpdated = true;
            }
        }
    }
}

/*!
 * \brief Shows a FITSHeaderEditAddUnitDialog on the selected keyword (provides the keyword's unit to the
 * dialog if it had one) and if the dialog was accepted then the new keyword unit is set and the tablewidget
 * is updated (filled with the modifications).
 */
void FITSHeaderEditWidget::addModifyKeywordUnit() {
    FITSHeaderEditAddUnitDialog* addUnitDialog;

    const int selectedRow = ui.twKeywordsTable->currentRow();
    int idx;
    bool fromNewKeyword = false;
    if (selectedRow > m_extensionDatas[m_seletedExtension].keywords.size()-1) {
        idx = selectedRow - m_extensionDatas[m_seletedExtension].keywords.size();
        fromNewKeyword = true;
    } else {
        idx = selectedRow;
    }
    QString unit;
    if (fromNewKeyword) {
        if (!m_extensionDatas[m_seletedExtension].updates.newKeywords.at(idx).unit.isEmpty()) {
            unit = m_extensionDatas[m_seletedExtension].updates.newKeywords.at(idx).unit;
        }
    } else {
        if (!m_extensionDatas[m_seletedExtension].keywords.at(idx).unit.isEmpty()) {
            unit = m_extensionDatas[m_seletedExtension].keywords.at(idx).unit;
        }
    }

    if (!unit.isNull()) {
        addUnitDialog = new FITSHeaderEditAddUnitDialog(unit);
    } else {
        addUnitDialog = new FITSHeaderEditAddUnitDialog;
    }

    if (addUnitDialog->exec() == KDialog::Accepted) {
        if (fromNewKeyword) {
            m_extensionDatas[m_seletedExtension].updates.newKeywords.operator [](idx).unit = addUnitDialog->unit();
            if (!m_extensionDatas[m_seletedExtension].updates.newKeywords.at(idx).unit.isEmpty()) {
                m_extensionDatas[m_seletedExtension].updates.newKeywords.operator [](idx).updates.unitUpdated = true;;
            }
        } else {
            m_extensionDatas[m_seletedExtension].keywords.operator [](idx).unit = addUnitDialog->unit();
            if (!m_extensionDatas[m_seletedExtension].keywords.at(idx).unit.isEmpty()) {
                m_extensionDatas[m_seletedExtension].keywords.operator [](idx).updates.unitUpdated = true;
            }
        }

        fillTable();
    }
    delete addUnitDialog;
}

/*!
 * \brief Removes the selected extension from the extensions treeview
 * If the last extension is removed from the tree, then the extension and the file will be removed too.
 */
void FITSHeaderEditWidget::removeExtension() {
    QTreeWidgetItem* current = ui.twExtensions->currentItem();
    QTreeWidgetItem* newCurrent = ui.twExtensions->itemBelow(current);
    if (current->parent()) {
        if (current->parent()->childCount() < 2) {
            delete current->parent();
        } else {
            delete current;
        }
    }
    const int selectedidx = m_extensionDatas.keys().indexOf(m_seletedExtension);
    if (selectedidx > 0) {
        const QString& ext = m_seletedExtension;
        m_extensionDatas.remove(ext);
        m_removedExtensions.append(ext);
        m_seletedExtension = m_extensionDatas.keys().at(selectedidx-1);

        fillTable();
    }
    ui.twExtensions->setCurrentItem(newCurrent);
}

/*!
 * \brief Returns a list of mandatory keywords according to the currently selected extension.
 * If the currently selected extension is an image then it returns the mandatory keywords of an image,
 * otherwise the mandatory keywords of a table
 * \return a list of mandatory keywords
 */
QList<QString> FITSHeaderEditWidget::mandatoryKeywords() const {
    QList<QString> mandatoryKeywords;
    const QTreeWidgetItem* currentItem = ui.twExtensions->currentItem();
    if (currentItem->parent()->text(0).compare(QLatin1String("Images"))) {
        mandatoryKeywords = FITSFilter::mandatoryImageExtensionKeywords();
    } else {
        mandatoryKeywords = FITSFilter::mandatoryTableExtensionKeywords();
    }
    return mandatoryKeywords;
}

/*!
 * \brief Manipulates the contextmenu event of the widget
 * \param watched the object on which the event occoured
 * \param event the event watched
 * \return
 */
bool FITSHeaderEditWidget::eventFilter(QObject * watched, QEvent * event) {
    if (event->type() == QEvent::ContextMenu) {
        QContextMenuEvent *cm_event = static_cast<QContextMenuEvent*>(event);
        const QPoint& global_pos = cm_event->globalPos();
        if (watched == ui.twKeywordsTable) {
            if (ui.twExtensions->selectedItems().size() != 0) {
                m_KeywordActionsMenu->exec(global_pos);
            }
        } else if (watched == ui.twExtensions) {
            if (ui.twExtensions->selectedItems().size() != 0) {
                QTreeWidgetItem* current = ui.twExtensions->currentItem();
                int col = ui.twExtensions->currentColumn();
                if (current->parent()) {
                    if ((current->text(col) != QLatin1String("Images")) &&
                            (current->text(col) != QLatin1String("Tables"))) {
                        m_ExtensionActionsMenu->exec(global_pos);
                    }
                }
            }
        }
        else
            return QWidget::eventFilter(watched, event);
        return true;
    } else
        return QWidget::eventFilter(watched, event);
}
