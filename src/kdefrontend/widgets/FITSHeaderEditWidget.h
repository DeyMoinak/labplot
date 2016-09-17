/***************************************************************************
File                 : FITSHeaderEditWidget.h
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
#ifndef FITSHEADEREDITWIDGET_H
#define FITSHEADEREDITWIDGET_H

#include <QWidget>
#include <QAction>
#include <QVector>
#include <QMap>
#include <QPair>
#include "backend/datasources/AbstractDataSource.h"
#include "ui_fitsheadereditwidget.h"
#include "backend/datasources/filters/FITSFilter.h"

class FITSHeaderEditWidget : public QWidget {
    Q_OBJECT

public:
    explicit FITSHeaderEditWidget(QWidget *parent = 0);
    ~FITSHeaderEditWidget();

private:
    Ui::FITSHeaderEditWidget ui;
    QAction* action_remove_keyword;
    QAction* action_add_keyword;
    QAction* action_addmodify_unit;
    QMenu* m_KeywordActionsMenu;

    QMenu* m_ExtensionActionsMenu;
    QAction* action_remove_extension;

    struct HeaderUpdate {
        QList<FITSFilter::Keyword> newKeywords;
        QVector<FITSFilter::Keyword> updatedKeywords;
        QList<FITSFilter::Keyword> removedKeywords;
    };

    struct ExtensionData {
        HeaderUpdate updates;
        QList<FITSFilter::Keyword> keywords;
    };

    QMap<QString, ExtensionData> m_extensionDatas;
    QStringList m_removedExtensions;
    QString m_seletedExtension;

    FITSFilter* m_fitsFilter;

    bool m_initializingTable;

    void initActions();
    void initContextMenus();
    void connectActions();
    void fillTable();
    QList<QString> mandatoryKeywords() const;
    bool eventFilter(QObject*, QEvent*);
public slots:
    bool save();
private slots:
    void openFile();

    void fillTable(QTreeWidgetItem* item, int col);
    void updateKeyword(QTableWidgetItem*);

    void removeKeyword();
    void removeExtension();
    void addKeyword();
    void addModifyKeywordUnit();
};

#endif // FITSHEADEREDITWIDGET_H
