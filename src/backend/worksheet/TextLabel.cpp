/***************************************************************************
    File                 : TextLabel.cpp
    Project              : LabPlot
    Description          : Text label supporting reach text and latex formatting
    --------------------------------------------------------------------
    Copyright            : (C) 2009 Tilman Benkert (thzs@gmx.net)
    Copyright            : (C) 2012-2015 Alexander Semke (alexander.semke@web.de)
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

#include "TextLabel.h"
#include "Worksheet.h"
#include "TextLabelPrivate.h"
#include "backend/lib/commandtemplates.h"
#include "backend/lib/XmlStreamReader.h"

#include <QApplication>
#include <QtConcurrentRun>
#include <QDesktopWidget>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>

#include <KIcon>
#include <KConfig>
#include <KConfigGroup>
#include <KLocale>

/**
 * \class TextLabel
 * \brief A label supporting rendering of html- and tex-formated textes.
 *
 * The label is aligned relative to the specified position.
 * The position can be either specified by providing the x- and y- coordinates
 * in parent's coordinate system, or by specifying one of the predefined position
 * flags (\ca HorizontalPosition, \ca VerticalPosition).
 */


TextLabel::TextLabel(const QString& name, Type type):WorksheetElement(name),
	d_ptr(new TextLabelPrivate(this)), m_type(type) {
	init();
}

TextLabel::TextLabel(const QString &name, TextLabelPrivate *dd, Type type):WorksheetElement(name),
	d_ptr(dd), m_type(type) {
	init();
}

TextLabel::Type TextLabel::type() const {
	return m_type;
}

void TextLabel::init() {
	Q_D(TextLabel);

	KConfig config;
	KConfigGroup group;
	if (m_type == AxisTitle)
		group = config.group("AxisTitle");
	else if (m_type == PlotTitle)
		group = config.group("PlotTitle");
	else if (m_type == PlotLegendTitle)
		group = config.group("PlotLegendTitle");
	else
		group = config.group("TextLabel");

	//properties common to all types
	d->textWrapper.teXUsed = group.readEntry("TeXUsed", false);
	d->teXFontSize = group.readEntry("TeXFontSize", 12);
	d->teXFontColor = group.readEntry("TeXFontColor", QColor(Qt::black));
	d->rotationAngle = group.readEntry("Rotation", 0.0);

	d->staticText.setTextFormat(Qt::RichText);
	// explicitly set no wrap mode for text label to avoid unnecessary line breaks
	QTextOption textOption;
	textOption.setWrapMode(QTextOption::NoWrap);
	d->staticText.setTextOption(textOption);

	//position and alignment relevant properties, dependent on the actual type
	if (m_type == PlotTitle || m_type == PlotLegendTitle) {
		d->position.horizontalPosition = (HorizontalPosition) group.readEntry("PositionX", (int)TextLabel::hPositionCenter);
		d->position.verticalPosition = (VerticalPosition) group.readEntry("PositionY", (int) TextLabel::vPositionTop);
		d->position.point.setX( group.readEntry("PositionXValue", Worksheet::convertToSceneUnits(1, Worksheet::Centimeter)) );
		d->position.point.setY( group.readEntry("PositionYValue", Worksheet::convertToSceneUnits(1, Worksheet::Centimeter)) );
		d->horizontalAlignment= (TextLabel::HorizontalAlignment) group.readEntry("HorizontalAlignment", (int)TextLabel::hAlignCenter);
		d->verticalAlignment= (TextLabel::VerticalAlignment) group.readEntry("VerticalAlignment", (int)TextLabel::vAlignBottom);
	} else {
		d->position.horizontalPosition = (HorizontalPosition) group.readEntry("PositionX", (int)TextLabel::hPositionCustom);
		d->position.verticalPosition = (VerticalPosition) group.readEntry("PositionY", (int) TextLabel::vPositionCustom);
		d->position.point.setX( group.readEntry("PositionXValue", Worksheet::convertToSceneUnits(1, Worksheet::Centimeter)) );
		d->position.point.setY( group.readEntry("PositionYValue", Worksheet::convertToSceneUnits(1, Worksheet::Centimeter)) );
		d->horizontalAlignment= (TextLabel::HorizontalAlignment) group.readEntry("HorizontalAlignment", (int)TextLabel::hAlignCenter);
		d->verticalAlignment= (TextLabel::VerticalAlignment) group.readEntry("VerticalAlignment", (int)TextLabel::vAlignCenter);
	}

	//scaling:
	//we need to scale from the font size specified in points to scene units.
	//furhermore, we create the tex-image in a higher resolution then usual desktop resolution
	// -> take this into account
	d->scaleFactor = Worksheet::convertToSceneUnits(1, Worksheet::Point);
	d->teXImageResolution = QApplication::desktop()->physicalDpiX();
	d->teXImageScaleFactor = Worksheet::convertToSceneUnits(2.54/QApplication::desktop()->physicalDpiX(), Worksheet::Centimeter);

	connect(&d->teXImageFutureWatcher, SIGNAL(finished()), this, SLOT(updateTeXImage()));

	this->initActions();
}

