/***************************************************************************
    File                 : Axis.cpp
    Project              : LabPlot
    Description          : Axis for cartesian coordinate systems.
    --------------------------------------------------------------------
    Copyright            : (C) 2011-2015 Alexander Semke (alexander.semke@web.de)
    Copyright            : (C) 2013 Stefan Gerlach  (stefan.gerlach@uni-konstanz.de)

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

#include "backend/worksheet/plots/cartesian/Axis.h"
#include "backend/worksheet/plots/cartesian/AxisPrivate.h"
#include "backend/worksheet/Worksheet.h"
#include "backend/worksheet/TextLabel.h"
#include "backend/worksheet/plots/cartesian/CartesianCoordinateSystem.h"
#include "backend/worksheet/plots/cartesian/CartesianPlot.h"
#include "backend/core/AbstractColumn.h"
#include "backend/lib/commandtemplates.h"
#include "backend/lib/XmlStreamReader.h"
#include "backend/lib/macros.h"

#include <QPainter>
#include <QMenu>
#include <QDebug>
#include <QTextDocument>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>

#include "kdefrontend/GuiTools.h"
#include <KConfigGroup>
#include <KIcon>
#include <KLocale>

#include <cmath>
#include <cfloat>

/**
 * \class AxisGrid
 * \brief Helper class to get the axis grid drawn with the z-Value=0.
 *
 * The painting of the grid lines is separated from the painting of the axis itself.
 * This allows to use a different z-values for the grid lines (z=0, drawn below all other objects )
 * and for the axis (z=FLT_MAX, drawn on top of all other objects)
 *
 *  \ingroup worksheet
 */
class AxisGrid : public QGraphicsItem {
	public:
		AxisGrid(AxisPrivate* a) {
			axis = a;
			setFlag(QGraphicsItem::ItemIsSelectable, false);
			setFlag(QGraphicsItem::ItemIsFocusable, false);
			setAcceptHoverEvents(false);
		}

		QRectF boundingRect() const {
			QPainterPath gridShape;
			gridShape.addPath(WorksheetElement::shapeFromPath(axis->majorGridPath, axis->majorGridPen));
			gridShape.addPath(WorksheetElement::shapeFromPath(axis->minorGridPath, axis->minorGridPen));
			QRectF boundingRectangle = gridShape.boundingRect();
			return boundingRectangle;
		}

		void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
			Q_UNUSED(option)
			Q_UNUSED(widget)

			if (!axis->isVisible()) return;
			if (axis->linePath.isEmpty()) return;

			//draw major grid
			if (axis->majorGridPen.style() != Qt::NoPen) {
				painter->setOpacity(axis->majorGridOpacity);
				painter->setPen(axis->majorGridPen);
				painter->setBrush(Qt::NoBrush);
				painter->drawPath(axis->majorGridPath);
			}

			//draw minor grid
			if (axis->minorGridPen.style() != Qt::NoPen) {
				painter->setOpacity(axis->minorGridOpacity);
				painter->setPen(axis->minorGridPen);
				painter->setBrush(Qt::NoBrush);
				painter->drawPath(axis->minorGridPath);
			}
		}

	private:
		AxisPrivate* axis;
};

/**
 * \class Axis
 * \brief Axis for cartesian coordinate systems.
 *
 *  \ingroup worksheet
 */
Axis::Axis(const QString &name, const AxisOrientation &orientation)
		: WorksheetElement(name), d_ptr(new AxisPrivate(this)) {
	d_ptr->orientation = orientation;
	init();
}

Axis::Axis(const QString &name, const AxisOrientation &orientation, AxisPrivate *dd)
		: WorksheetElement(name), d_ptr(dd) {
	d_ptr->orientation = orientation;
	init();
}

void Axis::init() {
	Q_D(Axis);

	KConfig config;
	KConfigGroup group = config.group( "Axis" );

	d->autoScale = true;
	d->position = Axis::AxisCustom;
	d->offset = group.readEntry("PositionOffset", 0);
	d->scale = (Axis::AxisScale) group.readEntry("Scale", (int) Axis::ScaleLinear);
	d->autoScale = group.readEntry("AutoScale", true);
	d->start = group.readEntry("Start", 0);
	d->end = group.readEntry("End", 10);
	d->zeroOffset = group.readEntry("ZeroOffset", 0);
	d->scalingFactor = group.readEntry("ScalingFactor", 1.0);

	d->linePen.setStyle( (Qt::PenStyle) group.readEntry("LineStyle", (int) Qt::SolidLine) );
	d->linePen.setWidthF( group.readEntry("LineWidth", Worksheet::convertToSceneUnits( 1.0, Worksheet::Point ) ) );
	d->lineOpacity = group.readEntry("LineOpacity", 1.0);
	d->arrowType = (Axis::ArrowType) group.readEntry("ArrowType", (int)Axis::NoArrow);
	d->arrowPosition = (Axis::ArrowPosition) group.readEntry("ArrowPosition", (int)Axis::ArrowRight);
	d->arrowSize = group.readEntry("ArrowSize", Worksheet::convertToSceneUnits(10, Worksheet::Point));

	// axis title
 	d->title = new TextLabel(this->name(), TextLabel::AxisTitle);
	connect( d->title, SIGNAL(changed()), this, SLOT(labelChanged()) );
	addChild(d->title);
	d->title->setHidden(true);
	d->title->graphicsItem()->setParentItem(graphicsItem());
	d->title->graphicsItem()->setFlag(QGraphicsItem::ItemIsMovable, false);
	d->title->graphicsItem()->setAcceptHoverEvents(false);
	d->title->setText(this->name());
	if (d->orientation == AxisVertical) d->title->setRotationAngle(90);
	d->titleOffsetX = Worksheet::convertToSceneUnits(2, Worksheet::Point); //distance to the axis tick labels
	d->titleOffsetY = Worksheet::convertToSceneUnits(2, Worksheet::Point); //distance to the axis tick labels

	d->majorTicksDirection = (Axis::TicksDirection) group.readEntry("MajorTicksDirection", (int) Axis::ticksOut);
	d->majorTicksType = (Axis::TicksType) group.readEntry("MajorTicksType", (int) Axis::TicksTotalNumber);
	d->majorTicksNumber = group.readEntry("MajorTicksNumber", 11);
	d->majorTicksIncrement = group.readEntry("MajorTicksIncrement", 1.0);
	d->majorTicksPen.setStyle((Qt::PenStyle) group.readEntry("MajorTicksLineStyle", (int)Qt::SolidLine) );
	d->majorTicksPen.setColor( group.readEntry("MajorTicksColor", QColor(Qt::black) ) );
	d->majorTicksPen.setWidthF( group.readEntry("MajorTicksWidth", Worksheet::convertToSceneUnits(1.0, Worksheet::Point) ) );
	d->majorTicksLength = group.readEntry("MajorTicksLength", Worksheet::convertToSceneUnits(6.0, Worksheet::Point));
	d->majorTicksOpacity = group.readEntry("MajorTicksOpacity", 1.0);

	d->minorTicksDirection = (Axis::TicksDirection) group.readEntry("MinorTicksDirection", (int) Axis::ticksOut);
	d->minorTicksType = (Axis::TicksType) group.readEntry("MinorTicksType", (int) Axis::TicksTotalNumber);
	d->minorTicksNumber = group.readEntry("MinorTicksNumber", 1);
	d->minorTicksIncrement = group.readEntry("MinorTicksIncrement", 0.5);
	d->minorTicksPen.setStyle((Qt::PenStyle) group.readEntry("MinorTicksLineStyle", (int)Qt::SolidLine) );
	d->minorTicksPen.setColor( group.readEntry("MinorTicksColor", QColor(Qt::black) ) );
	d->minorTicksPen.setWidthF( group.readEntry("MinorTicksWidth", Worksheet::convertToSceneUnits(1.0, Worksheet::Point) ) );
	d->minorTicksLength = group.readEntry("MinorTicksLength", Worksheet::convertToSceneUnits(3.0, Worksheet::Point));
	d->minorTicksOpacity = group.readEntry("MinorTicksOpacity", 1.0);

	//Labels
	d->labelsFormat = (Axis::LabelsFormat) group.readEntry("LabelsFormat", (int)Axis::FormatDecimal);
	d->labelsAutoPrecision = group.readEntry("LabelsAutoPrecision", true);
	d->labelsPrecision = group.readEntry("LabelsPrecision", 1);
	d->labelsPosition = (Axis::LabelsPosition) group.readEntry("LabelsPosition", (int) Axis::LabelsOut);
	d->labelsOffset= group.readEntry("LabelsOffset",  Worksheet::convertToSceneUnits( 5.0, Worksheet::Point ));
	d->labelsRotationAngle = group.readEntry("LabelsRotation", 0);
	d->labelsFont = group.readEntry("LabelsFont", QFont());
	d->labelsFont.setPixelSize( Worksheet::convertToSceneUnits( 10.0, Worksheet::Point ) );
	d->labelsColor = group.readEntry("LabelsFontColor", QColor(Qt::black));
	d->labelsPrefix =  group.readEntry("LabelsPrefix", "" );
	d->labelsSuffix =  group.readEntry("LabelsSuffix", "" );
	d->labelsOpacity = group.readEntry("LabelsOpacity", 1.0);

	//major grid
	d->majorGridPen.setStyle( (Qt::PenStyle) group.readEntry("MajorGridStyle", (int) Qt::NoPen) );
	d->majorGridPen.setColor(group.readEntry("MajorGridColor", QColor(Qt::gray)) );
	d->majorGridPen.setWidthF( group.readEntry("MajorGridWidth", Worksheet::convertToSceneUnits( 1.0, Worksheet::Point ) ) );
	d->majorGridOpacity = group.readEntry("MajorGridOpacity", 1.0);

	//minor grid
	d->minorGridPen.setStyle( (Qt::PenStyle) group.readEntry("MinorGridStyle", (int) Qt::NoPen) );
	d->minorGridPen.setColor(group.readEntry("MajorGridColor", QColor(Qt::gray)) );
	d->minorGridPen.setWidthF( group.readEntry("MinorGridWidth", Worksheet::convertToSceneUnits( 1.0, Worksheet::Point ) ) );
	d->minorGridOpacity = group.readEntry("MinorGridOpacity", 1.0);

	this->initActions();
	this->initMenus();
}

/*!
 * For the most frequently edited properties, create Actions and ActionGroups for the context menu.
 * For some ActionGroups the actual actions are created in \c GuiTool,
 */
void Axis::initActions() {
	visibilityAction = new QAction(i18n("visible"), this);
	visibilityAction->setCheckable(true);
	connect(visibilityAction, SIGNAL(triggered()), this, SLOT(visibilityChanged()));

	//Orientation
	orientationActionGroup = new QActionGroup(this);
	orientationActionGroup->setExclusive(true);
	connect(orientationActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(orientationChanged(QAction*)));

	orientationHorizontalAction = new QAction(i18n("horizontal"), orientationActionGroup);
	orientationHorizontalAction->setCheckable(true);

	orientationVerticalAction = new QAction(i18n("vertical"), orientationActionGroup);
	orientationVerticalAction->setCheckable(true);

	//Line
	lineStyleActionGroup = new QActionGroup(this);
	lineStyleActionGroup->setExclusive(true);
	connect(lineStyleActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(lineStyleChanged(QAction*)));

	lineColorActionGroup = new QActionGroup(this);
	lineColorActionGroup->setExclusive(true);
	connect(lineColorActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(lineColorChanged(QAction*)));

	//Ticks
	//TODO
}

