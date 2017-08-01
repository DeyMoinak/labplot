/***************************************************************************
    File                 : CartesianPlot.cpp
    Project              : LabPlot
    Description          : Cartesian plot
    --------------------------------------------------------------------
    Copyright            : (C) 2011-2017 by Alexander Semke (alexander.semke@web.de)
    Copyright            : (C) 2016 by Stefan Gerlach (stefan.gerlach@uni.kn)

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

#include "CartesianPlot.h"
#include "CartesianPlotPrivate.h"
#include "Axis.h"
#include "XYCurve.h"
#include "Histogram.h"
#include "XYEquationCurve.h"
#include "XYDataReductionCurve.h"
#include "XYDifferentiationCurve.h"
#include "XYIntegrationCurve.h"
#include "XYInterpolationCurve.h"
#include "XYSmoothCurve.h"
#include "XYFitCurve.h"
#include "XYFourierFilterCurve.h"
#include "XYFourierTransformCurve.h"
#include "backend/core/Project.h"
#include "backend/worksheet/plots/cartesian/CartesianPlotLegend.h"
#include "backend/worksheet/plots/cartesian/CustomPoint.h"
#include "backend/worksheet/plots/PlotArea.h"
#include "backend/worksheet/plots/AbstractPlotPrivate.h"
#include "backend/worksheet/Worksheet.h"
#include "backend/worksheet/plots/cartesian/Axis.h"
#include "backend/worksheet/TextLabel.h"
#include "backend/lib/XmlStreamReader.h"
#include "backend/lib/commandtemplates.h"
#include "backend/lib/macros.h"
#include "kdefrontend/ThemeHandler.h"
#include "kdefrontend/widgets/ThemesWidget.h"

#include <QDir>
#include <QMenu>
#include <QToolBar>
#include <QPainter>
#include <QIcon>
#include <QWidgetAction>

#include <KConfigGroup>
#include <KLocale>

#include <cfloat>	// DBL_MAX

/**
 * \class CartesianPlot
 * \brief A xy-plot.
 *
 *
 */
CartesianPlot::CartesianPlot(const QString &name):AbstractPlot(name, new CartesianPlotPrivate(this)),
	m_legend(0), m_zoomFactor(1.2) {
	init();
}

CartesianPlot::CartesianPlot(const QString &name, CartesianPlotPrivate *dd):AbstractPlot(name, dd),
	m_legend(0), m_zoomFactor(1.2) {
	init();
}

CartesianPlot::~CartesianPlot() {
	delete m_coordinateSystem;
	delete addNewMenu;
	delete zoomMenu;
	delete themeMenu;
	//don't need to delete objects added with addChild()

	//no need to delete the d-pointer here - it inherits from QGraphicsItem
	//and is deleted during the cleanup in QGraphicsScene
}

/*!
	initializes all member variables of \c CartesianPlot
*/
void CartesianPlot::init() {
	Q_D(CartesianPlot);

	d->cSystem = new CartesianCoordinateSystem(this);
	m_coordinateSystem = d->cSystem;

	d->rangeType = CartesianPlot::RangeFree;
	d->rangeLastValues = 1000;
	d->rangeFirstValues = 1000;
	d->autoScaleX = true;
	d->autoScaleY = true;
	d->xScale = ScaleLinear;
	d->yScale = ScaleLinear;
	d->xRangeBreakingEnabled = false;
	d->yRangeBreakingEnabled = false;

	//the following factor determines the size of the offset between the min/max points of the curves
	//and the coordinate system ranges, when doing auto scaling
	//Factor 1 corresponds to the exact match - min/max values of the curves correspond to the start/end values of the ranges.
	d->autoScaleOffsetFactor = 0.05;

	//TODO: make this factor optional.
	//Provide in the UI the possibility to choose between "exact" or 0% offset, 2%, 5% and 10% for the auto fit option

	m_plotArea = new PlotArea(name() + " plot area");
	addChild(m_plotArea);

	//offset between the plot area and the area defining the coordinate system, in scene units.
	d->horizontalPadding = Worksheet::convertToSceneUnits(1.5, Worksheet::Centimeter);
	d->verticalPadding = Worksheet::convertToSceneUnits(1.5, Worksheet::Centimeter);

	initActions();
	initMenus();

	connect(this, SIGNAL(aspectAdded(const AbstractAspect*)), this, SLOT(childAdded(const AbstractAspect*)));
	connect(this, SIGNAL(aspectRemoved(const AbstractAspect*,const AbstractAspect*,const AbstractAspect*)),
			this, SLOT(childRemoved(const AbstractAspect*,const AbstractAspect*,const AbstractAspect*)));

	graphicsItem()->setFlag(QGraphicsItem::ItemIsMovable, true);
	graphicsItem()->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
	graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable, true);
	graphicsItem()->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
	graphicsItem()->setFlag(QGraphicsItem::ItemIsFocusable, true);
}

/*!
	initializes all children of \c CartesianPlot and
	setups a default plot of type \c type with a plot title.
*/
void CartesianPlot::initDefault(Type type) {
	Q_D(CartesianPlot);

	switch (type) {
	case FourAxes: {
		d->xMin = 0;
		d->xMax = 1;
		d->yMin = 0;
		d->yMax = 1;

		//Axes
		Axis* axis = new Axis("x axis 1", this, Axis::AxisHorizontal);
		addChild(axis);
		axis->setPosition(Axis::AxisBottom);
		axis->setStart(0);
		axis->setEnd(1);
		axis->setMajorTicksDirection(Axis::ticksIn);
		axis->setMajorTicksNumber(6);
		axis->setMinorTicksDirection(Axis::ticksIn);
		axis->setMinorTicksNumber(1);
		QPen pen = axis->majorGridPen();
		pen.setStyle(Qt::SolidLine);
		axis->setMajorGridPen(pen);
		pen = axis->minorGridPen();
		pen.setStyle(Qt::DotLine);
		axis->setMinorGridPen(pen);

		axis = new Axis("x axis 2", this, Axis::AxisHorizontal);
		addChild(axis);
		axis->setPosition(Axis::AxisTop);
		axis->setStart(0);
		axis->setEnd(1);
		axis->setMajorTicksDirection(Axis::ticksIn);
		axis->setMajorTicksNumber(6);
		axis->setMinorTicksDirection(Axis::ticksIn);
		axis->setMinorTicksNumber(1);
		axis->setLabelsPosition(Axis::NoLabels);
		axis->title()->setText(QString());

		axis = new Axis("y axis 1", this, Axis::AxisVertical);
		addChild(axis);
		axis->setPosition(Axis::AxisLeft);
		axis->setStart(0);
		axis->setEnd(1);
		axis->setMajorTicksDirection(Axis::ticksIn);
		axis->setMajorTicksNumber(6);
		axis->setMinorTicksDirection(Axis::ticksIn);
		axis->setMinorTicksNumber(1);
		pen = axis->majorGridPen();
		pen.setStyle(Qt::SolidLine);
		axis->setMajorGridPen(pen);
		pen = axis->minorGridPen();
		pen.setStyle(Qt::DotLine);
		axis->setMinorGridPen(pen);

		axis = new Axis("y axis 2", this, Axis::AxisVertical);
		addChild(axis);
		axis->setPosition(Axis::AxisRight);
		axis->setStart(0);
		axis->setEnd(1);
		axis->setOffset(1);
		axis->setMajorTicksDirection(Axis::ticksIn);
		axis->setMajorTicksNumber(6);
		axis->setMinorTicksDirection(Axis::ticksIn);
		axis->setMinorTicksNumber(1);
		axis->setLabelsPosition(Axis::NoLabels);
		axis->title()->setText(QString());

		break;
	}
	case TwoAxes: {
		d->xMin = 0;
		d->xMax = 1;
		d->yMin = 0;
		d->yMax = 1;

		Axis* axis = new Axis("x axis 1", this, Axis::AxisHorizontal);
		addChild(axis);
		axis->setPosition(Axis::AxisBottom);
		axis->setStart(0);
		axis->setEnd(1);
		axis->setMajorTicksDirection(Axis::ticksBoth);
		axis->setMajorTicksNumber(6);
		axis->setMinorTicksDirection(Axis::ticksBoth);
		axis->setMinorTicksNumber(1);
		axis->setArrowType(Axis::FilledArrowSmall);

		axis = new Axis("y axis 1", this, Axis::AxisVertical);
		addChild(axis);
		axis->setPosition(Axis::AxisLeft);
		axis->setStart(0);
		axis->setEnd(1);
		axis->setMajorTicksDirection(Axis::ticksBoth);
		axis->setMajorTicksNumber(6);
		axis->setMinorTicksDirection(Axis::ticksBoth);
		axis->setMinorTicksNumber(1);
		axis->setArrowType(Axis::FilledArrowSmall);

		break;
	}
	case TwoAxesCentered: {
		d->xMin = -0.5;
		d->xMax = 0.5;
		d->yMin = -0.5;
		d->yMax = 0.5;

		d->horizontalPadding = Worksheet::convertToSceneUnits(1.0, Worksheet::Centimeter);
		d->verticalPadding = Worksheet::convertToSceneUnits(1.0, Worksheet::Centimeter);

		QPen pen = m_plotArea->borderPen();
		pen.setStyle(Qt::NoPen);
		m_plotArea->setBorderPen(pen);

		Axis* axis = new Axis("x axis 1", this, Axis::AxisHorizontal);
		addChild(axis);
		axis->setPosition(Axis::AxisCentered);
		axis->setStart(-0.5);
		axis->setEnd(0.5);
		axis->setMajorTicksDirection(Axis::ticksBoth);
		axis->setMajorTicksNumber(6);
		axis->setMinorTicksDirection(Axis::ticksBoth);
		axis->setMinorTicksNumber(1);
		axis->setArrowType(Axis::FilledArrowSmall);
		axis->title()->setText(QString());

		axis = new Axis("y axis 1", this, Axis::AxisVertical);
		addChild(axis);
		axis->setPosition(Axis::AxisCentered);
		axis->setStart(-0.5);
		axis->setEnd(0.5);
		axis->setMajorTicksDirection(Axis::ticksBoth);
		axis->setMajorTicksNumber(6);
		axis->setMinorTicksDirection(Axis::ticksBoth);
		axis->setMinorTicksNumber(1);
		axis->setArrowType(Axis::FilledArrowSmall);
		axis->title()->setText(QString());

		break;
	}
	case TwoAxesCenteredZero: {
		d->xMin = -0.5;
		d->xMax = 0.5;
		d->yMin = -0.5;
		d->yMax = 0.5;

		d->horizontalPadding = Worksheet::convertToSceneUnits(1.0, Worksheet::Centimeter);
		d->verticalPadding = Worksheet::convertToSceneUnits(1.0, Worksheet::Centimeter);

		QPen pen = m_plotArea->borderPen();
		pen.setStyle(Qt::NoPen);
		m_plotArea->setBorderPen(pen);

		Axis* axis = new Axis("x axis 1", this, Axis::AxisHorizontal);
		addChild(axis);
		axis->setPosition(Axis::AxisCustom);
		axis->setOffset(0);
		axis->setStart(-0.5);
		axis->setEnd(0.5);
		axis->setMajorTicksDirection(Axis::ticksBoth);
		axis->setMajorTicksNumber(6);
		axis->setMinorTicksDirection(Axis::ticksBoth);
		axis->setMinorTicksNumber(1);
		axis->setArrowType(Axis::FilledArrowSmall);
		axis->title()->setText(QString());

		axis = new Axis("y axis 1", this, Axis::AxisVertical);
		addChild(axis);
		axis->setPosition(Axis::AxisCustom);
		axis->setOffset(0);
		axis->setStart(-0.5);
		axis->setEnd(0.5);
		axis->setMajorTicksDirection(Axis::ticksBoth);
		axis->setMajorTicksNumber(6);
		axis->setMinorTicksDirection(Axis::ticksBoth);
		axis->setMinorTicksNumber(1);
		axis->setArrowType(Axis::FilledArrowSmall);
		axis->title()->setText(QString());

		break;
	}
	}

	d->xMinPrev = d->xMin;
	d->xMaxPrev = d->xMax;
	d->yMinPrev = d->yMin;
	d->yMaxPrev = d->yMax;

	//Plot title
	m_title = new TextLabel(this->name(), TextLabel::PlotTitle);
	addChild(m_title);
	m_title->setHidden(true);
	m_title->setParentGraphicsItem(m_plotArea->graphicsItem());

	//Geometry, specify the plot rect in scene coordinates.
	//TODO: Use default settings for left, top, width, height and for min/max for the coordinate system
	float x = Worksheet::convertToSceneUnits(2, Worksheet::Centimeter);
	float y = Worksheet::convertToSceneUnits(2, Worksheet::Centimeter);
	float w = Worksheet::convertToSceneUnits(10, Worksheet::Centimeter);
	float h = Worksheet::convertToSceneUnits(10, Worksheet::Centimeter);

	//all plot children are initialized -> set the geometry of the plot in scene coordinates.
	d->rect = QRectF(x,y,w,h);
	d->retransform();
}