void TextLabel::initActions() {
	visibilityAction = new QAction(i18n("visible"), this);
	visibilityAction->setCheckable(true);
	connect(visibilityAction, SIGNAL(triggered()), this, SLOT(visibilityChanged()));
}

TextLabel::~TextLabel() {
	//no need to delete the d-pointer here - it inherits from QGraphicsItem
	//and is deleted during the cleanup in QGraphicsScene
}

QGraphicsItem* TextLabel::graphicsItem() const {
	return d_ptr;
}

void TextLabel::setParentGraphicsItem(QGraphicsItem* item) {
	Q_D(TextLabel);
	d->setParentItem(item);
	d->updatePosition();
}

void TextLabel::retransform() {
	Q_D(TextLabel);
	d->retransform();
}

void TextLabel::handlePageResize(double horizontalRatio, double verticalRatio) {
	Q_UNUSED(horizontalRatio);
	Q_UNUSED(verticalRatio);

	Q_D(TextLabel);
	d->scaleFactor = Worksheet::convertToSceneUnits(1, Worksheet::Point);
}

/*!
	Returns an icon to be used in the project explorer.
*/
QIcon TextLabel::icon() const {
	return  KIcon("draw-text");
}

QMenu* TextLabel::createContextMenu() {
	QMenu *menu = WorksheetElement::createContextMenu();
	QAction* firstAction = menu->actions().at(1); //skip the first action because of the "title-action"

	visibilityAction->setChecked(isVisible());
	menu->insertAction(firstAction, visibilityAction);

	return menu;
}

/* ============================ getter methods ================= */
CLASS_SHARED_D_READER_IMPL(TextLabel, TextLabel::TextWrapper, text, textWrapper)
CLASS_SHARED_D_READER_IMPL(TextLabel, int, teXFontSize, teXFontSize);
CLASS_SHARED_D_READER_IMPL(TextLabel, QColor, teXFontColor, teXFontColor);
CLASS_SHARED_D_READER_IMPL(TextLabel, TextLabel::PositionWrapper, position, position);
BASIC_SHARED_D_READER_IMPL(TextLabel, TextLabel::HorizontalAlignment, horizontalAlignment, horizontalAlignment);
BASIC_SHARED_D_READER_IMPL(TextLabel, TextLabel::VerticalAlignment, verticalAlignment, verticalAlignment);
BASIC_SHARED_D_READER_IMPL(TextLabel, float, rotationAngle, rotationAngle);

/* ============================ setter methods and undo commands ================= */
STD_SETTER_CMD_IMPL_F_S(TextLabel, SetText, TextLabel::TextWrapper, textWrapper, updateText);
void TextLabel::setText(const TextWrapper &textWrapper) {
	Q_D(TextLabel);
	if ( (textWrapper.text != d->textWrapper.text) || (textWrapper.teXUsed != d->textWrapper.teXUsed) )
		exec(new TextLabelSetTextCmd(d, textWrapper, i18n("%1: set label text")));

}

STD_SETTER_CMD_IMPL_F_S(TextLabel, SetTeXFontSize, int, teXFontSize, updateText);
void TextLabel::setTeXFontSize(const int fontSize) {
	Q_D(TextLabel);
	if (fontSize != d->teXFontSize)
		exec(new TextLabelSetTeXFontSizeCmd(d, fontSize, i18n("%1: set TeX font size")));
}

STD_SETTER_CMD_IMPL_F_S(TextLabel, SetTeXFontColor, QColor, teXFontColor, updateText);
void TextLabel::setTeXFontColor(const QColor fontColor) {
	Q_D(TextLabel);
	if (fontColor != d->teXFontColor)
		exec(new TextLabelSetTeXFontColorCmd(d, fontColor, i18n("%1: set TeX font color")));
}

