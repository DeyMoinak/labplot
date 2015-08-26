/***************************************************************************
    File                 : DataPickerCurve.cpp
    Project              : LabPlot
    Description          : container for Curve-Point and Datasheet/Spreadsheet
                           of datapicker
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

#include "DataPickerCurve.h"
#include "backend/lib/XmlStreamReader.h"
#include "backend/datapicker/CustomItem.h"
#include "backend/datapicker/Transform.h"
#include "backend/datapicker/Datapicker.h"
#include "backend/spreadsheet/Spreadsheet.h"
#include "backend/datapicker/DataPickerCurvePrivate.h"

#include <QMenu>

#include <KLocale>
#include <KIcon>
#include <KAction>
#include <KConfigGroup>

/**
 * \class DataPickerCurve
 * \brief Top-level container for Curve-Point and Datasheet/Spreadsheet of datapicker.
 * \ingroup backend
 */

DataPickerCurve::DataPickerCurve(const QString &name) : AbstractAspect(name), d_ptr(new DataPickerCurvePrivate()) {

    init();
}

DataPickerCurve::DataPickerCurve(const QString &name, DataPickerCurvePrivate *dd) : AbstractAspect(name), d_ptr(dd) {

    init();
}

DataPickerCurve::~DataPickerCurve() {
}

void DataPickerCurve::init() {
    Q_D(DataPickerCurve);

    KConfig config;
    KConfigGroup group;
    group = config.group("DataPickerCurve");
    d->posXColumn = NULL;
    d->posYColumn = NULL;
    d->plusDeltaXColumn = NULL;
    d->minusDeltaXColumn = NULL;
    d->plusDeltaYColumn = NULL;
    d->minusDeltaYColumn = NULL;
    d->curveErrorTypes.x = (Image::ErrorType) group.readEntry("CurveErrorType_X", (int) Image::NoError);
    d->curveErrorTypes.y = (Image::ErrorType) group.readEntry("CurveErrorType_X", (int) Image::NoError);
    d->visible = group.readEntry("Visibility", true);

    this->initAction();
}

void DataPickerCurve::initAction() {
    visibilityAction = new QAction(i18n("visible"), this);
    visibilityAction->setCheckable(true);
    connect( visibilityAction, SIGNAL(triggered()), this, SLOT(visibilityChanged()) );

    updateDatasheetAction = new KAction(KIcon("view-refresh"), i18n("Update Spreadsheet"), this);
    connect( updateDatasheetAction, SIGNAL(triggered()), this, SLOT(updateDatasheet()) );
}

/*!
    Returns an icon to be used in the project explorer.
*/
QIcon DataPickerCurve::icon() const {
    return  KIcon("xy-curve");
}

/*!
    Return a new context menu
*/
QMenu* DataPickerCurve::createContextMenu() {
    QMenu *menu = AbstractAspect::createContextMenu();
    Q_ASSERT(menu);

    QAction* firstAction = 0;
    if (menu->actions().size()>1)
        firstAction = menu->actions().at(1);

    visibilityAction->setChecked(visible());
    menu->insertAction(firstAction, visibilityAction);
    menu->insertAction(firstAction, updateDatasheetAction);

    return menu;
}

Column* DataPickerCurve::appendColumn(const QString& name, Spreadsheet* datasheet) {
    Column* col = new Column(i18n("Column"), AbstractColumn::Numeric);
    col->setPlotDesignation(AbstractColumn::Y);
    col->insertRows(0, datasheet->rowCount());
    col->setName(name);
    datasheet->addChild(col);

    return col;
}

