/***************************************************************************
    File                 : CustomPoint.cpp
    Project              : LabPlot
    Description          : Custom user-defined point on the plot
    --------------------------------------------------------------------
    Copyright            : (C) 2015 Ankit Wagadre (wagadre.ankit@gmail.com)
    Copyright            : (C) 2015 Alexander Semke (alexander.semke@web.de)
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

#include "CustomPoint.h"
#include "CustomPointPrivate.h"
#include "backend/worksheet/Worksheet.h"
#include "backend/worksheet/plots/cartesian/CartesianPlot.h"
#include "backend/worksheet/plots/cartesian/CartesianCoordinateSystem.h"
#include "backend/lib/commandtemplates.h"
#include "backend/lib/XmlStreamReader.h"

#include <QPainter>
#include <QMenu>
#include <QGraphicsSceneMouseEvent>

#include <KIcon>
#include <KConfig>
#include <KConfigGroup>
#include <KLocale>


/**
 * \class CustomPoint
 * \brief A customizable point.
 *
 * The position can be either specified by mouse events or by providing the
 * x- and y- coordinates in parent's coordinate system
 */

CustomPoint::CustomPoint(const CartesianPlot* plot, const QString& name):WorksheetElement(name),
	d_ptr(new CustomPointPrivate(this,plot)) {

	init();
}

CustomPoint::CustomPoint(const QString& name, CustomPointPrivate* dd):WorksheetElement(name), d_ptr(dd) {
	init();
}

CustomPoint::~CustomPoint() {
	//no need to delete the d-pointer here - it inherits from QGraphicsItem
	//and is deleted during the cleanup in QGraphicsScene
}

void CustomPoint::init() {
	Q_D(CustomPoint);

	KConfig config;
	KConfigGroup group;
	group = config.group("CustomPoint");
	d->position.setX( group.readEntry("PositionXValue", d->plot->xMin() + (d->plot->xMax()-d->plot->xMin())/2) );
	d->position.setY( group.readEntry("PositionYValue", d->plot->yMin() + (d->plot->yMax()-d->plot->yMin())/2) );

	d->symbolStyle = (Symbol::Style)group.readEntry("SymbolStyle", (int)Symbol::Circle);
	d->symbolSize = group.readEntry("SymbolSize", Worksheet::convertToSceneUnits(5, Worksheet::Point));
	d->symbolRotationAngle = group.readEntry("SymbolRotation", 0.0);
	d->symbolOpacity = group.readEntry("SymbolOpacity", 1.0);
	d->symbolBrush.setStyle( (Qt::BrushStyle)group.readEntry("SymbolFillingStyle", (int)Qt::SolidPattern) );
	d->symbolBrush.setColor( group.readEntry("SymbolFillingColor", QColor(Qt::red)) );
	d->symbolPen.setStyle( (Qt::PenStyle)group.readEntry("SymbolBorderStyle", (int)Qt::SolidLine) );
	d->symbolPen.setColor( group.readEntry("SymbolBorderColor", QColor(Qt::black)) );
	d->symbolPen.setWidthF( group.readEntry("SymbolBorderWidth", Worksheet::convertToSceneUnits(0.0, Worksheet::Point)) );

	this->initActions();

	retransform();
}

void CustomPoint::initActions() {
	visibilityAction = new QAction(i18n("visible"), this);
	visibilityAction->setCheckable(true);
	connect(visibilityAction, SIGNAL(triggered()), this, SLOT(visibilityChanged()));
}

/*!
    Returns an icon to be used in the project explorer.
*/
QIcon CustomPoint::icon() const {
	return  KIcon("draw-cross");
}

QMenu* CustomPoint::createContextMenu() {
	QMenu* menu = WorksheetElement::createContextMenu();
	QAction* firstAction = menu->actions().at(1); //skip the first action because of the "title-action"
	visibilityAction->setChecked(isVisible());
	menu->insertAction(firstAction, visibilityAction);

	return menu;
}

QGraphicsItem* CustomPoint::graphicsItem() const {
	return d_ptr;
}

void CustomPoint::retransform() {
	Q_D(CustomPoint);
	d->retransform();
}