STD_SETTER_CMD_IMPL_F_S(TextLabel, SetPosition, TextLabel::PositionWrapper, position, retransform);
void TextLabel::setPosition(const PositionWrapper& pos) {
	Q_D(TextLabel);
	if (pos.point!=d->position.point || pos.horizontalPosition!=d->position.horizontalPosition || pos.verticalPosition!=d->position.verticalPosition)
		exec(new TextLabelSetPositionCmd(d, pos, i18n("%1: set position")));
}

/*!
	sets the position without undo/redo-stuff
*/
void TextLabel::setPosition(const QPointF& point) {
	Q_D(TextLabel);
	if (point != d->position.point) {
		d->position.point = point;
		retransform();
	}
}

/*!
 * position is set to invalid if the parent item is not drawn on the scene
 * (e.g. axis is not drawn because it's outside plot ranges -> don't draw axis' title label)
 */
void TextLabel::setPositionInvalid(bool invalid) {
	Q_D(TextLabel);
	if (invalid != d->positionInvalid) {
		d->positionInvalid = invalid;
	}
}

STD_SETTER_CMD_IMPL_F_S(TextLabel, SetRotationAngle, float, rotationAngle, recalcShapeAndBoundingRect);
void TextLabel::setRotationAngle(float angle) {
	Q_D(TextLabel);
	if (angle != d->rotationAngle)
		exec(new TextLabelSetRotationAngleCmd(d, angle, i18n("%1: set rotation angle")));
}

STD_SETTER_CMD_IMPL_F_S(TextLabel, SetHorizontalAlignment, TextLabel::HorizontalAlignment, horizontalAlignment, retransform);
void TextLabel::setHorizontalAlignment(const TextLabel::HorizontalAlignment hAlign) {
	Q_D(TextLabel);
	if (hAlign != d->horizontalAlignment)
		exec(new TextLabelSetHorizontalAlignmentCmd(d, hAlign, i18n("%1: set horizontal alignment")));
}

STD_SETTER_CMD_IMPL_F_S(TextLabel, SetVerticalAlignment, TextLabel::VerticalAlignment, verticalAlignment, retransform);
void TextLabel::setVerticalAlignment(const TextLabel::VerticalAlignment vAlign) {
	Q_D(TextLabel);
	if (vAlign != d->verticalAlignment)
		exec(new TextLabelSetVerticalAlignmentCmd(d, vAlign, i18n("%1: set vertical alignment")));
}

STD_SWAP_METHOD_SETTER_CMD_IMPL_F(TextLabel, SetVisible, bool, swapVisible, retransform);
void TextLabel::setVisible(bool on) {
	Q_D(TextLabel);
	exec(new TextLabelSetVisibleCmd(d, on, on ? i18n("%1: set visible") : i18n("%1: set invisible")));
}

bool TextLabel::isVisible() const {
	Q_D(const TextLabel);
	return d->isVisible();
}

void TextLabel::setPrinting(bool on) {
	Q_D(TextLabel);
	d->m_printing = on;
}

void TextLabel::updateTeXImage() {
	Q_D(TextLabel);
	d->updateTeXImage();
}

//##############################################################################
//######  SLOTs for changes triggered via QActions in the context menu  ########
//##############################################################################
void TextLabel::visibilityChanged() {
	Q_D(const TextLabel);
	this->setVisible(!d->isVisible());
}

//##############################################################################
//####################### Private implementation ###############################
//##############################################################################
TextLabelPrivate::TextLabelPrivate(TextLabel *owner)
	: positionInvalid(false),
	  suppressItemChangeEvent(false),
	  suppressRetransform(false),
	  m_printing(false),
	  m_hovered(false),
	  q(owner) {

	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemSendsGeometryChanges);
	setAcceptHoverEvents(true);
}

QString TextLabelPrivate::name() const {
	return q->name();
}

/*!
	calculates the position and the bounding box of the label. Called on geometry or text changes.
 */
