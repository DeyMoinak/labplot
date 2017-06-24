/***************************************************************************
File                 : AsciiFilter.cpp
Project              : LabPlot
Description          : ASCII I/O-filter
--------------------------------------------------------------------
Copyright            : (C) 2009-2017 Stefan Gerlach (stefan.gerlach@uni.kn)
Copyright            : (C) 2009-2017 Alexander Semke (alexander.semke@web.de)

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
#include "backend/datasources/filters/AsciiFilter.h"
#include "backend/datasources/filters/AsciiFilterPrivate.h"
#include "backend/datasources/FileDataSource.h"
#include "backend/core/column/Column.h"
#include "backend/lib/macros.h"

#include <QTextStream>
#include <KLocale>
#include <KFilterDev>
#include <QElapsedTimer>
#include <QProcess>

 /*!
	\class AsciiFilter
	\brief Manages the import/export of data organized as columns (vectors) from/to an ASCII-file.

	\ingroup datasources
 */

AsciiFilter::AsciiFilter() : AbstractFileFilter(), d(new AsciiFilterPrivate(this)) {

}

AsciiFilter::~AsciiFilter() {
	delete d;
}

/*!
  reads the content of the file \c fileName.
*/
QVector<QStringList> AsciiFilter::readDataFromFile(const QString& fileName, AbstractDataSource* dataSource, AbstractFileFilter::ImportMode importMode, int lines) {
	return d->readDataFromFile(fileName, dataSource, importMode, lines);
}

/*!
  reads the content of the file \c fileName to the data source \c dataSource.
*/
//void AsciiFilter::read(const QString& fileName, AbstractDataSource* dataSource, AbstractFileFilter::ImportMode importMode) {
//	d->read(fileName, dataSource, importMode);
//}


/*!
writes the content of the data source \c dataSource to the file \c fileName.
*/
void AsciiFilter::write(const QString& fileName, AbstractDataSource* dataSource) {
 	d->write(fileName, dataSource);
// 	emit()
}

/*!
  loads the predefined filter settings for \c filterName
*/
void AsciiFilter::loadFilterSettings(const QString& filterName) {
	Q_UNUSED(filterName);
}

/*!
  saves the current settings as a new filter with the name \c filterName
*/
void AsciiFilter::saveFilterSettings(const QString& filterName) const {
	Q_UNUSED(filterName);
}

/*!
  returns the list with the names of all saved
  (system wide or user defined) filter settings.
*/
QStringList AsciiFilter::predefinedFilters() {
	return QStringList();
}

/*!
  returns the list of all predefined separator characters.
*/
QStringList AsciiFilter::separatorCharacters() {
	return (QStringList() << "auto" << "TAB" << "SPACE" << "," << ";" << ":"
		<< ",TAB" << ";TAB" << ":TAB" << ",SPACE" << ";SPACE" << ":SPACE");
}

/*!
returns the list of all predefined comment characters.
*/
QStringList AsciiFilter::commentCharacters() {
	return (QStringList() << "#" << "!" << "//" << "+" << "c" << ":" << ";");
}

/*!
    returns the number of columns in the file \c fileName.
*/
int AsciiFilter::columnNumber(const QString& fileName, const QString& separator) {
	KFilterDev device(fileName);
	if (!device.open(QIODevice::ReadOnly)) {
		DEBUG("Could not open file " << fileName.toStdString() << " for determining number of columns");
		return -1;
	}

	QString line = device.readLine();
	line.remove(QRegExp("[\\n\\r]"));

	QStringList lineStringList;
	if (separator.length() > 0)
		lineStringList = line.split(separator);
	else
		lineStringList = line.split(QRegExp("\\s+"));
	DEBUG("number of columns : " << lineStringList.size());

	return lineStringList.size();
}