void CustomPoint::handlePageResize(double horizontalRatio, double verticalRatio) {
	Q_UNUSED(horizontalRatio);
	Q_UNUSED(verticalRatio);
}

/* ============================ getter methods ================= */
CLASS_SHARED_D_READER_IMPL(CustomPoint, QPointF, position, position)

//symbols
BASIC_SHARED_D_READER_IMPL(CustomPoint, Symbol::Style, symbolStyle, symbolStyle)
BASIC_SHARED_D_READER_IMPL(CustomPoint, qreal, symbolOpacity, symbolOpacity)
BASIC_SHARED_D_READER_IMPL(CustomPoint, qreal, symbolRotationAngle, symbolRotationAngle)
BASIC_SHARED_D_READER_IMPL(CustomPoint, qreal, symbolSize, symbolSize)
CLASS_SHARED_D_READER_IMPL(CustomPoint, QBrush, symbolBrush, symbolBrush)
CLASS_SHARED_D_READER_IMPL(CustomPoint, QPen, symbolPen, symbolPen)

/* ============================ setter methods and undo commands ================= */
STD_SETTER_CMD_IMPL_F_S(CustomPoint, SetPosition, QPointF, position, retransform)
void CustomPoint::setPosition(const QPointF& position) {
	Q_D(CustomPoint);
	if (position != d->position)
		exec(new CustomPointSetPositionCmd(d, position, i18n("%1: set position")));
}

//Symbol
STD_SETTER_CMD_IMPL_F_S(CustomPoint, SetSymbolStyle, Symbol::Style, symbolStyle, retransform)
void CustomPoint::setSymbolStyle(Symbol::Style style) {
	Q_D(CustomPoint);
	if (style != d->symbolStyle)
		exec(new CustomPointSetSymbolStyleCmd(d, style, i18n("%1: set symbol style")));
}

STD_SETTER_CMD_IMPL_F_S(CustomPoint, SetSymbolSize, qreal, symbolSize, retransform)
void CustomPoint::setSymbolSize(qreal size) {
	Q_D(CustomPoint);
	if (!qFuzzyCompare(1 + size, 1 + d->symbolSize))
		exec(new CustomPointSetSymbolSizeCmd(d, size, i18n("%1: set symbol size")));
}

STD_SETTER_CMD_IMPL_F_S(CustomPoint, SetSymbolRotationAngle, qreal, symbolRotationAngle, retransform)
void CustomPoint::setSymbolRotationAngle(qreal angle) {
	Q_D(CustomPoint);
	if (!qFuzzyCompare(1 + angle, 1 + d->symbolRotationAngle))
		exec(new CustomPointSetSymbolRotationAngleCmd(d, angle, i18n("%1: rotate symbols")));
}

STD_SETTER_CMD_IMPL_F_S(CustomPoint, SetSymbolBrush, QBrush, symbolBrush, update)
void CustomPoint::setSymbolBrush(const QBrush &brush) {
	Q_D(CustomPoint);
	if (brush != d->symbolBrush)
		exec(new CustomPointSetSymbolBrushCmd(d, brush, i18n("%1: set symbol filling")));
}

STD_SETTER_CMD_IMPL_F_S(CustomPoint, SetSymbolPen, QPen, symbolPen, update)
void CustomPoint::setSymbolPen(const QPen &pen) {
	Q_D(CustomPoint);
	if (pen != d->symbolPen)
		exec(new CustomPointSetSymbolPenCmd(d, pen, i18n("%1: set symbol outline style")));
}

STD_SETTER_CMD_IMPL_F_S(CustomPoint, SetSymbolOpacity, qreal, symbolOpacity, update)
void CustomPoint::setSymbolOpacity(qreal opacity) {
	Q_D(CustomPoint);
	if (opacity != d->symbolOpacity)
		exec(new CustomPointSetSymbolOpacityCmd(d, opacity, i18n("%1: set symbol opacity")));
}

STD_SWAP_METHOD_SETTER_CMD_IMPL_F(CustomPoint, SetVisible, bool, swapVisible, retransform);
void CustomPoint::setVisible(bool on) {
	Q_D(CustomPoint);
	exec(new CustomPointSetVisibleCmd(d, on, on ? i18n("%1: set visible") : i18n("%1: set invisible")));
}