void TextLabelPrivate::retransform() {
	if (suppressRetransform)
		return;

	if (position.horizontalPosition != TextLabel::hPositionCustom
	        || position.verticalPosition != TextLabel::vPositionCustom)
		updatePosition();

	float x = position.point.x();
	float y = position.point.y();

	//determine the size of the label in scene units.
	float w, h;
	if (textWrapper.teXUsed) {
		//image size is in pixel, convert to scene units
		w = teXImage.width()*teXImageScaleFactor;
		h = teXImage.height()*teXImageScaleFactor;
	}
	else {
		//size is in points, convert to scene units
		w = staticText.size().width()*scaleFactor;
		h = staticText.size().height()*scaleFactor;
	}

	//depending on the alignment, calculate the new GraphicsItem's position in parent's coordinate system
	QPointF itemPos;
	switch (horizontalAlignment) {
	case TextLabel::hAlignLeft:
		itemPos.setX( x - w/2 );
		break;
	case TextLabel::hAlignCenter:
		itemPos.setX( x );
		break;
	case TextLabel::hAlignRight:
		itemPos.setX( x +w/2);
		break;
	}

	switch (verticalAlignment) {
	case TextLabel::vAlignTop:
		itemPos.setY( y - h/2 );
		break;
	case TextLabel::vAlignCenter:
		itemPos.setY( y );
		break;
	case TextLabel::vAlignBottom:
		itemPos.setY( y + h/2 );
		break;
	}

	suppressItemChangeEvent=true;
	setPos(itemPos);
	suppressItemChangeEvent=false;

	boundingRectangle.setX(-w/2);
	boundingRectangle.setY(-h/2);
	boundingRectangle.setWidth(w);
	boundingRectangle.setHeight(h);

	recalcShapeAndBoundingRect();
}

/*!
	calculates the position of the label, when the position relative to the parent was specified (left, right, etc.)
*/
void TextLabelPrivate::updatePosition() {
	//determine the parent item
	QRectF parentRect;
	QGraphicsItem* parent = parentItem();
	if (parent) {
		parentRect = parent->boundingRect();
	} else {
		if (!scene())
			return;

		parentRect = scene()->sceneRect();
	}


	if (position.horizontalPosition != TextLabel::hPositionCustom) {
		if (position.horizontalPosition == TextLabel::hPositionLeft)
			position.point.setX( parentRect.x() );
		else if (position.horizontalPosition == TextLabel::hPositionCenter)
			position.point.setX( parentRect.x() + parentRect.width()/2 );
		else if (position.horizontalPosition == TextLabel::hPositionRight)
			position.point.setX( parentRect.x() + parentRect.width() );
	}

	if (position.verticalPosition != TextLabel::vPositionCustom) {
		if (position.verticalPosition == TextLabel::vPositionTop)
			position.point.setY( parentRect.y() );
		else if (position.verticalPosition == TextLabel::vPositionCenter)
			position.point.setY( parentRect.y() + parentRect.height()/2 );
		else if (position.verticalPosition == TextLabel::vPositionBottom)
			position.point.setY( parentRect.y() + parentRect.height() );
	}

	emit q->positionChanged(position);
}

/*!
	updates the static text.
 */
void TextLabelPrivate::updateText() {
	if (textWrapper.teXUsed) {
		QFuture<QImage> future = QtConcurrent::run(TeXRenderer::renderImageLaTeX, textWrapper.text, teXFontColor, teXFontSize, teXImageResolution);
		teXImageFutureWatcher.setFuture(future);

		//don't need to call retransorm() here since it is done in updateTeXImage
		//when the asynchronous rendering of the image is finished.
	} else {
		staticText.setText(textWrapper.text);

		//the size of the label was most probably changed.
		//call retransform() to recalculate the position and the bounding box of the label
		retransform();
	}
}

void TextLabelPrivate::updateTeXImage() {
	teXImage = teXImageFutureWatcher.result();

	//the size of the tex image was most probably changed.
	//call retransform() to recalculate the position and the bounding box of the label
	retransform();
}

bool TextLabelPrivate::swapVisible(bool on) {
	bool oldValue = isVisible();
	setVisible(on);
	emit q->changed();
	emit q->visibleChanged(on);
	return oldValue;
}

/*!
	Returns the outer bounds of the item as a rectangle.
 */
QRectF TextLabelPrivate::boundingRect() const {
	return transformedBoundingRectangle;
}

/*!
	Returns the shape of this item as a QPainterPath in local coordinates.
*/
QPainterPath TextLabelPrivate::shape() const {
	return labelShape;
}

/*!
  recalculates the outer bounds and the shape of the label.
*/
void TextLabelPrivate::recalcShapeAndBoundingRect() {
	prepareGeometryChange();

	QMatrix matrix;
	matrix.rotate(-rotationAngle);
	transformedBoundingRectangle = matrix.mapRect(boundingRectangle);

	labelShape = QPainterPath();
	labelShape.addRect(boundingRectangle);
	labelShape = matrix.map(labelShape);

	emit(q->changed());
}

