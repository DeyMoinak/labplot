/***************************************************************************
    File                 : Datapicker.h
    Project              : LabPlot
    Description          : Aspect providing a container for storing image and data
                           in form of worksheet and spreadsheets
    --------------------------------------------------------------------
    Copyright            : (C) 2015 by Ankit Wagadre (wagadre.ankit@gmail.com)
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

#ifndef DATAPICKER_H
#define DATAPICKER_H

#include "backend/core/AbstractPart.h"
#include "backend/core/AbstractScriptingEngine.h"

class Spreadsheet;
class Image;
class QXmlStreamWriter;
class XmlStreamReader;

class Datapicker : public AbstractPart, public scripted {
    Q_OBJECT

    public:
        Datapicker(AbstractScriptingEngine* engine, const QString& name);

        void initDefault();
        virtual QIcon icon() const;
        virtual QMenu* createContextMenu();
        virtual QWidget* view() const;

        Image* m_image;
        Spreadsheet* currentSpreadsheet() const;
        Image* currentImage() const;
        void setChildSelectedInView(int index, bool selected);

        virtual void save(QXmlStreamWriter*) const;
        virtual bool load(XmlStreamReader*);

    public slots:
        virtual void childSelected(const AbstractAspect*);

    private slots:
        virtual void childDeselected(const AbstractAspect*);
        void handleChildAspectAboutToBeRemoved(const AbstractAspect*);
        void handleChildAspectAdded(const AbstractAspect*);
        void handleAspectAdded(const AbstractAspect*);
        void handleAspectAboutToBeRemoved(const AbstractAspect*);

    signals:
        void datapickerItemSelected(int);
        void childAspectAdded(const AbstractAspect*);
};

#endif