void DataPickerCurve::addCustomItem(const QPointF& position) {
    QList<CustomItem*> childItems = children<CustomItem>(IncludeHidden);

    CustomItem* newItem = new CustomItem(i18n("Curve Point"));
    newItem->setPosition(position);
    newItem->setHidden(true);
    newItem->initErrorBar(curveErrorTypes());

    //set properties of added custom-item same as previous items
    if (!childItems.isEmpty()) {
        CustomItem* m_item = childItems.first();
        newItem->setUndoAware(false);
        newItem->setItemsBrush(m_item->itemsBrush());
        newItem->setItemsOpacity(m_item->itemsOpacity());
        newItem->setItemsPen(m_item->itemsPen());
        newItem->setItemsRotationAngle(m_item->itemsRotationAngle());
        newItem->setItemsSize(m_item->itemsSize());
        newItem->setItemsStyle(m_item->itemsStyle());
        newItem->setErrorBarBrush(m_item->errorBarBrush());
        newItem->setErrorBarSize(m_item->errorBarSize());
        newItem->setErrorBarPen(m_item->errorBarPen());
        newItem->setUndoAware(true);
    }

    addChild(newItem);
    updateData(newItem);
}
//##############################################################################
//##########################  getter methods  ##################################
//##############################################################################
BASIC_SHARED_D_READER_IMPL(DataPickerCurve, bool, visible, visible)
BASIC_SHARED_D_READER_IMPL(DataPickerCurve, Image::Errors, curveErrorTypes, curveErrorTypes)
BASIC_SHARED_D_READER_IMPL(DataPickerCurve, AbstractColumn*, posXColumn, posXColumn)
QString& DataPickerCurve::posXColumnPath() const { return d_ptr->posXColumnPath; }
BASIC_SHARED_D_READER_IMPL(DataPickerCurve, AbstractColumn*, posYColumn, posYColumn)
QString& DataPickerCurve::posYColumnPath() const { return d_ptr->posYColumnPath; }
BASIC_SHARED_D_READER_IMPL(DataPickerCurve, AbstractColumn*, plusDeltaXColumn, plusDeltaXColumn)
QString& DataPickerCurve::plusDeltaXColumnPath() const { return d_ptr->plusDeltaXColumnPath; }
BASIC_SHARED_D_READER_IMPL(DataPickerCurve, AbstractColumn*, minusDeltaXColumn, minusDeltaXColumn)
QString& DataPickerCurve::minusDeltaXColumnPath() const { return d_ptr->minusDeltaXColumnPath; }
BASIC_SHARED_D_READER_IMPL(DataPickerCurve, AbstractColumn*, plusDeltaYColumn, plusDeltaYColumn)
QString& DataPickerCurve::plusDeltaYColumnPath() const { return d_ptr->plusDeltaYColumnPath; }
BASIC_SHARED_D_READER_IMPL(DataPickerCurve, AbstractColumn*, minusDeltaYColumn, minusDeltaYColumn)
QString& DataPickerCurve::minusDeltaYColumnPath() const { return d_ptr->minusDeltaYColumnPath; }

//##############################################################################
//#########################  setter methods  ###################################
//##############################################################################
void DataPickerCurve::setVisible(const bool on) {
    Q_D(DataPickerCurve);

    if (on != d->visible) {
        d->visible = on;
        if (on)
            beginMacro(i18n("%1:set visibile", name()));
        else
            beginMacro(i18n("%1:set invisible", name()));

        foreach (WorksheetElement* elem, children<WorksheetElement>(IncludeHidden))
            elem->setVisible(on);

        endMacro();
    }
}

void DataPickerCurve::setCurveErrorTypes(const Image::Errors errors) {
    Q_D(DataPickerCurve);

    d->curveErrorTypes = errors;

    Spreadsheet* datasheet = new Spreadsheet(0, i18n("Data"));
    addChild(datasheet);

    d->posXColumn = datasheet->column(0);
    d->posXColumn->setName(i18n("x"));

    d->posYColumn = datasheet->column(1);
    d->posYColumn->setName(i18n("y"));

    if (d->curveErrorTypes.x == Image::AsymmetricError) {
        d->plusDeltaXColumn = appendColumn(i18n("+delta_x"), datasheet);
        d->minusDeltaXColumn = appendColumn(i18n("-delta_x"), datasheet);
    } else if (d->curveErrorTypes.x == Image::SymmetricError) {
        d->plusDeltaXColumn = appendColumn(i18n("+delta_x"), datasheet);
    }

    if (d->curveErrorTypes.y == Image::AsymmetricError) {
        d->plusDeltaYColumn = appendColumn(i18n("+delta_y"), datasheet);
        d->minusDeltaYColumn = appendColumn(i18n("-delta_y"), datasheet);
    } else if (d->curveErrorTypes.y == Image::SymmetricError) {
        d->plusDeltaYColumn = appendColumn(i18n("+delta_y"), datasheet);
    }
}