void TextLabelPrivate::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget) {
	Q_UNUSED(option)
	Q_UNUSED(widget)

	if (positionInvalid)
		return;

	if (textWrapper.text.isEmpty())
		return;

	painter->save();
	painter->rotate(-rotationAngle);

	if (textWrapper.teXUsed) {
		if (boundingRect().width()!=0.0 &&  boundingRect().height()!=0.0) {
			QImage todraw = teXImage.scaled(boundingRect().width(), boundingRect().height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			painter->drawImage(boundingRect(), todraw);
		}
	} else {
		painter->scale(scaleFactor, scaleFactor);
		float w = staticText.size().width();
		float h = staticText.size().height();
		painter->drawStaticText(QPoint(-w/2,-h/2), staticText);
	}
	painter->restore();

	if (m_hovered && !isSelected() && !m_printing) {
		painter->setPen(q->hoveredPen);
		painter->setOpacity(q->hoveredOpacity);
		painter->drawPath(labelShape);
	}

	if (isSelected() && !m_printing) {
		painter->setPen(q->selectedPen);
		painter->setOpacity(q->selectedOpacity);
		painter->drawPath(labelShape);
	}
}

QVariant TextLabelPrivate::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (suppressItemChangeEvent)
		return value;

	if (change == QGraphicsItem::ItemPositionChange) {
		//convert item's center point in parent's coordinates
		TextLabel::PositionWrapper tempPosition;
		tempPosition.point = positionFromItemPosition(value.toPointF());
		tempPosition.horizontalPosition = TextLabel::hPositionCustom;
		tempPosition.verticalPosition = TextLabel::vPositionCustom;

		//emit the signals in order to notify the UI.
		//we don't set the position related member variables during the mouse movements.
		//this is done on mouse release events only.
		emit q->positionChanged(tempPosition);
	}

	return QGraphicsItem::itemChange(change, value);
}

void TextLabelPrivate::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	//convert position of the item in parent coordinates to label's position
	QPointF point = positionFromItemPosition(pos());
	if (qAbs(point.x()-position.point.x())>20 && qAbs(point.y()-position.point.y())>20 ) {
		//position was changed -> set the position related member variables
		suppressRetransform = true;
		TextLabel::PositionWrapper tempPosition;
		tempPosition.point = point;
		tempPosition.horizontalPosition = TextLabel::hPositionCustom;
		tempPosition.verticalPosition = TextLabel::vPositionCustom;
		q->setPosition(tempPosition);
		suppressRetransform = false;
	}

	QGraphicsItem::mouseReleaseEvent(event);
}

/*!
 *	converts label's position to GraphicsItem's position.
 */
QPointF TextLabelPrivate::positionFromItemPosition(const QPointF& itemPos) {
	float x = itemPos.x();
	float y = itemPos.y();
	float w, h;
	QPointF tmpPosition;
	if (textWrapper.teXUsed) {
		w = teXImage.width()*scaleFactor;
		h = teXImage.height()*scaleFactor;
	}
	else {
		w = staticText.size().width()*scaleFactor;
		h = staticText.size().height()*scaleFactor;
	}

	//depending on the alignment, calculate the new position
	switch (horizontalAlignment) {
	case TextLabel::hAlignLeft:
		tmpPosition.setX( x + w/2 );
		break;
	case TextLabel::hAlignCenter:
		tmpPosition.setX( x );
		break;
	case TextLabel::hAlignRight:
		tmpPosition.setX( x - w/2 );
		break;
	}

	switch (verticalAlignment) {
	case TextLabel::vAlignTop:
		tmpPosition.setY( y + h/2 );
		break;
	case TextLabel::vAlignCenter:
		tmpPosition.setY( y );
		break;
	case TextLabel::vAlignBottom:
		tmpPosition.setY( y - h/2 );
		break;
	}

	return tmpPosition;
}

void TextLabelPrivate::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
	q->createContextMenu()->exec(event->screenPos());
}

void TextLabelPrivate::hoverEnterEvent(QGraphicsSceneHoverEvent*) {
	if (!isSelected()) {
		m_hovered = true;
		q->hovered();
		update();
	}
}

