/***************************************************************************
    File                 : ExportSpreadsheetDialog.cpp
    Project              : LabPlot
    Description          : export spreadsheet dialog
    --------------------------------------------------------------------
    Copyright            : (C) 2014-2016 by Alexander Semke (alexander.semke@web.de)

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

#include "ExportSpreadsheetDialog.h"

#include <QFileDialog>
#include <QStandardItemModel>
#include <KMessageBox>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QStandardItemModel>
#include <QLabel>
#include <QComboBox>

/*!
	\class ExportSpreadsheetDialog
	\brief Dialog for exporting a spreadsheet to a file.

	\ingroup kdefrontend
*/

ExportSpreadsheetDialog::ExportSpreadsheetDialog(QWidget* parent) : KDialog(parent),
		m_matrixMode(false), urlCompletion(new KUrlCompletion) {
	mainWidget = new QWidget(this);
	ui.setupUi(mainWidget);
	ui.gbOptions->hide();

	ui.kleFileName->setCompletionObject(urlCompletion);

	ui.cbFormat->addItem("ASCII");
	ui.cbFormat->addItem("Binary");
	ui.cbFormat->addItem("LaTeX");
	ui.cbFormat->addItem("FITS");

	ui.cbSeparator->addItem("TAB");
	ui.cbSeparator->addItem("SPACE");
	ui.cbSeparator->addItem(",");
	ui.cbSeparator->addItem(";");
	ui.cbSeparator->addItem(":");
	ui.cbSeparator->addItem(",TAB");
	ui.cbSeparator->addItem(";TAB");
	ui.cbSeparator->addItem(":TAB");
	ui.cbSeparator->addItem(",SPACE");
	ui.cbSeparator->addItem(";SPACE");
	ui.cbSeparator->addItem(":SPACE");

	ui.cbLaTeXExport->addItem(i18n("Export spreadsheet"));
	ui.cbLaTeXExport->addItem(i18n("Export selection"));

	ui.bOpen->setIcon( QIcon::fromTheme("document-open") );

	setMainWidget( mainWidget );

	setButtons( KDialog::Ok | KDialog::User1 | KDialog::Cancel );

	connect( ui.bOpen, SIGNAL(clicked()), this, SLOT (selectFile()) );
	connect( ui.kleFileName, SIGNAL(textChanged(QString)), this, SLOT(fileNameChanged(QString)) );
	connect(this,SIGNAL(user1Clicked()), this, SLOT(toggleOptions()));
	connect(ui.cbFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(formatChanged(int)));
	connect(ui.cbExportToFITS, SIGNAL(currentIndexChanged(int)), this, SLOT(fitsExportToChanged(int)));

	setCaption(i18n("Export spreadsheet"));
	setWindowIcon(QIcon::fromTheme("document-export-database"));

	//restore saved settings

	KConfigGroup conf(KSharedConfig::openConfig(), "ExportSpreadsheetDialog");
	ui.cbFormat->setCurrentIndex(conf.readEntry("Format", 0));
	ui.chkExportHeader->setChecked(conf.readEntry("Header", true));
	ui.cbSeparator->setCurrentItem(conf.readEntry("Separator", "TAB"));
	ui.chkHeaders->setChecked(conf.readEntry("LaTeXHeaders", true));
	ui.chkGridLines->setChecked(conf.readEntry("LaTeXGridLines", true));
	ui.chkCaptions->setChecked(conf.readEntry("LaTeXCaptions", true));
	ui.chkEmptyRows->setChecked(conf.readEntry("LaTeXSkipEmpty", false));
	ui.cbLaTeXExport->setCurrentIndex(conf.readEntry("ExportOnly", 0));
	ui.chkMatrixHHeader->setChecked(conf.readEntry("MatrixHorizontalHeader", true));
	ui.chkMatrixVHeader->setChecked(conf.readEntry("MatrixVerticalHeader", true));
	ui.chkMatrixVHeader->setChecked(conf.readEntry("FITSSpreadsheetColumnsUnits", true));
	ui.cbExportToFITS->setCurrentIndex(conf.readEntry("FITSTo", 0));
	m_showOptions = conf.readEntry("ShowOptions", false);
	ui.gbOptions->setVisible(m_showOptions);
	m_showOptions ? setButtonText(KDialog::User1,i18n("Hide Options")) : setButtonText(KDialog::User1,i18n("Show Options"));

	KWindowConfig::restoreWindowSize(windowHandle(), conf);
}