bool CustomPoint::isVisible() const {
	Q_D(const CustomPoint);
	return d->isVisible();
}

void CustomPoint::setPrinting(bool on) {
	Q_D(CustomPoint);
	d->m_printing = on;
}

//##############################################################################
//######  SLOTs for changes triggered via QActions in the context menu  ########
//##############################################################################
void CustomPoint::visibilityChanged() {
	Q_D(const CustomPoint);
	this->setVisible(!d->isVisible());
}

//##############################################################################
//####################### Private implementation ###############################
//##############################################################################
CustomPointPrivate::CustomPointPrivate(CustomPoint* owner, const CartesianPlot* p)
	: plot(p),
	suppressItemChangeEvent(false),
	suppressRetransform(false),
	m_printing(false),
	m_hovered(false),
	m_visible(true),
	q(owner) {

	setFlag(QGraphicsItem::ItemSendsGeometryChanges);
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
}

QString CustomPointPrivate::name() const {
	return q->name();
}

/*!
    calculates the position and the bounding box of the item/point. Called on geometry or properties changes.
 */
void CustomPointPrivate::retransform() {
	if (suppressRetransform)
		return;

	//calculate the point in the scene coordinates
	const CartesianCoordinateSystem* cSystem = dynamic_cast<const CartesianCoordinateSystem*>(plot->coordinateSystem());
	QList<QPointF> list, listScene;
	list<<position;
	listScene = cSystem->mapLogicalToScene(list, CartesianCoordinateSystem::DefaultMapping);
	if (listScene.size()) {
		m_visible = true;
		positionScene = listScene.at(0);
		suppressItemChangeEvent=true;
		setPos(positionScene);
		suppressItemChangeEvent=false;
	} else {
		m_visible = false;
	}

	recalcShapeAndBoundingRect();
}

bool CustomPointPrivate::swapVisible(bool on) {
	bool oldValue = isVisible();
	setVisible(on);
	emit q->changed();
	emit q->visibleChanged(on);
	return oldValue;
}

/*!
    Returns the outer bounds of the item as a rectangle.
 */
QRectF CustomPointPrivate::boundingRect() const {
	return transformedBoundingRectangle;
}

/*!
    Returns the shape of this item as a QPainterPath in local coordinates.
*/
QPainterPath CustomPointPrivate::shape() const {
	return pointShape;
}

/*!
  recalculates the outer bounds and the shape of the item.
*/
void CustomPointPrivate::recalcShapeAndBoundingRect() {
	prepareGeometryChange();

	pointShape = QPainterPath();
	if (m_visible && symbolStyle != Symbol::NoSymbols) {
		QPainterPath path = Symbol::pathFromStyle(symbolStyle);

		QTransform trafo;
		trafo.scale(symbolSize, symbolSize);
		path = trafo.map(path);
		trafo.reset();

		if (symbolRotationAngle != 0) {
			trafo.rotate(symbolRotationAngle);
			path = trafo.map(path);
		}

		pointShape = trafo.map(path);
		transformedBoundingRectangle = pointShape.boundingRect();
	}
}

void CustomPointPrivate::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
	Q_UNUSED(option)
	Q_UNUSED(widget)

	if (!m_visible)
		return;

	if (symbolStyle != Symbol::NoSymbols) {
		painter->setOpacity(symbolOpacity);
		painter->setPen(symbolPen);
		painter->setBrush(symbolBrush);
		painter->drawPath(pointShape);
	}

	if (m_hovered && !isSelected() && !m_printing) {
		painter->setPen(q->hoveredPen);
		painter->setOpacity(q->hoveredOpacity);
		painter->drawPath(pointShape);
	}

	if (isSelected() && !m_printing) {
		painter->setPen(q->selectedPen);
		painter->setOpacity(q->selectedOpacity);
		painter->drawPath(pointShape);
	}
}

QVariant CustomPointPrivate::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (suppressItemChangeEvent)
		return value;

	if (change == QGraphicsItem::ItemPositionChange) {
		//emit the signals in order to notify the UI.
		//we don't set the position related member variables during the mouse movements.
		//this is done on mouse release events only.
		const CartesianCoordinateSystem* cSystem = dynamic_cast<const CartesianCoordinateSystem*>(plot->coordinateSystem());
		emit q->positionChanged(cSystem->mapSceneToLogical(value.toPointF()));
	}

	return QGraphicsItem::itemChange(change, value);
}