void TextLabelPrivate::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
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
void TextLabel::save(QXmlStreamWriter* writer) const {
	Q_D(const TextLabel);

	writer->writeStartElement( "textLabel" );
	writeBasicAttributes(writer);
	writeCommentElement(writer);

	//geometry
	writer->writeStartElement( "geometry" );
	writer->writeAttribute( "x", QString::number(d->position.point.x()) );
	writer->writeAttribute( "y", QString::number(d->position.point.y()) );
	writer->writeAttribute( "horizontalPosition", QString::number(d->position.horizontalPosition) );
	writer->writeAttribute( "verticalPosition", QString::number(d->position.verticalPosition) );
	writer->writeAttribute( "horizontalAlignment", QString::number(d->horizontalAlignment) );
	writer->writeAttribute( "verticalAlignment", QString::number(d->verticalAlignment) );
	writer->writeAttribute( "rotationAngle", QString::number(d->rotationAngle) );
	writer->writeAttribute( "visible", QString::number(d->isVisible()) );
	writer->writeEndElement();

	writer->writeStartElement( "text" );
	writer->writeCharacters( d->textWrapper.text );
	writer->writeEndElement();

	writer->writeStartElement( "format" );
	writer->writeAttribute( "teXUsed", QString::number(d->textWrapper.teXUsed) );
	writer->writeAttribute( "teXFontSize", QString::number(d->teXFontSize) );
	writer->writeAttribute( "teXFontColor_r", QString::number(d->teXFontColor.red()) );
	writer->writeAttribute( "teXFontColor_g", QString::number(d->teXFontColor.green()) );
	writer->writeAttribute( "teXFontColor_b", QString::number(d->teXFontColor.blue()) );
	writer->writeEndElement();

	writer->writeEndElement(); // close "textLabel" section
}

//! Load from XML
bool TextLabel::load(XmlStreamReader* reader) {
	Q_D(TextLabel);

	if(!reader->isStartElement() || reader->name() != "textLabel") {
		reader->raiseError(i18n("no textLabel element found"));
		return false;
	}

	if (!readBasicAttributes(reader))
		return false;

	QString attributeWarning = i18n("Attribute '%1' missing or empty, default value is used");
	QXmlStreamAttributes attribs;
	QString str;
	QRectF rect;

	while (!reader->atEnd()) {
		reader->readNext();
		if (reader->isEndElement() && reader->name() == "textLabel")
			break;

		if (!reader->isStartElement())
			continue;

		if (reader->name() == "comment") {
			if (!readCommentElement(reader)) return false;
		} else if (reader->name() == "geometry") {
			attribs = reader->attributes();

			str = attribs.value("x").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'x'"));
			else
				d->position.point.setX(str.toDouble());

			str = attribs.value("y").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'y'"));
			else
				d->position.point.setY(str.toDouble());

			str = attribs.value("horizontalPosition").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'horizontalPosition'"));
			else
				d->position.horizontalPosition = (TextLabel::HorizontalPosition)str.toInt();

			str = attribs.value("verticalPosition").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'verticalPosition'"));
			else
				d->position.verticalPosition = (TextLabel::VerticalPosition)str.toInt();

			str = attribs.value("horizontalAlignment").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'horizontalAlignment'"));
			else
				d->horizontalAlignment = (TextLabel::HorizontalAlignment)str.toInt();

			str = attribs.value("verticalAlignment").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'verticalAlignment'"));
			else
				d->verticalAlignment = (TextLabel::VerticalAlignment)str.toInt();

			str = attribs.value("rotationAngle").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'rotationAngle'"));
			else
				d->rotationAngle = str.toInt();

			str = attribs.value("visible").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'visible'"));
			else
				d->setVisible(str.toInt());
		} else if (reader->name() == "text") {
			d->textWrapper.text = reader->readElementText();
		} else if (reader->name() == "format") {
			attribs = reader->attributes();

			str = attribs.value("teXUsed").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'teXUsed'"));
			else
				d->textWrapper.teXUsed = str.toInt();

			str = attribs.value("teXFontSize").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'teXFontSize'"));
			else
				d->teXFontSize = str.toInt();

			str = attribs.value("teXFontColor_r").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'teXFontColor_r'"));
			else
				d->teXFontColor.setRed( str.toInt() );

			str = attribs.value("teXFontColor_g").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'teXFontColor_g'"));
			else
				d->teXFontColor.setGreen( str.toInt() );

			str = attribs.value("teXFontColor_b").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'teXFontColor_b'"));
			else
				d->teXFontColor.setBlue( str.toInt() );
		} else { // unknown element
			reader->raiseWarning(i18n("unknown element '%1'", reader->name().toString()));
			if (!reader->skipToEndElement()) return false;
		}
	}

	d->updateText();

	return true;
}