ExportSpreadsheetDialog::~ExportSpreadsheetDialog() {
	//save current settings
	KConfigGroup conf(KSharedConfig::openConfig(), "ExportSpreadsheetDialog");
	conf.writeEntry("Format", ui.cbFormat->currentIndex());
	conf.writeEntry("Header", ui.chkExportHeader->isChecked());
	conf.writeEntry("Separator", ui.cbSeparator->currentIndex());
	conf.writeEntry("ShowOptions", m_showOptions);
	conf.writeEntry("LaTeXHeaders", ui.chkHeaders->isChecked());
	conf.writeEntry("LaTeXGridLines", ui.chkGridLines->isChecked());
	conf.writeEntry("LaTeXCaptions", ui.chkCaptions->isChecked());
	conf.writeEntry("LaTeXSkipEmpty", ui.chkEmptyRows->isChecked());
	conf.writeEntry("ExportOnly", ui.cbLaTeXExport->currentIndex());
	conf.writeEntry("MatrixVerticalHeader", ui.chkMatrixVHeader->isChecked());
	conf.writeEntry("MatrixHorizontalHeader", ui.chkMatrixHHeader->isChecked());
	conf.writeEntry("FITSTo", ui.cbExportToFITS->currentIndex());
	conf.writeEntry("FITSSpreadsheetColumnsUnits", ui.chkColumnsAsUnits->isChecked());

	KWindowConfig::saveWindowSize(windowHandle(), conf);
	delete urlCompletion;
}

void ExportSpreadsheetDialog::setFileName(const QString& name) {
	KConfigGroup conf(KSharedConfig::openConfig(), "ExportSpreadsheetDialog");
	QString dir = conf.readEntry("LastDir", "");
	if (dir.isEmpty()) dir = QDir::homePath();
	ui.kleFileName->setText(dir + QDir::separator() +  name);
	this->formatChanged(ui.cbFormat->currentIndex());
}

void ExportSpreadsheetDialog::fitsExportToChanged(int idx) {
    if (idx == 0) {
        ui.chkColumnsAsUnits->hide();
        ui.lColumnAsUnits->hide();
    } else {
        if (!m_matrixMode) {
            ui.chkColumnsAsUnits->show();
            ui.lColumnAsUnits->show();
        }
    }
}

void ExportSpreadsheetDialog::setMatrixMode(bool b) {
	if (b) {
		setCaption(i18n("Export matrix"));
		ui.lExportHeader->hide();
		ui.chkExportHeader->hide();
		ui.lEmptyRows->hide();
		ui.chkEmptyRows->hide();
        if (ui.cbFormat->currentIndex() != 3) {
            ui.chkMatrixHHeader->show();
            ui.chkMatrixVHeader->show();
            ui.lMatrixHHeader->show();
            ui.lMatrixVHeader->show();
        }

		ui.lHeader->hide();
		ui.chkHeaders->hide();
		ui.cbLaTeXExport->setItemText(0,i18n("Export matrix"));
        ui.cbExportToFITS->setCurrentIndex(0);

        ui.lColumnAsUnits->hide();
        ui.chkColumnsAsUnits->hide();

		m_matrixMode = b;
	}
}

QString ExportSpreadsheetDialog::path() const {
	return ui.kleFileName->text();
}

int ExportSpreadsheetDialog::exportToFits() const {
    return ui.cbExportToFITS->currentIndex();
}

bool ExportSpreadsheetDialog::exportHeader() const {
	return ui.chkExportHeader->isChecked();
}

bool ExportSpreadsheetDialog::captions() const {
	return ui.chkCaptions->isChecked();
}

bool ExportSpreadsheetDialog::exportLatexHeader() const {
	return ui.chkHeaders->isChecked();
}

bool ExportSpreadsheetDialog::gridLines() const {
	return ui.chkGridLines->isChecked();
}