size_t AsciiFilter::lineNumber(const QString& fileName) {
// wc is much faster
/*	KFilterDev device(fileName);
	if (!device.open(QIODevice::ReadOnly)) {
		DEBUG("Could not open file " << fileName.toStdString() << " for determining number of lines");
		return 0;
	}

	size_t rows = 0;
	while (!device.atEnd()) {
		device.readLine();
		rows++;
	}
*/
	QElapsedTimer myTimer;
	myTimer.start();
	QProcess wc;
	wc.start(QString("wc"), QStringList() << "-l" << fileName);
	size_t lineCount = 0;
	while (wc.waitForReadyRead())
		lineCount = wc.readLine().split(' ')[0].toInt();
	DEBUG(" Elapsed time counting lines : " << myTimer.elapsed() << " ms");

	return lineCount;
}

/*!
  returns the number of lines in the device \c device or 0 if not available
*/
size_t AsciiFilter::lineNumber(KFilterDev &device) {
	if (device.isSequential())
		return 0;

	size_t lineCount = 0;
	device.seek(0);
	while (!device.atEnd()) {
		device.readLine();
		lineCount++;
	}

	return lineCount;
}

void AsciiFilter::setTransposed(const bool b) {
	d->m_transposed = b;
}

bool AsciiFilter::isTransposed() const {
	return d->m_transposed;
}

void AsciiFilter::setCommentCharacter(const QString& s) {
	d->m_commentCharacter = s;
}

QString AsciiFilter::commentCharacter() const {
	return d->m_commentCharacter;
}

void AsciiFilter::setSeparatingCharacter(const QString& s) {
	d->m_separatingCharacter = s;
}

QString AsciiFilter::separatingCharacter() const {
	return d->m_separatingCharacter;
}

void AsciiFilter::setAutoModeEnabled(const bool b) {
	d->m_autoModeEnabled = b;
}

bool AsciiFilter::isAutoModeEnabled() const {
	return d->m_autoModeEnabled;
}

void AsciiFilter::setHeaderEnabled(const bool b) {
	d->m_headerEnabled = b;
}

bool AsciiFilter::isHeaderEnabled() const {
	return d->m_headerEnabled;
}

void AsciiFilter::setVectorNames(const QString s) {
	d->m_vectorNames = s.simplified();
}

QString AsciiFilter::vectorNames() const {
	return d->m_vectorNames;
}

void AsciiFilter::setSkipEmptyParts(const bool b) {
	d->m_skipEmptyParts = b;
}

bool AsciiFilter::skipEmptyParts() const {
	return d->m_skipEmptyParts;
}

void AsciiFilter::setSimplifyWhitespacesEnabled(bool b) {
	d->m_simplifyWhitespacesEnabled = b;
}

bool AsciiFilter::simplifyWhitespacesEnabled() const {
	return d->m_simplifyWhitespacesEnabled;
}

void AsciiFilter::setStartRow(const int r) {
	d->m_startRow = r;
}

int AsciiFilter::startRow() const {
	return d->m_startRow;
}

void AsciiFilter::setEndRow(const int r) {
	d->m_endRow = r;
}

int AsciiFilter::endRow() const {
	return d->m_endRow;
}

void AsciiFilter::setStartColumn(const int c) {
	d->m_startColumn = c;
}

int AsciiFilter::startColumn() const {
	return d->m_startColumn;
}

void AsciiFilter::setEndColumn(const int c) {
	d->m_endColumn = c;
}

int AsciiFilter::endColumn() const {
	return d->m_endColumn;
}

//#####################################################################
//################### Private implementation ##########################
//#####################################################################
AsciiFilterPrivate::AsciiFilterPrivate(AsciiFilter* owner) : q(owner),
	m_commentCharacter("#"),
	m_separatingCharacter("auto"),
	m_autoModeEnabled(true),
	m_headerEnabled(true),
	m_skipEmptyParts(false),
	m_simplifyWhitespacesEnabled(true),
	m_transposed(false),
	m_startRow(1),
	m_endRow(-1),
	m_startColumn(1),
	m_endColumn(-1) {
}

