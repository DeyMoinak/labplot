/***************************************************************************
    File                 : CartesianPlotPrivate.h
    Project              : LabPlot
    Description          : Private members of CartesianPlot.
    --------------------------------------------------------------------
    Copyright            : (C) 2014-2017 Alexander Semke (alexander.semke@web.de)
    Copyright            : (C) 2020 Stefan Gerlach (stefan.gerlach@uni.kn)

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

#ifndef CARTESIANPLOTPRIVATE_H
#define CARTESIANPLOTPRIVATE_H

#include "CartesianPlot.h"
#include "CartesianCoordinateSystem.h"
#include "backend/worksheet/Worksheet.h"
#include "../AbstractPlotPrivate.h"

#include <QGraphicsSceneMouseEvent>
#include <QStaticText>

class CartesianPlotPrivate : public AbstractPlotPrivate {
public:
	explicit CartesianPlotPrivate(CartesianPlot*);
	~CartesianPlotPrivate();

	void retransform() override;
	void retransformScales();
	void rangeChanged();
	void xRangeFormatChanged();
	void yRangeFormatChanged();
	void mouseMoveZoomSelectionMode(QPointF logicalPos, int cSystemIndex);
	void mouseMoveSelectionMode(QPointF logicalStart, QPointF logicalEnd);
	void mouseMoveCursorMode(int cursorNumber, QPointF logicalPos);
	bool mouseReleaseZoomSelectionMode(int cSystemIndex, bool suppressRetransform=false);
	void mouseHoverZoomSelectionMode(QPointF logicPos, int cSystemIndex);
	void mouseHoverOutsideDataRect();
	void mousePressZoomSelectionMode(QPointF logicalPos, int cSystemIndex);
	void mousePressCursorMode(int cursorNumber, QPointF logicalPos);
	void updateCursor();
	void setZoomSelectionBandShow(bool show);

	CartesianPlot::Type type{CartesianPlot::Type::FourAxes};
	QString theme;
	QRectF dataRect;
	CartesianPlot::RangeType rangeType{CartesianPlot::RangeType::Free};
	int rangeFirstValues{1000}, rangeLastValues{1000};

	const Range<double> xRange() const {
		return xRanges.at(defaultCoordinateSystem()->xIndex()).range;
	}
	Range<double>& xRange() {
		return xRanges[defaultCoordinateSystem()->xIndex()].range;
	}

	Range<double>& xRange(int cSystemIndex) {
		return xRanges[static_cast<CartesianCoordinateSystem*>(q->m_coordinateSystems[cSystemIndex])->xIndex()].range;
	}

	Range<double>& xRangeAutoScale(int cSystemIndex) {
		return xRanges[static_cast<CartesianCoordinateSystem*>(q->m_coordinateSystems[cSystemIndex])->xIndex()].autoScaleRange;
	}

	const Range<double> yRange() const {
		return yRanges.at(defaultCoordinateSystem()->yIndex()).range;
	}
	Range<double>& yRange() {
		return yRanges[defaultCoordinateSystem()->yIndex()].range;
	}

	Range<double>& yRange(int cSystemIndex) {
		return yRanges[static_cast<CartesianCoordinateSystem*>(q->m_coordinateSystems[cSystemIndex])->yIndex()].range;
	}

	Range<double>& yRangeAutoScale(int cSystemIndex) {
		return yRanges[static_cast<CartesianCoordinateSystem*>(q->m_coordinateSystems[cSystemIndex])->yIndex()].autoScaleRange;
	}
	Range<double> xPrevRange{}, yPrevRange{};

	bool autoScaleX(int index = -1) {
		if (index == -1) {
			for (int i=0; i < q->m_coordinateSystems.count(); i++)
				if (!autoScaleX(i))
					return false;
			return true;
		}
		return xRanges[static_cast<CartesianCoordinateSystem*>(q->m_coordinateSystems[index])->xIndex()].range.autoScale();
	}
	bool autoScaleY(int index = -1) {
		if (index == -1) {
			for (int i=0; i < q->m_coordinateSystems.count(); i++)
				if (!autoScaleY(i))
					return false;
			return true;
		}
		return yRanges[static_cast<CartesianCoordinateSystem*>(q->m_coordinateSystems[index])->yIndex()].range.autoScale();
	}
	void setAutoScaleX(int index = -1, bool b = true) {
		if (index == -1) {
			for (int i=0; i < q->m_coordinateSystems.count(); i++)
				setAutoScaleX(i, b);
			return;
		}
		xRanges[static_cast<CartesianCoordinateSystem*>(q->m_coordinateSystems[index])->xIndex()].range.setAutoScale(b);
	}
	void setAutoScaleY(int index = -1, bool b = true) {
		if (index == -1) {
			for (int i=0; i < q->m_coordinateSystems.count(); i++)
				setAutoScaleY(i, b);
			return;
		}
		yRanges[static_cast<CartesianCoordinateSystem*>(q->m_coordinateSystems[index])->yIndex()].range.setAutoScale(b);
	}

	//the following factor determines the size of the offset between the min/max points of the curves
	//and the coordinate system ranges, when doing auto scaling
	//Factor 0 corresponds to the exact match - min/max values of the curves correspond to the start/end values of the ranges.
	//TODO: make this factor optional.
	//Provide in the UI the possibility to choose between "exact" or 0% offset, 2%, 5% and 10% for the auto fit option
	double autoScaleOffsetFactor{0.0};
	//TODO: move to Range?
	bool xRangeBreakingEnabled{false}, yRangeBreakingEnabled{false};
	CartesianPlot::RangeBreaks xRangeBreaks, yRangeBreaks;

	//cached values of minimum and maximum for all visible curves
	//Range<double> curvesXRange{qInf(), -qInf()}, curvesYRange{qInf(), -qInf()};

	CartesianPlot* const q;
	int defaultCoordinateSystemIndex{0};

	struct RangeP {
		RangeP(const Range<double>& r=Range<double>(), const bool d=false): range(r), dirty(d) {}
		bool dirty{false};
		Range<double> range; // current range
		Range<double> autoScaleRange; // autoscale range. Cached to be faster in rescaling
	};

	QVector<RangeP> xRanges{{}}, yRanges{{}}; // at least one range must exist.

	CartesianCoordinateSystem* coordinateSystem(int index) const;
	QVector<AbstractCoordinateSystem*> coordinateSystems() const;
	CartesianCoordinateSystem* defaultCoordinateSystem() const {
		return static_cast<CartesianCoordinateSystem*>(q->m_coordinateSystems.at(defaultCoordinateSystemIndex));
	}

	CartesianPlot::MouseMode mouseMode{CartesianPlot::MouseMode::Selection};
	bool suppressRetransform{false};
	bool panningStarted{false};
	bool locked{false};

	// Cursor
	bool cursor0Enable{false};
	int selectedCursor{0};
	QPointF cursor0Pos{QPointF(qQNaN(), qQNaN())};
	bool cursor1Enable{false};
	QPointF cursor1Pos{QPointF(qQNaN(), qQNaN())};
	QPen cursorPen{Qt::red, Worksheet::convertToSceneUnits(1.0, Worksheet::Unit::Point), Qt::SolidLine};

	//other mouse cursor modes
	QPen zoomSelectPen{Qt::black, 3, Qt::SolidLine};
	QPen crossHairPen{Qt::black, 2, Qt::DotLine};

signals:
	void mousePressZoomSelectionModeSignal(QPointF logicalPos);
	void mousePressCursorModeSignal(QPointF logicalPos);

private:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
	void wheelEvent(QGraphicsSceneWheelEvent*) override;
	void keyPressEvent(QKeyEvent*) override;
	void hoverMoveEvent(QGraphicsSceneHoverEvent*) override;
	void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget* widget = nullptr) override;

	void updateDataRect();
	void checkXRange();
	void checkYRange();
	CartesianScale* createScale(RangeT::Scale,
		const Range<double> &sceneRange, const Range<double> &logicalRange);

	bool m_insideDataRect{false};
	bool m_selectionBandIsShown{false};
	QPointF m_selectionStart;
	QPointF m_selectionEnd;
	QLineF m_selectionStartLine;
	QPointF m_panningStart;
	QPointF m_crosshairPos; //current position of the mouse cursor in scene coordinates

	QStaticText m_cursor0Text{"1"};
	QStaticText m_cursor1Text{"2"};
};

#endif
