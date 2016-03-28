/***************************************************************************
    File                 : TreeViewComboBox.h
    Project              : LabPlot
    Description          : Provides a QTreeView in a QComboBox
    --------------------------------------------------------------------
    Copyright            : (C) 2008-2011 by Alexander Semke (alexander.semke@web.de)
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

#ifndef TREEVIEWCOMBOBOX_H
#define TREEVIEWCOMBOBOX_H

#include <QTreeView>
#include <QLineEdit>
#include <QComboBox>
#include <QEvent>

class AbstractAspect;

class TreeViewComboBox : public QComboBox{
Q_OBJECT

public:
    explicit TreeViewComboBox(QWidget* parent = 0);

 	void setModel(QAbstractItemModel *model);
	void setCurrentModelIndex(const QModelIndex&);
	void setTopLevelClasses(QList<const char*>);
	void setSelectableClasses(QList<const char*>);
	QModelIndex currentModelIndex() const;
	virtual void showPopup();
    virtual void hidePopup();

private:
    QLineEdit m_lineEdit;
	QTreeView m_treeView;
	QList<const char*> m_topLevelClasses;
	QList<const char*> m_selectableClasses;
    void showTopLevelOnly(const QModelIndex & index);
	bool eventFilter(QObject *obj, QEvent *event);

private slots:
	void treeViewIndexActivated(const QModelIndex&);
    void lineEditTextEdited(const QString &);

signals:
	void currentModelIndexChanged(const QModelIndex&);
};

#endif