int AsciiFilterPrivate::prepareDeviceToRead(KFilterDev& device) {
	if (!device.open(QIODevice::ReadOnly))
		return -1;

	if (device.atEnd()) // empty file
		return 1;

	//TODO: implement ???
	// if (transposed) ...

	// Skip rows until start row (ignoring comment lines)
	DEBUG("Skipping " << m_startRow - 1 << " lines");
	for (int i = 0; i < m_startRow - 1; i++) {
		QString line = device.readLine();

		if (device.atEnd())
			return 1;
		if (line.startsWith(m_commentCharacter))	// ignore commented lines
			i--;
	}

	// Parse the first line:
	// Determine the number of columns, create the columns and use (if selected) the first row to name them
	QString firstLine;
	qint64 startPosition = device.pos();
	DEBUG(" device start position = " << startPosition);
	do {	// skip comment lines
		firstLine = device.readLine();
		if (device.atEnd())
			return 1;
	} while (firstLine.startsWith(m_commentCharacter));
	DEBUG(" device position after first line and comments = " << device.pos());

	firstLine.remove(QRegExp("[\\n\\r]"));	// remove any newline
	if (m_simplifyWhitespacesEnabled)
		firstLine = firstLine.simplified();
	DEBUG("First line: \'" << firstLine.toStdString() << '\'');

	// determine separator and split first line
	QStringList firstLineStringList;
	if (m_separatingCharacter == "auto") {
		DEBUG("automatic separator");
		QRegExp regExp("(\\s+)|(,\\s+)|(;\\s+)|(:\\s+)");
		firstLineStringList = firstLine.split(regExp, QString::SkipEmptyParts);

		if (!firstLineStringList.isEmpty()) {
			int length1 = firstLineStringList.at(0).length();
			if (firstLineStringList.size() > 1) {
				int pos2 = firstLine.indexOf(firstLineStringList.at(1), length1);
				m_separator = firstLine.mid(length1, pos2 - length1);
			} else {
				//old: separator = line.right(line.length() - length1);
				m_separator = ' ';
			}
		}
	} else {	// use given separator
		m_separator = m_separatingCharacter.replace(QLatin1String("TAB"), QLatin1String(" "), Qt::CaseInsensitive);
		m_separator = m_separator.replace(QLatin1String("SPACE"), QLatin1String(" "), Qt::CaseInsensitive);
		firstLineStringList = firstLine.split(m_separator, QString::SkipEmptyParts);
	}
	DEBUG("separator: \'" << m_separator.toStdString() << '\'');
	DEBUG("number of columns: " << firstLineStringList.size());
	DEBUG("headerEnabled = " << m_headerEnabled);

	if (m_headerEnabled) {	// use first line to name vectors
		m_vectorNameList = firstLineStringList;
	} else {
		// create vector names out of the space separated vectorNames-string, if not empty
		if (!m_vectorNames.isEmpty())
			m_vectorNameList = m_vectorNames.split(' ');
	}
	//qDebug()<<"	vector names ="<<vectorNameList;

	// set range to read
	if (m_endColumn == -1)
		m_endColumn = firstLineStringList.size(); // last column
	m_actualCols = m_endColumn - m_startColumn + 1;

//TEST: readline-seek-readline fails
/*	qint64 testpos = device.pos();
	DEBUG("read data line @ pos " << testpos << " : " << device.readLine().toStdString());
	device.seek(testpos);
	testpos = device.pos();
	DEBUG("read data line again @ pos " << testpos << "  = " << device.readLine().toStdString());
*/
	m_actualRows = AsciiFilter::lineNumber(device);
	if (!device.seek(0)) {	// readLine() fails after seek(pos) if pos > 0! start form 0 again.
		DEBUG("Could not undo reading first line");
		return -1;
	}
	int actualEndRow = m_endRow;
	if (m_endRow == -1 || m_endRow > m_actualRows)
		actualEndRow = m_actualRows;

	m_actualRows = actualEndRow - m_startRow + 1;

	if (m_headerEnabled) {
		m_actualRows--;
		device.readLine();
	}

	DEBUG("start/end column: " << m_startColumn << ' ' << m_endColumn);
	DEBUG("start/end row: " << m_startRow << ' ' << actualEndRow);
	DEBUG("actual cols/rows: " << m_actualCols << ' ' << m_actualRows);

	if (m_actualRows == 0)
		return 1;

	return 0;
}
// special function for reading data from file