void CartesianPlot::initActions() {
	//"add new" actions
	addCurveAction = new QAction(QIcon::fromTheme("labplot-xy-curve"), i18n("xy-curve"), this);
	addHistogramPlot = new QAction(QIcon::fromTheme("labplot-xy-fourier_filter-curve"), i18n("Histogram"), this);
	addEquationCurveAction = new QAction(QIcon::fromTheme("labplot-xy-equation-curve"), i18n("xy-curve from a mathematical equation"), this);
// no icons yet
	addDataReductionCurveAction = new QAction(i18n("xy-curve from a data reduction"), this);
	addDifferentiationCurveAction = new QAction(i18n("xy-curve from a differentiation"), this);
	addIntegrationCurveAction = new QAction(i18n("xy-curve from an integration"), this);
	addInterpolationCurveAction = new QAction(i18n("xy-curve from an interpolation"), this);
	addSmoothCurveAction = new QAction(i18n("xy-curve from a smooth"), this);
	addFitCurveAction = new QAction(QIcon::fromTheme("labplot-xy-fit-curve"), i18n("xy-curve from a fit to data"), this);
	addFourierFilterCurveAction = new QAction(i18n("xy-curve from a Fourier filter"), this);
	addFourierTransformCurveAction = new QAction(i18n("xy-curve from a Fourier transform"), this);
//	addInterpolationCurveAction = new QAction(QIcon::fromTheme("labplot-xy-interpolation-curve"), i18n("xy-curve from an interpolation"), this);
//	addSmoothCurveAction = new QAction(QIcon::fromTheme("labplot-xy-smooth-curve"), i18n("xy-curve from a smooth"), this);
//	addFourierFilterCurveAction = new QAction(QIcon::fromTheme("labplot-xy-fourier_filter-curve"), i18n("xy-curve from a Fourier filter"), this);
//	addFourierTransformCurveAction = new QAction(QIcon::fromTheme("labplot-xy-fourier_transform-curve"), i18n("xy-curve from a Fourier transform"), this);
	addLegendAction = new QAction(QIcon::fromTheme("text-field"), i18n("legend"), this);
	addHorizontalAxisAction = new QAction(QIcon::fromTheme("labplot-axis-horizontal"), i18n("horizontal axis"), this);
	addVerticalAxisAction = new QAction(QIcon::fromTheme("labplot-axis-vertical"), i18n("vertical axis"), this);
	addCustomPointAction = new QAction(QIcon::fromTheme("draw-cross"), i18n("custom point"), this);

	connect(addCurveAction, SIGNAL(triggered()), SLOT(addCurve()));
	connect(addHistogramPlot,SIGNAL(triggered()), SLOT(addHistogram()));
	connect(addEquationCurveAction, SIGNAL(triggered()), SLOT(addEquationCurve()));
	connect(addDataReductionCurveAction, SIGNAL(triggered()), SLOT(addDataReductionCurve()));
	connect(addDifferentiationCurveAction, SIGNAL(triggered()), SLOT(addDifferentiationCurve()));
	connect(addIntegrationCurveAction, SIGNAL(triggered()), SLOT(addIntegrationCurve()));
	connect(addInterpolationCurveAction, SIGNAL(triggered()), SLOT(addInterpolationCurve()));
	connect(addSmoothCurveAction, SIGNAL(triggered()), SLOT(addSmoothCurve()));
	connect(addFitCurveAction, SIGNAL(triggered()), SLOT(addFitCurve()));
	connect(addFourierFilterCurveAction, SIGNAL(triggered()), SLOT(addFourierFilterCurve()));
	connect(addFourierTransformCurveAction, SIGNAL(triggered()), SLOT(addFourierTransformCurve()));
	connect(addLegendAction, SIGNAL(triggered()), SLOT(addLegend()));
	connect(addHorizontalAxisAction, SIGNAL(triggered()), SLOT(addHorizontalAxis()));
	connect(addVerticalAxisAction, SIGNAL(triggered()), SLOT(addVerticalAxis()));
	connect(addCustomPointAction, SIGNAL(triggered()), SLOT(addCustomPoint()));

	//Analysis menu actions
	addDataOperationAction = new QAction(i18n("Data operation"), this);
	addDataReductionAction = new QAction(i18n("Reduce data"), this);
	addDifferentiationAction = new QAction(i18n("Differentiate"), this);
	addIntegrationAction = new QAction(i18n("Integrate"), this);
	addInterpolationAction = new QAction(i18n("Interpolate"), this);
	addSmoothAction = new QAction(i18n("Smooth"), this);

	addFitAction.append(new QAction(i18n("Linear"), this));
	addFitAction.append(new QAction(i18n("Power"), this));
	addFitAction.append(new QAction(i18n("Exponential (degree 1)"), this));
	addFitAction.append(new QAction(i18n("Exponential (degree 2)"), this));
	addFitAction.append(new QAction(i18n("Inverse exponential"), this));
	addFitAction.append(new QAction(i18n("Gauss"), this));
	addFitAction.append(new QAction(i18n("Cauchy-Lorentz"), this));
	addFitAction.append(new QAction(i18n("Arc Tangent"), this));
	addFitAction.append(new QAction(i18n("Hyperbolic tangent"), this));
	addFitAction.append(new QAction(i18n("Error function"), this));
	addFitAction.append(new QAction(i18n("Custom"), this));

	addFourierFilterAction = new QAction(i18n("Fourier filter"), this);

	connect(addDataReductionAction, SIGNAL(triggered()), SLOT(addDataReductionCurve()));
	connect(addDifferentiationAction, SIGNAL(triggered()), SLOT(addDifferentiationCurve()));
	connect(addIntegrationAction, SIGNAL(triggered()), SLOT(addIntegrationCurve()));
	connect(addInterpolationAction, SIGNAL(triggered()), SLOT(addInterpolationCurve()));
	connect(addSmoothAction, SIGNAL(triggered()), SLOT(addSmoothCurve()));
	for (const auto& action: addFitAction)
		connect(action, SIGNAL(triggered()), SLOT(addFitCurve()));
	connect(addFourierFilterAction, SIGNAL(triggered()), SLOT(addFourierFilterCurve()));

	//zoom/navigate actions
	scaleAutoAction = new QAction(QIcon::fromTheme("labplot-auto-scale-all"), i18n("auto scale"), this);
	scaleAutoXAction = new QAction(QIcon::fromTheme("labplot-auto-scale-x"), i18n("auto scale X"), this);
	scaleAutoYAction = new QAction(QIcon::fromTheme("labplot-auto-scale-y"), i18n("auto scale Y"), this);
	zoomInAction = new QAction(QIcon::fromTheme("zoom-in"), i18n("zoom in"), this);
	zoomOutAction = new QAction(QIcon::fromTheme("zoom-out"), i18n("zoom out"), this);
	zoomInXAction = new QAction(QIcon::fromTheme("labplot-zoom-in-x"), i18n("zoom in X"), this);
	zoomOutXAction = new QAction(QIcon::fromTheme("labplot-zoom-out-x"), i18n("zoom out X"), this);
	zoomInYAction = new QAction(QIcon::fromTheme("labplot-zoom-in-y"), i18n("zoom in Y"), this);
	zoomOutYAction = new QAction(QIcon::fromTheme("labplot-zoom-out-y"), i18n("zoom out Y"), this);
	shiftLeftXAction = new QAction(QIcon::fromTheme("labplot-shift-left-x"), i18n("shift left X"), this);
	shiftRightXAction = new QAction(QIcon::fromTheme("labplot-shift-right-x"), i18n("shift right X"), this);
	shiftUpYAction = new QAction(QIcon::fromTheme("labplot-shift-up-y"), i18n("shift up Y"), this);
	shiftDownYAction = new QAction(QIcon::fromTheme("labplot-shift-down-y"), i18n("shift down Y"), this);

	connect(scaleAutoAction, SIGNAL(triggered()), SLOT(scaleAuto()));
	connect(scaleAutoXAction, SIGNAL(triggered()), SLOT(scaleAutoX()));
	connect(scaleAutoYAction, SIGNAL(triggered()), SLOT(scaleAutoY()));
	connect(zoomInAction, SIGNAL(triggered()), SLOT(zoomIn()));
	connect(zoomOutAction, SIGNAL(triggered()), SLOT(zoomOut()));
	connect(zoomInXAction, SIGNAL(triggered()), SLOT(zoomInX()));
	connect(zoomOutXAction, SIGNAL(triggered()), SLOT(zoomOutX()));
	connect(zoomInYAction, SIGNAL(triggered()), SLOT(zoomInY()));
	connect(zoomOutYAction, SIGNAL(triggered()), SLOT(zoomOutY()));
	connect(shiftLeftXAction, SIGNAL(triggered()), SLOT(shiftLeftX()));
	connect(shiftRightXAction, SIGNAL(triggered()), SLOT(shiftRightX()));
	connect(shiftUpYAction, SIGNAL(triggered()), SLOT(shiftUpY()));
	connect(shiftDownYAction, SIGNAL(triggered()), SLOT(shiftDownY()));

	//visibility action
	visibilityAction = new QAction(i18n("visible"), this);
	visibilityAction->setCheckable(true);
	connect(visibilityAction, SIGNAL(triggered()), this, SLOT(visibilityChanged()));
}

void CartesianPlot::initMenus() {
	addNewMenu = new QMenu(i18n("Add new"));
	addNewMenu->addAction(addCurveAction);
	addNewMenu->addAction(addHistogramPlot);
	addNewMenu->addAction(addEquationCurveAction);
	addNewMenu->addSeparator();
	addNewMenu->addAction(addDataReductionCurveAction);
	addNewMenu->addAction(addDifferentiationCurveAction);
	addNewMenu->addAction(addIntegrationCurveAction);
	addNewMenu->addAction(addInterpolationCurveAction);
	addNewMenu->addAction(addSmoothCurveAction);
	addNewMenu->addAction(addFitCurveAction);
	addNewMenu->addAction(addFourierFilterCurveAction);
	addNewMenu->addAction(addFourierTransformCurveAction);
	addNewMenu->addSeparator();
	addNewMenu->addAction(addLegendAction);
	addNewMenu->addSeparator();
	addNewMenu->addAction(addHorizontalAxisAction);
	addNewMenu->addAction(addVerticalAxisAction);
	addNewMenu->addSeparator();
	addNewMenu->addAction(addCustomPointAction);

	zoomMenu = new QMenu(i18n("Zoom"));
	zoomMenu->addAction(scaleAutoAction);
	zoomMenu->addAction(scaleAutoXAction);
	zoomMenu->addAction(scaleAutoYAction);
	zoomMenu->addSeparator();
	zoomMenu->addAction(zoomInAction);
	zoomMenu->addAction(zoomOutAction);
	zoomMenu->addSeparator();
	zoomMenu->addAction(zoomInXAction);
	zoomMenu->addAction(zoomOutXAction);
	zoomMenu->addSeparator();
	zoomMenu->addAction(zoomInYAction);
	zoomMenu->addAction(zoomOutYAction);
	zoomMenu->addSeparator();
	zoomMenu->addAction(shiftLeftXAction);
	zoomMenu->addAction(shiftRightXAction);
	zoomMenu->addSeparator();
	zoomMenu->addAction(shiftUpYAction);
	zoomMenu->addAction(shiftDownYAction);

	// Data manipulation menu
	QMenu* dataManipulationMenu = new QMenu(i18n("Data Manipulation"));
	dataManipulationMenu->setIcon(QIcon::fromTheme("zoom-draw"));
	dataManipulationMenu->addAction(addDataOperationAction);
	dataManipulationMenu->addAction(addDataReductionAction);

	// Data fit menu
	QMenu* dataFitMenu = new QMenu(i18n("Fit"));
	dataFitMenu->setIcon(QIcon::fromTheme("labplot-xy-fit-curve"));
	dataFitMenu->addAction(addFitAction.at(0));
	dataFitMenu->addAction(addFitAction.at(1));
	dataFitMenu->addAction(addFitAction.at(2));
	dataFitMenu->addAction(addFitAction.at(3));
	dataFitMenu->addAction(addFitAction.at(4));
	dataFitMenu->addSeparator();
	dataFitMenu->addAction(addFitAction.at(5));
	dataFitMenu->addAction(addFitAction.at(6));
	dataFitMenu->addSeparator();
	dataFitMenu->addAction(addFitAction.at(7));
	dataFitMenu->addAction(addFitAction.at(8));
	dataFitMenu->addAction(addFitAction.at(9));
	dataFitMenu->addSeparator();
	dataFitMenu->addAction(addFitAction.at(10));

	//analysis menu
	dataAnalysisMenu = new QMenu(i18n("Analysis"));
	dataAnalysisMenu->insertMenu(0, dataManipulationMenu);
	dataAnalysisMenu->addSeparator();
	dataAnalysisMenu->addAction(addDifferentiationAction);
	dataAnalysisMenu->addAction(addIntegrationAction);
	dataAnalysisMenu->addSeparator();
	dataAnalysisMenu->addAction(addInterpolationAction);
	dataAnalysisMenu->addAction(addSmoothAction);
	dataAnalysisMenu->addAction(addFourierFilterAction);
	dataAnalysisMenu->addSeparator();
	dataAnalysisMenu->addMenu(dataFitMenu);

	//themes menu
	themeMenu = new QMenu(i18n("Apply Theme"));
	ThemesWidget* themeWidget = new ThemesWidget(0);
	// TODO: SLOT: loadTheme(KConfig config)
	connect(themeWidget, SIGNAL(themeSelected(QString)), this, SLOT(loadTheme(QString)));
	connect(themeWidget, SIGNAL(themeSelected(QString)), themeMenu, SLOT(close()));

	QWidgetAction* widgetAction = new QWidgetAction(this);
	widgetAction->setDefaultWidget(themeWidget);
	themeMenu->addAction(widgetAction);
}