bool ExportSpreadsheetDialog::skipEmptyRows() const {
	return ui.chkEmptyRows->isChecked();
}

bool ExportSpreadsheetDialog::exportSelection() const {
	return ui.cbLaTeXExport->currentIndex() == 1;
}

bool ExportSpreadsheetDialog::entireSpreadheet() const {
	return ui.cbLaTeXExport->currentIndex() == 0;
}

bool ExportSpreadsheetDialog::matrixHorizontalHeader() const {
	return ui.chkMatrixHHeader->isChecked();
}

bool ExportSpreadsheetDialog::matrixVerticalHeader() const {
	return ui.chkMatrixVHeader->isChecked();
}

bool ExportSpreadsheetDialog::commentsAsUnitsFits() const {
    return ui.chkColumnsAsUnits->isChecked();
}

QString ExportSpreadsheetDialog::separator() const {
	return ui.cbSeparator->currentText();
}

void ExportSpreadsheetDialog::slotButtonClicked(int button) {
	if (button == KDialog::Ok)
		okClicked();
	else
		KDialog::slotButtonClicked(button);
}

void ExportSpreadsheetDialog::setExportToImage(bool possible) {
    if (!possible) {
        ui.cbExportToFITS->setCurrentIndex(1);
        ui.cbExportToFITS->setItemData(0, 0, Qt::UserRole - 1);
    }
}

//SLOTS
void ExportSpreadsheetDialog::okClicked() {
    if (format() != FITS)
        if ( QFile::exists(ui.kleFileName->text()) ) {
            int r=KMessageBox::questionYesNo(this, i18n("The file already exists. Do you really want to overwrite it?"), i18n("Export"));
            if (r==KMessageBox::No)
                return;
        }
    KConfigGroup conf(KSharedConfig::openConfig(), "ExportSpreadsheetDialog");
    conf.writeEntry("Format", ui.cbFormat->currentIndex());
    conf.writeEntry("Header", ui.chkExportHeader->isChecked());
	conf.writeEntry("Separator", ui.cbSeparator->currentText());

	QString path = ui.kleFileName->text();
	if (!path.isEmpty()) {
		QString dir = conf.readEntry("LastDir", "");
		ui.kleFileName->setText(path);
		int pos = path.lastIndexOf(QDir::separator());
		if (pos!=-1) {
			QString newDir = path.left(pos);
			if (newDir!=dir)
				conf.writeEntry("LastDir", newDir);
		}
	}

	accept();
}

/*!
	Shows/hides the GroupBox with export options in this dialog.
*/
void ExportSpreadsheetDialog::toggleOptions() {
	m_showOptions = !m_showOptions;
	ui.gbOptions->setVisible(m_showOptions);
	m_showOptions ? setButtonText(KDialog::User1, i18n("Hide Options")) : setButtonText(KDialog::User1, i18n("Show Options"));
	//resize the dialog
	mainWidget->resize(layout()->minimumSize());
	layout()->activate();
	resize( QSize(this->width(),0).expandedTo(minimumSize()) );
}

/*!
	opens a file dialog and lets the user select the file.
*/
void ExportSpreadsheetDialog::selectFile() {
	KConfigGroup conf(KSharedConfig::openConfig(), "ExportSpreadsheetDialog");
	QString dir = conf.readEntry("LastDir", "");
	QString path = QFileDialog::getOpenFileName(this, i18n("Export to file"), dir);
	if (!path.isEmpty()) {
		ui.kleFileName->setText(path);

		int pos = path.lastIndexOf(QDir::separator());
		if (pos!=-1) {
			QString newDir = path.left(pos);
			if (newDir!=dir)
				conf.writeEntry("LastDir", newDir);
		}
	}
}

/*!
	called when the output format was changed. Adjusts the extension for the specified file.
 */
