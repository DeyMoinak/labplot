/***************************************************************************
File                 : FITSHeaderEditAddUnitDialog.h
Project              : LabPlot
Description          : Widget for adding or modifying FITS header keyword units
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
#ifndef FITSHEADEREDITADDUNITDIALOG_H
#define FITSHEADEREDITADDUNITDIALOG_H

#include <KDialog>
#include "ui_fitsheadereditaddunitwidget.h"

class FITSHeaderEditAddUnitDialog : public KDialog {
    Q_OBJECT

public:
    explicit FITSHeaderEditAddUnitDialog(const QString& unit = QString(), QWidget *parent = 0);
    ~FITSHeaderEditAddUnitDialog();
    QString unit() const;
private:
    Ui::FITSHeaderEditAddUnitDialog  ui;
    QString m_unit;
private slots:
    void addUnit();
};

#endif // FITSHEADEREDITADDUNITDIALOG_H