void Axis::initMenus() {
	//Orientation
	orientationMenu = new QMenu(i18n("Orientation"));
	orientationMenu->addAction(orientationHorizontalAction);
	orientationMenu->addAction(orientationVerticalAction);

	//Line
	lineMenu = new QMenu(i18n("Line"));
	lineStyleMenu = new QMenu(i18n("style"), lineMenu);
	lineMenu->addMenu( lineStyleMenu );

	lineColorMenu = new QMenu(i18n("color"), lineMenu);
	GuiTools::fillColorMenu( lineColorMenu, lineColorActionGroup );
	lineMenu->addMenu( lineColorMenu );
}

QMenu* Axis::createContextMenu() {
	Q_D(const Axis);
	QMenu* menu = WorksheetElement::createContextMenu();
	QAction* firstAction = menu->actions().at(1); //skip the first action because of the "title-action"

	visibilityAction->setChecked(isVisible());
	menu->insertAction(firstAction, visibilityAction);

	//Orientation
	if ( d->orientation == AxisHorizontal )
		orientationHorizontalAction->setChecked(true);
	else
		orientationVerticalAction->setChecked(true);

	menu->insertMenu(firstAction, orientationMenu);

	//Line styles
	GuiTools::updatePenStyles( lineStyleMenu, lineStyleActionGroup, d->linePen.color() );
	GuiTools::selectPenStyleAction(lineStyleActionGroup, d->linePen.style() );

	GuiTools::selectColorAction(lineColorActionGroup, d->linePen.color() );

	menu->insertMenu(firstAction, lineMenu);
	menu->insertSeparator(firstAction);

	return menu;
}

/*!
	Returns an icon to be used in the project explorer.
*/
QIcon Axis::icon() const{
	Q_D(const Axis);
	QIcon ico;
	if (d->orientation == Axis::AxisHorizontal)
		ico = KIcon("labplot-axis-horizontal");
	else
		ico = KIcon("labplot-axis-vertical");

	return ico;
}

Axis::~Axis() {
	if (orientationMenu)
		delete orientationMenu;

	if (lineMenu)
		delete lineMenu;

	//no need to delete d->title, since it was added with addChild in init();

	//no need to delete the d-pointer here - it inherits from QGraphicsItem
	//and is deleted during the cleanup in QGraphicsScene
}

QGraphicsItem *Axis::graphicsItem() const {
	return d_ptr;
}

/*!
 * overrides the implementation in WorkhseetElement and sets the z-value to the maximal possible,
 * axes are drawn on top of all other object in the plot.
 */
void Axis::setZValue(qreal) {
	Q_D(Axis);
	d->setZValue(FLT_MAX);
	d->gridItem->setParentItem(d->parentItem());
	d->gridItem->setZValue(0);
}

void Axis::retransform() {
	Q_D(Axis);
	d->retransform();
}

void Axis::handlePageResize(double horizontalRatio, double verticalRatio) {
	Q_D(Axis);

	QPen pen = d->linePen;
	pen.setWidthF(pen.widthF() * (horizontalRatio + verticalRatio) / 2.0);
	setLinePen(pen);

	if (d->orientation == Axis::AxisHorizontal) {
		setMajorTicksLength(d->majorTicksLength * verticalRatio); // ticks are perpendicular to axis line -> verticalRatio relevant
		setMinorTicksLength(d->minorTicksLength * verticalRatio);
		//TODO setLabelsFontSize(d->labelsFontSize * verticalRatio);
	} else {
		setMajorTicksLength(d->majorTicksLength * horizontalRatio);
		setMinorTicksLength(d->minorTicksLength * horizontalRatio);
		//TODO setLabelsFontSize(d->labelsFontSize * verticalRatio); // this is not perfectly correct for rotated labels
															// when the page aspect ratio changes, but should not matter
	}
	//TODO setLabelsOffset(QPointF(d->labelsOffset.x() * horizontalRatio, d->labelsOffset.y() * verticalRatio));

	retransform();
	BaseClass::handlePageResize(horizontalRatio, verticalRatio);
}

/* ============================ getter methods ================= */
BASIC_SHARED_D_READER_IMPL(Axis, bool, autoScale, autoScale)
BASIC_SHARED_D_READER_IMPL(Axis, Axis::AxisOrientation, orientation, orientation)
BASIC_SHARED_D_READER_IMPL(Axis, Axis::AxisPosition, position, position)
BASIC_SHARED_D_READER_IMPL(Axis, Axis::AxisScale, scale, scale)
BASIC_SHARED_D_READER_IMPL(Axis, float, offset, offset)
BASIC_SHARED_D_READER_IMPL(Axis, float, start, start)
BASIC_SHARED_D_READER_IMPL(Axis, float, end, end)
BASIC_SHARED_D_READER_IMPL(Axis, qreal, scalingFactor, scalingFactor)
BASIC_SHARED_D_READER_IMPL(Axis, qreal, zeroOffset, zeroOffset)

BASIC_SHARED_D_READER_IMPL(Axis, TextLabel*, title, title)
BASIC_SHARED_D_READER_IMPL(Axis, float, titleOffsetX, titleOffsetX)
BASIC_SHARED_D_READER_IMPL(Axis, float, titleOffsetY, titleOffsetY)

CLASS_SHARED_D_READER_IMPL(Axis, QPen, linePen, linePen)
BASIC_SHARED_D_READER_IMPL(Axis, qreal, lineOpacity, lineOpacity)
BASIC_SHARED_D_READER_IMPL(Axis, Axis::ArrowType, arrowType, arrowType)
BASIC_SHARED_D_READER_IMPL(Axis, Axis::ArrowPosition, arrowPosition, arrowPosition)
BASIC_SHARED_D_READER_IMPL(Axis, float, arrowSize, arrowSize)

BASIC_SHARED_D_READER_IMPL(Axis, Axis::TicksDirection, majorTicksDirection, majorTicksDirection)
BASIC_SHARED_D_READER_IMPL(Axis, Axis::TicksType, majorTicksType, majorTicksType)
BASIC_SHARED_D_READER_IMPL(Axis, int, majorTicksNumber, majorTicksNumber)
BASIC_SHARED_D_READER_IMPL(Axis, qreal, majorTicksIncrement, majorTicksIncrement)
BASIC_SHARED_D_READER_IMPL(Axis, const AbstractColumn*, majorTicksColumn, majorTicksColumn)
QString& Axis::majorTicksColumnPath() const { return d_ptr->majorTicksColumnPath; }
BASIC_SHARED_D_READER_IMPL(Axis, qreal, majorTicksLength, majorTicksLength)
CLASS_SHARED_D_READER_IMPL(Axis, QPen, majorTicksPen, majorTicksPen)
BASIC_SHARED_D_READER_IMPL(Axis, qreal, majorTicksOpacity, majorTicksOpacity)

BASIC_SHARED_D_READER_IMPL(Axis, Axis::TicksDirection, minorTicksDirection, minorTicksDirection)
BASIC_SHARED_D_READER_IMPL(Axis, Axis::TicksType, minorTicksType, minorTicksType)
BASIC_SHARED_D_READER_IMPL(Axis, int, minorTicksNumber, minorTicksNumber)
BASIC_SHARED_D_READER_IMPL(Axis, qreal, minorTicksIncrement, minorTicksIncrement)
BASIC_SHARED_D_READER_IMPL(Axis, const AbstractColumn*, minorTicksColumn, minorTicksColumn)
QString& Axis::minorTicksColumnPath() const { return d_ptr->minorTicksColumnPath; }
BASIC_SHARED_D_READER_IMPL(Axis, qreal, minorTicksLength, minorTicksLength)
CLASS_SHARED_D_READER_IMPL(Axis, QPen, minorTicksPen, minorTicksPen)
BASIC_SHARED_D_READER_IMPL(Axis, qreal, minorTicksOpacity, minorTicksOpacity)

BASIC_SHARED_D_READER_IMPL(Axis, Axis::LabelsFormat, labelsFormat, labelsFormat);
BASIC_SHARED_D_READER_IMPL(Axis, bool, labelsAutoPrecision, labelsAutoPrecision);
BASIC_SHARED_D_READER_IMPL(Axis, int, labelsPrecision, labelsPrecision);
BASIC_SHARED_D_READER_IMPL(Axis, Axis::LabelsPosition, labelsPosition, labelsPosition);
BASIC_SHARED_D_READER_IMPL(Axis, float, labelsOffset, labelsOffset);
BASIC_SHARED_D_READER_IMPL(Axis, qreal, labelsRotationAngle, labelsRotationAngle);
CLASS_SHARED_D_READER_IMPL(Axis, QColor, labelsColor, labelsColor);
CLASS_SHARED_D_READER_IMPL(Axis, QFont, labelsFont, labelsFont);
CLASS_SHARED_D_READER_IMPL(Axis, QString, labelsPrefix, labelsPrefix);
CLASS_SHARED_D_READER_IMPL(Axis, QString, labelsSuffix, labelsSuffix);
BASIC_SHARED_D_READER_IMPL(Axis, qreal, labelsOpacity, labelsOpacity);

CLASS_SHARED_D_READER_IMPL(Axis, QPen, majorGridPen, majorGridPen)
BASIC_SHARED_D_READER_IMPL(Axis, qreal, majorGridOpacity, majorGridOpacity)
CLASS_SHARED_D_READER_IMPL(Axis, QPen, minorGridPen, minorGridPen)
BASIC_SHARED_D_READER_IMPL(Axis, qreal, minorGridOpacity, minorGridOpacity)

/* ============================ setter methods and undo commands ================= */
STD_SETTER_CMD_IMPL_F_S(Axis, SetAutoScale, bool, autoScale, retransform);
void Axis::setAutoScale(bool autoScale) {
	Q_D(Axis);
	if (autoScale != d->autoScale) {
		exec(new AxisSetAutoScaleCmd(d, autoScale, i18n("%1: set axis auto scaling")));

		if (autoScale) {
			CartesianPlot *plot = qobject_cast<CartesianPlot*>(parentAspect());
			if (!plot)
				return;

			if (d->orientation == Axis::AxisHorizontal) {
				d->end = plot->xMax();
				d->start = plot->xMin();
			} else {
				d->end = plot->yMax();
				d->start = plot->yMin();
			}
			retransform();
			emit endChanged(d->end);
			emit startChanged(d->start);
		}
	}
}

STD_SWAP_METHOD_SETTER_CMD_IMPL(Axis, SetVisible, bool, swapVisible);
void Axis::setVisible(bool on) {
	Q_D(Axis);
	exec(new AxisSetVisibleCmd(d, on, on ? i18n("%1: set visible") : i18n("%1: set invisible")));
}

bool Axis::isVisible() const {
	Q_D(const Axis);
	return d->isVisible();
}