void ExportSpreadsheetDialog::formatChanged(int index) {
	QStringList extensions;
	extensions << ".txt" << ".bin" << ".tex" << ".fits";
	QString path = ui.kleFileName->text();
	int i = path.indexOf(".");
	if (index != 1) {
		if (i==-1)
			path = path + extensions.at(index);
		else
			path=path.left(i) + extensions.at(index);
	}
	if (ui.cbFormat->currentIndex() == 2) {
		ui.cbSeparator->hide();
		ui.lSeparator->hide();

		ui.chkCaptions->show();
		ui.chkGridLines->show();;
		ui.lExportArea->show();
		ui.lGridLines->show();
		ui.lCaptions->show();
		ui.cbLaTeXExport->show();

		if (!m_matrixMode) {
			ui.lHeader->show();
			ui.chkHeaders->show();
			ui.lEmptyRows->show();
			ui.chkEmptyRows->show();
            ui.lMatrixHHeader->hide();
            ui.lMatrixVHeader->hide();
            ui.chkMatrixHHeader->hide();
            ui.chkMatrixVHeader->hide();
		} else {
			ui.lMatrixHHeader->show();
			ui.lMatrixVHeader->show();
			ui.chkMatrixHHeader->show();
			ui.chkMatrixVHeader->show();
		}
		ui.cbExportToFITS->hide();
		ui.lExportToFITS->hide();
		ui.lColumnAsUnits->hide();
		ui.chkColumnsAsUnits->hide();
	//FITS
	} else if(ui.cbFormat->currentIndex() == 3) {
		ui.lCaptions->hide();
		ui.lEmptyRows->hide();
		ui.lExportArea->hide();
		ui.lGridLines->hide();
		ui.lMatrixHHeader->hide();
		ui.lMatrixVHeader->hide();
		ui.lSeparator->hide();
		ui.lHeader->hide();
		ui.chkEmptyRows->hide();
		ui.chkHeaders->hide();
		ui.chkExportHeader->hide();
		ui.lExportHeader->hide();
		ui.chkGridLines->hide();
		ui.chkMatrixHHeader->hide();
		ui.chkMatrixVHeader->hide();
		ui.chkCaptions->hide();
		ui.cbLaTeXExport->hide();
		ui.cbSeparator->hide();

		ui.cbExportToFITS->show();
		ui.lExportToFITS->show();
		if (!m_matrixMode) {
			ui.lColumnAsUnits->show();
			ui.chkColumnsAsUnits->show();
		}
	} else {
		ui.cbSeparator->show();
		ui.lSeparator->show();

		ui.chkCaptions->hide();
		ui.chkEmptyRows->hide();
		ui.chkGridLines->hide();;
		ui.lEmptyRows->hide();
		ui.lExportArea->hide();
		ui.lGridLines->hide();
		ui.lCaptions->hide();
		ui.cbLaTeXExport->hide();
		ui.lMatrixHHeader->hide();
		ui.lMatrixVHeader->hide();
		ui.chkMatrixHHeader->hide();
		ui.chkMatrixVHeader->hide();

		ui.lHeader->hide();
		ui.chkHeaders->hide();

		ui.cbExportToFITS->hide();
		ui.lExportToFITS->hide();
		ui.lColumnAsUnits->hide();
		ui.chkColumnsAsUnits->hide();
	}

	if (!m_matrixMode) {
			ui.chkExportHeader->show();
			ui.lExportHeader->show();
	} else {
		ui.chkExportHeader->hide();
		ui.lExportHeader->hide();
	}
	if (ui.cbFormat->currentIndex() == 3) {
		ui.chkExportHeader->hide();
		ui.lExportHeader->hide();
	}

	setFormat(static_cast<Format>(index));
	ui.kleFileName->setText(path);
}

void ExportSpreadsheetDialog::setExportSelection(bool enable) {
	if (!enable) {
		const QStandardItemModel* areaToExportModel = qobject_cast<const QStandardItemModel*>(ui.cbLaTeXExport->model());
		QStandardItem* item = areaToExportModel->item(1);
		item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
	}
}

void ExportSpreadsheetDialog::setFormat(Format format) {
	m_format = format;
}

void ExportSpreadsheetDialog::setExportTo(const QStringList &to) {
	ui.cbExportToFITS->addItems(to);
}

ExportSpreadsheetDialog::Format ExportSpreadsheetDialog::format() const {
	return m_format;
}

void ExportSpreadsheetDialog::fileNameChanged(const QString& name) {
	enableButtonOk( !name.simplified().isEmpty() );
}
