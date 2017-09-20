/***************************************************************************
File                 : ImportFileWidget.cpp
Project              : LabPlot
Description          : import file data widget
--------------------------------------------------------------------
Copyright            : (C) 2009-2017 Stefan Gerlach (stefan.gerlach@uni.kn)
Copyright            : (C) 2009-2017 Alexander Semke (alexander.semke@web.de)
Copyright            : (C) 2017 Fabian Kristof (fkristofszabolcs@gmail.com)

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

#include "ImportFileWidget.h"
#include "FileInfoDialog.h"
#include "backend/datasources/filters/AsciiFilter.h"
#include "backend/datasources/filters/BinaryFilter.h"
#include "backend/datasources/filters/HDFFilter.h"
#include "backend/datasources/filters/NetCDFFilter.h"
#include "backend/datasources/filters/ImageFilter.h"
#include "backend/datasources/filters/FITSFilter.h"
#include "AsciiOptionsWidget.h"
#include "BinaryOptionsWidget.h"
#include "HDFOptionsWidget.h"
#include "ImageOptionsWidget.h"
#include "NetCDFOptionsWidget.h"
#include "FITSOptionsWidget.h"

#include <QTableWidget>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QLocalSocket>
#include <QInputDialog>
#include <QDir>
#include <QFileDialog>
#include <QProcess>
#include <QIntValidator>
#include <KUrlCompletion>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QTimer>
#include <QStandardItemModel>
#include <QImageReader>
#include <KUrlCompletion>

/*!
   \class ImportFileWidget
   \brief Widget for importing data from a file.

   \ingroup kdefrontend
*/
ImportFileWidget::ImportFileWidget(QWidget* parent, const QString& fileName) : QWidget(parent), m_fileName(fileName),
	m_liveDataSource(true) {
	ui.setupUi(this);

	KUrlCompletion *comp = new KUrlCompletion();
	ui.kleFileName->setCompletionObject(comp);

	ui.cbFileType->addItems(LiveDataSource::fileTypes());
	QStringList filterItems;
	filterItems << i18n("Automatic") << i18n("Custom");
	ui.cbFilter->addItems(filterItems);

	// file type specific option widgets
	QWidget* asciiw = new QWidget();
	m_asciiOptionsWidget = std::unique_ptr<AsciiOptionsWidget>(new AsciiOptionsWidget(asciiw));
	ui.swOptions->insertWidget(LiveDataSource::Ascii, asciiw);

	QWidget* binaryw = new QWidget();
	m_binaryOptionsWidget = std::unique_ptr<BinaryOptionsWidget>(new BinaryOptionsWidget(binaryw));
	ui.swOptions->insertWidget(LiveDataSource::Binary, binaryw);

	QWidget* imagew = new QWidget();
	m_imageOptionsWidget = std::unique_ptr<ImageOptionsWidget>(new ImageOptionsWidget(imagew));
	ui.swOptions->insertWidget(LiveDataSource::Image, imagew);

	QWidget* hdfw = new QWidget();
	m_hdfOptionsWidget = std::unique_ptr<HDFOptionsWidget>(new HDFOptionsWidget(hdfw, this));
	ui.swOptions->insertWidget(LiveDataSource::HDF, hdfw);

	QWidget* netcdfw = new QWidget();
	m_netcdfOptionsWidget = std::unique_ptr<NetCDFOptionsWidget>(new NetCDFOptionsWidget(netcdfw, this));
	ui.swOptions->insertWidget(LiveDataSource::NETCDF, netcdfw);

	QWidget* fitsw = new QWidget();
	m_fitsOptionsWidget = std::unique_ptr<FITSOptionsWidget>(new FITSOptionsWidget(fitsw, this));
	ui.swOptions->insertWidget(LiveDataSource::FITS, fitsw);

	// the table widget for preview
	m_twPreview = new QTableWidget(ui.tePreview);
	m_twPreview->verticalHeader()->hide();
	m_twPreview->setEditTriggers(QTableWidget::NoEditTriggers);
	QHBoxLayout* layout = new QHBoxLayout;
	layout->addWidget(m_twPreview);
	ui.tePreview->setLayout(layout);
	m_twPreview->hide();

	// default filter
	ui.swOptions->setCurrentIndex(LiveDataSource::Ascii);
#if !defined(HAVE_HDF5) || !defined(HAVE_NETCDF) || !defined(HAVE_FITS)
	const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(ui.cbFileType->model());
#endif
#ifndef HAVE_HDF5
	// disable HDF5 item
	QStandardItem* item = model->item(LiveDataSource::HDF);
	item->setFlags(item->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
#endif
#ifndef HAVE_NETCDF
	// disable NETCDF item
	QStandardItem* item2 = model->item(LiveDataSource::NETCDF);
	item2->setFlags(item2->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
#endif
#ifndef HAVE_FITS
	// disable FITS item
	QStandardItem* item3 = model->item(LiveDataSource::FITS);
	item3->setFlags(item3->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
#endif

	ui.lePort->setValidator( new QIntValidator(ui.lePort) );
	ui.gbOptions->hide();
	ui.gbUpdateOptions->hide();

	ui.bOpen->setIcon( QIcon::fromTheme("document-open") );
	ui.bFileInfo->setIcon( QIcon::fromTheme("help-about") );
	ui.bManageFilters->setIcon( QIcon::fromTheme("configure") );
	ui.bSaveFilter->setIcon( QIcon::fromTheme("document-save") );
	ui.bRefreshPreview->setIcon( QIcon::fromTheme("view-refresh") );

	connect( ui.kleFileName, SIGNAL(textChanged(QString)), SLOT(fileNameChanged(QString)) );
	connect( ui.bOpen, SIGNAL(clicked()), this, SLOT (selectFile()) );
	connect( ui.bFileInfo, SIGNAL(clicked()), this, SLOT (fileInfoDialog()) );
	connect( ui.bSaveFilter, SIGNAL(clicked()), this, SLOT (saveFilter()) );
	connect( ui.bManageFilters, SIGNAL(clicked()), this, SLOT (manageFilters()) );
	connect( ui.cbFileType, SIGNAL(currentIndexChanged(int)), SLOT(fileTypeChanged(int)) );
	connect( ui.cbUpdateType, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTypeChanged(int)));
	connect( ui.cbReadType, SIGNAL(currentIndexChanged(int)), this, SLOT(readingTypeChanged(int)));
	connect( ui.cbFilter, SIGNAL(activated(int)), SLOT(filterChanged(int)) );
	connect( ui.bRefreshPreview, SIGNAL(clicked()), SLOT(refreshPreview()) );

	connect(ui.leHost, SIGNAL(textChanged(QString)), this, SIGNAL(hostChanged()));
	connect(ui.lePort, SIGNAL(textChanged(QString)), this, SIGNAL(portChanged()));

	connect( ui.cbSourceType, SIGNAL(currentIndexChanged(int)), this, SLOT(sourceTypeChanged(int)));

	//TODO: implement save/load of user-defined settings later and activate these buttons again
	ui.bSaveFilter->hide();
	ui.bManageFilters->hide();

	//defer the loading of settings a bit in order to show the dialog prior to blocking the GUI in refreshPreview()
	QTimer::singleShot( 100, this, SLOT(loadSettings()) );
}

void ImportFileWidget::loadSettings() {
	//load last used settings
	QString confName;
	if (m_liveDataSource)
		confName = QLatin1String("LiveDataImport");
	else
		confName = QLatin1String("FileImport");
	KConfigGroup conf(KSharedConfig::openConfig(), confName);

	//settings for data type specific widgets
	m_asciiOptionsWidget->loadSettings();
	m_binaryOptionsWidget->loadSettings();
	m_imageOptionsWidget->loadSettings();

	//read the source type first since settings in fileNameChanged() depend on this
	ui.cbSourceType->setCurrentIndex(conf.readEntry("SourceType").toInt());

	//general settings
	ui.cbFileType->setCurrentIndex(conf.readEntry("Type", 0));
	ui.cbFilter->setCurrentIndex(conf.readEntry("Filter", 0));
	filterChanged(ui.cbFilter->currentIndex());	// needed if filter is not changed
	if (m_fileName.isEmpty())
		ui.kleFileName->setText(conf.readEntry("LastImportedFile", ""));
	else
		ui.kleFileName->setText(m_fileName);

	//live data related settings
	ui.cbBaudRate->setCurrentIndex(conf.readEntry("BaudRate").toInt());
	ui.cbReadType->setCurrentIndex(conf.readEntry("ReadType").toInt());
	ui.cbSerialPort->setCurrentIndex(conf.readEntry("SerialPort").toInt());
	ui.cbUpdateType->setCurrentIndex(conf.readEntry("UpdateType").toInt());
	ui.leHost->setText(conf.readEntry("Host",""));
	ui.leKeepLastValues->setText(conf.readEntry("KeepLastNValues",""));
	ui.lePort->setText(conf.readEntry("Port",""));
	ui.sbSampleRate->setValue(conf.readEntry("SampleRate").toInt());
	ui.sbUpdateInterval->setValue(conf.readEntry("UpdateInterval").toInt());
}

ImportFileWidget::~ImportFileWidget() {
	// save current settings
	QString confName;
	if (m_liveDataSource)
		confName = QLatin1String("LiveDataImport");
	else
		confName = QLatin1String("FileImport");
	KConfigGroup conf(KSharedConfig::openConfig(), confName);

	// general settings
	conf.writeEntry("Type", ui.cbFileType->currentIndex());
	conf.writeEntry("Filter", ui.cbFilter->currentIndex());
	conf.writeEntry("LastImportedFile", ui.kleFileName->text());

	//live data related settings
	conf.writeEntry("SourceType", ui.cbSourceType->currentIndex());
	conf.writeEntry("UpdateType", ui.cbUpdateType->currentIndex());
	conf.writeEntry("ReadType", ui.cbReadType->currentIndex());
	conf.writeEntry("SampleRate", ui.sbSampleRate->value());
	conf.writeEntry("KeepLastNValues", ui.leKeepLastValues->text());
	conf.writeEntry("BaudRate", ui.cbBaudRate->currentIndex());
	conf.writeEntry("SerialPort", ui.cbSerialPort->currentIndex());
	conf.writeEntry("Host", ui.leHost->text());
	conf.writeEntry("Port", ui.lePort->text());
	conf.writeEntry("UpdateInterval", ui.sbUpdateInterval->value());

	// data type specific settings
	m_asciiOptionsWidget->saveSettings();
	m_binaryOptionsWidget->saveSettings();
	m_imageOptionsWidget->saveSettings();
}

void ImportFileWidget::hideDataSource() {
	m_liveDataSource = false;
	ui.gbUpdateOptions->hide();

	ui.chbLinkFile->hide();

	ui.cbBaudRate->hide();
	ui.lBaudRate->hide();

	ui.lHost->hide();
	ui.leHost->hide();

	ui.lPort->hide();
	ui.lePort->hide();

	ui.cbSerialPort->hide();
	ui.lSerialPort->hide();

	ui.lSourceType->hide();
	ui.cbSourceType->hide();

	ui.cbUpdateType->hide();
	ui.lUpdateType->hide();

	ui.sbUpdateInterval->hide();
	ui.lUpdateInterval->hide();
}

void ImportFileWidget::showAsciiHeaderOptions(bool b) {
	m_asciiOptionsWidget->showAsciiHeaderOptions(b);
}

void ImportFileWidget::showOptions(bool b) {
	ui.gbOptions->setVisible(b);

	if (m_liveDataSource)
		ui.gbUpdateOptions->setVisible(b);

	resize(layout()->minimumSize());
}

QString ImportFileWidget::fileName() const {
	if (currentFileType() == LiveDataSource::FITS) {
		QString extensionName = m_fitsOptionsWidget->currentExtensionName();
		if (!extensionName.isEmpty())
			return ui.kleFileName->text() + QLatin1String("[") + extensionName + QLatin1String("]");
	}

	return ui.kleFileName->text();
}

QString ImportFileWidget::host() const {
	return ui.leHost->text();
}

QString ImportFileWidget::port() const {
	return ui.lePort->text();
}

QString ImportFileWidget::serialPort() const {
	return ui.cbSerialPort->currentText();
}

int ImportFileWidget::baudRate() const {
	return ui.cbBaudRate->currentText().toInt();
}

/*!
	saves the settings to the data source \c source.
*/
void ImportFileWidget::saveSettings(LiveDataSource* source) const {
	LiveDataSource::FileType fileType = static_cast<LiveDataSource::FileType>(ui.cbFileType->currentIndex());
	LiveDataSource::UpdateType updateType = static_cast<LiveDataSource::UpdateType>(ui.cbUpdateType->currentIndex());
	LiveDataSource::SourceType sourceType = static_cast<LiveDataSource::SourceType>(ui.cbSourceType->currentIndex());
	LiveDataSource::ReadingType readingType = static_cast<LiveDataSource::ReadingType>(ui.cbReadType->currentIndex());

	source->setComment( ui.kleFileName->text() );
	source->setFileType(fileType);
	source->setFilter(this->currentFileFilter());

	source->setSourceType(sourceType);
	source->setReadingType(readingType);

	if (updateType == LiveDataSource::UpdateType::TimeInterval)
		source->setUpdateInterval(ui.sbUpdateInterval->value());
	else
		source->setFileWatched(true);

	if (!ui.leKeepLastValues->text().isEmpty()) {
		source->setKeepLastValues(true);
		source->setKeepNvalues(ui.leKeepLastValues->text().toInt());
	}

	source->setUpdateType(updateType);

	if (readingType != LiveDataSource::ReadingType::TillEnd)
		source->setSampleRate(ui.sbSampleRate->value());

	switch (sourceType) {
	case LiveDataSource::SourceType::FileOrPipe:
		source->setFileName( ui.kleFileName->text() );
		source->setFileLinked( ui.chbLinkFile->isChecked() );
		break;
	case LiveDataSource::SourceType::LocalSocket:
		source->setLocalSocketName(ui.kleFileName->text());
		break;
	case LiveDataSource::SourceType::NetworkTcpSocket:
	case LiveDataSource::SourceType::NetworkUdpSocket:
		source->setHost(ui.leHost->text());
		source->setPort(ui.lePort->text().toInt());
		break;
	case LiveDataSource::SourceType::SerialPort:
		source->setBaudRate(ui.cbBaudRate->currentText().toInt());
		source->setSerialPort(ui.cbSerialPort->currentText());
		break;
	default:
		break;
	}
}

/*!
	returns the currently used file type.
*/
LiveDataSource::FileType ImportFileWidget::currentFileType() const {
	return static_cast<LiveDataSource::FileType>(ui.cbFileType->currentIndex());
}

LiveDataSource::SourceType ImportFileWidget::currentSourceType() const {
	return static_cast<LiveDataSource::SourceType>(ui.cbSourceType->currentIndex());
}

/*!
	returns the currently used filter.
*/
AbstractFileFilter* ImportFileWidget::currentFileFilter() const {
	DEBUG("currentFileFilter()");
	LiveDataSource::FileType fileType = static_cast<LiveDataSource::FileType>(ui.cbFileType->currentIndex());

	switch (fileType) {
	case LiveDataSource::Ascii: {
//TODO			std::unique_ptr<AsciiFilter> filter(new AsciiFilter());
			AsciiFilter* filter = new AsciiFilter();

			if (ui.cbFilter->currentIndex() == 0)     //"automatic"
				filter->setAutoModeEnabled(true);
			else if (ui.cbFilter->currentIndex() == 1) { //"custom"
				filter->setAutoModeEnabled(false);
				m_asciiOptionsWidget->applyFilterSettings(filter);
			} else
				filter->loadFilterSettings( ui.cbFilter->currentText() );

			//save the data portion to import
			filter->setStartRow( ui.sbStartRow->value());
			filter->setEndRow( ui.sbEndRow->value() );
			filter->setStartColumn( ui.sbStartColumn->value());
			filter->setEndColumn( ui.sbEndColumn->value());

			return filter;
		}
	case LiveDataSource::Binary: {
			BinaryFilter* filter = new BinaryFilter();
			if ( ui.cbFilter->currentIndex() == 0 ) 	//"automatic"
				filter->setAutoModeEnabled(true);
			else if ( ui.cbFilter->currentIndex() == 1 ) {	//"custom"
				filter->setAutoModeEnabled(false);
				m_binaryOptionsWidget->applyFilterSettings(filter);
			} else {
				//TODO: load filter settings
// 			filter->setFilterName( ui.cbFilter->currentText() );
			}

			filter->setStartRow( ui.sbStartRow->value() );
			filter->setEndRow( ui.sbEndRow->value() );

			return filter;
		}
	case LiveDataSource::Image: {
			ImageFilter* filter = new ImageFilter();

			filter->setImportFormat(m_imageOptionsWidget->currentFormat());
			filter->setStartRow( ui.sbStartRow->value() );
			filter->setEndRow( ui.sbEndRow->value() );
			filter->setStartColumn( ui.sbStartColumn->value() );
			filter->setEndColumn( ui.sbEndColumn->value() );

			return filter;
		}
	case LiveDataSource::HDF: {
			HDFFilter* filter = new HDFFilter();
			QStringList names = selectedHDFNames();
			if (!names.isEmpty())
				filter->setCurrentDataSetName(names[0]);
			filter->setStartRow( ui.sbStartRow->value() );
			filter->setEndRow( ui.sbEndRow->value() );
			filter->setStartColumn( ui.sbStartColumn->value() );
			filter->setEndColumn( ui.sbEndColumn->value() );

			return filter;
		}
	case LiveDataSource::NETCDF: {
			NetCDFFilter* filter = new NetCDFFilter();

			if (!selectedNetCDFNames().isEmpty())
				filter->setCurrentVarName(selectedNetCDFNames()[0]);
			filter->setStartRow( ui.sbStartRow->value() );
			filter->setEndRow( ui.sbEndRow->value() );
			filter->setStartColumn( ui.sbStartColumn->value() );
			filter->setEndColumn( ui.sbEndColumn->value() );

			return filter;
		}
	case LiveDataSource::FITS: {
			FITSFilter* filter = new FITSFilter();
			filter->setStartRow( ui.sbStartRow->value());
			filter->setEndRow( ui.sbEndRow->value() );
			filter->setStartColumn( ui.sbStartColumn->value());
			filter->setEndColumn( ui.sbEndColumn->value());
			return filter;
		}
	}

	return 0;
}

/*!
	opens a file dialog and lets the user select the file data source.
*/
void ImportFileWidget::selectFile() {
	KConfigGroup conf(KSharedConfig::openConfig(), "ImportFileWidget");
	QString dir = conf.readEntry("LastDir", "");
	QString path = QFileDialog::getOpenFileName(this, i18n("Select the File Data Source"), dir);
	if (path.isEmpty())
		return; //cancel was clicked in the file-dialog

	int pos = path.lastIndexOf(QDir::separator());
	if (pos != -1) {
		QString newDir = path.left(pos);
		if (newDir != dir)
			conf.writeEntry("LastDir", newDir);
	}

	ui.kleFileName->setText(path);

	//TODO: decide whether the selection of several files should be possible
// 	QStringList filelist = QFileDialog::getOpenFileNames(this,i18n("Select one or more files to open"));
// 	if (! filelist.isEmpty() )
// 		ui.kleFileName->setText(filelist.join(";"));
}

/************** SLOTS **************************************************************/

/*!
	called on file name changes.
	Determines the file format (ASCII, binary etc.), if the file exists,
	and activates the corresponding options.
*/
void ImportFileWidget::fileNameChanged(const QString& name) {
	QString fileName = name;
#ifndef HAVE_WINDOWS
	// make relative path
	if ( !fileName.isEmpty() && fileName.left(1) != QDir::separator())
		fileName = QDir::homePath() + QDir::separator() + fileName;
#endif

	bool fileExists = QFile::exists(fileName);
	if (fileExists)
		ui.kleFileName->setStyleSheet("");
	else
		ui.kleFileName->setStyleSheet("QLineEdit{background:red;}");

	ui.gbOptions->setEnabled(fileExists);
	ui.bFileInfo->setEnabled(fileExists);
	ui.cbFileType->setEnabled(fileExists);
	ui.cbFilter->setEnabled(fileExists);
	ui.bManageFilters->setEnabled(fileExists);
	ui.chbLinkFile->setEnabled(fileExists);
	if (!fileExists) {
		//file doesn't exist -> delete the content preview that is still potentially
		//available from the previously selected file
		ui.tePreview->clear();
		m_twPreview->clear();
		m_hdfOptionsWidget->clear();
		m_netcdfOptionsWidget->clear();
		m_fitsOptionsWidget->clear();

		emit fileNameChanged();
		return;
	}

	if (currentSourceType() == LiveDataSource::FileOrPipe) {
		QString fileInfo;
#ifndef HAVE_WINDOWS
		//check, if we can guess the file type by content
		QProcess* proc = new QProcess(this);
		QStringList args;
		args << "-b" << ui.kleFileName->text();
		proc->start("file", args);
		if (proc->waitForReadyRead(1000) == false) {
			QDEBUG("ERROR: reading file type of file" << fileName);
			return;
		}
		fileInfo = proc->readLine();
#endif

		QByteArray imageFormat = QImageReader::imageFormat(fileName);
		if (fileInfo.contains(QLatin1String("compressed data")) || fileInfo.contains(QLatin1String("ASCII")) ||
		        fileName.endsWith(QLatin1String("dat"), Qt::CaseInsensitive) || fileName.endsWith(QLatin1String("txt"), Qt::CaseInsensitive)) {
			//probably ascii data
			ui.cbFileType->setCurrentIndex(LiveDataSource::Ascii);
		} else if (fileInfo.contains(QLatin1String("Hierarchical Data Format")) || fileName.endsWith(QLatin1String("h5"), Qt::CaseInsensitive) ||
		           fileName.endsWith(QLatin1String("hdf"), Qt::CaseInsensitive) || fileName.endsWith(QLatin1String("hdf5"), Qt::CaseInsensitive) ) {
			ui.cbFileType->setCurrentIndex(LiveDataSource::HDF);

			// update HDF tree widget using current selected file
			m_hdfOptionsWidget->updateContent((HDFFilter*)this->currentFileFilter(), fileName);
		} else if (fileInfo.contains(QLatin1String("NetCDF Data Format")) || fileName.endsWith(QLatin1String("nc"), Qt::CaseInsensitive) ||
		           fileName.endsWith(QLatin1String("netcdf"), Qt::CaseInsensitive) || fileName.endsWith(QLatin1String("cdf"), Qt::CaseInsensitive)) {
			ui.cbFileType->setCurrentIndex(LiveDataSource::NETCDF);

			// update NetCDF tree widget using current selected file
			m_netcdfOptionsWidget->updateContent((NetCDFFilter*)this->currentFileFilter(), fileName);
		} else if (fileInfo.contains(QLatin1String("FITS image data")) || fileName.endsWith(QLatin1String("fits"), Qt::CaseInsensitive) ||
		           fileName.endsWith(QLatin1String("fit"), Qt::CaseInsensitive) || fileName.endsWith(QLatin1String("fts"), Qt::CaseInsensitive)) {
#ifdef HAVE_FITS
			ui.cbFileType->setCurrentIndex(LiveDataSource::FITS);
#endif

			// update FITS tree widget using current selected file
			m_fitsOptionsWidget->updateContent((FITSFilter*)this->currentFileFilter(), fileName);
		} else if (fileInfo.contains("image") || fileInfo.contains("bitmap") || !imageFormat.isEmpty())
			ui.cbFileType->setCurrentIndex(LiveDataSource::Image);
		else
			ui.cbFileType->setCurrentIndex(LiveDataSource::Binary);
	}

	refreshPreview();
	emit fileNameChanged();
}

/*!
  saves the current filter settings
*/
void ImportFileWidget::saveFilter() {
	bool ok;
	QString text = QInputDialog::getText(this, i18n("Save Filter Settings as"),
	                                     i18n("Filter name:"), QLineEdit::Normal, i18n("new filter"), &ok);
	if (ok && !text.isEmpty()) {
		//TODO
		//AsciiFilter::saveFilter()
	}
}

/*!
  opens a dialog for managing all available predefined filters.
*/
void ImportFileWidget::manageFilters() {
	//TODO
}

/*!
	Depending on the selected file type, activates the corresponding options in the data portion tab
	and populates the combobox with the available pre-defined fllter settings for the selected type.
*/
void ImportFileWidget::fileTypeChanged(int fileType) {
	ui.swOptions->setCurrentIndex(fileType);

	//default
	ui.lFilter->show();
	ui.cbFilter->show();

	//if we switch from netCDF-format (only two tabs available), add the data preview-tab again
	if (ui.tabWidget->count() == 2) {
		ui.tabWidget->setTabText(0, i18n("Data format"));
		ui.tabWidget->insertTab(1, ui.tabDataPreview, i18n("Preview"));
	}
	ui.lPreviewLines->show();
	ui.sbPreviewLines->show();
	ui.lStartColumn->show();
	ui.sbStartColumn->show();
	ui.lEndColumn->show();
	ui.sbEndColumn->show();

	switch (fileType) {
	case LiveDataSource::Ascii:
		break;
	case LiveDataSource::Binary:
		ui.lStartColumn->hide();
		ui.sbStartColumn->hide();
		ui.lEndColumn->hide();
		ui.sbEndColumn->hide();
		break;
	case LiveDataSource::HDF:
	case LiveDataSource::NETCDF:
		ui.lFilter->hide();
		ui.cbFilter->hide();
		// hide global preview tab. we have our own
		ui.tabWidget->setTabText(0, i18n("Data format && preview"));
		ui.tabWidget->removeTab(1);
		ui.tabWidget->setCurrentIndex(0);
		break;
	case LiveDataSource::Image:
		ui.lPreviewLines->hide();
		ui.sbPreviewLines->hide();
		ui.lFilter->hide();
		ui.cbFilter->hide();
		break;
	case LiveDataSource::FITS:
		ui.lFilter->hide();
		ui.cbFilter->hide();
		ui.tabWidget->setTabText(0, i18n("Data format && preview"));
		ui.tabWidget->removeTab(1);
		ui.tabWidget->setCurrentIndex(0);
		break;
	default:
		DEBUG("unknown file type");
	}

	m_hdfOptionsWidget->clear();
	m_netcdfOptionsWidget->clear();

	int lastUsedFilterIndex = ui.cbFilter->currentIndex();
	ui.cbFilter->clear();
	ui.cbFilter->addItem( i18n("Automatic") );
	ui.cbFilter->addItem( i18n("Custom") );

	//TODO: populate the combobox with the available pre-defined filter settings for the selected type
	ui.cbFilter->setCurrentIndex(lastUsedFilterIndex);
	filterChanged(lastUsedFilterIndex);

	refreshPreview();
}


const QStringList ImportFileWidget::selectedHDFNames() const {
	return m_hdfOptionsWidget->selectedHDFNames();
}

const QStringList ImportFileWidget::selectedNetCDFNames() const {
	return m_netcdfOptionsWidget->selectedNetCDFNames();
}

const QStringList ImportFileWidget::selectedFITSExtensions() const {
	return m_fitsOptionsWidget->selectedFITSExtensions();
}

/*!
	shows the dialog with the information about the file(s) to be imported.
*/
void ImportFileWidget::fileInfoDialog() {
	QStringList files = ui.kleFileName->text().split(';');
	FileInfoDialog* dlg = new FileInfoDialog(this);
	dlg->setFiles(files);
	dlg->exec();
}

/*!
	enables the options if the filter "custom" was chosen. Disables the options otherwise.
*/
void ImportFileWidget::filterChanged(int index) {
	// ignore filter for these formats
	if (ui.cbFileType->currentIndex() == LiveDataSource::HDF || ui.cbFileType->currentIndex() == LiveDataSource::NETCDF
	        || ui.cbFileType->currentIndex() == LiveDataSource::Image || ui.cbFileType->currentIndex() == LiveDataSource::FITS) {
		ui.swOptions->setEnabled(true);
		return;
	}

	if (index == 0) { // "automatic"
		ui.swOptions->setEnabled(false);
		ui.bSaveFilter->setEnabled(false);
	} else if (index == 1) { //custom
		ui.swOptions->setEnabled(true);
		ui.bSaveFilter->setEnabled(true);
	} else {
		// predefined filter settings were selected.
		//load and show them in the GUI.
		//TODO
	}
}

void ImportFileWidget::refreshPreview() {
	DEBUG("refreshPreview()");
	WAIT_CURSOR;

	QString fileName = ui.kleFileName->text();
#ifndef HAVE_WINDOWS
	if (fileName.left(1) != QDir::separator())
		fileName = QDir::homePath() + QDir::separator() + fileName;
#endif

	QVector<QStringList> importedStrings;
	LiveDataSource::FileType fileType = (LiveDataSource::FileType)ui.cbFileType->currentIndex();

	// generic table widget
	if (fileType == LiveDataSource::Ascii || fileType == LiveDataSource::Binary)
		m_twPreview->show();
	else
		m_twPreview->hide();

	int lines = ui.sbPreviewLines->value();

	bool ok = true;
	QTableWidget *tmpTableWidget = nullptr;
	QStringList vectorNameList;
	QVector<AbstractColumn::ColumnMode> columnModes;
	switch (fileType) {
	case LiveDataSource::Ascii: {
			ui.tePreview->clear();

			AsciiFilter *filter = static_cast<AsciiFilter*>(this->currentFileFilter());

			switch (currentSourceType()) {
			case LiveDataSource::SourceType::FileOrPipe: {
					importedStrings = filter->preview(fileName, lines);
					break;
				}
			case LiveDataSource::SourceType::LocalSocket: {

					QLocalSocket* lsocket = new QLocalSocket(this);
					lsocket->connectToServer(fileName, QLocalSocket::ReadOnly);
					bool localSocketConnected = lsocket->waitForConnected(2000);

					if (localSocketConnected) {
						qDebug() << "localPreviewConnected";
						bool canread = lsocket->waitForReadyRead(500);
						if (canread)
							importedStrings = filter->preview(*lsocket);
					}
					if (lsocket->state() == QLocalSocket::ConnectedState) {
						lsocket->disconnectFromServer();
						connect(lsocket, SIGNAL(disconnected()), lsocket, SLOT(deleteLater()));
					} else
						delete lsocket;

					break;
				}
			case LiveDataSource::SourceType::NetworkTcpSocket: {
					QTcpSocket* tsocket = new QTcpSocket(this);
					tsocket->connectToHost(host(), port().toInt(), QTcpSocket::ReadOnly);
					bool tcpSocketConnected = tsocket->waitForConnected(2000);

					if (tcpSocketConnected) {
						bool canread = tsocket->waitForReadyRead(500);
						qDebug() << "tcpPreviewconnected";
						if (canread)
							importedStrings = filter->preview(*tsocket);
					}

					if (tsocket->state() == QTcpSocket::ConnectedState) {
						tsocket->disconnectFromHost();
						connect(tsocket, SIGNAL(disconnected()), tsocket, SLOT(deleteLater()));
					} else
						delete tsocket;

					break;
				}
			case LiveDataSource::SourceType::NetworkUdpSocket: {
					QUdpSocket* usocket = new QUdpSocket(this);
					usocket->connectToHost(host(), port().toInt(), QUdpSocket::ReadOnly);
					bool udpSocketConnected = usocket->waitForConnected(2000);

					if (udpSocketConnected) {
						bool canread = usocket->waitForReadyRead(500);
						qDebug() << "tcpPreviewconnected";
						if (canread)
							importedStrings = filter->preview(*usocket);
					}

					if (usocket->state() == QUdpSocket::ConnectedState) {
						usocket->disconnectFromHost();
						connect(usocket, SIGNAL(disconnected()), usocket, SLOT(deleteLater()));
					} else
						delete usocket;

					break;
				}
			case LiveDataSource::SourceType::SerialPort: {
					QSerialPort* sPort = new QSerialPort(this);

					sPort->setBaudRate(baudRate());
					sPort->setPortName(serialPort());

					bool serialPortOpened = sPort->open(QIODevice::ReadOnly);
					if (serialPortOpened) {

					}
					break;
				}
			}

			tmpTableWidget = m_twPreview;
			vectorNameList = filter->vectorNames();
			columnModes = filter->columnModes();
			break;
		}
	case LiveDataSource::Binary: {
			ui.tePreview->clear();
			BinaryFilter *filter = (BinaryFilter *)this->currentFileFilter();
			importedStrings = filter->preview(fileName, lines);
			tmpTableWidget = m_twPreview;
			break;
		}
	case LiveDataSource::Image: {
			ui.tePreview->clear();

			QImage image(fileName);
			QTextCursor cursor = ui.tePreview->textCursor();
			cursor.insertImage(image);
			RESET_CURSOR;
			return;
		}
	case LiveDataSource::HDF: {
			HDFFilter *filter = (HDFFilter *)this->currentFileFilter();
			lines = m_hdfOptionsWidget->lines();
			importedStrings = filter->readCurrentDataSet(fileName, NULL, ok, AbstractFileFilter::Replace, lines);
			tmpTableWidget = m_hdfOptionsWidget->previewWidget();
			break;
		}
	case LiveDataSource::NETCDF: {
			NetCDFFilter *filter = (NetCDFFilter *)this->currentFileFilter();
			lines = m_netcdfOptionsWidget->lines();
			importedStrings = filter->readCurrentVar(fileName, NULL, AbstractFileFilter::Replace, lines);
			tmpTableWidget = m_netcdfOptionsWidget->previewWidget();
			break;
		}
	case LiveDataSource::FITS: {
			FITSFilter* filter = (FITSFilter*)this->currentFileFilter();
			lines = m_fitsOptionsWidget->lines();

			QString extensionName = m_fitsOptionsWidget->extensionName(&ok);
			if (!extensionName.isEmpty())
				fileName = extensionName;

			bool readFitsTableToMatrix;
			importedStrings = filter->readChdu(fileName, &readFitsTableToMatrix, lines);
			emit checkedFitsTableToMatrix(readFitsTableToMatrix);

			tmpTableWidget = m_fitsOptionsWidget->previewWidget();
			break;
		}
	}

	// fill the table widget
	tmpTableWidget->setRowCount(0);
	tmpTableWidget->setColumnCount(0);
	if( !importedStrings.isEmpty() ) {
		QDEBUG("importedStrings =" << importedStrings);
		if (!ok) {
			// show imported strings as error message
			tmpTableWidget->setRowCount(1);
			tmpTableWidget->setColumnCount(1);
			QTableWidgetItem* item = new QTableWidgetItem();
			item->setText(importedStrings[0][0]);
			tmpTableWidget->setItem(0, 0, item);
		} else {
			//TODO: maxrows not used
			const int rows = qMax(importedStrings.size(), 1);
			const int maxColumns = 300;
			tmpTableWidget->setRowCount(rows);

			for (int i = 0; i < rows; i++) {
				QDEBUG(importedStrings[i]);

				int cols = importedStrings[i].size() > maxColumns ? maxColumns : importedStrings[i].size();	// new
				if (cols > tmpTableWidget->columnCount())
					tmpTableWidget->setColumnCount(cols);

				for (int j = 0; j < cols; j++) {
					QTableWidgetItem* item = new QTableWidgetItem(importedStrings[i][j]);
					tmpTableWidget->setItem(i, j, item);
				}
			}

			// set header if columnMode available
			for (int i = 0; i < qMin(tmpTableWidget->columnCount(), columnModes.size()); i++) {
				QString columnName = QString::number(i+1);
				if (i < vectorNameList.size())
					columnName = vectorNameList[i];
				auto* item = new QTableWidgetItem(columnName + QLatin1String(" {") + ENUM_TO_STRING(AbstractColumn, ColumnMode, columnModes[i]) + QLatin1String("}"));
				item->setTextAlignment(Qt::AlignLeft);
				item->setIcon(AbstractColumn::iconForMode(columnModes[i]));

				tmpTableWidget->setHorizontalHeaderItem(i, item);
			}
		}

		tmpTableWidget->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	}
	RESET_CURSOR;
}

void ImportFileWidget::updateTypeChanged(int idx) {
	LiveDataSource::UpdateType type = static_cast<LiveDataSource::UpdateType>(idx);

	if (type == LiveDataSource::UpdateType::TimeInterval) {
		ui.lUpdateInterval->show();
		ui.sbUpdateInterval->show();
		ui.lUpdateIntervalUnit->show();
	} else if (type == LiveDataSource::UpdateType::NewData) {
		ui.lUpdateInterval->hide();
		ui.sbUpdateInterval->hide();
		ui.lUpdateIntervalUnit->hide();
	}
}

void ImportFileWidget::readingTypeChanged(int idx) {
	LiveDataSource::ReadingType type = static_cast<LiveDataSource::ReadingType>(idx);

	if (type == LiveDataSource::ReadingType::TillEnd) {
		ui.lSampleRate->hide();
		ui.sbSampleRate->hide();
	} else {
		ui.lSampleRate->show();
		ui.sbSampleRate->show();
	}
}

void ImportFileWidget::sourceTypeChanged(int idx) {
	LiveDataSource::SourceType type = static_cast<LiveDataSource::SourceType>(idx);

	switch (type) {
	case LiveDataSource::SourceType::FileOrPipe:
		ui.lFileName->show();
		ui.kleFileName->show();
		ui.bFileInfo->show();
		ui.bOpen->show();

		ui.cbBaudRate->hide();
		ui.lBaudRate->hide();
		ui.lHost->hide();
		ui.leHost->hide();
		ui.lPort->hide();
		ui.lePort->hide();
		ui.cbSerialPort->hide();
		ui.lSerialPort->hide();
		break;
	case LiveDataSource::SourceType::LocalSocket:
		ui.lFileName->show();
		ui.kleFileName->show();
		ui.bOpen->show();

		ui.bFileInfo->hide();
		ui.cbBaudRate->hide();
		ui.lBaudRate->hide();
		ui.lHost->hide();
		ui.leHost->hide();
		ui.lPort->hide();
		ui.lePort->hide();
		ui.cbSerialPort->hide();
		ui.lSerialPort->hide();
		break;
	case LiveDataSource::SourceType::NetworkTcpSocket:
	case LiveDataSource::SourceType::NetworkUdpSocket:
		ui.lHost->show();
		ui.leHost->show();
		ui.lePort->show();
		ui.lPort->show();

		ui.lBaudRate->hide();
		ui.cbBaudRate->hide();
		ui.lSerialPort->hide();
		ui.cbSerialPort->hide();

		ui.lFileName->hide();
		ui.kleFileName->hide();
		ui.bFileInfo->hide();
		ui.bOpen->hide();
		break;
	case LiveDataSource::SourceType::SerialPort:
		ui.lBaudRate->show();
		ui.cbBaudRate->show();
		ui.lSerialPort->show();
		ui.cbSerialPort->show();

		ui.lHost->hide();
		ui.leHost->hide();
		ui.lePort->hide();
		ui.lPort->hide();
		ui.lFileName->hide();
		ui.kleFileName->hide();
		ui.bFileInfo->hide();
		ui.bOpen->hide();
		break;
	default:
		break;
	}

	emit sourceTypeChanged();
}

void ImportFileWidget::initializeAndFillPortsAndBaudRates() {
	for (int i = 2; i < ui.swOptions->count(); ++i)
		ui.swOptions->removeWidget(ui.swOptions->widget(i));

	const int size = ui.cbFileType->count();
	for (int i = 2; i < size; ++i)
		ui.cbFileType->removeItem(2);

	ui.cbBaudRate->hide();
	ui.lBaudRate->hide();

	ui.lHost->hide();
	ui.leHost->hide();

	ui.lPort->hide();
	ui.lePort->hide();

	ui.cbSerialPort->hide();
	ui.lSerialPort->hide();

	ui.cbBaudRate->addItems(LiveDataSource::supportedBaudRates());
	ui.cbSerialPort->addItems(LiveDataSource::availablePorts());

	ui.leKeepLastValues->setValidator(new QIntValidator(2, 100000));
	ui.tabWidget->removeTab(2);
}
