/***************************************************************************
    File                 : PlotArea.cpp
    Project              : LabPlot
    Description          : Plot area (for background filling and clipping).
    --------------------------------------------------------------------
    Copyright            : (C) 2011-2015 by Alexander Semke (alexander.semke@web.de)
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

#include "backend/worksheet/Worksheet.h"
#include "backend/worksheet/plots/PlotArea.h"
#include "backend/worksheet/plots/PlotAreaPrivate.h"
#include "backend/lib/commandtemplates.h"
#include "backend/lib/XmlStreamReader.h"

#include <QPainter>
#include <KConfig>
#include <KConfigGroup>
#include <KLocale>

/**
 * \class PlotArea
 * \brief Plot area (for background filling and clipping).
 *
 * \ingroup worksheet
 */

PlotArea::PlotArea(const QString &name):WorksheetElement(name),
	d_ptr(new PlotAreaPrivate(this)) {
	init();
}

PlotArea::PlotArea(const QString &name, PlotAreaPrivate *dd)
	: WorksheetElement(name), d_ptr(dd) {
	init();
}

PlotArea::~PlotArea() {
	//no need to delete the d-pointer here - it inherits from QGraphicsItem
	//and is deleted during the cleanup in QGraphicsScene
}

void PlotArea::init() {
	Q_D(PlotArea);

	setHidden(true);//we don't show PlotArea aspect in the model view.
	d->rect = QRectF(0, 0, 1, 1);
	d->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

	KConfig config;
	KConfigGroup group = config.group( "PlotArea" );

	//Background
	d->backgroundType = (PlotArea::BackgroundType) group.readEntry("BackgroundType", (int)PlotArea::Color);
	d->backgroundColorStyle = (PlotArea::BackgroundColorStyle) group.readEntry("BackgroundColorStyle", (int) PlotArea::SingleColor);
	d->backgroundImageStyle = (PlotArea::BackgroundImageStyle) group.readEntry("BackgroundImageStyle", (int) PlotArea::Scaled);
	d->backgroundBrushStyle = (Qt::BrushStyle) group.readEntry("BackgroundBrushStyle", (int) Qt::SolidPattern);
	d->backgroundFileName = group.readEntry("BackgroundFileName", QString());
	d->backgroundFirstColor = group.readEntry("BackgroundFirstColor", QColor(Qt::white));
	d->backgroundSecondColor = group.readEntry("BackgroundSecondColor", QColor(Qt::black));
	d->backgroundOpacity = group.readEntry("BackgroundOpacity", 1.0);

	//Border
	d->borderPen = QPen(group.readEntry("BorderColor", QColor(Qt::black)),
	                    group.readEntry("BorderWidth", Worksheet::convertToSceneUnits(1.0, Worksheet::Point)),
	                    (Qt::PenStyle) group.readEntry("BorderStyle", (int)Qt::SolidLine));
	d->borderCornerRadius = group.readEntry("BorderCornerRadius", 0.0);
	d->borderOpacity = group.readEntry("BorderOpacity", 1.0);
}

QGraphicsItem *PlotArea::graphicsItem() const {
	return d_ptr;
}

STD_SWAP_METHOD_SETTER_CMD_IMPL(PlotArea, SetVisible, bool, swapVisible)
void PlotArea::setVisible(bool on) {
	Q_D(PlotArea);
	exec(new PlotAreaSetVisibleCmd(d, on, on ? i18n("%1: set visible") : i18n("%1: set invisible")));
}

bool PlotArea::isVisible() const {
	Q_D(const PlotArea);
	return d->isVisible();
}

void PlotArea::handlePageResize(double horizontalRatio, double verticalRatio) {
	Q_D(PlotArea);

	d->rect.setWidth(d->rect.width()*horizontalRatio);
	d->rect.setHeight(d->rect.height()*verticalRatio);

	// TODO: scale line width
	BaseClass::handlePageResize(horizontalRatio, verticalRatio);
}

void PlotArea::retransform() {

}

/* ============================ getter methods ================= */
BASIC_SHARED_D_READER_IMPL(PlotArea, bool, clippingEnabled, clippingEnabled())
CLASS_SHARED_D_READER_IMPL(PlotArea, QRectF, rect, rect)