void Axis::setPrinting(bool on) {
	Q_D(Axis);
	d->m_printing = on;
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetOrientation, Axis::AxisOrientation, orientation, retransform);
void Axis::setOrientation( AxisOrientation orientation) {
	Q_D(Axis);
	if (orientation != d->orientation)
		exec(new AxisSetOrientationCmd(d, orientation, i18n("%1: set axis orientation")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetPosition, Axis::AxisPosition, position, retransform);
void Axis::setPosition(AxisPosition position) {
	Q_D(Axis);
	if (position != d->position)
		exec(new AxisSetPositionCmd(d, position, i18n("%1: set axis position")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetScaling, Axis::AxisScale, scale, retransformTicks);
void Axis::setScale(AxisScale scale) {
	Q_D(Axis);
	if (scale != d->scale)
		exec(new AxisSetScalingCmd(d, scale, i18n("%1: set axis scale")));
}

STD_SETTER_CMD_IMPL_F(Axis, SetOffset, float, offset, retransform);
void Axis::setOffset(float offset, bool undo) {
	Q_D(Axis);
	if (offset != d->offset) {
		if (undo) {
			exec(new AxisSetOffsetCmd(d, offset, i18n("%1: set axis offset")));
		} else {
			d->offset = offset;
			//don't need to call retransform() afterward
			//since the only usage of this call is in CartesianPlot, where retransform is called for all children anyway.
		}
		emit positionChanged(offset);
	}
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetStart, float, start, retransform);
void Axis::setStart(float start) {
	Q_D(Axis);
	if (start != d->start)
		exec(new AxisSetStartCmd(d, start, i18n("%1: set axis start")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetEnd, float, end, retransform);
void Axis::setEnd(float end) {
	Q_D(Axis);
	if (end != d->end)
		exec(new AxisSetEndCmd(d, end, i18n("%1: set axis end")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetZeroOffset, qreal, zeroOffset, retransform);
void Axis::setZeroOffset(qreal zeroOffset) {
	Q_D(Axis);
	if (zeroOffset != d->zeroOffset)
		exec(new AxisSetZeroOffsetCmd(d, zeroOffset, i18n("%1: set axis zero offset")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetScalingFactor, qreal, scalingFactor, retransform);
void Axis::setScalingFactor(qreal scalingFactor) {
	Q_D(Axis);
	if (scalingFactor != d->scalingFactor)
		exec(new AxisSetScalingFactorCmd(d, scalingFactor, i18n("%1: set axis scaling factor")));
}

//Title
STD_SETTER_CMD_IMPL_F_S(Axis, SetTitleOffsetX, float, titleOffsetX, retransform);
void Axis::setTitleOffsetX(float offset) {
	Q_D(Axis);
	if (offset != d->titleOffsetX)
		exec(new AxisSetTitleOffsetXCmd(d, offset, i18n("%1: set title offset")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetTitleOffsetY, float, titleOffsetY, retransform);
void Axis::setTitleOffsetY(float offset) {
	Q_D(Axis);
	if (offset != d->titleOffsetY)
		exec(new AxisSetTitleOffsetYCmd(d, offset, i18n("%1: set title offset")));
}

//Line
STD_SETTER_CMD_IMPL_F_S(Axis, SetLinePen, QPen, linePen, recalcShapeAndBoundingRect);
void Axis::setLinePen(const QPen &pen) {
	Q_D(Axis);
	if (pen != d->linePen)
		exec(new AxisSetLinePenCmd(d, pen, i18n("%1: set line style")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLineOpacity, qreal, lineOpacity, update);
void Axis::setLineOpacity(qreal opacity) {
	Q_D(Axis);
	if (opacity != d->lineOpacity)
		exec(new AxisSetLineOpacityCmd(d, opacity, i18n("%1: set line opacity")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetArrowType, Axis::ArrowType, arrowType, retransformArrow);
void Axis::setArrowType(ArrowType type) {
	Q_D(Axis);
	if (type != d->arrowType)
		exec(new AxisSetArrowTypeCmd(d, type, i18n("%1: set arrow type")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetArrowPosition, Axis::ArrowPosition, arrowPosition, retransformArrow);
void Axis::setArrowPosition(ArrowPosition position) {
	Q_D(Axis);
	if (position != d->arrowPosition)
		exec(new AxisSetArrowPositionCmd(d, position, i18n("%1: set arrow position")));
}


STD_SETTER_CMD_IMPL_F_S(Axis, SetArrowSize, float, arrowSize, retransformArrow);
void Axis::setArrowSize(float arrowSize) {
	Q_D(Axis);
	if (arrowSize != d->arrowSize)
		exec(new AxisSetArrowSizeCmd(d, arrowSize, i18n("%1: set arrow size")));
}

//Major ticks
STD_SETTER_CMD_IMPL_F_S(Axis, SetMajorTicksDirection, Axis::TicksDirection, majorTicksDirection, retransformTicks);
void Axis::setMajorTicksDirection(const TicksDirection majorTicksDirection) {
	Q_D(Axis);
	if (majorTicksDirection != d->majorTicksDirection)
		exec(new AxisSetMajorTicksDirectionCmd(d, majorTicksDirection, i18n("%1: set major ticks direction")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMajorTicksType, Axis::TicksType, majorTicksType, retransformTicks);
void Axis::setMajorTicksType(const TicksType majorTicksType) {
	Q_D(Axis);
	if (majorTicksType!= d->majorTicksType)
		exec(new AxisSetMajorTicksTypeCmd(d, majorTicksType, i18n("%1: set major ticks type")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMajorTicksNumber, int, majorTicksNumber, retransformTicks);
void Axis::setMajorTicksNumber(int majorTicksNumber) {
	Q_D(Axis);
	if (majorTicksNumber != d->majorTicksNumber)
		exec(new AxisSetMajorTicksNumberCmd(d, majorTicksNumber, i18n("%1: set the total number of the major ticks")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMajorTicksIncrement, qreal, majorTicksIncrement, retransformTicks);
void Axis::setMajorTicksIncrement(qreal majorTicksIncrement) {
	Q_D(Axis);
	if (majorTicksIncrement != d->majorTicksIncrement)
		exec(new AxisSetMajorTicksIncrementCmd(d, majorTicksIncrement, i18n("%1: set the increment for the major ticks")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMajorTicksColumn, const AbstractColumn*, majorTicksColumn, retransformTicks)
void Axis::setMajorTicksColumn(const AbstractColumn* column) {
	Q_D(Axis);
	if (column != d->majorTicksColumn) {
		exec(new AxisSetMajorTicksColumnCmd(d, column, i18n("%1: assign major ticks' values")));

		if (column) {
			connect(column, SIGNAL(dataChanged(const AbstractColumn*)), this, SLOT(retransformTicks()));
			connect(column->parentAspect(), SIGNAL(aspectAboutToBeRemoved(const AbstractAspect*)),
					this, SLOT(majorTicksColumnAboutToBeRemoved(const AbstractAspect*)));
			//TODO: add disconnect in the undo-function
		}
	}
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMajorTicksPen, QPen, majorTicksPen, recalcShapeAndBoundingRect);
void Axis::setMajorTicksPen(const QPen &pen) {
	Q_D(Axis);
	if (pen != d->majorTicksPen)
		exec(new AxisSetMajorTicksPenCmd(d, pen, i18n("%1: set major ticks style")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMajorTicksLength, qreal, majorTicksLength, retransformTicks);
void Axis::setMajorTicksLength(qreal majorTicksLength) {
	Q_D(Axis);
	if (majorTicksLength != d->majorTicksLength)
		exec(new AxisSetMajorTicksLengthCmd(d, majorTicksLength, i18n("%1: set major ticks length")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMajorTicksOpacity, qreal, majorTicksOpacity, update);
void Axis::setMajorTicksOpacity(qreal opacity) {
	Q_D(Axis);
	if (opacity != d->majorTicksOpacity)
		exec(new AxisSetMajorTicksOpacityCmd(d, opacity, i18n("%1: set major ticks opacity")));
}

//Minor ticks
STD_SETTER_CMD_IMPL_F_S(Axis, SetMinorTicksDirection, Axis::TicksDirection, minorTicksDirection, retransformTicks);
void Axis::setMinorTicksDirection(const TicksDirection minorTicksDirection) {
	Q_D(Axis);
	if (minorTicksDirection != d->minorTicksDirection)
		exec(new AxisSetMinorTicksDirectionCmd(d, minorTicksDirection, i18n("%1: set minor ticks direction")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMinorTicksType, Axis::TicksType, minorTicksType, retransformTicks);
void Axis::setMinorTicksType(const TicksType minorTicksType) {
	Q_D(Axis);
	if (minorTicksType!= d->minorTicksType)
		exec(new AxisSetMinorTicksTypeCmd(d, minorTicksType, i18n("%1: set minor ticks type")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMinorTicksNumber, int, minorTicksNumber, retransformTicks);
void Axis::setMinorTicksNumber(int minorTicksNumber) {
	Q_D(Axis);
	if (minorTicksNumber != d->minorTicksNumber)
		exec(new AxisSetMinorTicksNumberCmd(d, minorTicksNumber, i18n("%1: set the total number of the minor ticks")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMinorTicksIncrement, qreal, minorTicksIncrement, retransformTicks);
void Axis::setMinorTicksIncrement(qreal minorTicksIncrement) {
	Q_D(Axis);
	if (minorTicksIncrement != d->minorTicksIncrement)
		exec(new AxisSetMinorTicksIncrementCmd(d, minorTicksIncrement, i18n("%1: set the increment for the minor ticks")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMinorTicksColumn, const AbstractColumn*, minorTicksColumn, retransformTicks)
void Axis::setMinorTicksColumn(const AbstractColumn* column) {
	Q_D(Axis);
	if (column != d->minorTicksColumn) {
		exec(new AxisSetMinorTicksColumnCmd(d, column, i18n("%1: assign minor ticks' values")));

		if (column) {
			connect(column, SIGNAL(dataChanged(const AbstractColumn*)), this, SLOT(retransformTicks()));
			connect(column->parentAspect(), SIGNAL(aspectAboutToBeRemoved(const AbstractAspect*)),
					this, SLOT(minorTicksColumnAboutToBeRemoved(const AbstractAspect*)));
			//TODO: add disconnect in the undo-function
		}
	}
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMinorTicksPen, QPen, minorTicksPen, recalcShapeAndBoundingRect);
void Axis::setMinorTicksPen(const QPen &pen) {
	Q_D(Axis);
	if (pen != d->minorTicksPen)
		exec(new AxisSetMinorTicksPenCmd(d, pen, i18n("%1: set minor ticks style")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMinorTicksLength, qreal, minorTicksLength, retransformTicks);
void Axis::setMinorTicksLength(qreal minorTicksLength) {
	Q_D(Axis);
	if (minorTicksLength != d->minorTicksLength)
		exec(new AxisSetMinorTicksLengthCmd(d, minorTicksLength, i18n("%1: set minor ticks length")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMinorTicksOpacity, qreal, minorTicksOpacity, update);
void Axis::setMinorTicksOpacity(qreal opacity) {
	Q_D(Axis);
	if (opacity != d->minorTicksOpacity)
		exec(new AxisSetMinorTicksOpacityCmd(d, opacity, i18n("%1: set minor ticks opacity")));
}

//Labels
STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsFormat, Axis::LabelsFormat, labelsFormat, retransformTicks);
void Axis::setLabelsFormat(const LabelsFormat labelsFormat) {
	Q_D(Axis);
	if (labelsFormat != d->labelsFormat)
		exec(new AxisSetLabelsFormatCmd(d, labelsFormat, i18n("%1: set labels format")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsAutoPrecision, bool, labelsAutoPrecision, retransformTickLabelStrings);
void Axis::setLabelsAutoPrecision(const bool labelsAutoPrecision) {
	Q_D(Axis);
	if (labelsAutoPrecision != d->labelsAutoPrecision)
		exec(new AxisSetLabelsAutoPrecisionCmd(d, labelsAutoPrecision, i18n("%1: set labels precision")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsPrecision, int, labelsPrecision, retransformTickLabelStrings);
void Axis::setLabelsPrecision(const int labelsPrecision) {
	Q_D(Axis);
	if (labelsPrecision != d->labelsPrecision)
		exec(new AxisSetLabelsPrecisionCmd(d, labelsPrecision, i18n("%1: set labels precision")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsPosition, Axis::LabelsPosition, labelsPosition, retransformTickLabelPositions);
void Axis::setLabelsPosition(const LabelsPosition labelsPosition) {
	Q_D(Axis);
	if (labelsPosition != d->labelsPosition)
		exec(new AxisSetLabelsPositionCmd(d, labelsPosition, i18n("%1: set labels position")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsOffset, float, labelsOffset, retransformTickLabelPositions);
void Axis::setLabelsOffset(float offset) {
	Q_D(Axis);
	if (offset != d->labelsOffset)
		exec(new AxisSetLabelsOffsetCmd(d, offset, i18n("%1: set label offset")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsRotationAngle, qreal, labelsRotationAngle, recalcShapeAndBoundingRect);
void Axis::setLabelsRotationAngle(qreal angle) {
	Q_D(Axis);
	if (angle != d->labelsRotationAngle)
		exec(new AxisSetLabelsRotationAngleCmd(d, angle, i18n("%1: set label rotation angle")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsColor, QColor, labelsColor, update);
void Axis::setLabelsColor(const QColor &color) {
	Q_D(Axis);
	if (color != d->labelsColor)
		exec(new AxisSetLabelsColorCmd(d, color, i18n("%1: set label color")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsFont, QFont, labelsFont, retransformTickLabelStrings);
void Axis::setLabelsFont(const QFont &font) {
	Q_D(Axis);
	if (font != d->labelsFont)
		exec(new AxisSetLabelsFontCmd(d, font, i18n("%1: set label font")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsPrefix, QString, labelsPrefix, retransformTickLabelStrings);
void Axis::setLabelsPrefix(const QString& prefix) {
	Q_D(Axis);
	if (prefix != d->labelsPrefix)
		exec(new AxisSetLabelsPrefixCmd(d, prefix, i18n("%1: set label prefix")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsSuffix, QString, labelsSuffix, retransformTickLabelStrings);
void Axis::setLabelsSuffix(const QString& suffix) {
	Q_D(Axis);
	if (suffix != d->labelsSuffix)
		exec(new AxisSetLabelsSuffixCmd(d, suffix, i18n("%1: set label suffix")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetLabelsOpacity, qreal, labelsOpacity, update);
void Axis::setLabelsOpacity(qreal opacity) {
	Q_D(Axis);
	if (opacity != d->labelsOpacity)
		exec(new AxisSetLabelsOpacityCmd(d, opacity, i18n("%1: set labels opacity")));
}

//Major grid
STD_SETTER_CMD_IMPL_F_S(Axis, SetMajorGridPen, QPen, majorGridPen, retransformMajorGrid);
void Axis::setMajorGridPen(const QPen &pen) {
	Q_D(Axis);
	if (pen != d->majorGridPen)
		exec(new AxisSetMajorGridPenCmd(d, pen, i18n("%1: set major grid style")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMajorGridOpacity, qreal, majorGridOpacity, update);
void Axis::setMajorGridOpacity(qreal opacity) {
	Q_D(Axis);
	if (opacity != d->majorGridOpacity)
		exec(new AxisSetMajorGridOpacityCmd(d, opacity, i18n("%1: set major grid opacity")));
}

//Minor grid
STD_SETTER_CMD_IMPL_F_S(Axis, SetMinorGridPen, QPen, minorGridPen, retransformMinorGrid);
void Axis::setMinorGridPen(const QPen &pen) {
	Q_D(Axis);
	if (pen != d->minorGridPen)
		exec(new AxisSetMinorGridPenCmd(d, pen, i18n("%1: set minor grid style")));
}

STD_SETTER_CMD_IMPL_F_S(Axis, SetMinorGridOpacity, qreal, minorGridOpacity, update);
void Axis::setMinorGridOpacity(qreal opacity) {
	Q_D(Axis);
	if (opacity != d->minorGridOpacity)
		exec(new AxisSetMinorGridOpacityCmd(d, opacity, i18n("%1: set minor grid opacity")));
}

//##############################################################################
//####################################  SLOTs   ################################
//##############################################################################
void Axis::labelChanged() {
	Q_D(Axis);
	d->recalcShapeAndBoundingRect();
}

void Axis::retransformTicks() {
	Q_D(Axis);
	d->retransformTicks();
}

void Axis::majorTicksColumnAboutToBeRemoved(const AbstractAspect* aspect) {
	Q_D(Axis);
	if (aspect==d->majorTicksColumn) {
		d->majorTicksColumn = 0;
		d->retransformTicks();
	}
}

void Axis::minorTicksColumnAboutToBeRemoved(const AbstractAspect* aspect) {
	Q_D(Axis);
	if (aspect==d->minorTicksColumn) {
		d->minorTicksColumn = 0;
		d->retransformTicks();
	}
}

//##############################################################################
//######  SLOTs for changes triggered via QActions in the context menu  ########
//##############################################################################
void Axis::orientationChanged(QAction* action) {
	if (action == orientationHorizontalAction)
		this->setOrientation(AxisHorizontal);
	else
		this->setOrientation(AxisVertical);
}

void Axis::lineStyleChanged(QAction* action) {
	Q_D(const Axis);
	QPen pen = d->linePen;
	pen.setStyle(GuiTools::penStyleFromAction(lineStyleActionGroup, action));
	this->setLinePen(pen);
}

void Axis::lineColorChanged(QAction* action) {
	Q_D(const Axis);
	QPen pen = d->linePen;
	pen.setColor(GuiTools::colorFromAction(lineColorActionGroup, action));
	this->setLinePen(pen);
}

void Axis::visibilityChanged() {
	Q_D(const Axis);
	this->setVisible(!d->isVisible());
}

//#####################################################################
//################### Private implementation ##########################
//#####################################################################
AxisPrivate::AxisPrivate(Axis *owner) : m_plot(0), m_cSystem(0), m_printing(false), m_hovered(false),
	majorTicksColumn(0), minorTicksColumn(0), gridItem(new AxisGrid(this)), q(owner) {

	setFlag(QGraphicsItem::ItemIsSelectable, true);
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	setAcceptHoverEvents(true);
}

QString AxisPrivate::name() const{
	return q->name();
}

bool AxisPrivate::swapVisible(bool on) {
	bool oldValue = isVisible();
	setVisible(on);
	emit q->visibilityChanged(on);
	return oldValue;
}

QRectF AxisPrivate::boundingRect() const{
	return boundingRectangle;
}

/*!
  Returns the shape of the XYCurve as a QPainterPath in local coordinates
*/
QPainterPath AxisPrivate::shape() const{
	return axisShape;
}

/*!
	recalculates the position of the axis on the worksheet
 */
void AxisPrivate::retransform() {
	m_plot = qobject_cast<CartesianPlot*>(q->parentAspect());
	if (!m_plot)
		return;

	m_cSystem = dynamic_cast<const CartesianCoordinateSystem*>(m_plot->coordinateSystem());
	if (!m_cSystem)
		return;

	retransformLine();
}

void AxisPrivate::retransformLine() {
	linePath = QPainterPath();
	lines.clear();

	QPointF startPoint;
	QPointF endPoint;

	if (orientation == Axis::AxisHorizontal) {
		if (position == Axis::AxisTop)
			offset = m_plot->yMax();
		else if (position == Axis::AxisBottom)
			offset = m_plot->yMin();
		else if (position == Axis::AxisCentered)
			offset = m_plot->yMin() + (m_plot->yMax()-m_plot->yMin())/2;

		startPoint.setX(start);
		startPoint.setY(offset);
		endPoint.setX(end);
		endPoint.setY(offset);
	} else { // vertical
		if (position == Axis::AxisLeft)
			offset = m_plot->xMin();
		else if (position == Axis::AxisRight)
			offset = m_plot->xMax();
		else if (position == Axis::AxisCentered)
			offset = m_plot->xMin() + (m_plot->xMax()-m_plot->xMin())/2;

		startPoint.setX(offset);
		startPoint.setY(start);
		endPoint.setY(end);
		endPoint.setX(offset);
	}

	lines.append(QLineF(startPoint, endPoint));
	lines = m_cSystem->mapLogicalToScene(lines, AbstractCoordinateSystem::MarkGaps);
	foreach (const QLineF& line, lines) {
		linePath.moveTo(line.p1());
		linePath.lineTo(line.p2());
	}

	if (linePath.isEmpty()) {
		recalcShapeAndBoundingRect();
		return;
	} else {
		retransformArrow();
		retransformTicks();
	}
}

void AxisPrivate::retransformArrow() {
	arrowPath = QPainterPath();
	if (arrowType == Axis::NoArrow || lines.isEmpty()) {
		recalcShapeAndBoundingRect();
		return;
	}

	if (arrowPosition==Axis::ArrowRight || arrowPosition==Axis::ArrowBoth) {
		const QPointF& endPoint = lines.at(lines.size()-1).p2();
		this->addArrow(endPoint, 1);
	}

	if (arrowPosition==Axis::ArrowLeft || arrowPosition==Axis::ArrowBoth) {
		const QPointF& endPoint = lines.at(0).p1();
		this->addArrow(endPoint, -1);
	}

	recalcShapeAndBoundingRect();
}

void AxisPrivate::addArrow(const QPointF& startPoint, int direction) {
	static const float cos_phi = cos(3.14159/6);

	if (orientation==Axis::AxisHorizontal) {
		QPointF endPoint = QPointF(startPoint.x()+direction*arrowSize, startPoint.y());
		arrowPath.moveTo(startPoint);
		arrowPath.lineTo(endPoint);

		switch (arrowType) {
			case Axis::NoArrow:
				break;
			case Axis::SimpleArrowSmall:
				arrowPath.moveTo(endPoint);
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/4, endPoint.y()-arrowSize/4*cos_phi));
				arrowPath.moveTo(endPoint);
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/4, endPoint.y()+arrowSize/4*cos_phi));
				break;
			case Axis::SimpleArrowBig:
				arrowPath.moveTo(endPoint);
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/2, endPoint.y()-arrowSize/2*cos_phi));
				arrowPath.moveTo(endPoint);
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/2, endPoint.y()+arrowSize/2*cos_phi));
				break;
			case Axis::FilledArrowSmall:
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/4, endPoint.y()-arrowSize/4*cos_phi));
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/4, endPoint.y()+arrowSize/4*cos_phi));
				arrowPath.lineTo(endPoint);
				break;
			case Axis::FilledArrowBig:
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/2, endPoint.y()-arrowSize/2*cos_phi));
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/2, endPoint.y()+arrowSize/2*cos_phi));
				arrowPath.lineTo(endPoint);
				break;
			case Axis::SemiFilledArrowSmall:
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/4, endPoint.y()-arrowSize/4*cos_phi));
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/8, endPoint.y()));
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/4, endPoint.y()+arrowSize/4*cos_phi));
				arrowPath.lineTo(endPoint);
				break;
			case Axis::SemiFilledArrowBig:
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/2, endPoint.y()-arrowSize/2*cos_phi));
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/4, endPoint.y()));
				arrowPath.lineTo(QPointF(endPoint.x()-direction*arrowSize/2, endPoint.y()+arrowSize/2*cos_phi));
				arrowPath.lineTo(endPoint);
				break;
		}
	} else { //vertical orientation
		QPointF endPoint = QPointF(startPoint.x(), startPoint.y()-direction*arrowSize);
		arrowPath.moveTo(startPoint);
		arrowPath.lineTo(endPoint);

		switch (arrowType) {
			case Axis::NoArrow:
				break;
			case Axis::SimpleArrowSmall:
				arrowPath.moveTo(endPoint);
				arrowPath.lineTo(QPointF(endPoint.x()-arrowSize/4*cos_phi, endPoint.y()+direction*arrowSize/4));
				arrowPath.moveTo(endPoint);
				arrowPath.lineTo(QPointF(endPoint.x()+arrowSize/4*cos_phi, endPoint.y()+direction*arrowSize/4));
				break;
			case Axis::SimpleArrowBig:
				arrowPath.moveTo(endPoint);
				arrowPath.lineTo(QPointF(endPoint.x()-arrowSize/2*cos_phi, endPoint.y()+direction*arrowSize/2));
				arrowPath.moveTo(endPoint);
				arrowPath.lineTo(QPointF(endPoint.x()+arrowSize/2*cos_phi, endPoint.y()+direction*arrowSize/2));
				break;
			case Axis::FilledArrowSmall:
				arrowPath.lineTo(QPointF(endPoint.x()-arrowSize/4*cos_phi, endPoint.y()+direction*arrowSize/4));
				arrowPath.lineTo(QPointF(endPoint.x()+arrowSize/4*cos_phi, endPoint.y()+direction*arrowSize/4));
				arrowPath.lineTo(endPoint);
				break;
			case Axis::FilledArrowBig:
				arrowPath.lineTo(QPointF(endPoint.x()-arrowSize/2*cos_phi, endPoint.y()+direction*arrowSize/2));
				arrowPath.lineTo(QPointF(endPoint.x()+arrowSize/2*cos_phi, endPoint.y()+direction*arrowSize/2));
				arrowPath.lineTo(endPoint);
				break;
			case Axis::SemiFilledArrowSmall:
				arrowPath.lineTo(QPointF(endPoint.x()-arrowSize/4*cos_phi, endPoint.y()+direction*arrowSize/4));
				arrowPath.lineTo(QPointF(endPoint.x(), endPoint.y()+direction*arrowSize/8));
				arrowPath.lineTo(QPointF(endPoint.x()+arrowSize/4*cos_phi, endPoint.y()+direction*arrowSize/4));
				arrowPath.lineTo(endPoint);
				break;
			case Axis::SemiFilledArrowBig:
				arrowPath.lineTo(QPointF(endPoint.x()-arrowSize/2*cos_phi, endPoint.y()+direction*arrowSize/2));
				arrowPath.lineTo(QPointF(endPoint.x(), endPoint.y()+direction*arrowSize/4));
				arrowPath.lineTo(QPointF(endPoint.x()+arrowSize/2*cos_phi, endPoint.y()+direction*arrowSize/2));
				arrowPath.lineTo(endPoint);
				break;
		}
	}
}

//! helper function for retransformTicks()
bool AxisPrivate::transformAnchor(QPointF* anchorPoint) {
	QList<QPointF> points;
	points.append(*anchorPoint);
	points = m_cSystem->mapLogicalToScene(points);

	if (points.count() != 1) { // point is not mappable or in a coordinate gap
		return false;
	} else {
		*anchorPoint = points.at(0);
		return true;
	}
}

/*!
	recalculates the position of the axis ticks.
 */
void AxisPrivate::retransformTicks() {
	//TODO: check that start and end are > 0 for log and >=0 for sqrt, etc.

	majorTicksPath = QPainterPath();
	minorTicksPath = QPainterPath();
	majorTickPoints.clear();
	minorTickPoints.clear();
	tickLabelValues.clear();

	if ( majorTicksNumber<1 || (majorTicksDirection == Axis::noTicks && minorTicksDirection == Axis::noTicks) ) {
		retransformTickLabelPositions(); //this calls recalcShapeAndBoundingRect()
		return;
	}

	//determine the spacing for the major ticks
	double majorTicksSpacing=0;
	int tmpMajorTicksNumber=0;
	if (majorTicksType == Axis::TicksTotalNumber) {
		//the total number of the major ticks is given - > determine the spacing
		tmpMajorTicksNumber = majorTicksNumber;
		switch (scale) {
			case Axis::ScaleLinear:
				majorTicksSpacing = (end-start)/(majorTicksNumber-1);
				break;
			case Axis::ScaleLog10:
				majorTicksSpacing = (log10(end)-log10(start))/(majorTicksNumber-1);
				break;
			case Axis::ScaleLog2:
				majorTicksSpacing = (log(end)-log(start))/log(2)/(majorTicksNumber-1);
				break;
			case Axis::ScaleLn:
				majorTicksSpacing = (log(end)-log(start))/(majorTicksNumber-1);
				break;
			case Axis::ScaleSqrt:
				majorTicksSpacing = (sqrt(end)-sqrt(start))/(majorTicksNumber-1);
				break;
			case Axis::ScaleX2:
				majorTicksSpacing = (pow(end,2)-pow(start,2))/(majorTicksNumber-1);
		}
	} else if (majorTicksType == Axis::TicksIncrement) {
		//the spacing (increment) of the major ticks is given - > determine the number
		majorTicksSpacing = majorTicksIncrement;
		switch (scale) {
			case Axis::ScaleLinear:
				tmpMajorTicksNumber = qRound((end-start)/majorTicksSpacing + 1);
				break;
			case Axis::ScaleLog10:
				tmpMajorTicksNumber = qRound((log10(end)-log10(start))/majorTicksSpacing + 1);
				break;
			case Axis::ScaleLog2:
				tmpMajorTicksNumber = qRound((log(end)-log(start))/log(2)/majorTicksSpacing + 1);
				break;
			case Axis::ScaleLn:
				tmpMajorTicksNumber = qRound((log(end)-log(start))/majorTicksSpacing + 1);
				break;
			case Axis::ScaleSqrt:
				tmpMajorTicksNumber = qRound((sqrt(end)-sqrt(start))/majorTicksSpacing + 1);
				break;
			case Axis::ScaleX2:
				tmpMajorTicksNumber = qRound((pow(end,2)-pow(start,2))/majorTicksSpacing + 1);
		}
	} else {
		//custom column was provided
		if (majorTicksColumn) {
			tmpMajorTicksNumber = majorTicksColumn->rowCount();
		} else {
			retransformTickLabelPositions(); //this calls recalcShapeAndBoundingRect()
			return;
		}
	}

	int tmpMinorTicksNumber;
	if (minorTicksType == Axis::TicksTotalNumber)
		tmpMinorTicksNumber = minorTicksNumber;
	else if (minorTicksType == Axis::TicksIncrement)
		tmpMinorTicksNumber = (end - start)/ (majorTicksNumber - 1)/minorTicksIncrement - 1;
	else
		(minorTicksColumn) ? tmpMinorTicksNumber = minorTicksColumn->rowCount() : tmpMinorTicksNumber = 0;

	QPointF anchorPoint;
	QPointF startPoint;
	QPointF endPoint;
	qreal majorTickPos=0.0;
	qreal minorTickPos;
	qreal nextMajorTickPos = 0.0;
	int xDirection = m_cSystem->xDirection();
	int yDirection = m_cSystem->yDirection();
	float middleX = m_plot->xMin() + (m_plot->xMax() - m_plot->xMin())/2;
	float middleY = m_plot->yMin() + (m_plot->yMax() - m_plot->yMin())/2;
	bool valid;

	for (int iMajor = 0; iMajor < tmpMajorTicksNumber; iMajor++) {
		//calculate major tick's position
		if (majorTicksType != Axis::TicksCustomColumn) {
			switch (scale) {
				case Axis::ScaleLinear:
					majorTickPos = start + majorTicksSpacing*iMajor;
					nextMajorTickPos = start + majorTicksSpacing*(iMajor+1);
					break;
				case Axis::ScaleLog10:
					majorTickPos = pow(10, log10(start) + majorTicksSpacing*iMajor);
					nextMajorTickPos = pow(10, log10(start) + majorTicksSpacing*(iMajor+1));
					break;
				case Axis::ScaleLog2:
					majorTickPos = pow(2, log(start)/log(2) + majorTicksSpacing*iMajor);
					nextMajorTickPos = pow(2, log(start)/log(2) + majorTicksSpacing*(iMajor+1));
					break;
				case Axis::ScaleLn:
					majorTickPos = exp(log(start) + majorTicksSpacing*iMajor);
					nextMajorTickPos = exp(log(start) + majorTicksSpacing*(iMajor+1));
					break;
				case Axis::ScaleSqrt:
					majorTickPos = pow(sqrt(start) + majorTicksSpacing*iMajor, 2);
					nextMajorTickPos = pow(sqrt(start) + majorTicksSpacing*(iMajor+1), 2);
					break;
				case Axis::ScaleX2:
					majorTickPos = sqrt(sqrt(start) + majorTicksSpacing*iMajor);
					nextMajorTickPos = sqrt(sqrt(start) + majorTicksSpacing*(iMajor+1));
					break;
			}
		} else {
			majorTickPos = majorTicksColumn->valueAt(iMajor);
			if (std::isnan(majorTickPos))
				break; //stop iterating after the first non numerical value in the column
		}

		//calculate start and end points for major tick's line
		if (majorTicksDirection != Axis::noTicks ) {
			if (orientation == Axis::AxisHorizontal) {
				anchorPoint.setX(majorTickPos);
				anchorPoint.setY(offset);
				valid = transformAnchor(&anchorPoint);
				if (valid) {
					if (offset < middleY) {
						startPoint = anchorPoint + QPointF(0, (majorTicksDirection & Axis::ticksIn)  ? yDirection * majorTicksLength  : 0);
						endPoint   = anchorPoint + QPointF(0, (majorTicksDirection & Axis::ticksOut) ? -yDirection * majorTicksLength : 0);
					} else {
						startPoint = anchorPoint + QPointF(0, (majorTicksDirection & Axis::ticksOut)  ? yDirection * majorTicksLength  : 0);
						endPoint   = anchorPoint + QPointF(0, (majorTicksDirection & Axis::ticksIn) ? -yDirection * majorTicksLength : 0);
					}
				}
			} else { // vertical
				anchorPoint.setY(majorTickPos);
				anchorPoint.setX(offset);
				valid = transformAnchor(&anchorPoint);

				if (valid) {
					if (offset < middleX) {
						startPoint = anchorPoint + QPointF((majorTicksDirection & Axis::ticksIn)  ? xDirection * majorTicksLength  : 0, 0);
						endPoint = anchorPoint + QPointF((majorTicksDirection & Axis::ticksOut) ? -xDirection * majorTicksLength : 0, 0);
					} else {
						startPoint = anchorPoint + QPointF((majorTicksDirection & Axis::ticksOut) ? xDirection * majorTicksLength : 0, 0);
						endPoint = anchorPoint + QPointF((majorTicksDirection & Axis::ticksIn)  ? -xDirection *  majorTicksLength  : 0, 0);
					}
				}
			}

			//add major tick's line to the painter path
			if (valid) {
				majorTicksPath.moveTo(startPoint);
				majorTicksPath.lineTo(endPoint);
				majorTickPoints << anchorPoint;
				tickLabelValues<< scalingFactor*majorTickPos+zeroOffset;
			}
		}

		//minor ticks
		if ((Axis::noTicks != minorTicksDirection) && (tmpMajorTicksNumber > 1) && (tmpMinorTicksNumber > 0) && (iMajor<tmpMajorTicksNumber-1)) {
			//minor ticks are placed at equidistant positions independent of the selected scaling for the major ticks positions
			double minorTicksSpacing = (nextMajorTickPos-majorTickPos)/(tmpMinorTicksNumber+1);

			for (int iMinor = 0; iMinor < tmpMinorTicksNumber; iMinor++) {
				//calculate minor tick's position
				if (minorTicksType != Axis::TicksCustomColumn) {
					minorTickPos = majorTickPos + (iMinor+1)*minorTicksSpacing;
				} else {
					minorTickPos = minorTicksColumn->valueAt(iMinor);
					if (std::isnan(minorTickPos))
						break; //stop iterating after the first non numerical value in the column

					//in the case a custom column is used for the minor ticks, we draw them _once_ for the whole range of the axis.
					//execute the minor ticks loop only once.
					if (iMajor>0)
						break;
				}

				//calculate start and end points for minor tick's line
				if (orientation == Axis::AxisHorizontal) {
					anchorPoint.setX(minorTickPos);
					anchorPoint.setY(offset);
					valid = transformAnchor(&anchorPoint);

					if (valid) {
						if (offset < middleY) {
							startPoint = anchorPoint + QPointF(0, (minorTicksDirection & Axis::ticksIn)  ? yDirection * minorTicksLength  : 0);
							endPoint   = anchorPoint + QPointF(0, (minorTicksDirection & Axis::ticksOut) ? -yDirection * minorTicksLength : 0);
						} else {
							startPoint = anchorPoint + QPointF(0, (minorTicksDirection & Axis::ticksOut)  ? yDirection * minorTicksLength  : 0);
							endPoint   = anchorPoint + QPointF(0, (minorTicksDirection & Axis::ticksIn) ? -yDirection * minorTicksLength : 0);
						}
					}
				} else { // vertical
					anchorPoint.setY(minorTickPos);
					anchorPoint.setX(offset);
					valid = transformAnchor(&anchorPoint);

					if (valid) {
						if (offset < middleX) {
							startPoint = anchorPoint + QPointF((minorTicksDirection & Axis::ticksIn)  ? xDirection * minorTicksLength  : 0, 0);
							endPoint   = anchorPoint + QPointF((minorTicksDirection & Axis::ticksOut) ? -xDirection * minorTicksLength : 0, 0);
						} else {
							startPoint = anchorPoint + QPointF((minorTicksDirection & Axis::ticksOut)  ? xDirection * minorTicksLength  : 0, 0);
							endPoint   = anchorPoint + QPointF((minorTicksDirection & Axis::ticksIn) ? -xDirection * minorTicksLength : 0, 0);
						}
					}
				}

				//add minor tick's line to the painter path
				if (valid) {
					minorTicksPath.moveTo(startPoint);
					minorTicksPath.lineTo(endPoint);
					minorTickPoints << anchorPoint;
				}
			}
		}
	}

	//tick positions where changed -> update the position of the tick labels and grid lines
	retransformTickLabelStrings();
	retransformMajorGrid();
	retransformMinorGrid();
}

/*!
	creates the tick label strings starting with the most optimal
	(=the smallest possible number of float digits) precision for the floats
*/
void AxisPrivate::retransformTickLabelStrings() {
	if (labelsAutoPrecision) {
		//check, whether we need to increase the current precision
		int newPrecision = upperLabelsPrecision(labelsPrecision);
		if (newPrecision!= labelsPrecision) {
			labelsPrecision = newPrecision;
			emit q->labelsPrecisionChanged(labelsPrecision);
		} else {
			//check, whether we can reduce the current precision
			newPrecision = lowerLabelsPrecision(labelsPrecision);
			if (newPrecision!= labelsPrecision) {
				labelsPrecision = newPrecision;
				emit q->labelsPrecisionChanged(labelsPrecision);
			}
		}
	}

	tickLabelStrings.clear();
	QString str;
	if (labelsFormat == Axis::FormatDecimal) {
		QString nullStr = QString::number(0, 'f', labelsPrecision);
		foreach(float value, tickLabelValues) {
			str = QString::number(value, 'f', labelsPrecision);
			if (str == "-"+nullStr) str=nullStr;
			str = labelsPrefix + str + labelsSuffix;
			tickLabelStrings << str;
		}
	} else if (labelsFormat == Axis::FormatScientificE) {
		QString nullStr = QString::number(0, 'e', labelsPrecision);
		foreach(float value, tickLabelValues) {
			str = QString::number(value, 'e', labelsPrecision);
			if (str == "-"+nullStr) str=nullStr;
			tickLabelStrings << str;
		}
	} else if (labelsFormat == Axis::FormatPowers10) {
		foreach(float value, tickLabelValues) {
			str = "10<span style=\"vertical-align:super\">"+ QString::number(log10(value),'f',labelsPrecision)+"</span>";
			str = labelsPrefix + str + labelsSuffix;
			tickLabelStrings << str;
		}
	} else if (labelsFormat == Axis::FormatPowers2) {
		foreach(float value, tickLabelValues) {
			str = "2<span style=\"vertical-align:super\">"+ QString::number(log2(value),'f',labelsPrecision)+"</span>";
			str = labelsPrefix + str + labelsSuffix;
			tickLabelStrings << str;
		}
	} else if (labelsFormat == Axis::FormatPowersE) {
		foreach(float value, tickLabelValues) {
			str = "e<span style=\"vertical-align:super\">"+ QString::number(log(value),'f',labelsPrecision)+"</span>";
			str = labelsPrefix + str + labelsSuffix;
			tickLabelStrings << str;
		}
	} else if (labelsFormat == Axis::FormatMultipliesPi) {
		foreach(float value, tickLabelValues) {
			str = "<span>"+ QString::number(value / M_PI,'f',labelsPrecision)+"</span>" + QChar(0x03C0);
			str = labelsPrefix + str + labelsSuffix;
			tickLabelStrings << str;
		}
	}

	//recalculate the position of the tick labels
	retransformTickLabelPositions();
}

/*!
	returns the smalles upper limit for the precision
	where no duplicates for the tick label float occur.
 */
int AxisPrivate::upperLabelsPrecision(int precision) {
	//round float to the current precision and look for duplicates.
	//if there are duplicates, increase the precision.
	QList<float> tempValues;
	for (int i=0; i<tickLabelValues.size(); ++i) {
		tempValues.append( round(tickLabelValues[i], precision) );
	}

	for (int i=0; i<tempValues.size(); ++i) {
		for (int j=0; j<tempValues.size(); ++j) {
			if (i==j) continue;
			if ( AbstractCoordinateSystem::essentiallyEqual(tempValues.at(i), tempValues.at(j), pow(10,-precision)) ) {
				//duplicate for the current precision found, increase the precision and check again
				return upperLabelsPrecision(precision+1);
			}
		}
	}

	//no duplicates for the current precision found, return the current value
	return precision;
}

/*!
	returns highest lower limit for the precision
	where no duplicates for the tick label float occur.
*/
int AxisPrivate::lowerLabelsPrecision(int precision) {
	//round float to the current precision and look for duplicates.
	//if there are duplicates, decrease the precision.
	QList<float> tempValues;
	for (int i=0; i<tickLabelValues.size(); ++i) {
		tempValues.append( round(tickLabelValues[i], precision-1) );
	}

	for (int i=0; i<tempValues.size(); ++i) {
		for (int j=0; j<tempValues.size(); ++j) {
			if (i==j) continue;
			if ( AbstractCoordinateSystem::essentiallyEqual(tempValues.at(i), tempValues.at(j), pow(10,-precision)) ) {
				//duplicate found for the reduced precision
				//-> current precision cannot be reduced, return the current value + 1
				return precision+1;
			}
		}
	}

	//no duplicates found, reduce further, and check again
	if (precision == 0)
		return 0;
	else
		return lowerLabelsPrecision(precision-1);
}

double AxisPrivate::round(double value, int precision) {
	return double(value*pow(10, precision))/pow(10, precision);
}

/*!
	recalculates the position of the tick labels.
	Called when the geometry related properties (position, offset, font size, suffix, prefix) of the labels are changed.
 */
void AxisPrivate::retransformTickLabelPositions() {
	tickLabelPoints.clear();
	if (majorTicksDirection == Axis::noTicks || labelsPosition == Axis::NoLabels) {
		recalcShapeAndBoundingRect();
		return;
	}

	QFontMetrics fm(labelsFont);
	float width = 0;
	float height = fm.ascent();
	QString label;
	QPointF pos;
	float middleX = m_plot->xMin() + (m_plot->xMax() - m_plot->xMin())/2;
	float middleY = m_plot->yMin() + (m_plot->yMax() - m_plot->yMin())/2;
	int xDirection = m_cSystem->xDirection();
	int yDirection = m_cSystem->yDirection();

	QPointF startPoint, endPoint, anchorPoint;

	QTextDocument td;
	td.setDefaultFont(labelsFont);

	for ( int i=0; i<majorTickPoints.size(); i++ ) {
		if (labelsFormat == Axis::FormatDecimal || labelsFormat == Axis::FormatScientificE) {
			width = fm.width(tickLabelStrings.at(i));
		} else {
			td.setHtml(tickLabelStrings.at(i));
			width = td.size().width();
			height = td.size().height();
		}
		anchorPoint = majorTickPoints.at(i);

		//center align all labels with respect to the end point of the tick line
		if (orientation == Axis::AxisHorizontal) {
			if (offset < middleY) {
				startPoint = anchorPoint + QPointF(0, (majorTicksDirection & Axis::ticksIn)  ? yDirection * majorTicksLength  : 0);
				endPoint   = anchorPoint + QPointF(0, (majorTicksDirection & Axis::ticksOut) ? -yDirection * majorTicksLength : 0);
			}
			else {
				startPoint = anchorPoint + QPointF(0, (majorTicksDirection & Axis::ticksOut)  ? yDirection * majorTicksLength  : 0);
				endPoint   = anchorPoint + QPointF(0, (majorTicksDirection & Axis::ticksIn) ? -yDirection * majorTicksLength : 0);
			}
			if (labelsPosition == Axis::LabelsOut) {
				pos.setX( endPoint.x() - width/2);
				pos.setY( endPoint.y() + height + labelsOffset );
			} else {
				pos.setX( startPoint.x() - width/2);
				pos.setY( startPoint.y() - labelsOffset );
			}
		} else {// vertical
			if (offset < middleX) {
				startPoint = anchorPoint + QPointF((majorTicksDirection & Axis::ticksIn)  ? xDirection * majorTicksLength  : 0, 0);
				endPoint = anchorPoint + QPointF((majorTicksDirection & Axis::ticksOut) ? -xDirection * majorTicksLength : 0, 0);
			} else {
				startPoint = anchorPoint + QPointF((majorTicksDirection & Axis::ticksOut) ? xDirection * majorTicksLength : 0, 0);
				endPoint = anchorPoint + QPointF((majorTicksDirection & Axis::ticksIn)  ? -xDirection *  majorTicksLength  : 0, 0);
			}
			if (labelsPosition == Axis::LabelsOut) {
				pos.setX( endPoint.x() - width - labelsOffset );
				pos.setY( endPoint.y() + height/2 );
			} else {
				pos.setX( startPoint.x() + labelsOffset );
				pos.setY( startPoint.y() + height/2 );
			}
		}
		tickLabelPoints<<pos;
	}

	recalcShapeAndBoundingRect();
}

void AxisPrivate::retransformMajorGrid() {
	majorGridPath = QPainterPath();
	if (majorGridPen.style() == Qt::NoPen || majorTickPoints.size() == 0) {
		recalcShapeAndBoundingRect();
		return;
	}

	//major tick points are already in scene coordinates, convert them back to logical...
	//TODO: mapping should work without SuppressPageClipping-flag, check float comparisons in the map-function.
	//Currently, grid lines disappear somtimes without this flag
	QList<QPointF> logicalMajorTickPoints = m_cSystem->mapSceneToLogical(majorTickPoints, AbstractCoordinateSystem::SuppressPageClipping);

	if (!logicalMajorTickPoints.size())
		return;

	//TODO:
	//when iterating over all grid lines, skip the first and the last points for auto scaled axes,
	//since we don't want to paint any grid lines at the plot boundaries
	bool skipLowestTick, skipUpperTick;
	if (orientation == Axis::AxisHorizontal) { //horizontal axis
		skipLowestTick = qFuzzyCompare((float)logicalMajorTickPoints.at(0).x(), m_plot->xMin());
		skipUpperTick = qFuzzyCompare((float)logicalMajorTickPoints.at(logicalMajorTickPoints.size()-1).x(), m_plot->xMax());
	} else {
		skipLowestTick = qFuzzyCompare((float)logicalMajorTickPoints.at(0).y(), m_plot->yMin());
		skipUpperTick = qFuzzyCompare((float)logicalMajorTickPoints.at(logicalMajorTickPoints.size()-1).y(), m_plot->yMax());
	}

	int start, end;
	if (skipLowestTick) {
		if (logicalMajorTickPoints.size()>1)
			start = 1;
		else
			start = 0;
	} else {
		start = 0;
	}

	if ( skipUpperTick ) {
		if (logicalMajorTickPoints.size()>1)
			end = logicalMajorTickPoints.size()-1;
		else
			end = 0;

	} else {
		end = logicalMajorTickPoints.size();
	}

	QList<QLineF> lines;
	if (orientation == Axis::AxisHorizontal) { //horizontal axis
		float yMin = m_plot->yMin();
		float yMax = m_plot->yMax();

		for (int i=start; i<end; ++i) {
			const QPointF& point = logicalMajorTickPoints.at(i);
			lines.append( QLineF(point.x(), yMin, point.x(), yMax) );
		}
	} else { //vertical axis
		float xMin = m_plot->xMin();
		float xMax = m_plot->xMax();

		//skip the first and the last points, since we don't want to paint any grid lines at the plot boundaries
		for (int i=start; i<end; ++i) {
			const QPointF& point = logicalMajorTickPoints.at(i);
			lines.append( QLineF(xMin, point.y(), xMax, point.y()) );
		}
	}

	lines = m_cSystem->mapLogicalToScene(lines, AbstractCoordinateSystem::SuppressPageClipping | AbstractCoordinateSystem::MarkGaps);
	foreach (const QLineF& line, lines) {
		majorGridPath.moveTo(line.p1());
		majorGridPath.lineTo(line.p2());
	}

	recalcShapeAndBoundingRect();
}

void AxisPrivate::retransformMinorGrid() {
	minorGridPath = QPainterPath();
	if (minorGridPen.style() == Qt::NoPen) {
		recalcShapeAndBoundingRect();
		return;
	}

	//minor tick points are already in scene coordinates, convert them back to logical...
	//TODO: mapping should work without SuppressPageClipping-flag, check float comparisons in the map-function.
	//Currently, grid lines disappear somtimes without this flag
	QList<QPointF> logicalMinorTickPoints = m_cSystem->mapSceneToLogical(minorTickPoints, AbstractCoordinateSystem::SuppressPageClipping | AbstractCoordinateSystem::MarkGaps);

	QList<QLineF> lines;
	if (orientation == Axis::AxisHorizontal) { //horizontal axis
		float yMin = m_plot->yMin();
		float yMax = m_plot->yMax();

		for (int i=0; i<logicalMinorTickPoints.size(); ++i) {
			const QPointF& point = logicalMinorTickPoints.at(i);
			lines.append( QLineF(point.x(), yMin, point.x(), yMax) );
		}
	} else { //vertical axis
		float xMin = m_plot->xMin();
		float xMax = m_plot->xMax();

		for (int i=0; i<logicalMinorTickPoints.size(); ++i) {
			const QPointF& point = logicalMinorTickPoints.at(i);
			lines.append( QLineF(xMin, point.y(), xMax, point.y()) );
		}
	}

	lines = m_cSystem->mapLogicalToScene(lines, AbstractCoordinateSystem::MarkGaps);
	foreach (const QLineF& line, lines) {
		minorGridPath.moveTo(line.p1());
		minorGridPath.lineTo(line.p2());
	}

	recalcShapeAndBoundingRect();
}

void AxisPrivate::recalcShapeAndBoundingRect() {
	prepareGeometryChange();

	if (linePath.isEmpty()) {
		axisShape = QPainterPath();
		boundingRectangle = QRectF();
		title->setPositionInvalid(true);
		if (m_plot) m_plot->prepareGeometryChange();
		return;
	} else {
		title->setPositionInvalid(false);
	}

	axisShape = WorksheetElement::shapeFromPath(linePath, linePen);
	axisShape.addPath(WorksheetElement::shapeFromPath(arrowPath, linePen));
	axisShape.addPath(WorksheetElement::shapeFromPath(majorTicksPath, majorTicksPen));
	axisShape.addPath(WorksheetElement::shapeFromPath(minorTicksPath, minorTicksPen));

	QPainterPath  tickLabelsPath = QPainterPath();
	if (labelsPosition != Axis::NoLabels) {
		QTransform trafo;
		QPainterPath tempPath;
		QFontMetrics fm(labelsFont);
		QTextDocument td;
		td.setDefaultFont(labelsFont);
	  	for (int i=0; i<tickLabelPoints.size(); i++) {
			tempPath = QPainterPath();
			if (labelsFormat == Axis::FormatDecimal || labelsFormat == Axis::FormatScientificE) {
				tempPath.addRect( fm.boundingRect(tickLabelStrings.at(i)) );
			} else {
				td.setHtml(tickLabelStrings.at(i));
				tempPath.addRect( QRectF(0, -td.size().height(), td.size().width(), td.size().height()) );
			}

			trafo.reset();
			trafo.translate( tickLabelPoints.at(i).x(), tickLabelPoints.at(i).y() );
			trafo.rotate( -labelsRotationAngle );
			tempPath = trafo.map(tempPath);

			tickLabelsPath.addPath(WorksheetElement::shapeFromPath(tempPath, linePen));
		}
		axisShape.addPath(WorksheetElement::shapeFromPath(tickLabelsPath, QPen()));
	}

	//add title label, if available
	if ( title->isVisible() && !title->text().text.isEmpty() ) {
		//determine the new position of the title label:
		//we calculate the new position here and not in retransform(),
		//since it depends on the size and position of the tick labels, tickLabelsPath, available here.
		QRectF rect=linePath.boundingRect();
		float offsetX = titleOffsetX - labelsOffset; //the distance to the axis line
		float offsetY = titleOffsetY - labelsOffset; //the distance to the axis line
		if (orientation == Axis::AxisHorizontal) {
			offsetY -= title->graphicsItem()->boundingRect().height()/2 + tickLabelsPath.boundingRect().height();
			title->setPosition( QPointF( (rect.topLeft().x() + rect.topRight().x())/2 + offsetX, rect.bottomLeft().y() - offsetY ) );
		} else {
			offsetX -= title->graphicsItem()->boundingRect().width()/2 + tickLabelsPath.boundingRect().width();
			title->setPosition( QPointF( rect.topLeft().x() + offsetX, (rect.topLeft().y() + rect.bottomLeft().y())/2 - offsetY) );
		}
		axisShape.addPath(WorksheetElement::shapeFromPath(title->graphicsItem()->mapToParent(title->graphicsItem()->shape()), linePen));
	}

	boundingRectangle = axisShape.boundingRect();

	//if the axis goes beyond the current bounding box of the plot (too high offset is used, too long labels etc.)
	//request a prepareGeometryChange() for the plot in order to properly keep track of geometry changes
	if (m_plot)
		m_plot->prepareGeometryChange();
}

/*!
	paints the content of the axis. Reimplemented from \c QGraphicsItem.
	\sa QGraphicsItem::paint()
 */
void AxisPrivate::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget) {
	Q_UNUSED(option)
	Q_UNUSED(widget)

	if (!isVisible())
		return;

	if (linePath.isEmpty())
		return;

	//draw the line
	if (linePen.style() != Qt::NoPen) {
		painter->setOpacity(lineOpacity);
		painter->setPen(linePen);
		painter->setBrush(Qt::SolidPattern);
		painter->drawPath(linePath);

		//draw the arrow
		if (arrowType != Axis::NoArrow)
			painter->drawPath(arrowPath);
	}

	//draw the major ticks
	if (majorTicksDirection != Axis::noTicks) {
		painter->setOpacity(majorTicksOpacity);
		painter->setPen(majorTicksPen);
		painter->setBrush(Qt::NoBrush);
		painter->drawPath(majorTicksPath);
	}

	//draw the minor ticks
	if (minorTicksDirection != Axis::noTicks) {
		painter->setOpacity(minorTicksOpacity);
		painter->setPen(minorTicksPen);
		painter->setBrush(Qt::NoBrush);
		painter->drawPath(minorTicksPath);
	}

	// draw tick labels
	if (labelsPosition != Axis::NoLabels) {
		painter->setOpacity(labelsOpacity);
		painter->setPen(QPen(labelsColor));
		painter->setFont(labelsFont);
		QTextDocument td;
		td.setDefaultFont(labelsFont);
		for (int i=0; i<tickLabelPoints.size(); i++) {
			painter->translate(tickLabelPoints.at(i));
			painter->save();
			painter->rotate(-labelsRotationAngle);

			if (labelsFormat == Axis::FormatDecimal || labelsFormat == Axis::FormatScientificE) {
				painter->drawText(QPoint(0,0), tickLabelStrings.at(i));
			} else {
				td.setHtml(tickLabelStrings.at(i));
				painter->translate(0, -td.size().height());
				td.drawContents(painter);
			}

			painter->restore();
			painter->translate(-tickLabelPoints.at(i));
		}
	}

	if (m_hovered && !isSelected() && !m_printing) {
		painter->setPen(q->hoveredPen);
		painter->setOpacity(q->hoveredOpacity);
		painter->drawPath(axisShape);
	}

	if (isSelected() && !m_printing) {
		painter->setPen(q->selectedPen);
		painter->setOpacity(q->selectedOpacity);
		painter->drawPath(axisShape);
	}
}

void AxisPrivate::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
	q->createContextMenu()->exec(event->screenPos());
}

void AxisPrivate::hoverEnterEvent(QGraphicsSceneHoverEvent*) {
	if (!isSelected()) {
		m_hovered = true;
		q->hovered();
		update(axisShape.boundingRect());
	}
}

void AxisPrivate::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
	if (m_hovered) {
		m_hovered = false;
		q->unhovered();
		update(axisShape.boundingRect());
	}
}

//##############################################################################
//##################  Serialization/Deserialization  ###########################
//##############################################################################
//! Save as XML
void Axis::save(QXmlStreamWriter* writer) const{
	Q_D(const Axis);

	writer->writeStartElement( "axis" );
	writeBasicAttributes( writer );
	writeCommentElement( writer );

	//general
	writer->writeStartElement( "general" );
	writer->writeAttribute( "autoScale", QString::number(d->autoScale) );
	writer->writeAttribute( "orientation", QString::number(d->orientation) );
	writer->writeAttribute( "position", QString::number(d->position) );
	writer->writeAttribute( "scale", QString::number(d->scale) );
	writer->writeAttribute( "offset", QString::number(d->offset) );
	writer->writeAttribute( "start", QString::number(d->start) );
	writer->writeAttribute( "end", QString::number(d->end) );
	writer->writeAttribute( "scalingFactor", QString::number(d->scalingFactor) );
	writer->writeAttribute( "zeroOffset", QString::number(d->zeroOffset) );
	writer->writeAttribute( "titleOffsetX", QString::number(d->titleOffsetX) );
	writer->writeAttribute( "titleOffsetY", QString::number(d->titleOffsetY) );
	writer->writeAttribute( "visible", QString::number(d->isVisible()) );
	writer->writeEndElement();

	//label
	d->title->save( writer );

	//line
	writer->writeStartElement( "line" );
	WRITE_QPEN(d->linePen);
	writer->writeAttribute( "opacity", QString::number(d->lineOpacity) );
	writer->writeAttribute( "arrowType", QString::number(d->arrowType) );
	writer->writeAttribute( "arrowPosition", QString::number(d->arrowPosition) );
	writer->writeAttribute( "arrowSize", QString::number(d->arrowSize) );
	writer->writeEndElement();

	//major ticks
	writer->writeStartElement( "majorTicks" );
	writer->writeAttribute( "direction", QString::number(d->majorTicksDirection) );
	writer->writeAttribute( "type", QString::number(d->majorTicksType) );
	writer->writeAttribute( "number", QString::number(d->majorTicksNumber) );
	writer->writeAttribute( "increment", QString::number(d->majorTicksIncrement) );
	WRITE_COLUMN(d->majorTicksColumn, majorTicksColumn);
	writer->writeAttribute( "length", QString::number(d->majorTicksLength) );
	WRITE_QPEN(d->majorTicksPen);
	writer->writeAttribute( "opacity", QString::number(d->majorTicksOpacity) );
	writer->writeEndElement();

	//minor ticks
	writer->writeStartElement( "minorTicks" );
	writer->writeAttribute( "direction", QString::number(d->minorTicksDirection) );
	writer->writeAttribute( "type", QString::number(d->minorTicksType) );
	writer->writeAttribute( "number", QString::number(d->minorTicksNumber) );
	writer->writeAttribute( "increment", QString::number(d->minorTicksIncrement) );
	WRITE_COLUMN(d->minorTicksColumn, minorTicksColumn);
	writer->writeAttribute( "length", QString::number(d->minorTicksLength) );
	WRITE_QPEN(d->minorTicksPen);
	writer->writeAttribute( "opacity", QString::number(d->minorTicksOpacity) );
	writer->writeEndElement();

	//extra ticks

	//labels
	writer->writeStartElement( "labels" );
	writer->writeAttribute( "position", QString::number(d->labelsPosition) );
	writer->writeAttribute( "offset", QString::number(d->labelsOffset) );
	writer->writeAttribute( "rotation", QString::number(d->labelsRotationAngle) );
	writer->writeAttribute( "format", QString::number(d->labelsFormat) );
	writer->writeAttribute( "precision", QString::number(d->labelsPrecision) );
	writer->writeAttribute( "autoPrecision", QString::number(d->labelsAutoPrecision) );
	WRITE_QCOLOR(d->labelsColor);
	WRITE_QFONT(d->labelsFont);
	writer->writeAttribute( "prefix", d->labelsPrefix );
	writer->writeAttribute( "suffix", d->labelsSuffix );
	writer->writeAttribute( "opacity", QString::number(d->labelsOpacity) );
	writer->writeEndElement();

	//grid
	writer->writeStartElement( "majorGrid" );
	WRITE_QPEN(d->majorGridPen);
	writer->writeAttribute( "opacity", QString::number(d->majorGridOpacity) );
	writer->writeEndElement();

	writer->writeStartElement( "minorGrid" );
	WRITE_QPEN(d->minorGridPen);
	writer->writeAttribute( "opacity", QString::number(d->minorGridOpacity) );
	writer->writeEndElement();

	writer->writeEndElement(); // close "axis" section
}

//! Load from XML
bool Axis::load(XmlStreamReader* reader) {
	Q_D(Axis);

	if (!reader->isStartElement() || reader->name() != "axis") {
		reader->raiseError(i18n("no axis element found"));
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
		if (reader->isEndElement() && reader->name() == "axis")
			break;

		if (!reader->isStartElement())
			continue;

		if (reader->name() == "comment") {
			if (!readCommentElement(reader)) return false;
		} else if (reader->name() == "general") {
			attribs = reader->attributes();

			str = attribs.value("autoScale").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'autoScale'"));
			else
				d->autoScale = (bool)str.toInt();

			str = attribs.value("orientation").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'orientation'"));
			else
				d->orientation = (Axis::AxisOrientation)str.toInt();

			str = attribs.value("position").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'position'"));
			else
				d->position = (Axis::AxisPosition)str.toInt();

			str = attribs.value("scale").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'scale'"));
			else
				d->scale = (Axis::AxisScale)str.toInt();

			str = attribs.value("offset").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'offset'"));
			else
				d->offset = str.toDouble();

			str = attribs.value("start").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'start'"));
			else
				d->start = str.toDouble();

			str = attribs.value("end").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'end'"));
			else
				d->end = str.toDouble();

			str = attribs.value("scalingFactor").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'scalingFactor'"));
			else
				d->scalingFactor = str.toDouble();

			str = attribs.value("zeroOffset").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'zeroOffset'"));
			else
				d->zeroOffset = str.toDouble();

			str = attribs.value("titleOffsetX").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'titleOffsetX'"));
			else
				d->titleOffsetX = str.toDouble();
			str = attribs.value("titleOffsetY").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'titleOffsetY'"));
			else
				d->titleOffsetY = str.toDouble();

			str = attribs.value("visible").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'visible'"));
			else
				d->setVisible(str.toInt());
		} else if (reader->name() == "textLabel") {
			d->title->load(reader);
		} else if (reader->name() == "line") {
			attribs = reader->attributes();

			READ_QPEN(d->linePen);

			str = attribs.value("opacity").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'opacity'"));
			else
				d->lineOpacity = str.toDouble();

			str = attribs.value("arrowType").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'arrowType'"));
			else
				d->arrowType = (Axis::ArrowType)str.toInt();

			str = attribs.value("arrowPosition").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'arrowPosition'"));
			else
				d->arrowPosition = (Axis::ArrowPosition)str.toInt();

			str = attribs.value("arrowSize").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'arrowSize'"));
			else
				d->arrowSize = str.toDouble();
		} else if (reader->name() == "majorTicks") {
			attribs = reader->attributes();

			str = attribs.value("direction").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'direction'"));
			else
				d->majorTicksDirection = (Axis::TicksDirection)str.toInt();

			str = attribs.value("type").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'type'"));
			else
				d->majorTicksType = (Axis::TicksType)str.toInt();

			str = attribs.value("number").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'number'"));
			else
				d->majorTicksNumber = str.toInt();

			str = attribs.value("increment").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'increment'"));
			else
				d->majorTicksIncrement = str.toDouble();

			READ_COLUMN(majorTicksColumn);

			str = attribs.value("length").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'length'"));
			else
				d->majorTicksLength = str.toDouble();

			READ_QPEN(d->majorTicksPen);

			str = attribs.value("opacity").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'opacity'"));
			else
				d->majorTicksOpacity = str.toDouble();
		} else if (reader->name() == "minorTicks") {
			attribs = reader->attributes();

			str = attribs.value("direction").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'direction'"));
			else
				d->minorTicksDirection = (Axis::TicksDirection)str.toInt();

			str = attribs.value("type").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'type'"));
			else
				d->minorTicksType = (Axis::TicksType)str.toInt();

			str = attribs.value("number").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'number'"));
			else
				d->minorTicksNumber = str.toInt();

			str = attribs.value("increment").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'increment'"));
			else
				d->minorTicksIncrement = str.toDouble();

			READ_COLUMN(minorTicksColumn);

			str = attribs.value("length").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'length'"));
			else
				d->minorTicksLength = str.toDouble();

			READ_QPEN(d->minorTicksPen);

			str = attribs.value("opacity").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'opacity'"));
			else
				d->minorTicksOpacity = str.toDouble();
		} else if (reader->name() == "labels") {
			attribs = reader->attributes();

			str = attribs.value("position").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'position'"));
			else
				d->labelsPosition = (Axis::LabelsPosition)str.toInt();

			str = attribs.value("offset").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'offset'"));
			else
				d->labelsOffset = str.toDouble();

			str = attribs.value("rotation").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'rotation'"));
			else
				d->labelsRotationAngle = str.toDouble();

			str = attribs.value("format").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'format'"));
			else
				d->labelsFormat = (Axis::LabelsFormat)str.toInt();

			str = attribs.value("precision").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'precision'"));
			else
				d->labelsPrecision = str.toInt();

			str = attribs.value("autoPrecision").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'autoPrecision'"));
			else
				d->labelsAutoPrecision = str.toInt();

			READ_QCOLOR(d->labelsColor);
			READ_QFONT(d->labelsFont);

			//don't produce any warning if no prefix or suffix is set (empty string is allowd here in xml)
			d->labelsPrefix = attribs.value("prefix").toString();
			d->labelsSuffix = attribs.value("suffix").toString();

			str = attribs.value("opacity").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'opacity'"));
			else
				d->labelsOpacity = str.toDouble();
		} else if (reader->name() == "majorGrid") {
			attribs = reader->attributes();

			READ_QPEN(d->majorGridPen);

			str = attribs.value("opacity").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'opacity'"));
			else
				d->majorGridOpacity = str.toDouble();
		} else if (reader->name() == "minorGrid") {
			attribs = reader->attributes();

			READ_QPEN(d->minorGridPen);

			str = attribs.value("opacity").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'opacity'"));
			else
				d->minorGridOpacity = str.toDouble();
		} else { // unknown element
			reader->raiseWarning(i18n("unknown element '%1'", reader->name().toString()));
			if (!reader->skipToEndElement()) return false;
		}
	}

	return true;
}
