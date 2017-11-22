/***************************************************************************
File                 : BinaryFilter.h
Project              : LabPlot
Description          : Binary I/O-filter
--------------------------------------------------------------------
Copyright            : (C) 2015-2017 Stefan Gerlach (stefan.gerlach@uni.kn)
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
#ifndef BINARYFILTER_H
#define BINARYFILTER_H

#include "backend/datasources/filters/AbstractFileFilter.h"

class BinaryFilterPrivate;
class QStringList;
class QIODevice;

class BinaryFilter : public AbstractFileFilter {
	Q_OBJECT
	Q_ENUMS(DataType)
	Q_ENUMS(ByteOrder)

  public:
	//TODO; use ColumnMode?
	enum DataType {INT8, INT16, INT32, INT64, UINT8, UINT16, UINT32, UINT64, REAL32, REAL64};
	enum ByteOrder {LittleEndian, BigEndian};

	BinaryFilter();
	~BinaryFilter();

	static QStringList dataTypes();
	static QStringList byteOrders();
	static int dataSize(BinaryFilter::DataType);
	static size_t rowNumber(const QString& fileName, const size_t vectors, const BinaryFilter::DataType);

	// read data from any device
	void readDataFromDevice(QIODevice&, AbstractDataSource* = nullptr,
		AbstractFileFilter::ImportMode = AbstractFileFilter::Replace, int lines = -1);
	QVector<QStringList> readDataFromFile(const QString& fileName, AbstractDataSource*,
					      AbstractFileFilter::ImportMode = AbstractFileFilter::Replace, int lines = -1);
	void write(const QString& fileName, AbstractDataSource*);
	QVector<QStringList> preview(const QString& fileName, int lines);

	void loadFilterSettings(const QString&);
	void saveFilterSettings(const QString&) const;

	void setVectors(const size_t);
	size_t vectors() const;

	void setDataType(const BinaryFilter::DataType);
	BinaryFilter::DataType dataType() const;
	void setByteOrder(const BinaryFilter::ByteOrder);
	BinaryFilter::ByteOrder byteOrder() const;
	void setSkipStartBytes(const size_t);
	size_t skipStartBytes() const;
	void setStartRow(const int);
	int startRow() const;
	void setEndRow(const int);
	int endRow() const;
	void setSkipBytes(const size_t);
	size_t skipBytes() const;
	void setCreateIndexEnabled(const bool);

	void setAutoModeEnabled(const bool);
	bool isAutoModeEnabled() const;

	virtual void save(QXmlStreamWriter*) const;
	virtual bool load(XmlStreamReader*);

  private:
	std::unique_ptr<BinaryFilterPrivate> const d;
	friend class BinaryFilterPrivate;
};

#endif
