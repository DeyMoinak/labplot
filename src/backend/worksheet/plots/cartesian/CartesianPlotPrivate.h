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

	void retransform() override;
	void retransformScales();
	void rangeChanged();
	void xRangeFormatChanged();
	void yRangeFormatChanged();
	void mouseMoveZoomSelectionMode(QPointF logicalPos);
	void mouseMoveCursorMode(int cursorNumber, QPointF logicalPos);
	void mouseReleaseZoomSelectionMode();
	void mouseHoverZoomSelectionMode(QPointF logicPos);
	void mouseHoverOutsideDataRect();
	void mousePressZoomSelectionMode(QPointF logicalPos);
	void mousePressCursorMode(int cursorNumber, QPointF logicalPos);
	void updateCursor();
	void setZoomSelectionBandShow(bool show);

	CartesianPlot::Type type{CartesianPlot::Type::FourAxes};

	QRectF dataRect;
	CartesianPlot::RangeType rangeType{CartesianPlot::RangeType::Free};
	CartesianPlot::RangeFormat xRangeFormat{CartesianPlot::RangeFormat::Numeric};
	CartesianPlot::RangeFormat yRangeFormat{CartesianPlot::RangeFormat::Numeric};
	QString xRangeDateTimeFormat, yRangeDateTimeFormat;
	int rangeFirstValues{1000}, rangeLastValues{1000};
	Range<double> xRange{0.0, 1.0}, yRange{0.0, 1.0};
	//TODO: ranges
	double xMinPrev{0.0}, xMaxPrev{1.0}, yMinPrev{0.0}, yMaxPrev{1.0};
	bool autoScaleX{true}, autoScaleY{true};
	double autoScaleOffsetFactor{0.0f};
	CartesianPlot::Scale xScale{CartesianPlot::Scale::Linear}, yScale{CartesianPlot::Scale::Linear};
	bool xRangeBreakingEnabled{false}, yRangeBreakingEnabled{false};
	CartesianPlot::RangeBreaks xRangeBreaks, yRangeBreaks;
	QString theme;

	//cached values of minimum and maximum for all visible curves
	bool curvesXMinMaxIsDirty{false}, curvesYMinMaxIsDirty{false};
	//TODO: numeric limits
	//TODO: ranges
	double curvesXMin{INFINITY}, curvesXMax{-INFINITY}, curvesYMin{INFINITY}, curvesYMax{-INFINITY};

	CartesianPlot* const q;
	CartesianCoordinateSystem* coordinateSystem{nullptr};
	CartesianPlot::MouseMode mouseMode{CartesianPlot::MouseMode::Selection};
	bool suppressRetransform{false};
	bool panningStarted{false};
	bool locked{false};

	// Cursor
	bool cursor0Enable{false};
	int selectedCursor{0};
	// numeric limits
	QPointF cursor0Pos{QPointF(NAN, NAN)};
	bool cursor1Enable{false};
	QPointF cursor1Pos{QPointF(NAN, NAN)};
	QPen cursorPen{Qt::red, Worksheet::convertToSceneUnits(1.0, Worksheet::Unit::Point), Qt::SolidLine};

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
	CartesianScale* createScale(CartesianPlot::Scale type,
		double sceneStart, double sceneEnd,
		double logicalStart, double logicalEnd);

	bool m_insideDataRect{false};
	bool m_selectionBandIsShown{false};
	QPointF m_selectionStart;
	QPointF m_selectionEnd;
	QLineF m_selectionStartLine;
	QPointF m_panningStart;

	QStaticText m_cursor0Text{"1"};
	QStaticText m_cursor1Text{"2"};
};

#endif