void CustomPointPrivate::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	//position was changed -> set the position member variables
	suppressRetransform = true;
	const CartesianCoordinateSystem* cSystem = dynamic_cast<const CartesianCoordinateSystem*>(plot->coordinateSystem());
	emit q->setPosition(cSystem->mapSceneToLogical(pos()));
	suppressRetransform = false;

	QGraphicsItem::mouseReleaseEvent(event);
}

void CustomPointPrivate::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
	q->createContextMenu()->exec(event->screenPos());
}

void CustomPointPrivate::hoverEnterEvent(QGraphicsSceneHoverEvent*) {
	if (!isSelected()) {
		m_hovered = true;
		q->hovered();
		update();
	}
}

void CustomPointPrivate::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
	if (m_hovered) {
		m_hovered = false;
		q->unhovered();
		update();
	}
}

//##############################################################################
//##################  Serialization/Deserialization  ###########################
//##############################################################################
//! Save as XML
void CustomPoint::save(QXmlStreamWriter* writer) const {
	Q_D(const CustomPoint);

	writer->writeStartElement("customPoint");
	writeBasicAttributes(writer);
	writeCommentElement(writer);

	//geometry
	writer->writeStartElement("geometry");
	writer->writeAttribute( "x", QString::number(d->position.x()) );
	writer->writeAttribute( "y", QString::number(d->position.y()) );
	writer->writeAttribute( "visible", QString::number(d->isVisible()) );
	writer->writeEndElement();

	//Symbols
	writer->writeStartElement("symbol");
	writer->writeAttribute( "symbolStyle", QString::number(d->symbolStyle) );
	writer->writeAttribute( "opacity", QString::number(d->symbolOpacity) );
	writer->writeAttribute( "rotation", QString::number(d->symbolRotationAngle) );
	writer->writeAttribute( "size", QString::number(d->symbolSize) );
	WRITE_QBRUSH(d->symbolBrush);
	WRITE_QPEN(d->symbolPen);
	writer->writeEndElement();

	writer->writeEndElement(); // close "CustomPoint" section
}

//! Load from XML
bool CustomPoint::load(XmlStreamReader* reader) {
	Q_D(CustomPoint);

	if (!reader->isStartElement() || reader->name() != "customPoint") {
		reader->raiseError(i18n("no custom point element found"));
		return false;
	}

	if (!readBasicAttributes(reader))
		return false;

	QString attributeWarning = i18n("Attribute '%1' missing or empty, default value is used");
	QXmlStreamAttributes attribs;
	QString str;

	while (!reader->atEnd()) {
		reader->readNext();
		if (reader->isEndElement() && reader->name() == "customPoint")
			break;

		if (!reader->isStartElement())
			continue;

		if (reader->name() == "comment") {
			if (!readCommentElement(reader)) return false;
		} else if (reader->name() == "geometry") {
			attribs = reader->attributes();

			str = attribs.value("x").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'x'"));
			else
				d->position.setX(str.toDouble());

			str = attribs.value("y").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'y'"));
			else
				d->position.setY(str.toDouble());

			str = attribs.value("visible").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'visible'"));
			else
				d->setVisible(str.toInt());
		} else if (reader->name() == "symbol") {
			attribs = reader->attributes();

			str = attribs.value("symbolStyle").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'symbolStyle'"));
			else
				d->symbolStyle = (Symbol::Style)str.toInt();

			str = attribs.value("opacity").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'opacity'"));
			else
				d->symbolOpacity = str.toDouble();

			str = attribs.value("rotation").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'rotation'"));
			else
				d->symbolRotationAngle = str.toDouble();

			str = attribs.value("size").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'size'"));
			else
				d->symbolSize = str.toDouble();

			READ_QBRUSH(d->symbolBrush);
			READ_QPEN(d->symbolPen);
		} else { // unknown element
			reader->raiseWarning(i18n("unknown element '%1'", reader->name().toString()));
			if (!reader->skipToEndElement()) return false;
		}
	}

	retransform();
	return true;
}