BASIC_SHARED_D_READER_IMPL(PlotArea, PlotArea::BackgroundType, backgroundType, backgroundType)
BASIC_SHARED_D_READER_IMPL(PlotArea, PlotArea::BackgroundColorStyle, backgroundColorStyle, backgroundColorStyle)
BASIC_SHARED_D_READER_IMPL(PlotArea, PlotArea::BackgroundImageStyle, backgroundImageStyle, backgroundImageStyle)
CLASS_SHARED_D_READER_IMPL(PlotArea, Qt::BrushStyle, backgroundBrushStyle, backgroundBrushStyle)
CLASS_SHARED_D_READER_IMPL(PlotArea, QColor, backgroundFirstColor, backgroundFirstColor)
CLASS_SHARED_D_READER_IMPL(PlotArea, QColor, backgroundSecondColor, backgroundSecondColor)
CLASS_SHARED_D_READER_IMPL(PlotArea, QString, backgroundFileName, backgroundFileName)
BASIC_SHARED_D_READER_IMPL(PlotArea, qreal, backgroundOpacity, backgroundOpacity)

CLASS_SHARED_D_READER_IMPL(PlotArea, QPen, borderPen, borderPen)
BASIC_SHARED_D_READER_IMPL(PlotArea, qreal, borderCornerRadius, borderCornerRadius)
BASIC_SHARED_D_READER_IMPL(PlotArea, qreal, borderOpacity, borderOpacity)


/* ============================ setter methods and undo commands ================= */

STD_SWAP_METHOD_SETTER_CMD_IMPL(PlotArea, SetClippingEnabled, bool, toggleClipping);
void PlotArea::setClippingEnabled(bool on) {
	Q_D(PlotArea);

	if (d->clippingEnabled() != on)
		exec(new PlotAreaSetClippingEnabledCmd(d, on, i18n("%1: toggle clipping")));
}

/*!
 * sets plot area rect in scene coordinates.
 */
void PlotArea::setRect(const QRectF &newRect) {
	Q_D(PlotArea);
	d->setRect(newRect);
}

//Background
STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBackgroundType, PlotArea::BackgroundType, backgroundType, update)
void PlotArea::setBackgroundType(BackgroundType type) {
	Q_D(PlotArea);
	if (type != d->backgroundType)
		exec(new PlotAreaSetBackgroundTypeCmd(d, type, i18n("%1: background type changed")));
}

STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBackgroundColorStyle, PlotArea::BackgroundColorStyle, backgroundColorStyle, update)
void PlotArea::setBackgroundColorStyle(BackgroundColorStyle style) {
	Q_D(PlotArea);
	if (style != d->backgroundColorStyle)
		exec(new PlotAreaSetBackgroundColorStyleCmd(d, style, i18n("%1: background color style changed")));
}

STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBackgroundImageStyle, PlotArea::BackgroundImageStyle, backgroundImageStyle, update)
void PlotArea::setBackgroundImageStyle(PlotArea::BackgroundImageStyle style) {
	Q_D(PlotArea);
	if (style != d->backgroundImageStyle)
		exec(new PlotAreaSetBackgroundImageStyleCmd(d, style, i18n("%1: background image style changed")));
}

STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBackgroundBrushStyle, Qt::BrushStyle, backgroundBrushStyle, update)
void PlotArea::setBackgroundBrushStyle(Qt::BrushStyle style) {
	Q_D(PlotArea);
	if (style != d->backgroundBrushStyle)
		exec(new PlotAreaSetBackgroundBrushStyleCmd(d, style, i18n("%1: background brush style changed")));
}

STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBackgroundFirstColor, QColor, backgroundFirstColor, update)
void PlotArea::setBackgroundFirstColor(const QColor &color) {
	Q_D(PlotArea);
	if (color!= d->backgroundFirstColor)
		exec(new PlotAreaSetBackgroundFirstColorCmd(d, color, i18n("%1: set background first color")));
}

STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBackgroundSecondColor, QColor, backgroundSecondColor, update)
void PlotArea::setBackgroundSecondColor(const QColor &color) {
	Q_D(PlotArea);
	if (color!= d->backgroundSecondColor)
		exec(new PlotAreaSetBackgroundSecondColorCmd(d, color, i18n("%1: set background second color")));
}

STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBackgroundFileName, QString, backgroundFileName, update)
void PlotArea::setBackgroundFileName(const QString& fileName) {
	Q_D(PlotArea);
	if (fileName!= d->backgroundFileName)
		exec(new PlotAreaSetBackgroundFileNameCmd(d, fileName, i18n("%1: set background image")));
}

STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBackgroundOpacity, qreal, backgroundOpacity, update)
void PlotArea::setBackgroundOpacity(qreal opacity) {
	Q_D(PlotArea);
	if (opacity != d->backgroundOpacity)
		exec(new PlotAreaSetBackgroundOpacityCmd(d, opacity, i18n("%1: set plot area opacity")));
}

//Border
STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBorderPen, QPen, borderPen, update)
void PlotArea::setBorderPen(const QPen &pen) {
	Q_D(PlotArea);
	if (pen != d->borderPen)
		exec(new PlotAreaSetBorderPenCmd(d, pen, i18n("%1: set plot area border")));
}

STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBorderCornerRadius, qreal, borderCornerRadius, update)
void PlotArea::setBorderCornerRadius(qreal radius) {
	Q_D(PlotArea);
	if (radius != d->borderCornerRadius)
		exec(new PlotAreaSetBorderCornerRadiusCmd(d, radius, i18n("%1: set plot area corner radius")));
}

STD_SETTER_CMD_IMPL_F_S(PlotArea, SetBorderOpacity, qreal, borderOpacity, update)
void PlotArea::setBorderOpacity(qreal opacity) {
	Q_D(PlotArea);
	if (opacity != d->borderOpacity)
		exec(new PlotAreaSetBorderOpacityCmd(d, opacity, i18n("%1: set plot area border opacity")));
}

//#####################################################################
//################### Private implementation ##########################
//#####################################################################
PlotAreaPrivate::PlotAreaPrivate(PlotArea *owner):q(owner) {
}

PlotAreaPrivate::~PlotAreaPrivate() {
}

QString PlotAreaPrivate::name() const {
	return q->name();
}

bool PlotAreaPrivate::clippingEnabled() const {
	return (flags() & QGraphicsItem::ItemClipsChildrenToShape);
}

bool PlotAreaPrivate::toggleClipping(bool on) {
	bool oldValue = clippingEnabled();
	setFlag(QGraphicsItem::ItemClipsChildrenToShape, on);
	return oldValue;
}

bool PlotAreaPrivate::swapVisible(bool on) {
	bool oldValue = isVisible();
	setVisible(on);
	return oldValue;
}

void PlotAreaPrivate::setRect(const QRectF& r) {
	prepareGeometryChange();
	rect = mapRectFromScene(r);
}

QRectF PlotAreaPrivate::boundingRect () const {
	float width = rect.width();
	float height = rect.height();
	float penWidth = borderPen.width();
	return QRectF(-width/2 - penWidth/2, -height/2 - penWidth/2,
	              width + penWidth, height + penWidth);
}

QPainterPath PlotAreaPrivate::shape() const {
	QPainterPath path;
	if ( qFuzzyIsNull(borderCornerRadius) )
		path.addRect(rect);
	else
		path.addRoundedRect(rect, borderCornerRadius, borderCornerRadius);

	return path;
}

