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

#include "ImportFileDialog.h"
#include "ImportFileWidget.h"
#include "backend/core/AspectTreeModel.h"
#include "backend/datasources/LiveDataSource.h"
#include "backend/datasources/filters/AbstractFileFilter.h"
#include "backend/datasources/filters/HDFFilter.h"
#include "backend/datasources/filters/NetCDFFilter.h"
#include "backend/spreadsheet/Spreadsheet.h"
#include "backend/matrix/Matrix.h"
#include "backend/core/Workbook.h"
#include "commonfrontend/widgets/TreeViewComboBox.h"
#include "kdefrontend/MainWin.h"

#include <KMessageBox>
#include <KInputDialog>
#include <KSharedConfig>
#include <KWindowConfig>
#include <KLocalizedString>

#include <QDialogButtonBox>
#include <QProgressBar>
#include <QTcpSocket>
#include <QLocalSocket>
#include <QUdpSocket>
#include <QStatusBar>
#include <QDir>
#include <QInputDialog>
#include <QMenu>

/*!
	\class ImportFileDialog
	\brief Dialog for importing data from a file. Embeds \c ImportFileWidget and provides the standard buttons.

	\ingroup kdefrontend
 */

ImportFileDialog::ImportFileDialog(MainWin* parent, bool liveDataSource, const QString& fileName) : ImportDialog(parent),
	m_importFileWidget(new ImportFileWidget(this, fileName)),
	m_showOptions(false) {

	vLayout->addWidget(m_importFileWidget);

	//dialog buttons
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Reset |QDialogButtonBox::Cancel);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	m_optionsButton = buttonBox->button(QDialogButtonBox::Reset); //we highjack the default "Reset" button and use if for showing/hiding the options
	okButton->setEnabled(false); //ok is only available if a valid container was selected
	vLayout->addWidget(buttonBox);

	//hide the data-source related widgets
	if (!liveDataSource) {
		setModel();
		//TODO: disable for file data sources
		m_importFileWidget->hideDataSource();
	} else
		m_importFileWidget->initializeAndFillPortsAndBaudRates();

	//Signals/Slots
	connect(m_importFileWidget, SIGNAL(checkedFitsTableToMatrix(bool)), this, SLOT(checkOnFitsTableToMatrix(bool)));
	connect(m_importFileWidget, SIGNAL(fileNameChanged()), this, SLOT(checkOkButton()));
	connect(m_importFileWidget, SIGNAL(sourceTypeChanged()), this, SLOT(checkOkButton()));
	connect(m_importFileWidget, SIGNAL(hostChanged()), this, SLOT(checkOkButton()));
	connect(m_importFileWidget, SIGNAL(portChanged()), this, SLOT(checkOkButton()));
	connect(m_optionsButton, SIGNAL(clicked()), this, SLOT(toggleOptions()));
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	if (!liveDataSource) {
		setWindowTitle(i18n("Import Data to Spreadsheet or Matrix"));
		m_importFileWidget->hideDataSource();
	} else
		setWindowTitle(i18n("Add new live data source"));

	setWindowIcon(QIcon::fromTheme("document-import-database"));

	QTimer::singleShot(0, this, &ImportFileDialog::loadSettings);
}

void ImportFileDialog::loadSettings() {
	//restore saved settings
	QApplication::processEvents(QEventLoop::AllEvents, 0);
	KConfigGroup conf(KSharedConfig::openConfig(), "ImportFileDialog");
	m_showOptions = conf.readEntry("ShowOptions", false);
	m_showOptions ? m_optionsButton->setText(i18n("Hide Options")) : m_optionsButton->setText(i18n("Show Options"));
	m_importFileWidget->showOptions(m_showOptions);

	KWindowConfig::restoreWindowSize(windowHandle(), conf);
}

ImportFileDialog::~ImportFileDialog() {
	//save current settings
	KConfigGroup conf(KSharedConfig::openConfig(), "ImportFileDialog");
	conf.writeEntry("ShowOptions", m_showOptions);
	if (cbPosition)
		conf.writeEntry("Position", cbPosition->currentIndex());

	KWindowConfig::saveWindowSize(windowHandle(), conf);
}

