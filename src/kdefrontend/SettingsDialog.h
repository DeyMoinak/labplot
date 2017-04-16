/***************************************************************************
    File                 : SettingsDialog.h
    Project              : LabPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2008-2016 by Alexander Semke (alexander.semke@web.de)
    Description          : application settings dialog
                           
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
#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <KPageDialog>

class SettingsGeneralPage;
class SettingsWorksheetPage;

class SettingsDialog : public KPageDialog {
	Q_OBJECT

	public:
		explicit SettingsDialog(QWidget*);
		virtual ~SettingsDialog();

	private slots:
		void changed();

	protected slots:
		virtual void slotButtonClicked(int button);

	private:
		bool m_changed;
		SettingsGeneralPage* generalPage;
		SettingsWorksheetPage* worksheetPage;

		void applySettings();
		void restoreDefaults();

	signals:
		void settingsChanged();
};

#endif
