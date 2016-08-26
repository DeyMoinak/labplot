#ifndef FITSFILTERPRIVATE_H
#define FITSFILTERPRIVATE_H

/***************************************************************************
File                 : FITSFilterPrivate.cpp
Project              : LabPlot
Description          : FITS I/O-filter
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
#ifdef HAVE_FITS
#include "fitsio.h"
#endif
class AbstractDataSource;

class FITSFilterPrivate {

public:
    explicit FITSFilterPrivate(FITSFilter*);
    ~FITSFilterPrivate();
    QString readCHDU(const QString & fileName, AbstractDataSource* dataSource,
                     AbstractFileFilter::ImportMode importMode = AbstractFileFilter::Replace, bool*okToMatrix = 0, int lines= -1);
    void writeCHDU(const QString & fileName, AbstractDataSource* dataSource);

    const FITSFilter* q;
    QMultiMap<QString, QString> extensionNames(const QString &fileName);
    void updateKeywords(const QString &fileName, const QList<FITSFilter::Keyword> &originals, const QVector<FITSFilter::Keyword> &updates);
    void addNewKeyword(const QString &fileName, const QList<FITSFilter::Keyword> &keywords);
    void addKeywordUnit(const QString& fileName, const QList<FITSFilter::Keyword> &keywords);
    void deleteKeyword(const QString &fileName, const QList<FITSFilter::Keyword>& keywords);
    void removeExtensions(const QStringList& extensions);
    const QString valueOf(const QString &fileName, const char* key);
    int imagesCount(const QString& fileName) ;
    int tablesCount(const QString& fileName) ;
    QList<FITSFilter::Keyword> chduKeywords(const QString &fileName);
    void parseHeader(const QString& fileName, QTableWidget* headerEditTable,
                     bool readKeys = true,
                     const QList<FITSFilter::Keyword> &keys = QList<FITSFilter::Keyword>());
    void parseExtensions(const QString& fileName, QTreeWidget *tw, bool checkPrimary = false);
    int startRow;
    int endRow;
    int startColumn;
    int endColumn;

    bool commentsAsUnits;
    int exportTo;
private:
    void printError(int status) const;

#ifdef HAVE_FITS
    fitsfile* fitsFile;
#endif

};

#endif // FITSFILTERPRIVATE_H