void DataPickerCurve::setPosXColumn(AbstractColumn* column) {
    Q_D(DataPickerCurve);
    d->posXColumn = column;
}

void DataPickerCurve::setPosYColumn(AbstractColumn* column) {
    Q_D(DataPickerCurve);
    d->posYColumn = column;
}

void DataPickerCurve::setPlusDeltaXColumn(AbstractColumn* column) {
    Q_D(DataPickerCurve);
    d->plusDeltaXColumn = column;
}

void DataPickerCurve::setMinusDeltaXColumn(AbstractColumn* column) {
    Q_D(DataPickerCurve);
    d->minusDeltaXColumn = column;
}

void DataPickerCurve::setPlusDeltaYColumn(AbstractColumn* column) {
    Q_D(DataPickerCurve);
    d->plusDeltaYColumn = column;
}

void DataPickerCurve::setMinusDeltaYColumn(AbstractColumn* column) {
    Q_D(DataPickerCurve);
    d->minusDeltaYColumn = column;
}

void DataPickerCurve::setPrinting(bool on) {
    foreach (WorksheetElement* elem, children<WorksheetElement>(IncludeHidden))
        elem->setPrinting(on);
}
//##############################################################################
//######  SLOTs for changes triggered via QActions in the context menu  ########
//##############################################################################
void DataPickerCurve::visibilityChanged() {
    this->setVisible(!visible());
}

void DataPickerCurve::updateDatasheet() {
    beginMacro(i18n("%1:update datasheet", name()));

    foreach (CustomItem* item, children<CustomItem>(IncludeHidden))
        updateData(item);

    endMacro();
}

/*!
    Update datasheet for corresponding custom-item(curve-point),
    it is called every time whenever there is any change in position
    of curve-point or its error-bar so keep it undo unaware
    no need to create extra entry in undo stack
*/
void DataPickerCurve::updateData(const CustomItem* item) {
    Q_D(DataPickerCurve);
    Datapicker* datapicker = dynamic_cast<Datapicker*>(parentAspect());
    if (!datapicker)
        return;
    Q_ASSERT(datapicker->m_image);

    int row = indexOfChild<CustomItem>(item ,AbstractAspect::IncludeHidden);
    QPointF data;
    Transform transform;
    data = transform.mapSceneToLogical(item->position().point, datapicker->m_image->axisPoints());

    if(d->posXColumn) {
        d->posXColumn->setUndoAware(false);
        d->posXColumn->setValueAt(row, data.x());
        d->posXColumn->setUndoAware(true);
    }

    if(d->posYColumn) {
        d->posYColumn->setUndoAware(false);
        d->posYColumn->setValueAt(row, data.y());
        d->posYColumn->setUndoAware(true);
    }

    if (d->plusDeltaXColumn) {
        data = transform.mapSceneLengthToLogical(QPointF(item->plusDeltaXPos().x(), 0), datapicker->m_image->axisPoints());
        d->plusDeltaXColumn->setUndoAware(false);
        d->plusDeltaXColumn->setValueAt(row, qAbs(data.x()));
        d->plusDeltaXColumn->setUndoAware(true);
    }

    if (d->minusDeltaXColumn) {
        data = transform.mapSceneLengthToLogical(QPointF(item->minusDeltaXPos().x(), 0), datapicker->m_image->axisPoints());
        d->minusDeltaXColumn->setUndoAware(false);
        d->minusDeltaXColumn->setValueAt(row, qAbs(data.x()));
        d->minusDeltaXColumn->setUndoAware(true);
    }

    if (d->plusDeltaYColumn) {
        data = transform.mapSceneLengthToLogical(QPointF(0, item->plusDeltaYPos().y()), datapicker->m_image->axisPoints());
        d->plusDeltaYColumn->setUndoAware(false);
        d->plusDeltaYColumn->setValueAt(row, qAbs(data.y()));
        d->plusDeltaYColumn->setUndoAware(true);
    }

    if (d->minusDeltaYColumn) {
        data = transform.mapSceneLengthToLogical(QPointF(0, item->minusDeltaYPos().y()), datapicker->m_image->axisPoints());
        d->minusDeltaYColumn->setUndoAware(false);
        d->minusDeltaYColumn->setValueAt(row, qAbs(data.y()));
        d->minusDeltaYColumn->setUndoAware(true);
    }
}
//##############################################################################
//##################  Serialization/Deserialization  ###########################
//##############################################################################
//! Save as XML
void DataPickerCurve::save(QXmlStreamWriter* writer) const{
    Q_D(const DataPickerCurve);

    writer->writeStartElement( "dataPickerCurve" );
    writeBasicAttributes(writer);
    writeCommentElement(writer);

    //general
    writer->writeStartElement( "general" );
    WRITE_COLUMN(d->posXColumn, posXColumn);
    WRITE_COLUMN(d->posYColumn, posYColumn);
    WRITE_COLUMN(d->plusDeltaXColumn, plusDeltaXColumn);
    WRITE_COLUMN(d->minusDeltaXColumn, minusDeltaXColumn);
    WRITE_COLUMN(d->plusDeltaYColumn, plusDeltaYColumn);
    WRITE_COLUMN(d->minusDeltaYColumn, minusDeltaYColumn);
    writer->writeAttribute( "curveErrorType_X", QString::number(d->curveErrorTypes.x) );
    writer->writeAttribute( "curveErrorType_Y", QString::number(d->curveErrorTypes.y) );
    writer->writeAttribute( "visible", QString::number(d->visible) );
    writer->writeEndElement();

    //serialize all children
    QList<AbstractAspect *> childrenAspect = children<AbstractAspect>(IncludeHidden);
    foreach(AbstractAspect *child, childrenAspect)
        child->save(writer);

    writer->writeEndElement(); // close section
}

