/***************************************************************************
File                 : ImageFilter.h
Project              : LabPlot
Description          : Image I/O-filter
--------------------------------------------------------------------
Copyright            : (C) 2015 Stefan Gerlach (stefan.gerlach@uni.kn)
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
#ifndef IMAGEFILTER_H
#define IMAGEFILTER_H

#include "backend/datasources/filters/AbstractFileFilter.h"

class ImageFilterPrivate;
class QStringList;

class ImageFilter : public AbstractFileFilter {
	Q_OBJECT
	Q_ENUMS(ImportFormat)

public:
	enum ImportFormat {MATRIX,XYZ,XYRGB};

	ImageFilter();
	~ImageFilter();

	static QStringList importFormats();

	QVector<QStringList> readDataFromFile(const QString& fileName, AbstractDataSource* = nullptr,
					      AbstractFileFilter::ImportMode = AbstractFileFilter::Replace, int lines = -1);
	void write(const QString& fileName, AbstractDataSource*);

	void loadFilterSettings(const QString&);
	void saveFilterSettings(const QString&) const;

	void setImportFormat(const ImageFilter::ImportFormat);
	ImageFilter::ImportFormat importFormat() const;

	void setStartRow(const int);
	int startRow() const;
	void setEndRow(const int);
	int endRow() const;
	void setStartColumn(const int);
	int startColumn() const;
	void setEndColumn(const int);
	int endColumn() const;

	virtual void save(QXmlStreamWriter*) const;
	virtual bool load(XmlStreamReader*);

private:
	std::unique_ptr<ImageFilterPrivate> const d;
	friend class ImageFilterPrivate;
};

#endif