void PlotAreaPrivate::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option)
	Q_UNUSED(widget)

	if (!isVisible())
		return;

	//draw the area
	painter->setOpacity(backgroundOpacity);
	painter->setPen(Qt::NoPen);
	if (backgroundType == PlotArea::Color) {
		switch (backgroundColorStyle) {
		case PlotArea::SingleColor: {
			painter->setBrush(QBrush(backgroundFirstColor));
			break;
		}
		case PlotArea::HorizontalLinearGradient: {
			QLinearGradient linearGrad(rect.topLeft(), rect.topRight());
			linearGrad.setColorAt(0, backgroundFirstColor);
			linearGrad.setColorAt(1, backgroundSecondColor);
			painter->setBrush(QBrush(linearGrad));
			break;
		}
		case PlotArea::VerticalLinearGradient: {
			QLinearGradient linearGrad(rect.topLeft(), rect.bottomLeft());
			linearGrad.setColorAt(0, backgroundFirstColor);
			linearGrad.setColorAt(1, backgroundSecondColor);
			painter->setBrush(QBrush(linearGrad));
			break;
		}
		case PlotArea::TopLeftDiagonalLinearGradient: {
			QLinearGradient linearGrad(rect.topLeft(), rect.bottomRight());
			linearGrad.setColorAt(0, backgroundFirstColor);
			linearGrad.setColorAt(1, backgroundSecondColor);
			painter->setBrush(QBrush(linearGrad));
			break;
		}
		case PlotArea::BottomLeftDiagonalLinearGradient: {
			QLinearGradient linearGrad(rect.bottomLeft(), rect.topRight());
			linearGrad.setColorAt(0, backgroundFirstColor);
			linearGrad.setColorAt(1, backgroundSecondColor);
			painter->setBrush(QBrush(linearGrad));
			break;
		}
		case PlotArea::RadialGradient: {
			QRadialGradient radialGrad(rect.center(), rect.width()/2);
			radialGrad.setColorAt(0, backgroundFirstColor);
			radialGrad.setColorAt(1, backgroundSecondColor);
			painter->setBrush(QBrush(radialGrad));
			break;
		}
		}
	} else if (backgroundType == PlotArea::Image) {
		if ( !backgroundFileName.trimmed().isEmpty() ) {
			QPixmap pix(backgroundFileName);
			switch (backgroundImageStyle) {
			case PlotArea::ScaledCropped:
				pix = pix.scaled(rect.size().toSize(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);
				painter->setBrush(QBrush(pix));
				painter->setBrushOrigin(pix.size().width()/2,pix.size().height()/2);
				painter->drawRoundedRect(rect, borderCornerRadius, borderCornerRadius);
				break;
			case PlotArea::Scaled:
				pix = pix.scaled(rect.size().toSize(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
				painter->setBrush(QBrush(pix));
				painter->setBrushOrigin(pix.size().width()/2,pix.size().height()/2);
				painter->drawRoundedRect(rect, borderCornerRadius, borderCornerRadius);
				break;
			case PlotArea::ScaledAspectRatio:
				pix = pix.scaled(rect.size().toSize(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
				painter->setBrush(QBrush(pix));
				painter->setBrushOrigin(pix.size().width()/2,pix.size().height()/2);
				painter->drawRoundedRect(rect, borderCornerRadius, borderCornerRadius);
				break;
			case PlotArea::Centered:
				painter->drawPixmap(QPointF(rect.center().x()-pix.size().width()/2,rect.center().y()-pix.size().height()/2),pix);
				break;
			case PlotArea::Tiled:
				painter->setBrush(QBrush(pix));
				painter->drawRoundedRect(rect, borderCornerRadius, borderCornerRadius);
				break;
			case PlotArea::CenterTiled:
				painter->setBrush(QBrush(pix));
				painter->setBrushOrigin(pix.size().width()/2,pix.size().height()/2);
				painter->drawRoundedRect(rect, borderCornerRadius, borderCornerRadius);
			}
		}
	} else if (backgroundType == PlotArea::Pattern) {
		painter->setBrush(QBrush(backgroundFirstColor,backgroundBrushStyle));
	}

	if ( qFuzzyIsNull(borderCornerRadius) )
		painter->drawRect(rect);
	else
		painter->drawRoundedRect(rect, borderCornerRadius, borderCornerRadius);

	//draw the border
	if (borderPen.style() != Qt::NoPen) {
		painter->setPen(borderPen);
		painter->setBrush(Qt::NoBrush);
		painter->setOpacity(borderOpacity);
		if ( qFuzzyIsNull(borderCornerRadius) )
			painter->drawRect(rect);
		else
			painter->drawRoundedRect(rect, borderCornerRadius, borderCornerRadius);
	}
}


//##############################################################################
//##################  Serialization/Deserialization  ###########################
//##############################################################################

//! Save as XML
void PlotArea::save(QXmlStreamWriter* writer) const {
	Q_D(const PlotArea);

	writer->writeStartElement("plotArea");
	writeBasicAttributes(writer);
	writeCommentElement(writer);

	writer->writeStartElement( "background" );
	writer->writeAttribute( "type", QString::number(d->backgroundType) );
	writer->writeAttribute( "colorStyle", QString::number(d->backgroundColorStyle) );
	writer->writeAttribute( "imageStyle", QString::number(d->backgroundImageStyle) );
	writer->writeAttribute( "brushStyle", QString::number(d->backgroundBrushStyle) );
	writer->writeAttribute( "firstColor_r", QString::number(d->backgroundFirstColor.red()) );
	writer->writeAttribute( "firstColor_g", QString::number(d->backgroundFirstColor.green()) );
	writer->writeAttribute( "firstColor_b", QString::number(d->backgroundFirstColor.blue()) );
	writer->writeAttribute( "secondColor_r", QString::number(d->backgroundSecondColor.red()) );
	writer->writeAttribute( "secondColor_g", QString::number(d->backgroundSecondColor.green()) );
	writer->writeAttribute( "secondColor_b", QString::number(d->backgroundSecondColor.blue()) );
	writer->writeAttribute( "fileName", d->backgroundFileName );
	writer->writeAttribute( "opacity", QString::number(d->backgroundOpacity) );
	writer->writeEndElement();

	//border
	writer->writeStartElement( "border" );
	WRITE_QPEN(d->borderPen);
	writer->writeAttribute( "borderOpacity", QString::number(d->borderOpacity) );
	writer->writeAttribute( "borderCornerRadius", QString::number(d->borderCornerRadius) );
	writer->writeEndElement();

	writer->writeEndElement();
}

//! Load from XML
bool PlotArea::load(XmlStreamReader* reader) {
	Q_D(PlotArea);

	if(!reader->isStartElement() || reader->name() != "plotArea") {
		reader->raiseError(i18n("no plot area element found"));
		return false;
	}

	if (!readBasicAttributes(reader))
		return false;

	QString attributeWarning = i18n("Attribute '%1' missing or empty, default value is used");
	QXmlStreamAttributes attribs;
	QString str;

	while (!reader->atEnd()) {
		reader->readNext();
		if (reader->isEndElement() && reader->name() == "plotArea")
			break;

		if (!reader->isStartElement())
			continue;

		if (reader->name() == "comment") {
			if (!readCommentElement(reader)) return false;
		} else if (reader->name() == "background") {
			attribs = reader->attributes();

			str = attribs.value("type").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("type"));
			else
				d->backgroundType = PlotArea::BackgroundType(str.toInt());

			str = attribs.value("colorStyle").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("colorStyle"));
			else
				d->backgroundColorStyle = PlotArea::BackgroundColorStyle(str.toInt());

			str = attribs.value("imageStyle").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("imageStyle"));
			else
				d->backgroundImageStyle = PlotArea::BackgroundImageStyle(str.toInt());

			str = attribs.value("brushStyle").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("brushStyle"));
			else
				d->backgroundBrushStyle = Qt::BrushStyle(str.toInt());

			str = attribs.value("firstColor_r").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("firstColor_r"));
			else
				d->backgroundFirstColor.setRed(str.toInt());

			str = attribs.value("firstColor_g").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("firstColor_g"));
			else
				d->backgroundFirstColor.setGreen(str.toInt());

			str = attribs.value("firstColor_b").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("firstColor_b"));
			else
				d->backgroundFirstColor.setBlue(str.toInt());

			str = attribs.value("secondColor_r").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("secondColor_r"));
			else
				d->backgroundSecondColor.setRed(str.toInt());

			str = attribs.value("secondColor_g").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("secondColor_g"));
			else
				d->backgroundSecondColor.setGreen(str.toInt());

			str = attribs.value("secondColor_b").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("secondColor_b"));
			else
				d->backgroundSecondColor.setBlue(str.toInt());

			str = attribs.value("fileName").toString();
			d->backgroundFileName = str;

			str = attribs.value("opacity").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("opacity"));
			else
				d->backgroundOpacity = str.toDouble();
		} else if (reader->name() == "border") {
			attribs = reader->attributes();

			READ_QPEN(d->borderPen);

			str = attribs.value("borderOpacity").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("borderOpacity"));
			else
				d->borderOpacity = str.toDouble();

			str = attribs.value("borderCornerRadius").toString();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("borderCornerRadius"));
			else
				d->borderCornerRadius = str.toDouble();
		} else { // unknown element
			reader->raiseWarning(i18n("unknown element '%1'", reader->name().toString()));
			if (!reader->skipToEndElement()) return false;
		}
	}

	return true;
}
