/***************************************************************************
File                 : FITSHeaderEditDialog.cpp
Project              : LabPlot
Description          : Dialog for listing/editing FITS header keywords
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
#ifndef FITSHEADEREDITDIALOG_H
#define FITSHEADEREDITDIALOG_H

#include "FITSHeaderEditWidget.h"
#include "backend/datasources/AbstractDataSource.h"
#include <QWidget>
#include <KDialog>

class FITSHeaderEditDialog : public KDialog {
    Q_OBJECT

public:
    explicit FITSHeaderEditDialog( QWidget *parent = 0);
    ~FITSHeaderEditDialog();
    bool saved() const;
private:
    FITSHeaderEditWidget* m_HeaderEditWidget;
    bool m_saved;
private slots:
    void save();
};

#endif // FITSHEADEREDITDIALOG_H