QMenu* CartesianPlot::createContextMenu() {
	QMenu* menu = WorksheetElement::createContextMenu();
	QAction* firstAction = menu->actions().at(1);

	visibilityAction->setChecked(isVisible());
	menu->insertAction(firstAction, visibilityAction);

	menu->insertMenu(firstAction, addNewMenu);
	menu->insertMenu(firstAction, zoomMenu);
	menu->insertSeparator(firstAction);
	menu->insertMenu(firstAction, themeMenu);
	menu->insertSeparator(firstAction);

	return menu;
}

QMenu* CartesianPlot::analysisMenu() const {
	return dataAnalysisMenu;
}

/*!
	Returns an icon to be used in the project explorer.
*/
QIcon CartesianPlot::icon() const {
	return QIcon::fromTheme("office-chart-line");
}

void CartesianPlot::navigate(CartesianPlot::NavigationOperation op) {
	if (op == ScaleAuto) scaleAuto();
	else if (op == ScaleAutoX) scaleAutoX();
	else if (op == ScaleAutoY) scaleAutoY();
	else if (op == ZoomIn) zoomIn();
	else if (op == ZoomOut) zoomOut();
	else if (op == ZoomInX) zoomInX();
	else if (op == ZoomOutX) zoomOutX();
	else if (op == ZoomInY) zoomInY();
	else if (op == ZoomOutY) zoomOutY();
	else if (op == ShiftLeftX) shiftLeftX();
	else if (op == ShiftRightX) shiftRightX();
	else if (op == ShiftUpY) shiftUpY();
	else if (op == ShiftDownY) shiftDownY();
}

//##############################################################################
//################################  getter methods  ############################
//##############################################################################
BASIC_SHARED_D_READER_IMPL(CartesianPlot, CartesianPlot::RangeType, rangeType, rangeType)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, int, rangeLastValues, rangeLastValues)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, int, rangeFirstValues, rangeFirstValues)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, bool, autoScaleX, autoScaleX)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, float, xMin, xMin)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, float, xMax, xMax)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, CartesianPlot::Scale, xScale, xScale)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, bool, xRangeBreakingEnabled, xRangeBreakingEnabled)
CLASS_SHARED_D_READER_IMPL(CartesianPlot, CartesianPlot::RangeBreaks, xRangeBreaks, xRangeBreaks)

BASIC_SHARED_D_READER_IMPL(CartesianPlot, bool, autoScaleY, autoScaleY)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, float, yMin, yMin)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, float, yMax, yMax)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, CartesianPlot::Scale, yScale, yScale)
BASIC_SHARED_D_READER_IMPL(CartesianPlot, bool, yRangeBreakingEnabled, yRangeBreakingEnabled)
CLASS_SHARED_D_READER_IMPL(CartesianPlot, CartesianPlot::RangeBreaks, yRangeBreaks, yRangeBreaks)

CLASS_SHARED_D_READER_IMPL(CartesianPlot, QString, theme, theme)

/*!
	return the actual bounding rectangular of the plot (plot's rectangular minus padding)
	in plot's coordinates
 */
//TODO: return here a private variable only, update this variable on rect and padding changes.
QRectF CartesianPlot::plotRect() {
	Q_D(const CartesianPlot);
	QRectF rect = d->mapRectFromScene(d->rect);
	rect.setX(rect.x() + d->horizontalPadding);
	rect.setY(rect.y() + d->verticalPadding);
	rect.setWidth(rect.width() - d->horizontalPadding);
	rect.setHeight(rect.height()-d->verticalPadding);
	return rect;
}

CartesianPlot::MouseMode CartesianPlot::mouseMode() const {
	Q_D(const CartesianPlot);
	return d->mouseMode;
}

//##############################################################################
//######################  setter methods and undo commands  ####################
//##############################################################################
/*!
	set the rectangular, defined in scene coordinates
 */
class CartesianPlotSetRectCmd : public QUndoCommand {
public:
	CartesianPlotSetRectCmd(CartesianPlotPrivate* private_obj, QRectF rect) : m_private(private_obj), m_rect(rect) {
		setText(i18n("%1: change geometry rect", m_private->name()));
	};

	virtual void redo() {
		QRectF tmp = m_private->rect;
		const double horizontalRatio = m_rect.width() / m_private->rect.width();
		const double verticalRatio = m_rect.height() / m_private->rect.height();
		m_private->q->handleResize(horizontalRatio, verticalRatio, false);
		m_private->rect = m_rect;
		m_rect = tmp;
		m_private->retransform();
		emit m_private->q->rectChanged(m_private->rect);
	};

	virtual void undo() {
		redo();
	}

private:
	CartesianPlotPrivate* m_private;
	QRectF m_rect;
};

void CartesianPlot::setRect(const QRectF& rect) {
	Q_D(CartesianPlot);
	if (rect != d->rect)
		exec(new CartesianPlotSetRectCmd(d, rect));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetRangeType, CartesianPlot::RangeType, rangeType, rangeChanged);