/*!
  triggers data import to the live data source \c source
*/
void ImportFileDialog::importToLiveDataSource(LiveDataSource* source, QStatusBar* statusBar) const {
	m_importFileWidget->saveSettings(source);

	//show a progress bar in the status bar
	QProgressBar* progressBar = new QProgressBar();
	progressBar->setRange(0, 100);
	connect(source->filter(), SIGNAL(completed(int)), progressBar, SLOT(setValue(int)));

	statusBar->clearMessage();
	statusBar->addWidget(progressBar, 1);
	WAIT_CURSOR;

	QTime timer;
	timer.start();
	source->read();
	statusBar->showMessage( i18n("Live data source created in %1 seconds.", (float)timer.elapsed()/1000) );

	RESET_CURSOR;
	statusBar->removeWidget(progressBar);
	source->ready();
}
/*!
  triggers data import to the currently selected data container
*/
void ImportFileDialog::importTo(QStatusBar* statusBar) const {
	DEBUG("ImportFileDialog::importTo()");
	QDEBUG("cbAddTo->currentModelIndex() =" << cbAddTo->currentModelIndex());
	AbstractAspect* aspect = static_cast<AbstractAspect*>(cbAddTo->currentModelIndex().internalPointer());
	if (!aspect) {
		DEBUG("ERROR in importTo(): No aspect available");
		DEBUG("cbAddTo->currentModelIndex().isValid() = " << cbAddTo->currentModelIndex().isValid());
		DEBUG("cbAddTo->currentModelIndex() row/column = " << cbAddTo->currentModelIndex().row() << ' ' << cbAddTo->currentModelIndex().column());
		return;
	}

	if (m_importFileWidget->isFileEmpty()) {
		KMessageBox::information(0, i18n("No data to import."), i18n("No Data"));
		return;
	}

	QString fileName = m_importFileWidget->fileName();
	AbstractFileFilter* filter = m_importFileWidget->currentFileFilter();
	AbstractFileFilter::ImportMode mode = AbstractFileFilter::ImportMode(cbPosition->currentIndex());

	//show a progress bar in the status bar
	QProgressBar* progressBar = new QProgressBar();
	progressBar->setRange(0, 100);
	connect(filter, SIGNAL(completed(int)), progressBar, SLOT(setValue(int)));

	statusBar->clearMessage();
	statusBar->addWidget(progressBar, 1);

	WAIT_CURSOR;
	QApplication::processEvents(QEventLoop::AllEvents, 100);

	QTime timer;
	timer.start();
	if (aspect->inherits("Matrix")) {
		Matrix* matrix = qobject_cast<Matrix*>(aspect);
		filter->readDataFromFile(fileName, matrix, mode);
	} else if (aspect->inherits("Spreadsheet")) {
		Spreadsheet* spreadsheet = qobject_cast<Spreadsheet*>(aspect);
		filter->readDataFromFile(fileName, spreadsheet, mode);
	} else if (aspect->inherits("Workbook")) {
		Workbook* workbook = qobject_cast<Workbook*>(aspect);
		QVector<AbstractAspect*> sheets = workbook->children<AbstractAspect>();

		QStringList names;
		LiveDataSource::FileType fileType = m_importFileWidget->currentFileType();
		if (fileType == LiveDataSource::HDF)
			names = m_importFileWidget->selectedHDFNames();
		else if (fileType == LiveDataSource::NETCDF)
			names = m_importFileWidget->selectedNetCDFNames();

		//multiple extensions selected

		// multiple data sets/variables for HDF/NetCDF
		if (fileType == LiveDataSource::HDF || fileType == LiveDataSource::NETCDF) {
			int nrNames = names.size(), offset = sheets.size();

			int start=0;
			if (mode == AbstractFileFilter::Replace)
				start=offset;

			// add additional sheets
			for (int i = start; i < nrNames; ++i) {
				Spreadsheet *spreadsheet = new Spreadsheet(0, i18n("Spreadsheet"));
				if (mode == AbstractFileFilter::Prepend)
					workbook->insertChildBefore(spreadsheet,sheets[0]);
				else
					workbook->addChild(spreadsheet);
			}

			if (mode != AbstractFileFilter::Append)
				offset = 0;

			// import to sheets
			sheets = workbook->children<AbstractAspect>();
			for (int i = 0; i < nrNames; ++i) {
				if (fileType == LiveDataSource::HDF)
					((HDFFilter*) filter)->setCurrentDataSetName(names[i]);
				else
					((NetCDFFilter*) filter)->setCurrentVarName(names[i]);

				if (sheets[i+offset]->inherits("Matrix"))
					filter->readDataFromFile(fileName, qobject_cast<Matrix*>(sheets[i+offset]));
				else if (sheets[i+offset]->inherits("Spreadsheet"))
					filter->readDataFromFile(fileName, qobject_cast<Spreadsheet*>(sheets[i+offset]));
			}
		} else { // single import file types
			// use active spreadsheet/matrix if present, else new spreadsheet
			Spreadsheet* spreadsheet = workbook->currentSpreadsheet();
			Matrix* matrix = workbook->currentMatrix();
			if (spreadsheet)
				filter->readDataFromFile(fileName, spreadsheet, mode);
			else if (matrix)
				filter->readDataFromFile(fileName, matrix, mode);
			else {
				spreadsheet = new Spreadsheet(0, i18n("Spreadsheet"));
				workbook->addChild(spreadsheet);
				filter->readDataFromFile(fileName, spreadsheet, mode);
			}
		}
	}
	statusBar->showMessage( i18n("File %1 imported in %2 seconds.", fileName, (float)timer.elapsed()/1000) );

	RESET_CURSOR;
	statusBar->removeWidget(progressBar);
	delete filter;
}

