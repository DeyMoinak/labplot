/***************************************************************************
File                 : NetCDFFilter.h
Project              : LabPlot
Description          : NetCDF I/O-filter
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
#ifndef NETCDFFILTER_H
#define NETCDFFILTER_H

#include <QStringList>
#include <QTreeWidgetItem>
#include "backend/datasources/filters/AbstractFileFilter.h"

class NetCDFFilterPrivate;
class NetCDFFilter : public AbstractFileFilter{
	Q_OBJECT

  public:

	NetCDFFilter();
	~NetCDFFilter();

	void parse(const QString & fileName, QTreeWidgetItem* rootItem);
	void read(const QString & fileName, AbstractDataSource* dataSource, AbstractFileFilter::ImportMode importMode=AbstractFileFilter::Replace);
	QString readAttribute(const QString & fileName, const QString & name, const QString & varName);
	QList<QStringList> readCurrentVar(const QString & fileName, AbstractDataSource* dataSource, AbstractFileFilter::ImportMode importMode=AbstractFileFilter::Replace, int lines=-1);
	void write(const QString & fileName, AbstractDataSource* dataSource);

	void loadFilterSettings(const QString&);
	void saveFilterSettings(const QString&) const;

	void setCurrentVarName(const QString);
	const QString currentVarName() const;

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
	NetCDFFilterPrivate* const d;
	friend class NetCDFFilterPrivate;
};

#endif