//! Load from XML
bool DataPickerCurve::load(XmlStreamReader* reader) {
    Q_D(DataPickerCurve);

    if(!reader->isStartElement() || reader->name() != "dataPickerCurve") {
        reader->raiseError(i18n("no image element found"));
        return false;
    }

    if (!readBasicAttributes(reader))
        return false;

    QString attributeWarning = i18n("Attribute '%1' missing or empty, default value is used");
    QXmlStreamAttributes attribs;
    QString str;

    while (!reader->atEnd()) {
        reader->readNext();
        if (reader->isEndElement() && reader->name() == "dataPickerCurve")
            break;

        if (!reader->isStartElement())
            continue;

        if (reader->name() == "comment"){
            if (!readCommentElement(reader)) return false;
        } else if (reader->name() == "general") {
            attribs = reader->attributes();

            str = attribs.value("curveErrorType_X").toString();
            if(str.isEmpty())
                reader->raiseWarning(attributeWarning.arg("curveErrorType_X"));
            else
                d->curveErrorTypes.x = Image::ErrorType(str.toInt());

            str = attribs.value("curveErrorType_Y").toString();
            if(str.isEmpty())
                reader->raiseWarning(attributeWarning.arg("curveErrorType_Y"));
            else
                d->curveErrorTypes.y = Image::ErrorType(str.toInt());

            str = attribs.value("visible").toString();
            if(str.isEmpty())
                reader->raiseWarning(attributeWarning.arg("visible"));
            else
                d->visible = str.toInt();

            READ_COLUMN(posXColumn);
            READ_COLUMN(posYColumn);
            READ_COLUMN(plusDeltaXColumn);
            READ_COLUMN(minusDeltaXColumn);
            READ_COLUMN(plusDeltaYColumn);
            READ_COLUMN(minusDeltaYColumn);

        } else if (reader->name() == "customItem") {
            CustomItem* curvePoint = new CustomItem("");
            curvePoint->setHidden(true);
            if (!curvePoint->load(reader)){
                delete curvePoint;
                return false;
            } else {
                addChild(curvePoint);
                curvePoint->initErrorBar(curveErrorTypes());
            }
        } else if (reader->name() == "spreadsheet") {
            Spreadsheet* datasheet = new Spreadsheet(0, "spreadsheet", true);
            if (!datasheet->load(reader)){
                delete datasheet;
                return false;
            } else {
                addChild(datasheet);
            }
        } else { // unknown element
            reader->raiseWarning(i18n("unknown element '%1'", reader->name().toString()));
            if (!reader->skipToEndElement()) return false;
        }
    }

    return true;
}