/*!
    reads the content of the file \c fileName to the data source \c dataSource (if given) or return "lines" rows as string list for preview.
    Uses the settings defined in the data source (if given).
*/
QVector<QStringList> AsciiFilterPrivate::readDataFromFile(const QString& fileName, AbstractDataSource* dataSource, AbstractFileFilter::ImportMode importMode, int lines) {
	DEBUG("AsciiFilterPrivate::readData(): fileName = \'" << fileName.toStdString() << "\', dataSource = " << dataSource
		<< ", mode = " << ENUM_TO_STRING(AbstractFileFilter, ImportMode, importMode) << ", lines = " << lines);
	QVector<QStringList> dataStrings;

	// TODO: also support other devices. Add parameter for input device type?
	KFilterDev device(fileName);
	DEBUG("device is sequential = " << device.isSequential());
	int deviceError = prepareDeviceToRead(device);
	DEBUG("Device error = " << deviceError);

	if (deviceError == 1 && importMode == AbstractFileFilter::Replace && dataSource)
		dataSource->clear();
	if (deviceError)
		return dataStrings;

	int columnOffset = 0;	// indexes the "start column" in the datasource. Data will be imported starting from this column.
	QVector<void*> dataContainer;	// pointers to the actual data containers
	if (dataSource)
		columnOffset = dataSource->prepareImport(dataContainer, importMode, m_actualRows, m_actualCols, m_vectorNameList);

	// Read the data
	int currentRow = 0;	// indexes the position in the vector(column)
	if (lines == -1)
		lines = m_actualRows;

	DEBUG("reading " << qMin(lines, m_actualRows)  << " lines");
	DEBUG("device position before reading = " << device.pos());
	for (int i = 0; i < qMin(lines, m_actualRows); i++) {
		QString line = device.readLine();
		if (m_simplifyWhitespacesEnabled)
			line = line.simplified();
//		DEBUG("simplified line = " << line.toStdString());

		if (line.isEmpty() || line.startsWith(m_commentCharacter)) // skip empty or commented lines
			continue;

		QStringList lineStringList = line.split(m_separator, QString::SkipEmptyParts);
		QStringList lineString;
//		QDEBUG("split line = " << lineStringList);
		for (int n = 0; n < m_actualCols; n++) {
			if (n < lineStringList.size()) {
				bool isNumber;
				// TODO : read other data types (strings, datetime, etc.) too
				const double value = lineStringList.at(n).toDouble(&isNumber);
				if (dataSource)
					static_cast<QVector<double>*>(dataContainer[n])->operator[](currentRow) = (isNumber ? value : NAN);
				else
					lineString += (isNumber ? QString::number(value) : QString("NAN"));
			} else {

				if (dataSource)
					static_cast<QVector<double>*>(dataContainer[n])->operator[](currentRow) = NAN;
				else
					lineString += QLatin1String("NAN");
			}
		}

		dataStrings << lineString;
		currentRow++;
		emit q->completed(100 * currentRow/m_actualRows);
	}

	if (!dataSource)
		return dataStrings;

	// set the comments for each of the columns if datasource is a spreadsheet
	// TODO: make everything undo/redo-able again
	Spreadsheet* spreadsheet = dynamic_cast<Spreadsheet*>(dataSource);
	if (spreadsheet) {
		const int rows = (m_headerEnabled ? currentRow : currentRow + 1);
		//TODO: generalize to different data types
		QString comment = i18np("numerical data, %1 element", "numerical data, %1 elements", rows);
		for (int n = m_startColumn; n <= m_endColumn; n++) {
			Column* column = spreadsheet->column(columnOffset + n - m_startColumn);
			column->setComment(comment);
			if (importMode == AbstractFileFilter::Replace) {
				column->setSuppressDataChangedSignal(false);
				column->setChanged();
			}
		}
	}

	dataSource->finalizeImport();
	return dataStrings;
}