void CartesianPlot::setRangeType(RangeType type) {
	Q_D(CartesianPlot);
	if (type != d->rangeType)
		exec(new CartesianPlotSetRangeTypeCmd(d, type, i18n("%1: set range type")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetRangeLastValues, int, rangeLastValues, rangeChanged);
void CartesianPlot::setRangeLastValues(int values) {
	Q_D(CartesianPlot);
	if (values != d->rangeLastValues)
		exec(new CartesianPlotSetRangeLastValuesCmd(d, values, i18n("%1: set range")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetRangeFirstValues, int, rangeFirstValues, rangeChanged);
void CartesianPlot::setRangeFirstValues(int values) {
	Q_D(CartesianPlot);
	if (values != d->rangeFirstValues)
		exec(new CartesianPlotSetRangeFirstValuesCmd(d, values, i18n("%1: set range")));
}


class CartesianPlotSetAutoScaleXCmd : public QUndoCommand {
public:
	CartesianPlotSetAutoScaleXCmd(CartesianPlotPrivate* private_obj, bool autoScale) :
		m_private(private_obj), m_autoScale(autoScale), m_minOld(0.0), m_maxOld(0.0) {
		setText(i18n("%1: change x-range auto scaling", m_private->name()));
	};

	virtual void redo() {
		m_autoScaleOld = m_private->autoScaleX;
		if (m_autoScale) {
			m_minOld = m_private->xMin;
			m_maxOld = m_private->xMax;
			m_private->q->scaleAutoX();
		}
		m_private->autoScaleX = m_autoScale;
		emit m_private->q->xAutoScaleChanged(m_autoScale);
	};

	virtual void undo() {
		if (!m_autoScaleOld) {
			m_private->xMin = m_minOld;
			m_private->xMax = m_maxOld;
			m_private->retransformScales();
		}
		m_private->autoScaleX = m_autoScaleOld;
		emit m_private->q->xAutoScaleChanged(m_autoScaleOld);
	}

private:
	CartesianPlotPrivate* m_private;
	bool m_autoScale;
	bool m_autoScaleOld;
	float m_minOld;
	float m_maxOld;
};

void CartesianPlot::setAutoScaleX(bool autoScaleX) {
	Q_D(CartesianPlot);
	if (autoScaleX != d->autoScaleX)
		exec(new CartesianPlotSetAutoScaleXCmd(d, autoScaleX));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetXMin, float, xMin, retransformScales)
void CartesianPlot::setXMin(float xMin) {
	Q_D(CartesianPlot);
	if (xMin != d->xMin)
		exec(new CartesianPlotSetXMinCmd(d, xMin, i18n("%1: set min x")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetXMax, float, xMax, retransformScales)
void CartesianPlot::setXMax(float xMax) {
	Q_D(CartesianPlot);
	if (xMax != d->xMax)
		exec(new CartesianPlotSetXMaxCmd(d, xMax, i18n("%1: set max x")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetXScale, CartesianPlot::Scale, xScale, retransformScales)
void CartesianPlot::setXScale(Scale scale) {
	Q_D(CartesianPlot);
	if (scale != d->xScale)
		exec(new CartesianPlotSetXScaleCmd(d, scale, i18n("%1: set x scale")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetXRangeBreakingEnabled, bool, xRangeBreakingEnabled, retransformScales)
void CartesianPlot::setXRangeBreakingEnabled(bool enabled) {
	Q_D(CartesianPlot);
	if (enabled != d->xRangeBreakingEnabled)
		exec(new CartesianPlotSetXRangeBreakingEnabledCmd(d, enabled, i18n("%1: x-range breaking enabled")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetXRangeBreaks, CartesianPlot::RangeBreaks, xRangeBreaks, retransformScales)
void CartesianPlot::setXRangeBreaks(const RangeBreaks& breakings) {
	Q_D(CartesianPlot);
	exec(new CartesianPlotSetXRangeBreaksCmd(d, breakings, i18n("%1: x-range breaks changed")));
}

class CartesianPlotSetAutoScaleYCmd : public QUndoCommand {
public:
	CartesianPlotSetAutoScaleYCmd(CartesianPlotPrivate* private_obj, bool autoScale) :
		m_private(private_obj), m_autoScale(autoScale), m_minOld(0.0), m_maxOld(0.0) {
		setText(i18n("%1: change y-range auto scaling", m_private->name()));
	};

	virtual void redo() {
		m_autoScaleOld = m_private->autoScaleY;
		if (m_autoScale) {
			m_minOld = m_private->yMin;
			m_maxOld = m_private->yMax;
			m_private->q->scaleAutoY();
		}
		m_private->autoScaleY = m_autoScale;
		emit m_private->q->yAutoScaleChanged(m_autoScale);
	};

	virtual void undo() {
		if (!m_autoScaleOld) {
			m_private->yMin = m_minOld;
			m_private->yMax = m_maxOld;
			m_private->retransformScales();
		}
		m_private->autoScaleY = m_autoScaleOld;
		emit m_private->q->yAutoScaleChanged(m_autoScaleOld);
	}

private:
	CartesianPlotPrivate* m_private;
	bool m_autoScale;
	bool m_autoScaleOld;
	float m_minOld;
	float m_maxOld;
};

void CartesianPlot::setAutoScaleY(bool autoScaleY) {
	Q_D(CartesianPlot);
	if (autoScaleY != d->autoScaleY)
		exec(new CartesianPlotSetAutoScaleYCmd(d, autoScaleY));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetYMin, float, yMin, retransformScales)
void CartesianPlot::setYMin(float yMin) {
	Q_D(CartesianPlot);
	if (yMin != d->yMin)
		exec(new CartesianPlotSetYMinCmd(d, yMin, i18n("%1: set min y")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetYMax, float, yMax, retransformScales)
void CartesianPlot::setYMax(float yMax) {
	Q_D(CartesianPlot);
	if (yMax != d->yMax)
		exec(new CartesianPlotSetYMaxCmd(d, yMax, i18n("%1: set max y")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetYScale, CartesianPlot::Scale, yScale, retransformScales)
void CartesianPlot::setYScale(Scale scale) {
	Q_D(CartesianPlot);
	if (scale != d->yScale)
		exec(new CartesianPlotSetYScaleCmd(d, scale, i18n("%1: set y scale")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetYRangeBreakingEnabled, bool, yRangeBreakingEnabled, retransformScales)
void CartesianPlot::setYRangeBreakingEnabled(bool enabled) {
	Q_D(CartesianPlot);
	if (enabled != d->yRangeBreakingEnabled)
		exec(new CartesianPlotSetYRangeBreakingEnabledCmd(d, enabled, i18n("%1: y-range breaking enabled")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlot, SetYRangeBreaks, CartesianPlot::RangeBreaks, yRangeBreaks, retransformScales)
void CartesianPlot::setYRangeBreaks(const RangeBreaks& breaks) {
	Q_D(CartesianPlot);
	exec(new CartesianPlotSetYRangeBreaksCmd(d, breaks, i18n("%1: y-range breaks changed")));
}

STD_SETTER_CMD_IMPL_S(CartesianPlot, SetTheme, QString, theme)
void CartesianPlot::setTheme(const QString& theme) {
	Q_D(CartesianPlot);
	if (theme != d->theme) {
		if (!theme.isEmpty()) {
			beginMacro( i18n("%1: load theme %2", name(), theme) );
			exec(new CartesianPlotSetThemeCmd(d, theme, i18n("%1: set theme")));
			loadTheme(theme);
			endMacro();
		} else {
			exec(new CartesianPlotSetThemeCmd(d, theme, i18n("%1: disable theming")));
		}
	}
}

//################################################################
//########################## Slots ###############################
//################################################################
void CartesianPlot::addHorizontalAxis() {
	Axis* axis = new Axis("x-axis", this, Axis::AxisHorizontal);
	if (axis->autoScale()) {
		axis->setUndoAware(false);
		axis->setStart(xMin());
		axis->setEnd(xMax());
		axis->setUndoAware(true);
	}
	addChild(axis);
}

void CartesianPlot::addVerticalAxis() {
	Axis* axis = new Axis("y-axis", this, Axis::AxisVertical);
	if (axis->autoScale()) {
		axis->setUndoAware(false);
		axis->setStart(yMin());
		axis->setEnd(yMax());
		axis->setUndoAware(true);
	}
	addChild(axis);
}

XYCurve* CartesianPlot::addCurve() {
	XYCurve* curve = new XYCurve("xy-curve");
	this->addChild(curve);
	this->applyThemeOnNewCurve(curve);
	return curve;
}

XYEquationCurve* CartesianPlot::addEquationCurve() {
	XYEquationCurve* curve = new XYEquationCurve("f(x)");
	this->addChild(curve);
	this->applyThemeOnNewCurve(curve);
	return curve;
}

XYDataReductionCurve* CartesianPlot::addDataReductionCurve() {
	XYDataReductionCurve* curve = new XYDataReductionCurve("Data reduction");
	this->addChild(curve);
	return curve;
}

/*!
 * returns the first selected XYCurve in the plot
 */
const XYCurve* CartesianPlot::currentCurve() const {
	for (const auto* curve: this->children<const XYCurve>()) {
		if (curve->graphicsItem()->isSelected())
			return curve;
	}

	return 0;
}

XYDifferentiationCurve* CartesianPlot::addDifferentiationCurve() {
	XYDifferentiationCurve* curve = new XYDifferentiationCurve("Differentiation");
	const XYCurve* curCurve = currentCurve();
	if (curCurve) {
		beginMacro( i18n("%1: differentiate '%2'", name(), curCurve->name()) );
		curve->setName( i18n("Derivative of '%1'", curCurve->name()) );
		curve->setDataSourceType(XYCurve::DataSourceCurve);
		curve->setDataSourceCurve(curCurve);
		this->addChild(curve);
		curve->recalculate();
		emit curve->differentiationDataChanged(curve->differentiationData());
	} else {
		beginMacro(i18n("%1: add differentiation curve", name()));
		this->addChild(curve);
	}

	this->applyThemeOnNewCurve(curve);
	endMacro();

	return curve;
}

XYIntegrationCurve* CartesianPlot::addIntegrationCurve() {
	XYIntegrationCurve* curve = new XYIntegrationCurve("Integration");
	const XYCurve* curCurve = currentCurve();
	if (curCurve) {
		beginMacro( i18n("%1: integrate '%2'", name(), curCurve->name()) );
		curve->setName( i18n("Integral of '%1'", curCurve->name()) );
		curve->setDataSourceType(XYCurve::DataSourceCurve);
		curve->setDataSourceCurve(curCurve);
		this->addChild(curve);
		curve->recalculate();
		emit curve->integrationDataChanged(curve->integrationData());
	} else {
		beginMacro(i18n("%1: add differentiation curve", name()));
		this->addChild(curve);
	}

	this->applyThemeOnNewCurve(curve);
	endMacro();

	return curve;
}

XYInterpolationCurve* CartesianPlot::addInterpolationCurve() {
	XYInterpolationCurve* curve = new XYInterpolationCurve("Interpolation");
	const XYCurve* curCurve = currentCurve();
	if (curCurve) {
		beginMacro( i18n("%1: interpolate '%2'", name(), curCurve->name()) );
		curve->setName( i18n("Interpolation of '%1'", curCurve->name()) );
		curve->setDataSourceType(XYCurve::DataSourceCurve);
		curve->setDataSourceCurve(curCurve);
		this->addChild(curve);
		curve->recalculate();
		emit curve->interpolationDataChanged(curve->interpolationData());
	} else {
		beginMacro(i18n("%1: add interpolation curve", name()));
		this->addChild(curve);
	}

	this->applyThemeOnNewCurve(curve);
	endMacro();

	return curve;
}

Histogram* CartesianPlot::addHistogram(){
	Histogram* curve= new Histogram("Histogram");
	this->addChild(curve);
	return curve;
}

XYSmoothCurve* CartesianPlot::addSmoothCurve() {
	XYSmoothCurve* curve = new XYSmoothCurve("Smooth");
	const XYCurve* curCurve = currentCurve();
	if (curCurve) {
		beginMacro( i18n("%1: smooth '%2'", name(), curCurve->name()) );
		curve->setName( i18n("Smoothing of '%1'", curCurve->name()) );
		curve->setDataSourceType(XYCurve::DataSourceCurve);
		curve->setDataSourceCurve(curCurve);
		this->addChild(curve);
		curve->recalculate();
		emit curve->smoothDataChanged(curve->smoothData());
	} else {
		beginMacro(i18n("%1: add smoothing curve", name()));
		this->addChild(curve);
	}

	this->applyThemeOnNewCurve(curve);
	endMacro();

	return curve;
}

XYFitCurve* CartesianPlot::addFitCurve() {
	XYFitCurve* curve = new XYFitCurve("fit");
	const XYCurve* curCurve = currentCurve();
	if (curCurve) {
		beginMacro( i18n("%1: fit to '%2'", name(), curCurve->name()) );
		curve->setName( i18n("Fit to '%1'", curCurve->name()) );
		curve->setDataSourceType(XYCurve::DataSourceCurve);
		curve->setDataSourceCurve(curCurve);

		//set the fit model category and type
		const QAction* action = qobject_cast<const QAction*>(QObject::sender());
		curve->initFitData(action, addFitAction);

		this->addChild(curve);
		curve->recalculate();
		emit curve->fitDataChanged(curve->fitData());
	} else {
		beginMacro(i18n("%1: add fit curve", name()));
		this->addChild(curve);
	}

	this->applyThemeOnNewCurve(curve);
	endMacro();

	return curve;
}

XYFourierFilterCurve* CartesianPlot::addFourierFilterCurve() {
	XYFourierFilterCurve* curve = new XYFourierFilterCurve("Fourier filter");
	const XYCurve* curCurve = currentCurve();
	if (curCurve) {
		beginMacro( i18n("%1: Fourier filtering of '%2'", name(), curCurve->name()) );
		curve->setName( i18n("Fourier filtering of '%1'", curCurve->name()) );
		curve->setDataSourceType(XYCurve::DataSourceCurve);
		curve->setDataSourceCurve(curCurve);
		this->addChild(curve);
	} else {
		beginMacro(i18n("%1: add Fourier filter curve", name()));
		this->addChild(curve);
	}

	this->applyThemeOnNewCurve(curve);
	endMacro();

	return curve;
}

XYFourierTransformCurve* CartesianPlot::addFourierTransformCurve() {
	XYFourierTransformCurve* curve = new XYFourierTransformCurve("Fourier transform");
	this->addChild(curve);
	this->applyThemeOnNewCurve(curve);
	return curve;
}

void CartesianPlot::addLegend() {
	//don't do anything if there's already a legend
	if (m_legend)
		return;

	m_legend = new CartesianPlotLegend(this, "legend");
	this->addChild(m_legend);
	m_legend->retransform();

	//only one legend is allowed -> disable the action
	addLegendAction->setEnabled(false);
}

void CartesianPlot::addCustomPoint() {
	CustomPoint* point = new CustomPoint(this, "custom point");
	this->addChild(point);
}

void CartesianPlot::childAdded(const AbstractAspect* child) {
	Q_D(CartesianPlot);
	const XYCurve* curve = qobject_cast<const XYCurve*>(child);
	if (curve) {
		connect(curve, SIGNAL(dataChanged()), this, SLOT(dataChanged()));
		connect(curve, SIGNAL(xDataChanged()), this, SLOT(xDataChanged()));
		connect(curve, SIGNAL(yDataChanged()), this, SLOT(yDataChanged()));
		connect(curve, SIGNAL(visibilityChanged(bool)), this, SLOT(curveVisibilityChanged()));

		//update the legend on changes of the name, line and symbol styles
		connect(curve, SIGNAL(aspectDescriptionChanged(const AbstractAspect*)), this, SLOT(updateLegend()));
		connect(curve, SIGNAL(lineTypeChanged(XYCurve::LineType)), this, SLOT(updateLegend()));
		connect(curve, SIGNAL(linePenChanged(QPen)), this, SLOT(updateLegend()));
		connect(curve, SIGNAL(lineOpacityChanged(qreal)), this, SLOT(updateLegend()));
		connect(curve, SIGNAL(symbolsStyleChanged(Symbol::Style)), this, SLOT(updateLegend()));
		connect(curve, SIGNAL(symbolsSizeChanged(qreal)), this, SLOT(updateLegend()));
		connect(curve, SIGNAL(symbolsRotationAngleChanged(qreal)), this, SLOT(updateLegend()));
		connect(curve, SIGNAL(symbolsOpacityChanged(qreal)), this, SLOT(updateLegend()));
		connect(curve, SIGNAL(symbolsBrushChanged(QBrush)), this, SLOT(updateLegend()));
		connect(curve, SIGNAL(symbolsPenChanged(QPen)), this, SLOT(updateLegend()));

		updateLegend();
		d->curvesXMinMaxIsDirty = true;
		d->curvesYMinMaxIsDirty = true;
	} else {
		const Histogram* histo = qobject_cast<const Histogram*>(child);
		if (histo) {
			connect(histo, SIGNAL(HistogramdataChanged()), this, SLOT(HistogramdataChanged()));
			connect(histo, SIGNAL(xHistogramDataChanged()), this, SLOT(xHistogramDataChanged()));
			connect(histo, SIGNAL(yHistogramDataChanged()), this, SLOT(yHistogramDataChanged()));
			connect(histo, SIGNAL(visibilityChanged(bool)), this, SLOT(curveVisibilityChanged()));
		}
	}

	//if a theme was selected, apply the theme settings for newly added children, too
	if (!d->theme.isEmpty() && !isLoading()) {
		const WorksheetElement* el = dynamic_cast<const WorksheetElement*>(child);
		if (el) {
			KConfig config(ThemeHandler::themeFilePath(d->theme), KConfig::SimpleConfig);
			const_cast<WorksheetElement*>(el)->loadThemeConfig(config);
		}
	}
}

void CartesianPlot::childRemoved(const AbstractAspect* parent, const AbstractAspect* before, const AbstractAspect* child) {
	Q_UNUSED(parent);
	Q_UNUSED(before);
	if (m_legend == child) {
		addLegendAction->setEnabled(true);
		m_legend = nullptr;
	} else {
		const XYCurve* curve = qobject_cast<const XYCurve*>(child);
		if (curve)
			updateLegend();
	}
}

void CartesianPlot::updateLegend() {
	if (m_legend)
		m_legend->retransform();
}

/*!
	called when in one of the curves the data was changed.
	Autoscales the coordinate system and the x-axes, when "auto-scale" is active.
*/
void CartesianPlot::dataChanged() {
	Q_D(CartesianPlot);
	XYCurve* curve = dynamic_cast<XYCurve*>(QObject::sender());
	Q_ASSERT(curve);
	d->curvesXMinMaxIsDirty = true;
	d->curvesYMinMaxIsDirty = true;
	if (d->autoScaleX && d->autoScaleY)
		this->scaleAuto();
	else if (d->autoScaleX)
		this->scaleAutoX();
	else if (d->autoScaleY)
		this->scaleAutoY();
	else
		curve->retransform();
}

void CartesianPlot::HistogramdataChanged(){
	Q_D(CartesianPlot);
	Histogram* curve = dynamic_cast<Histogram*>(QObject::sender());
	Q_ASSERT(curve);
	d->curvesXMinMaxIsDirty = true;
	d->curvesYMinMaxIsDirty = true;
    if (d->autoScaleX && d->autoScaleY)
        this->scaleAuto();
    else if (d->autoScaleX)
        this->scaleAutoY();
    else if (d->autoScaleY)
        this->scaleAutoY();
    else
		curve->retransform();
}
/*!
	called when in one of the curves the x-data was changed.
	Autoscales the coordinate system and the x-axes, when "auto-scale" is active.
*/
void CartesianPlot::xDataChanged() {
	if (project()->isLoading())
		return;

	Q_D(CartesianPlot);
	XYCurve* curve = dynamic_cast<XYCurve*>(QObject::sender());
	Q_ASSERT(curve);
    d->curvesXMinMaxIsDirty = true;

	if (d->autoScaleX)
		this->scaleAutoX();
	else
		curve->retransform();
}

void CartesianPlot::xHistogramDataChanged(){
	if (project()->isLoading())
		return;

	Q_D(CartesianPlot);
	Histogram* curve = dynamic_cast<Histogram*>(QObject::sender());
	Q_ASSERT(curve);
	d->curvesXMinMaxIsDirty = true;
	if (d->autoScaleX) {
		this->scaleAutoX();
	} else
		curve->retransform();
}
/*!
	called when in one of the curves the x-data was changed.
	Autoscales the coordinate system and the x-axes, when "auto-scale" is active.
*/
void CartesianPlot::yDataChanged() {
	if (project()->isLoading())
		return;

	Q_D(CartesianPlot);
	XYCurve* curve = dynamic_cast<XYCurve*>(QObject::sender());
	Q_ASSERT(curve);
	d->curvesYMinMaxIsDirty = true;
	if (d->autoScaleY)
		this->scaleAutoY();
	else
		curve->retransform();
}
void CartesianPlot::yHistogramDataChanged(){
	if (project()->isLoading())
		return;

	Q_D(CartesianPlot);
	Histogram* curve = dynamic_cast<Histogram*>(QObject::sender());
	Q_ASSERT(curve);
	d->curvesYMinMaxIsDirty = true;
	if (d->autoScaleY)
		this->scaleAutoY();
	else
		curve->retransform();
}

void CartesianPlot::curveVisibilityChanged() {
	Q_D(CartesianPlot);
	d->curvesXMinMaxIsDirty = true;
	d->curvesYMinMaxIsDirty = true;
	updateLegend();
	if (d->autoScaleX && d->autoScaleY)
		this->scaleAuto();
	else if (d->autoScaleX)
		this->scaleAutoX();
	else if (d->autoScaleY)
		this->scaleAutoY();
}

void CartesianPlot::setMouseMode(const MouseMode mouseMode) {
	Q_D(CartesianPlot);

	d->mouseMode = mouseMode;
	d->setHandlesChildEvents(mouseMode != CartesianPlot::SelectionMode);

	QList<QGraphicsItem*> items = d->childItems();
	if (d->mouseMode == CartesianPlot::SelectionMode) {
		for (auto* item: items)
			item->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
	} else {
		for (auto* item: items)
			item->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
	}

	//when doing zoom selection, prevent the graphics item from being movable
	//if it's currently movable (no worksheet layout available)
	const Worksheet* worksheet = dynamic_cast<const Worksheet*>(parentAspect());
	if (worksheet) {
		if (mouseMode == CartesianPlot::SelectionMode) {
			if (worksheet->layout() != Worksheet::NoLayout)
				graphicsItem()->setFlag(QGraphicsItem::ItemIsMovable, false);
			else
				graphicsItem()->setFlag(QGraphicsItem::ItemIsMovable, true);
		} else { //zoom m_selection
			graphicsItem()->setFlag(QGraphicsItem::ItemIsMovable, false);
		}
	}
}

void CartesianPlot::scaleAutoX() {
	Q_D(CartesianPlot);
	if (d->curvesXMinMaxIsDirty) {
		int count = 0;
		switch (d->rangeType) {
		case CartesianPlot::RangeFree:
			count = 0;
			break;
		case CartesianPlot::RangeLast:
			count = -d->rangeLastValues;
			break;
		case CartesianPlot::RangeFirst:
			count = d->rangeFirstValues;
			break;
		}

		d->curvesXMin = INFINITY;
		d->curvesXMax = -INFINITY;

		//loop over all xy-curves and determine the maximum and minimum x-values
		for (const auto* curve: this->children<const XYCurve>()) {
			if (!curve->isVisible())
				continue;
			if (!curve->xColumn())
				continue;

            const double min = curve->xColumn()->minimum(count);
            if (min < d->curvesXMin)
                d->curvesXMin = min;

            const double max = curve->xColumn()->maximum(count);
            if (max > d->curvesXMax)
                d->curvesXMax = max;
		}

		//loop over all histograms and determine the maximum and minimum x-values
		for (const auto* curve: this->children<const Histogram>()) {
			if (!curve->isVisible())
				continue;
			if (!curve->xColumn())
				continue;

            const double min = curve->xColumn()->minimum(count);
            if (min < d->curvesXMin)
                d->curvesXMin = min;

            const double max = curve->xColumn()->maximum(count);
            if (max > d->curvesXMax)
                d->curvesXMax = max;
		}

		d->curvesXMinMaxIsDirty = false;
	}

	bool update = false;
	if (d->curvesXMin != d->xMin && d->curvesXMin != INFINITY) {
		d->xMin = d->curvesXMin;
		update = true;
	}

	if (d->curvesXMax != d->xMax && d->curvesXMax != -INFINITY) {
		d->xMax = d->curvesXMax;
		update = true;
	}

	if (update) {
		if (d->xMax == d->xMin) {
			//in case min and max are equal (e.g. if we plot a single point), subtract/add 10% of the value
			if (d->xMax != 0) {
				d->xMax = d->xMax*1.1;
				d->xMin = d->xMin*0.9;
			} else {
				d->xMax = 0.1;
				d->xMin = -0.1;
			}
		} else {
			float offset = (d->xMax - d->xMin)*d->autoScaleOffsetFactor;
			d->xMin -= offset;
			d->xMax += offset;
		}
		d->retransformScales();
	}
}

void CartesianPlot::scaleAutoY() {
	Q_D(CartesianPlot);

	if (d->curvesYMinMaxIsDirty) {
		int count = 0;
		switch (d->rangeType) {
		case CartesianPlot::RangeFree:
			count = 0;
			break;
		case CartesianPlot::RangeLast:
			count = -d->rangeLastValues;
			break;
		case CartesianPlot::RangeFirst:
			count = d->rangeFirstValues;
			break;
		}

		d->curvesYMin = INFINITY;
		d->curvesYMax = -INFINITY;

		//loop over all xy-curves and determine the maximum and minimum y-values
		for (const auto* curve: this->children<const XYCurve>()) {
			if (!curve->isVisible())
				continue;
			if (!curve->yColumn())
				continue;

            const double min = curve->yColumn()->minimum(count);
            if (min < d->curvesYMin)
                d->curvesYMin = min;

            const double max = curve->yColumn()->maximum(count);
            if (max > d->curvesYMax)
                d->curvesYMax = max;
		}

		//loop over all histograms and determine the maximum y-value
		for (const auto* curve: this->children<const Histogram>()) {
			if (!curve->isVisible())
				continue;

			if (d->curvesYMin > 0.0)
				d->curvesYMin = 0.0;

			if ( curve->getYMaximum() > d->curvesYMax)
				d->curvesYMax = curve->getYMaximum();
		}

		d->curvesYMinMaxIsDirty = false;
	}


	bool update = false;
	if (d->curvesYMin != d->yMin && d->curvesYMin != INFINITY) {
		d->yMin = d->curvesYMin;
		update = true;
	}

	if (d->curvesYMax != d->yMax && d->curvesYMax != -INFINITY) {
		d->yMax = d->curvesYMax;
		update = true;
	}

	if (update) {
		if (d->yMax == d->yMin) {
			//in case min and max are equal (e.g. if we plot a single point), subtract/add 10% of the value
			if (d->yMax != 0) {
				d->yMax = d->yMax*1.1;
				d->yMin = d->yMin*0.9;
			} else {
				d->yMax = 0.1;
				d->yMin = -0.1;
			}
		} else {
			float offset = (d->yMax - d->yMin)*d->autoScaleOffsetFactor;
			d->yMin -= offset;
			d->yMax += offset;
		}
		d->retransformScales();
	}
}

void CartesianPlot::scaleAuto() {
	Q_D(CartesianPlot);

	int count = 0;
	switch (d->rangeType) {
	case CartesianPlot::RangeFree:
		count = 0;
		break;
	case CartesianPlot::RangeLast:
		count = -d->rangeLastValues;
		break;
	case CartesianPlot::RangeFirst:
		count = d->rangeFirstValues;
		break;
	}

	if (d->curvesXMinMaxIsDirty) {
		d->curvesXMin = INFINITY;
		d->curvesXMax = -INFINITY;

		//loop over all xy-curves and determine the maximum and minimum x-values
		for (const auto* curve: this->children<const XYCurve>()) {
			if (!curve->isVisible())
				continue;
			if (!curve->xColumn())
				continue;

			const double min = curve->xColumn()->minimum(count);
			if (min < d->curvesXMin)
				d->curvesXMin = min;

			double max = curve->xColumn()->maximum(count);
			if (max > d->curvesXMax)
				d->curvesXMax = max;
		}

		//loop over all histograms and determine the maximum and minimum x-values
		for (const auto* curve: this->children<const Histogram>()) {
			if (!curve->isVisible())
				continue;
			if (!curve->xColumn())
				continue;

            const double min = curve->xColumn()->minimum(count);
			if (min < d->curvesXMin)
				d->curvesXMin = min;

            const double max = curve->xColumn()->maximum(count);
			if (max > d->curvesXMax)
				d->curvesXMax = max;
		}

		d->curvesXMinMaxIsDirty = false;
	}

	if (d->curvesYMinMaxIsDirty) {
		d->curvesYMin = INFINITY;
		d->curvesYMax = -INFINITY;

        //loop over all xy-curves and determine the maximum and minimum y-values
		for (const auto* curve: this->children<const XYCurve>()) {
			if (!curve->isVisible())
				continue;
            if (!curve->yColumn())
				continue;

			const double min = curve->yColumn()->minimum(count);
			if (min < d->curvesYMin)
				d->curvesYMin = min;

			const double max = curve->yColumn()->maximum(count);
			if (max > d->curvesYMax)
				d->curvesYMax = max;
		}

		//loop over all histograms and determine the maximum y-value
		for (const auto* curve: this->children<const Histogram>()) {
			if (!curve->isVisible())
				continue;

			if (d->curvesYMin > 0.0)
				d->curvesYMin = 0.0;

			const double max = curve->getYMaximum();
			if (max > d->curvesYMax)
				d->curvesYMax = max;
		}
	}

	bool updateX = false;
	bool updateY = false;
	if (d->curvesXMin != d->xMin && d->curvesXMin != INFINITY) {
		d->xMin = d->curvesXMin;
		updateX = true;
	}

	if (d->curvesXMax != d->xMax && d->curvesXMax != -INFINITY) {
		d->xMax = d->curvesXMax;
		updateX = true;
	}

	if (d->curvesYMin != d->yMin && d->curvesYMin != INFINITY) {
		d->yMin = d->curvesYMin;
		updateY = true;
	}

	if (d->curvesYMax != d->yMax && d->curvesYMax != -INFINITY) {
		d->yMax = d->curvesYMax;
		updateY = true;
	}

	if (updateX || updateY) {
		if (updateX) {
			if (d->xMax == d->xMin) {
				//in case min and max are equal (e.g. if we plot a single point), subtract/add 10% of the value
				if (d->xMax != 0) {
					d->xMax = d->xMax*1.1;
					d->xMin = d->xMin*0.9;
				} else {
					d->xMax = 0.1;
					d->xMin = -0.1;
				}
			} else {
				float offset = (d->xMax - d->xMin)*d->autoScaleOffsetFactor;
				d->xMin -= offset;
				d->xMax += offset;
			}
		}
		if (updateY) {
			if (d->yMax == d->yMin) {
				//in case min and max are equal (e.g. if we plot a single point), subtract/add 10% of the value
				if (d->yMax != 0) {
					d->yMax = d->yMax*1.1;
					d->yMin = d->yMin*0.9;
				} else {
					d->yMax = 0.1;
					d->yMin = -0.1;
				}
			} else {
				float offset = (d->yMax - d->yMin)*d->autoScaleOffsetFactor;
				d->yMin -= offset;
				d->yMax += offset;
			}
		}
		d->retransformScales();
	}
}

void CartesianPlot::zoomIn() {
	DEBUG("CartesianPlot::zoomIn()");
	Q_D(CartesianPlot);

	float oldRange = (d->xMax - d->xMin);
	float newRange = (d->xMax - d->xMin) / m_zoomFactor;
	d->xMax = d->xMax + (newRange - oldRange) / 2;
	d->xMin = d->xMin - (newRange - oldRange) / 2;

	oldRange = (d->yMax - d->yMin);
	newRange = (d->yMax - d->yMin) / m_zoomFactor;
	d->yMax = d->yMax + (newRange - oldRange) / 2;
	d->yMin = d->yMin - (newRange - oldRange) / 2;

	d->retransformScales();
}

void CartesianPlot::zoomOut() {
	Q_D(CartesianPlot);
	float oldRange = (d->xMax-d->xMin);
	float newRange = (d->xMax-d->xMin)*m_zoomFactor;
	d->xMax = d->xMax + (newRange-oldRange)/2;
	d->xMin = d->xMin - (newRange-oldRange)/2;

	oldRange = (d->yMax-d->yMin);
	newRange = (d->yMax-d->yMin)*m_zoomFactor;
	d->yMax = d->yMax + (newRange-oldRange)/2;
	d->yMin = d->yMin - (newRange-oldRange)/2;

	d->retransformScales();
}

void CartesianPlot::zoomInX() {
	Q_D(CartesianPlot);
	float oldRange = (d->xMax-d->xMin);
	float newRange = (d->xMax-d->xMin)/m_zoomFactor;
	d->xMax = d->xMax + (newRange-oldRange)/2;
	d->xMin = d->xMin - (newRange-oldRange)/2;
	d->retransformScales();
}

void CartesianPlot::zoomOutX() {
	Q_D(CartesianPlot);
	float oldRange = (d->xMax-d->xMin);
	float newRange = (d->xMax-d->xMin)*m_zoomFactor;
	d->xMax = d->xMax + (newRange-oldRange)/2;
	d->xMin = d->xMin - (newRange-oldRange)/2;
	d->retransformScales();
}

void CartesianPlot::zoomInY() {
	Q_D(CartesianPlot);
	float oldRange = (d->yMax-d->yMin);
	float newRange = (d->yMax-d->yMin)/m_zoomFactor;
	d->yMax = d->yMax + (newRange-oldRange)/2;
	d->yMin = d->yMin - (newRange-oldRange)/2;
	d->retransformScales();
}

void CartesianPlot::zoomOutY() {
	Q_D(CartesianPlot);
	float oldRange = (d->yMax-d->yMin);
	float newRange = (d->yMax-d->yMin)*m_zoomFactor;
	d->yMax = d->yMax + (newRange-oldRange)/2;
	d->yMin = d->yMin - (newRange-oldRange)/2;
	d->retransformScales();
}

void CartesianPlot::shiftLeftX() {
	Q_D(CartesianPlot);
	float offsetX = (d->xMax-d->xMin)*0.1;
	d->xMax -= offsetX;
	d->xMin -= offsetX;
	d->retransformScales();
}

void CartesianPlot::shiftRightX() {
	Q_D(CartesianPlot);
	float offsetX = (d->xMax-d->xMin)*0.1;
	d->xMax += offsetX;
	d->xMin += offsetX;
	d->retransformScales();
}

void CartesianPlot::shiftUpY() {
	Q_D(CartesianPlot);
	float offsetY = (d->yMax-d->yMin)*0.1;
	d->yMax += offsetY;
	d->yMin += offsetY;
	d->retransformScales();
}

void CartesianPlot::shiftDownY() {
	Q_D(CartesianPlot);
	float offsetY = (d->yMax-d->yMin)*0.1;
	d->yMax -= offsetY;
	d->yMin -= offsetY;
	d->retransformScales();
}

//##############################################################################
//######  SLOTs for changes triggered via QActions in the context menu  ########
//##############################################################################
void CartesianPlot::visibilityChanged() {
	Q_D(CartesianPlot);
	this->setVisible(!d->isVisible());
}

//#####################################################################
//################### Private implementation ##########################
//#####################################################################
CartesianPlotPrivate::CartesianPlotPrivate(CartesianPlot* plot) : AbstractPlotPrivate(plot),
	curvesXMinMaxIsDirty(false), curvesYMinMaxIsDirty(false),
	curvesXMin(INFINITY), curvesXMax(-INFINITY), curvesYMin(INFINITY), curvesYMax(-INFINITY),
	q(plot),
	mouseMode(CartesianPlot::SelectionMode),
	cSystem(nullptr),
	m_suppressRetransform(false),
//	m_printing(false),
	m_selectionBandIsShown(false) {

	setData(0, WorksheetElement::NameCartesianPlot);
}

/*!
	updates the position of plot rectangular in scene coordinates to \c r and recalculates the scales.
	The size of the plot corresponds to the size of the plot area, the area which is filled with the background color etc.
	and which can pose the parent item for several sub-items (like TextLabel).
	Note, the size of the area used to define the coordinate system doesn't need to be equal to this plot area.
	Also, the size (=bounding box) of CartesianPlot can be greater than the size of the plot area.
 */
void CartesianPlotPrivate::retransform() {
	DEBUG("CartesianPlotPrivate::retransform()");
	if (m_suppressRetransform)
		return;

	prepareGeometryChange();
	setPos( rect.x()+rect.width()/2, rect.y()+rect.height()/2);

	retransformScales();

	//plotArea position is always (0, 0) in parent's coordinates, don't need to update here
	q->plotArea()->setRect(rect);

	//call retransform() for the title and the legend (if available)
	//when a predefined position relative to the (Left, Centered etc.) is used,
	//the actual position needs to be updated on plot's geometry changes.
	if (q->title())
		q->title()->retransform();
	if (q->m_legend)
		q->m_legend->retransform();

	WorksheetElementContainerPrivate::recalcShapeAndBoundingRect();
}

void CartesianPlotPrivate::retransformScales() {
	DEBUG("CartesianPlotPrivate::retransformScales()");

	CartesianPlot* plot = dynamic_cast<CartesianPlot*>(q);
	QList<CartesianScale*> scales;

	//perform the mapping from the scene coordinates to the plot's coordinates here.
	QRectF itemRect = mapRectFromScene(rect);

	//check ranges for log-scales
	if (xScale != CartesianPlot::ScaleLinear)
		checkXRange();

	//check whether we have x-range breaks - the first break, if available, should be valid
	bool hasValidBreak = (xRangeBreakingEnabled && !xRangeBreaks.list.isEmpty() && xRangeBreaks.list.first().isValid());

	static const int breakGap = 20;
	double sceneStart, sceneEnd, logicalStart, logicalEnd;

	//create x-scales
	int plotSceneStart = itemRect.x() + horizontalPadding;
	int plotSceneEnd = itemRect.x() + itemRect.width() - horizontalPadding;
	if (!hasValidBreak) {
		//no breaks available -> range goes from the plot beginning to the end of the plot
		sceneStart = plotSceneStart;
		sceneEnd = plotSceneEnd;
		logicalStart = xMin;
		logicalEnd = xMax;

		//TODO: how should we handle the case sceneStart == sceneEnd?
		//(to reproduce, create plots and adjust the spacing/pading to get zero size for the plots)
		if (sceneStart != sceneEnd)
			scales << this->createScale(xScale, sceneStart, sceneEnd, logicalStart, logicalEnd);
	} else {
		int sceneEndLast = plotSceneStart;
		int logicalEndLast = xMin;
		for (const auto& rb: xRangeBreaks.list) {
			if (!rb.isValid())
				break;

			//current range goes from the end of the previous one (or from the plot beginning) to curBreak.start
			sceneStart = sceneEndLast;
			if (&rb == &xRangeBreaks.list.first()) sceneStart += breakGap;
			sceneEnd = plotSceneStart + (plotSceneEnd-plotSceneStart) * rb.position;
			logicalStart = logicalEndLast;
			logicalEnd = rb.start;

			if (sceneStart != sceneEnd)
				scales << this->createScale(xScale, sceneStart, sceneEnd, logicalStart, logicalEnd);

			sceneEndLast = sceneEnd;
			logicalEndLast = rb.end;
		}

		//add the remaining range going from the last available range break to the end of the plot (=end of the x-data range)
		sceneStart = sceneEndLast+breakGap;
		sceneEnd = plotSceneEnd;
		logicalStart = logicalEndLast;
		logicalEnd = xMax;

		if (sceneStart != sceneEnd)
			scales << this->createScale(xScale, sceneStart, sceneEnd, logicalStart, logicalEnd);
	}

	cSystem->setXScales(scales);

	//check ranges for log-scales
	if (yScale != CartesianPlot::ScaleLinear)
		checkYRange();

	//check whether we have y-range breaks - the first break, if available, should be valid
	hasValidBreak = (yRangeBreakingEnabled && !yRangeBreaks.list.isEmpty() && yRangeBreaks.list.first().isValid());

	//create y-scales
	scales.clear();
	plotSceneStart = itemRect.y()+itemRect.height()-verticalPadding;
	plotSceneEnd = itemRect.y()+verticalPadding;
	if (!hasValidBreak) {
		//no breaks available -> range goes from the plot beginning to the end of the plot
		sceneStart = plotSceneStart;
		sceneEnd = plotSceneEnd;
		logicalStart = yMin;
		logicalEnd = yMax;

		if (sceneStart != sceneEnd)
			scales << this->createScale(yScale, sceneStart, sceneEnd, logicalStart, logicalEnd);
	} else {
		int sceneEndLast = plotSceneStart;
		int logicalEndLast = yMin;
		for (const auto& rb: yRangeBreaks.list) {
			if (!rb.isValid())
				break;

			//current range goes from the end of the previous one (or from the plot beginning) to curBreak.start
			sceneStart = sceneEndLast;
			if (&rb == &yRangeBreaks.list.first()) sceneStart -= breakGap;
			sceneEnd = plotSceneStart + (plotSceneEnd-plotSceneStart) * rb.position;
			logicalStart = logicalEndLast;
			logicalEnd = rb.start;

			if (sceneStart != sceneEnd)
				scales << this->createScale(yScale, sceneStart, sceneEnd, logicalStart, logicalEnd);

			sceneEndLast = sceneEnd;
			logicalEndLast = rb.end;
		}

		//add the remaining range going from the last available range break to the end of the plot (=end of the y-data range)
		sceneStart = sceneEndLast-breakGap;
		sceneEnd = plotSceneEnd;
		logicalStart = logicalEndLast;
		logicalEnd = yMax;

		if (sceneStart != sceneEnd)
			scales << this->createScale(yScale, sceneStart, sceneEnd, logicalStart, logicalEnd);
	}

	cSystem->setYScales(scales);

	//calculate the changes in x and y and save the current values for xMin, xMax, yMin, yMax
	float deltaXMin = 0;
	float deltaXMax = 0;
	float deltaYMin = 0;
	float deltaYMax = 0;

	if (xMin != xMinPrev) {
		deltaXMin = xMin - xMinPrev;
		emit plot->xMinChanged(xMin);
	}

	if (xMax != xMaxPrev) {
		deltaXMax = xMax - xMaxPrev;
		emit plot->xMaxChanged(xMax);
	}

	if (yMin != yMinPrev) {
		deltaYMin = yMin - yMinPrev;
		emit plot->yMinChanged(yMin);
	}

	if (yMax!=yMaxPrev) {
		deltaYMax = yMax - yMaxPrev;
		emit plot->yMaxChanged(yMax);
	}

	xMinPrev = xMin;
	xMaxPrev = xMax;
	yMinPrev = yMin;
	yMaxPrev = yMax;
	//adjust auto-scale axes
	for (auto* axis: q->children<Axis>()) {
		if (!axis->autoScale())
			continue;

		if (axis->orientation() == Axis::AxisHorizontal) {
			if (deltaXMax != 0) {
				axis->setUndoAware(false);
				axis->setEnd(xMax);
				axis->setUndoAware(true);
			}
			if (deltaXMin != 0) {
				axis->setUndoAware(false);
				axis->setStart(xMin);
				axis->setUndoAware(true);
			}
			//TODO;
// 			if (axis->position() == Axis::AxisCustom && deltaYMin != 0) {
// 				axis->setOffset(axis->offset() + deltaYMin, false);
// 			}
		} else {
			if (deltaYMax != 0) {
				axis->setUndoAware(false);
				axis->setEnd(yMax);
				axis->setUndoAware(true);
			}
			if (deltaYMin != 0) {
				axis->setUndoAware(false);
				axis->setStart(yMin);
				axis->setUndoAware(true);
			}

			//TODO;
// 			if (axis->position() == Axis::AxisCustom && deltaXMin != 0) {

// 				axis->setOffset(axis->offset() + deltaXMin, false);
// 			}
		}
	}
	// call retransform() on the parent to trigger the update of all axes and curves
	q->retransform();
}

void CartesianPlotPrivate::rangeChanged() {
	curvesXMinMaxIsDirty = true;
	curvesYMinMaxIsDirty = true;
	if (autoScaleX && autoScaleY)
		q->scaleAuto();
	else if (autoScaleX)
		q->scaleAutoX();
	else if (autoScaleY)
		q->scaleAutoY();
}

/*!
 * don't allow any negative values for the x range when log or sqrt scalings are used
 */
void CartesianPlotPrivate::checkXRange() {
	double min = 0.01;

	if (xMin <= 0.0) {
		(min < xMax*min) ? xMin = min : xMin = xMax*min;
		emit q->xMinChanged(xMin);
	} else if (xMax <= 0.0) {
		(-min > xMin*min) ? xMax = -min : xMax = xMin*min;
		emit q->xMaxChanged(xMax);
	}
}

/*!
 * don't allow any negative values for the y range when log or sqrt scalings are used
 */
void CartesianPlotPrivate::checkYRange() {
	double min = 0.01;

	if (yMin <= 0.0) {
		(min < yMax*min) ? yMin = min : yMin = yMax*min;
		emit q->yMinChanged(yMin);
	} else if (yMax <= 0.0) {
		(-min > yMin*min) ? yMax = -min : yMax = yMin*min;
		emit q->yMaxChanged(yMax);
	}
}

CartesianScale* CartesianPlotPrivate::createScale(CartesianPlot::Scale type, double sceneStart, double sceneEnd, double logicalStart, double logicalEnd) {
// 	Interval<double> interval (logicalStart-0.01, logicalEnd+0.01); //TODO: move this to CartesianScale
	Interval<double> interval (-1E15, 1E15);
// 	Interval<double> interval (logicalStart, logicalEnd);
	if (type == CartesianPlot::ScaleLinear) {
		return CartesianScale::createLinearScale(interval, sceneStart, sceneEnd, logicalStart, logicalEnd);
	} else {
		float base;
		if (type == CartesianPlot::ScaleLog10)
			base = 10.0;
		else if (type == CartesianPlot::ScaleLog2)
			base = 2.0;
		else
			base = M_E;

		return CartesianScale::createLogScale(interval, sceneStart, sceneEnd, logicalStart, logicalEnd, base);
	}
}

/*!
 * Reimplemented from QGraphicsItem.
 */
QVariant CartesianPlotPrivate::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (change == QGraphicsItem::ItemPositionChange) {
		const QPointF& itemPos = value.toPointF();//item's center point in parent's coordinates;
		float x = itemPos.x();
		float y = itemPos.y();

		//calculate the new rect and forward the changes to the frontend
		QRectF newRect;
		float w = rect.width();
		float h = rect.height();
		newRect.setX(x-w/2);
		newRect.setY(y-h/2);
		newRect.setWidth(w);
		newRect.setHeight(h);
		emit q->rectChanged(newRect);
	}
	return QGraphicsItem::itemChange(change, value);
}

void CartesianPlotPrivate::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	if (mouseMode == CartesianPlot::ZoomSelectionMode || mouseMode == CartesianPlot::ZoomXSelectionMode || mouseMode == CartesianPlot::ZoomYSelectionMode) {

		if (mouseMode == CartesianPlot::ZoomSelectionMode) {
			m_selectionStart = event->pos();
		} else if (mouseMode == CartesianPlot::ZoomXSelectionMode) {
			m_selectionStart.setX(event->pos().x());
			m_selectionStart.setY(q->plotRect().height()/2);
		} else if (mouseMode == CartesianPlot::ZoomYSelectionMode) {
			m_selectionStart.setX(-q->plotRect().width()/2);
			m_selectionStart.setY(event->pos().y());
		}

		m_selectionEnd = m_selectionStart;
		m_selectionBandIsShown = true;
	} else {
		QGraphicsItem::mousePressEvent(event);
	}
}

void CartesianPlotPrivate::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
	if (mouseMode == CartesianPlot::SelectionMode) {
		QGraphicsItem::mouseMoveEvent(event);
	} else if (mouseMode == CartesianPlot::ZoomSelectionMode || mouseMode == CartesianPlot::ZoomXSelectionMode || mouseMode == CartesianPlot::ZoomYSelectionMode) {
		QGraphicsItem::mouseMoveEvent(event);
		if ( !boundingRect().contains(event->pos()) ) {
			q->info("");
			return;
		}

		QString info;
		QPointF logicalStart = cSystem->mapSceneToLogical(m_selectionStart);
		if (mouseMode == CartesianPlot::ZoomSelectionMode) {
			m_selectionEnd = event->pos();
			QPointF logicalEnd = cSystem->mapSceneToLogical(m_selectionEnd);
			info = QString::fromUtf8("Δx=") + QString::number(logicalEnd.x()-logicalStart.x()) + QString::fromUtf8(", Δy=") + QString::number(logicalEnd.y()-logicalStart.y());
		} else if (mouseMode == CartesianPlot::ZoomXSelectionMode) {
			m_selectionEnd.setX(event->pos().x());
			m_selectionEnd.setY(-q->plotRect().height()/2);
			QPointF logicalEnd = cSystem->mapSceneToLogical(m_selectionEnd);
			info = QString::fromUtf8("Δx=") + QString::number(logicalEnd.x()-logicalStart.x());
		} else if (mouseMode == CartesianPlot::ZoomYSelectionMode) {
			m_selectionEnd.setX(q->plotRect().width()/2);
			m_selectionEnd.setY(event->pos().y());
			QPointF logicalEnd = cSystem->mapSceneToLogical(m_selectionEnd);
			info = QString::fromUtf8("Δy=") + QString::number(logicalEnd.y()-logicalStart.y());
		}
		q->info(info);
		update();
	}

	//TODO: implement the navigation in plot on mouse move events,
	//calculate the position changes and call shift*()-functions
}

void CartesianPlotPrivate::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	if (mouseMode == CartesianPlot::SelectionMode) {
		const QPointF& itemPos = pos();//item's center point in parent's coordinates;
		float x = itemPos.x();
		float y = itemPos.y();

		//calculate the new rect and set it
		QRectF newRect;
		float w = rect.width();
		float h = rect.height();
		newRect.setX(x-w/2);
		newRect.setY(y-h/2);
		newRect.setWidth(w);
		newRect.setHeight(h);

		m_suppressRetransform = true;
		q->setRect(newRect);
		m_suppressRetransform = false;

		QGraphicsItem::mouseReleaseEvent(event);
	} else if (mouseMode == CartesianPlot::ZoomSelectionMode || mouseMode == CartesianPlot::ZoomXSelectionMode || mouseMode == CartesianPlot::ZoomYSelectionMode) {
		//don't zoom if very small region was selected, avoid occasional/unwanted zooming
		if ( qAbs(m_selectionEnd.x()-m_selectionStart.x()) < 20 || qAbs(m_selectionEnd.y()-m_selectionStart.y()) < 20 ) {
			m_selectionBandIsShown = false;
			return;
		}

		//determine the new plot ranges
		QPointF logicalZoomStart = cSystem->mapSceneToLogical(m_selectionStart, AbstractCoordinateSystem::SuppressPageClipping);
		QPointF logicalZoomEnd = cSystem->mapSceneToLogical(m_selectionEnd, AbstractCoordinateSystem::SuppressPageClipping);
		if (m_selectionEnd.x() > m_selectionStart.x()) {
			xMin = logicalZoomStart.x();
			xMax = logicalZoomEnd.x();
		} else {
			xMin = logicalZoomEnd.x();
			xMax = logicalZoomStart.x();
		}

		if (m_selectionEnd.y() > m_selectionStart.y()) {
			yMin = logicalZoomEnd.y();
			yMax = logicalZoomStart.y();
		} else {
			yMin = logicalZoomStart.y();
			yMax = logicalZoomEnd.y();
		}

		m_selectionBandIsShown = false;
		retransformScales();
	}
}

void CartesianPlotPrivate::wheelEvent(QGraphicsSceneWheelEvent* event) {
	//determine first, which axes are selected and zoom only in the corresponding direction.
	//zoom the entire plot if no axes selected.
	bool zoomX = false;
	bool zoomY = false;
	for (auto* axis: q->children<Axis>()) {
		if (!axis->graphicsItem()->isSelected())
			continue;

		if (axis->orientation() == Axis::AxisHorizontal)
			zoomX  = true;
		else
			zoomY = true;
	}

	if (event->delta() > 0) {
		if (!zoomX && !zoomY) {
			//no special axis selected -> zoom in everything
			q->zoomIn();
		} else {
			if (zoomX) q->zoomInX();
			if (zoomY) q->zoomInY();
		}
	} else {
		if (!zoomX && !zoomY) {
			//no special axis selected -> zoom in everything
			q->zoomOut();
		} else {
			if (zoomX) q->zoomOutX();
			if (zoomY) q->zoomOutY();
		}
	}
}

void CartesianPlotPrivate::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
	QPointF point = event->pos();
	QString info;
	if (q->plotRect().contains(point)) {
		QPointF logicalPoint = cSystem->mapSceneToLogical(point);
		if (mouseMode == CartesianPlot::ZoomSelectionMode && !m_selectionBandIsShown) {
			info = "x=" + QString::number(logicalPoint.x()) + ", y=" + QString::number(logicalPoint.y());
		} else if (mouseMode == CartesianPlot::ZoomXSelectionMode && !m_selectionBandIsShown) {
			QPointF p1(logicalPoint.x(), yMin);
			QPointF p2(logicalPoint.x(), yMax);
			m_selectionStartLine.setP1(cSystem->mapLogicalToScene(p1));
			m_selectionStartLine.setP2(cSystem->mapLogicalToScene(p2));
			info = "x=" + QString::number(logicalPoint.x());
			update();
		} else if (mouseMode == CartesianPlot::ZoomYSelectionMode && !m_selectionBandIsShown) {
			QPointF p1(xMin, logicalPoint.y());
			QPointF p2(xMax, logicalPoint.y());
			m_selectionStartLine.setP1(cSystem->mapLogicalToScene(p1));
			m_selectionStartLine.setP2(cSystem->mapLogicalToScene(p2));
			info = "y=" + QString::number(logicalPoint.y());
			update();
		}
	}
	q->info(info);

	QGraphicsItem::hoverMoveEvent(event);
}

void CartesianPlotPrivate::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget) {
// 	DEBUG("CartesianPlotPrivate::paint()");

	if (!isVisible())
		return;

	painter->setPen(QPen(Qt::black, 3));
	if ((mouseMode == CartesianPlot::ZoomXSelectionMode || mouseMode == CartesianPlot::ZoomYSelectionMode)
			&& (!m_selectionBandIsShown)) {
		painter->drawLine(m_selectionStartLine);
	}

	if (m_selectionBandIsShown) {
		painter->save();
		painter->setPen(QPen(Qt::black, 5));
		painter->drawRect(QRectF(m_selectionStart, m_selectionEnd));
		painter->setBrush(Qt::blue);
		painter->setOpacity(0.2);
		painter->drawRect(QRectF(m_selectionStart, m_selectionEnd));
		painter->restore();
	}

	WorksheetElementContainerPrivate::paint(painter, option, widget);
// 	DEBUG("CartesianPlotPrivate::paint() DONE");
}

//##############################################################################
//##################  Serialization/Deserialization  ###########################
//##############################################################################

//! Save as XML
void CartesianPlot::save(QXmlStreamWriter* writer) const {
	Q_D(const CartesianPlot);

	writer->writeStartElement( "cartesianPlot" );
	writeBasicAttributes(writer);
	writeCommentElement(writer);

	//applied theme
	if (!d->theme.isEmpty()){
		writer->writeStartElement( "theme" );
		writer->writeAttribute("name", d->theme);
		writer->writeEndElement();
	}

	//geometry
	writer->writeStartElement( "geometry" );
	writer->writeAttribute( "x", QString::number(d->rect.x()) );
	writer->writeAttribute( "y", QString::number(d->rect.y()) );
	writer->writeAttribute( "width", QString::number(d->rect.width()) );
	writer->writeAttribute( "height", QString::number(d->rect.height()) );
	writer->writeAttribute( "visible", QString::number(d->isVisible()) );
	writer->writeEndElement();

	//coordinate system and padding
	writer->writeStartElement( "coordinateSystem" );
	writer->writeAttribute( "autoScaleX", QString::number(d->autoScaleX) );
	writer->writeAttribute( "autoScaleY", QString::number(d->autoScaleY) );
	writer->writeAttribute( "xMin", QString::number(d->xMin) );
	writer->writeAttribute( "xMax", QString::number(d->xMax) );
	writer->writeAttribute( "yMin", QString::number(d->yMin) );
	writer->writeAttribute( "yMax", QString::number(d->yMax) );
	writer->writeAttribute( "xScale", QString::number(d->xScale) );
	writer->writeAttribute( "yScale", QString::number(d->yScale) );
	writer->writeAttribute( "horizontalPadding", QString::number(d->horizontalPadding) );
	writer->writeAttribute( "verticalPadding", QString::number(d->verticalPadding) );
	writer->writeEndElement();

	//x-scale breaks
	if (d->xRangeBreakingEnabled || !d->xRangeBreaks.list.isEmpty()) {
		writer->writeStartElement("xRangeBreaks");
		writer->writeAttribute( "enabled", QString::number(d->xRangeBreakingEnabled) );
		for (const auto& rb: d->xRangeBreaks.list) {
			writer->writeStartElement("xRangeBreak");
			writer->writeAttribute("start", QString::number(rb.start));
			writer->writeAttribute("end", QString::number(rb.end));
			writer->writeAttribute("position", QString::number(rb.position));
			writer->writeAttribute("style", QString::number(rb.style));
			writer->writeEndElement();
		}
		writer->writeEndElement();
	}

	//y-scale breaks
	if (d->yRangeBreakingEnabled || !d->yRangeBreaks.list.isEmpty()) {
		writer->writeStartElement("yRangeBreaks");
		writer->writeAttribute( "enabled", QString::number(d->yRangeBreakingEnabled) );
		for (const auto& rb: d->yRangeBreaks.list) {
			writer->writeStartElement("yRangeBreak");
			writer->writeAttribute("start", QString::number(rb.start));
			writer->writeAttribute("end", QString::number(rb.end));
			writer->writeAttribute("position", QString::number(rb.position));
			writer->writeAttribute("style", QString::number(rb.style));
			writer->writeEndElement();
		}
		writer->writeEndElement();
	}

	//serialize all children (plot area, title text label, axes and curves)
	for (auto *elem: children<WorksheetElement>(IncludeHidden))
		elem->save(writer);

	writer->writeEndElement(); // close "cartesianPlot" section
}


//! Load from XML
bool CartesianPlot::load(XmlStreamReader* reader) {
	Q_D(CartesianPlot);

	if (!reader->isStartElement() || reader->name() != "cartesianPlot") {
		reader->raiseError(i18n("no cartesianPlot element found"));
		return false;
	}

	if (!readBasicAttributes(reader))
		return false;

	QString attributeWarning = i18n("Attribute '%1' missing or empty, default value is used");
	QXmlStreamAttributes attribs;
	QString str;
	QString tmpTheme;

	while (!reader->atEnd()) {
		reader->readNext();
		if (reader->isEndElement() && reader->name() == "cartesianPlot")
			break;

		if (!reader->isStartElement())
			continue;

		if (reader->name() == "comment") {
			if (!readCommentElement(reader))
				return false;
		} else if (reader->name() == "theme") {
			attribs = reader->attributes();
			tmpTheme = attribs.value("name").toString();
		} else if (reader->name() == "geometry") {
			attribs = reader->attributes();

			str = attribs.value("x").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'x'"));
			else
				d->rect.setX( str.toDouble() );

			str = attribs.value("y").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'y'"));
			else
				d->rect.setY( str.toDouble() );

			str = attribs.value("width").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'width'"));
			else
				d->rect.setWidth( str.toDouble() );

			str = attribs.value("height").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'height'"));
			else
				d->rect.setHeight( str.toDouble() );

			str = attribs.value("visible").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'visible'"));
			else
				d->setVisible(str.toInt());
		} else if (reader->name() == "coordinateSystem") {
			attribs = reader->attributes();

			str = attribs.value("autoScaleX").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'autoScaleX'"));
			else
				d->autoScaleX = bool(str.toInt());

			str = attribs.value("autoScaleY").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'autoScaleY'"));
			else
				d->autoScaleY = bool(str.toInt());

			str = attribs.value("xMin").toString();
			if (str.isEmpty()) {
				reader->raiseWarning(attributeWarning.arg("'xMin'"));
			} else {
				d->xMin = str.toDouble();
				d->xMinPrev = d->xMin;
			}

			str = attribs.value("xMax").toString();
			if (str.isEmpty()) {
				reader->raiseWarning(attributeWarning.arg("'xMax'"));
			} else {
				d->xMax = str.toDouble();
				d->xMaxPrev = d->xMax;
			}

			str = attribs.value("yMin").toString();
			if (str.isEmpty()) {
				reader->raiseWarning(attributeWarning.arg("'yMin'"));
			} else {
				d->yMin = str.toDouble();
				d->yMinPrev = d->yMin;
			}

			str = attribs.value("yMax").toString();
			if (str.isEmpty()) {
				reader->raiseWarning(attributeWarning.arg("'yMax'"));
			} else {
				d->yMax = str.toDouble();
				d->yMaxPrev = d->yMax;
			}

			str = attribs.value("xScale").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'xScale'"));
			else
				d->xScale = CartesianPlot::Scale(str.toInt());

			str = attribs.value("yScale").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'yScale'"));
			else
				d->yScale = CartesianPlot::Scale(str.toInt());

			str = attribs.value("horizontalPadding").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'horizontalPadding'"));
			else
				d->horizontalPadding = str.toDouble();

			str = attribs.value("verticalPadding").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'verticalPadding'"));
			else
				d->verticalPadding = str.toDouble();
		} else if (reader->name() == "xRangeBreaks") {
			//delete default rang break
			d->xRangeBreaks.list.clear();

			attribs = reader->attributes();
			str = attribs.value("enabled").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'enabled'"));
			else
				d->xRangeBreakingEnabled = str.toInt();
		} else if (reader->name() == "xRangeBreak") {
			attribs = reader->attributes();

			RangeBreak b;
			str = attribs.value("start").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'start'"));
			else
				b.start = str.toDouble();

			str = attribs.value("end").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'end'"));
			else
				b.end = str.toDouble();

			str = attribs.value("position").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'position'"));
			else
				b.position = str.toDouble();

			str = attribs.value("style").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'style'"));
			else
				b.style = CartesianPlot::RangeBreakStyle(str.toInt());

			d->xRangeBreaks.list << b;
		} else if (reader->name() == "yRangeBreaks") {
			//delete default rang break
			d->yRangeBreaks.list.clear();

			attribs = reader->attributes();
			str = attribs.value("enabled").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'enabled'"));
			else
				d->yRangeBreakingEnabled = str.toInt();
		} else if (reader->name() == "yRangeBreak") {
			attribs = reader->attributes();

			RangeBreak b;
			str = attribs.value("start").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'start'"));
			else
				b.start = str.toDouble();

			str = attribs.value("end").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'end'"));
			else
				b.end = str.toDouble();

			str = attribs.value("position").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'position'"));
			else
				b.position = str.toDouble();

			str = attribs.value("style").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'style'"));
			else
				b.style = CartesianPlot::RangeBreakStyle(str.toInt());

			d->yRangeBreaks.list << b;
		} else if (reader->name() == "textLabel") {
			m_title = new TextLabel("");
			if (!m_title->load(reader)) {
				delete m_title;
				m_title=0;
				return false;
			} else {
				addChild(m_title);
			}
		} else if (reader->name() == "plotArea") {
			m_plotArea->load(reader);
		} else if (reader->name() == "axis") {
			Axis* axis = new Axis("", this);
			if (!axis->load(reader)) {
				delete axis;
				return false;
			} else {
				addChild(axis);
			}
		} else if (reader->name() == "xyCurve") {
			XYCurve* curve = addCurve();
			if (!curve->load(reader)) {
				removeChild(curve);
				return false;
			}
		} else if (reader->name() == "xyEquationCurve") {
			XYEquationCurve* curve = addEquationCurve();
			if (!curve->load(reader)) {
				removeChild(curve);
				return false;
			}
		} else if (reader->name() == "xyDataReductionCurve") {
			XYDataReductionCurve* curve = addDataReductionCurve();
			if (!curve->load(reader)) {
				removeChild(curve);
				return false;
			}
		} else if (reader->name() == "xyDifferentiationCurve") {
			XYDifferentiationCurve* curve = addDifferentiationCurve();
			if (!curve->load(reader)) {
				removeChild(curve);
				return false;
			}
		} else if (reader->name() == "xyIntegrationCurve") {
			XYIntegrationCurve* curve = addIntegrationCurve();
			if (!curve->load(reader)) {
				removeChild(curve);
				return false;
			}
		} else if (reader->name() == "xyInterpolationCurve") {
			XYInterpolationCurve* curve = addInterpolationCurve();
			if (!curve->load(reader)) {
				removeChild(curve);
				return false;
			}
		} else if (reader->name() == "xyFitCurve") {
			XYFitCurve* curve = addFitCurve();
			if (!curve->load(reader)) {
				removeChild(curve);
				return false;
			}
		} else if (reader->name() == "xyFourierFilterCurve") {
			XYFourierFilterCurve* curve = addFourierFilterCurve();
			if (!curve->load(reader)) {
				removeChild(curve);
				return false;
			}
		} else if (reader->name() == "xyFourierTransformCurve") {
			XYFourierTransformCurve* curve = addFourierTransformCurve();
			if (!curve->load(reader)) {
				removeChild(curve);
				return false;
			}
		} else if (reader->name() == "xySmoothCurve") {
			XYSmoothCurve* curve = addSmoothCurve();
			if (!curve->load(reader)) {
				removeChild(curve);
				return false;
			}
		} else if (reader->name() == "cartesianPlotLegend") {
			m_legend = new CartesianPlotLegend(this, "");
			if (!m_legend->load(reader)) {
				delete m_legend;
				return false;
			} else {
				addChild(m_legend);
				addLegendAction->setEnabled(false);	//only one legend is allowed -> disable the action
			}
		} else if (reader->name() == "customPoint") {
			CustomPoint* point = new CustomPoint(this, "");
			if (!point->load(reader)) {
				delete point;
				return false;
			} else {
				addChild(point);
			}
		}else if(reader->name() == "Histogram"){
			Histogram* curve = addHistogram();
			if (!curve->load(reader)){
				removeChild(curve);
				return false;
			}
		} else { // unknown element
			reader->raiseWarning(i18n("unknown cartesianPlot element '%1'", reader->name().toString()));
			if (!reader->skipToEndElement()) return false;
		}
	}

	d->retransform();//TODO: This is expensive. why do we need this on load?
	if (m_title) {
		m_title->setHidden(true);
		m_title->graphicsItem()->setParentItem(m_plotArea->graphicsItem());
	}

	//if a theme was used, assign the value to the private member at the very end of load()
	//so we don't try to load the theme in applyThemeOnNewCurve() when adding curves on project load and calculate the palette
	if (!tmpTheme.isEmpty()){
		KConfig config( ThemeHandler::themeFilePath(tmpTheme), KConfig::SimpleConfig );
		//TODO: check whether the theme config really exists
		d->theme = tmpTheme;
		this->setColorPalette(config);
	}

	return true;
}

//##############################################################################
//#########################  Theme management ##################################
//##############################################################################
void CartesianPlot::loadTheme(const QString& theme) {
	KConfig config(ThemeHandler::themeFilePath(theme), KConfig::SimpleConfig);
	loadThemeConfig(config);
}

void CartesianPlot::loadThemeConfig(const KConfig& config) {
	QString str = config.name();
	str = str.right(str.length() - str.lastIndexOf(QDir::separator()) - 1);
	beginMacro( i18n("%1: Load theme %2.", AbstractAspect::name(), str) );
	this->setTheme(str);

	//load the color palettes for the curves
	this->setColorPalette(config);

	//load the theme for all the children
	for (auto* child: children<WorksheetElement>(AbstractAspect::IncludeHidden))
		child->loadThemeConfig(config);

	Q_D(CartesianPlot);
	d->update(this->rect());

	endMacro();
}

void CartesianPlot::saveTheme(KConfig &config) {
	const QList<Axis*>& axisElements = children<Axis>(AbstractAspect::IncludeHidden);
	const QList<PlotArea*>& plotAreaElements = children<PlotArea>(AbstractAspect::IncludeHidden);
	const QList<TextLabel*>& textLabelElements = children<TextLabel>(AbstractAspect::IncludeHidden);

	axisElements.at(0)->saveThemeConfig(config);
	plotAreaElements.at(0)->saveThemeConfig(config);
	textLabelElements.at(0)->saveThemeConfig(config);

	for (auto *child: children<XYCurve>(AbstractAspect::IncludeHidden))
		child->saveThemeConfig(config);
}

//Generating colors from 5-color theme palette
void CartesianPlot::setColorPalette(const KConfig& config) {
	KConfigGroup group = config.group("Theme");

	//read the five colors defining the palette
	m_themeColorPalette.clear();
	m_themeColorPalette.append(group.readEntry("ThemePaletteColor1", QColor()));
	m_themeColorPalette.append(group.readEntry("ThemePaletteColor2", QColor()));
	m_themeColorPalette.append(group.readEntry("ThemePaletteColor3", QColor()));
	m_themeColorPalette.append(group.readEntry("ThemePaletteColor4", QColor()));
	m_themeColorPalette.append(group.readEntry("ThemePaletteColor5", QColor()));

	//generate 30 additional shades if the color palette contains more than one color
	if (m_themeColorPalette.at(0) != m_themeColorPalette.at(1)) {
		QColor c;

		//3 factors to create shades from theme's palette
		float fac[3] = {0.25,0.45,0.65};

		//Generate 15 lighter shades
		for (int i = 0; i < 5; i++) {
			for (int j = 1; j < 4; j++) {
				c.setRed( m_themeColorPalette.at(i).red()*(1-fac[j-1]) );
				c.setGreen( m_themeColorPalette.at(i).green()*(1-fac[j-1]) );
				c.setBlue( m_themeColorPalette.at(i).blue()*(1-fac[j-1]) );
				m_themeColorPalette.append(c);
			}
		}

		//Generate 15 darker shades
		for (int i = 0; i < 5; i++) {
			for (int j = 4; j < 7; j++) {
				c.setRed( m_themeColorPalette.at(i).red()+((255-m_themeColorPalette.at(i).red())*fac[j-4]) );
				c.setGreen( m_themeColorPalette.at(i).green()+((255-m_themeColorPalette.at(i).green())*fac[j-4]) );
				c.setBlue( m_themeColorPalette.at(i).blue()+((255-m_themeColorPalette.at(i).blue())*fac[j-4]) );
				m_themeColorPalette.append(c);
			}
		}
	}
}

const QList<QColor>& CartesianPlot::themeColorPalette() const {
	return m_themeColorPalette;
}

void CartesianPlot::applyThemeOnNewCurve(XYCurve* curve) {
	Q_D(const CartesianPlot);
	if (!d->theme.isEmpty()) {
		KConfig config( ThemeHandler::themeFilePath(d->theme), KConfig::SimpleConfig );
		curve->loadThemeConfig(config);
	}
}