void ImportFileDialog::toggleOptions() {
	m_importFileWidget->showOptions(!m_showOptions);
	m_showOptions = !m_showOptions;
	m_showOptions ? m_optionsButton->setText(i18n("Hide Options")) : m_optionsButton->setText(i18n("Show Options"));

	//resize the dialog
	layout()->activate();
	resize( QSize(this->width(), 0).expandedTo(minimumSize()) );
}

void ImportFileDialog::checkOnFitsTableToMatrix(const bool enable) {
	if (cbAddTo) {
		QDEBUG("cbAddTo->currentModelIndex() = " << cbAddTo->currentModelIndex());
		AbstractAspect* aspect = static_cast<AbstractAspect*>(cbAddTo->currentModelIndex().internalPointer());
		if (!aspect) {
			DEBUG("ERROR: no aspect available.");
			return;
		}

		if(aspect->inherits("Matrix")) {
			okButton->setEnabled(enable);
			if (enable)
				okButton->setToolTip(i18n("Close the dialog and import the data."));
			else
				okButton->setToolTip(i18n("Cannot import into a matrix since the data contains non-numerical data."));
		}
	}
}

void ImportFileDialog::checkOkButton() {
	DEBUG("ImportFileDialog::checkOkButton()");
	if (cbAddTo) { //only check for the target container when no file data source is being added
		QDEBUG(" cbAddTo->currentModelIndex() = " << cbAddTo->currentModelIndex());
		AbstractAspect* aspect = static_cast<AbstractAspect*>(cbAddTo->currentModelIndex().internalPointer());
		if (!aspect) {
			okButton->setEnabled(false);
			okButton->setToolTip(i18n("Select a data container where the data has to be imported into."));
			lPosition->setEnabled(false);
			cbPosition->setEnabled(false);
			return;
		} else {
			lPosition->setEnabled(true);
			cbPosition->setEnabled(true);

			//when doing ASCII import to a matrix, hide the options for using the file header (first line)
			//to name the columns since the column names are fixed in a matrix
			const Matrix* matrix = dynamic_cast<const Matrix*>(aspect);
			m_importFileWidget->showAsciiHeaderOptions(matrix == NULL);
		}
	}

	QString fileName = m_importFileWidget->fileName();
#ifndef HAVE_WINDOWS
	if (!fileName.isEmpty() && fileName.left(1) != QDir::separator())
		fileName = QDir::homePath() + QDir::separator() + fileName;
#endif

	DEBUG(" fileName = " << fileName.toUtf8().constData());

	switch (m_importFileWidget->currentSourceType()) {
	case LiveDataSource::SourceType::FileOrPipe: {
		const bool enable = QFile::exists(fileName);
		okButton->setEnabled(enable);
		if (enable)
			okButton->setToolTip(i18n("Close the dialog and import the data."));
		else
			okButton->setToolTip(i18n("Provide an existing file."));

		break;
	}
	case LiveDataSource::SourceType::LocalSocket: {
		const bool enable = QFile::exists(fileName);
		if (enable) {
			QLocalSocket* socket = new QLocalSocket(this);
			socket->connectToServer(fileName, QLocalSocket::ReadOnly);
			bool localSocketConnected = socket->waitForConnected(2000);

			okButton->setEnabled(localSocketConnected);
			if (localSocketConnected)
				okButton->setToolTip(i18n("Close the dialog and import the data."));
			else
				okButton->setToolTip(i18n("Couldn't connect to the provided local socket."));

			if (socket->state() == QLocalSocket::ConnectedState) {
				socket->disconnectFromServer();
				socket->waitForDisconnected(1000);
				connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
			} else
				delete socket;

		} else {
			okButton->setEnabled(false);
			okButton->setToolTip(i18n("Selected local socket doesn't exist."));
		}

		break;
	}
	case LiveDataSource::SourceType::NetworkTcpSocket: {
		const bool enable = !m_importFileWidget->host().isEmpty() && !m_importFileWidget->port().isEmpty();
		if (enable) {
			QTcpSocket* socket = new QTcpSocket(this);
			socket->connectToHost(m_importFileWidget->host(), m_importFileWidget->port().toUShort(), QTcpSocket::ReadOnly);
			bool tcpSocketConnected = socket->waitForConnected(2000);

			okButton->setEnabled(tcpSocketConnected);
			if (tcpSocketConnected)
				okButton->setToolTip(i18n("Close the dialog and import the data."));
			else
				okButton->setToolTip(i18n("Couldn't connect to the provided TCP socket."));

			if (socket->state() == QTcpSocket::ConnectedState) {
				socket->disconnectFromHost();
				socket->waitForDisconnected(1000);
				connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
			} else
				delete socket;
		} else {
			okButton->setEnabled(false);
			okButton->setToolTip(i18n("Either the host name or the port number is missing."));
		}
		break;
	}
	case LiveDataSource::SourceType::NetworkUdpSocket: {
		const bool enable = !m_importFileWidget->host().isEmpty() && !m_importFileWidget->port().isEmpty();
		if (enable) {
			QUdpSocket* socket = new QUdpSocket(this);
			socket->connectToHost(m_importFileWidget->host(), m_importFileWidget->port().toUShort(), QUdpSocket::ReadOnly);
			bool udpSocketConnected = socket->waitForConnected(2000);

			okButton->setEnabled(udpSocketConnected);
			if (udpSocketConnected)
				okButton->setToolTip(i18n("Close the dialog and import the data."));
			else
				okButton->setToolTip(i18n("Couldn't connect to the provided UDP socket."));

			if (socket->state() == QUdpSocket::ConnectedState) {
				socket->disconnectFromHost();
				socket->waitForDisconnected(1000);
				connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
			} else
				delete socket;
		} else {
			okButton->setEnabled(false);
			okButton->setToolTip(i18n("Either the host name or the port number is missing."));
		}

		break;
	}
	case LiveDataSource::SourceType::SerialPort: {
		const bool enable = !m_importFileWidget->serialPort().isEmpty();
		if (enable) {
			QSerialPort* serialPort = new QSerialPort(this);

			serialPort->setBaudRate(m_importFileWidget->baudRate());
			serialPort->setPortName(m_importFileWidget->serialPort());

			bool serialPortOpened = serialPort->open(QIODevice::ReadOnly);
			okButton->setEnabled(serialPortOpened);
			if (serialPortOpened)
				okButton->setToolTip(i18n("Close the dialog and import the data."));
			else
				okButton->setToolTip(i18n("Couldn't connect to the provided UDP socket."));
		} else {
			okButton->setEnabled(false);
			okButton->setToolTip(i18n("Serial port number is missing."));
		}
	}
	}
}

QString ImportFileDialog::selectedObject() const {
	QString path = m_importFileWidget->fileName();

	//determine the file name only
	QString name = path.right( path.length()-path.lastIndexOf(QDir::separator())-1 );

	//strip away the extension if available
	if (name.indexOf('.') != -1)
		name = name.left(name.lastIndexOf('.'));
	return name;
}