/*!
    reads the content of the file \c fileName to the data source \c dataSource.
*/
//void AsciiFilterPrivate::read(const QString& fileName, AbstractDataSource* dataSource, AbstractFileFilter::ImportMode mode) {
//	readData(fileName, dataSource, mode);
//}

/*!
    writes the content of \c dataSource to the file \c fileName.
*/
void AsciiFilterPrivate::write(const QString & fileName, AbstractDataSource* dataSource) {
	Q_UNUSED(fileName);
	Q_UNUSED(dataSource);
	//TODO
}

//##############################################################################
//##################  Serialization/Deserialization  ###########################
//##############################################################################
/*!
  Saves as XML.
 */
void AsciiFilter::save(QXmlStreamWriter* writer) const {
	writer->writeStartElement( "asciiFilter" );
	writer->writeAttribute( "commentCharacter", d->m_commentCharacter );
	writer->writeAttribute( "separatingCharacter", d->m_separatingCharacter );
	writer->writeAttribute( "autoMode", QString::number(d->m_autoModeEnabled) );
	writer->writeAttribute( "header", QString::number(d->m_headerEnabled) );
	writer->writeAttribute( "vectorNames", d->m_vectorNames );
	writer->writeAttribute( "skipEmptyParts", QString::number(d->m_skipEmptyParts) );
	writer->writeAttribute( "simplifyWhitespaces", QString::number(d->m_simplifyWhitespacesEnabled) );
	writer->writeAttribute( "transposed", QString::number(d->m_transposed) );
	writer->writeAttribute( "startRow", QString::number(d->m_startRow) );
	writer->writeAttribute( "endRow", QString::number(d->m_endRow) );
	writer->writeAttribute( "startColumn", QString::number(d->m_startColumn) );
	writer->writeAttribute( "endColumn", QString::number(d->m_endColumn) );
	writer->writeEndElement();
}

/*!
  Loads from XML.
*/
bool AsciiFilter::load(XmlStreamReader* reader) {
	if (!reader->isStartElement() || reader->name() != "asciiFilter") {
		reader->raiseError(i18n("no ascii filter element found"));
		return false;
	}

	QString attributeWarning = i18n("Attribute '%1' missing or empty, default value is used");
	QXmlStreamAttributes attribs = reader->attributes();

	QString str = attribs.value("commentCharacter").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'commentCharacter'"));
	else
		d->m_commentCharacter = str;

	str = attribs.value("separatingCharacter").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'separatingCharacter'"));
	else
		d->m_separatingCharacter = str;

	str = attribs.value("autoMode").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'autoMode'"));
	else
		d->m_autoModeEnabled = str.toInt();

	str = attribs.value("header").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'header'"));
	else
		d->m_headerEnabled = str.toInt();

	str = attribs.value("vectorNames").toString();
	d->m_vectorNames = str; //may be empty

	str = attribs.value("simplifyWhitespaces").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'simplifyWhitespaces'"));
	else
		d->m_simplifyWhitespacesEnabled = str.toInt();

	str = attribs.value("skipEmptyParts").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'skipEmptyParts'"));
	else
		d->m_skipEmptyParts = str.toInt();

	str = attribs.value("transposed").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'transposed'"));
	else
		d->m_transposed = str.toInt();

	str = attribs.value("startRow").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'startRow'"));
	else
		d->m_startRow = str.toInt();

	str = attribs.value("endRow").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'endRow'"));
	else
		d->m_endRow = str.toInt();

	str = attribs.value("startColumn").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'startColumn'"));
	else
		d->m_startColumn = str.toInt();

	str = attribs.value("endColumn").toString();
	if (str.isEmpty())
		reader->raiseWarning(attributeWarning.arg("'endColumn'"));
	else
		d->m_endColumn = str.toInt();

	return true;
}
