
/***************************************************************************
    File                 : CartesianPlot.h
    Project              : LabPlot
    Description          : Cartesian plot
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

#ifndef CARTESIANPLOT_H
#define CARTESIANPLOT_H

#include "backend/worksheet/plots/AbstractPlot.h"
#include <cmath>

class QToolBar;
class CartesianPlotPrivate;
class CartesianPlotLegend;
class XYCurve;
class XYEquationCurve;
class XYInterpolationCurve;
class XYSmoothCurve;
class XYFitCurve;
class XYFourierFilterCurve;
class XYFourierTransformCurve;

class CartesianPlot:public AbstractPlot{
	Q_OBJECT

	public:
		explicit CartesianPlot(const QString &name);
		virtual ~CartesianPlot();

		enum Scale {ScaleLinear, ScaleLog10, ScaleLog2, ScaleLn, ScaleSqrt, ScaleX2};
		enum Type {FourAxes, TwoAxes, TwoAxesCentered, TwoAxesCenteredZero};
		enum RangeBreakStyle {RangeBreakSimple, RangeBreakVertical, RangeBreakSloped};
		enum MouseMode {SelectionMode, ZoomSelectionMode, ZoomXSelectionMode, ZoomYSelectionMode};
		enum NavigationOperation {ScaleAuto, ScaleAutoX, ScaleAutoY, ZoomIn, ZoomOut, ZoomInX, ZoomOutX,
									ZoomInY, ZoomOutY, ShiftLeftX, ShiftRightX, ShiftUpY, ShiftDownY};

		struct RangeBreak {
			RangeBreak() : start(NAN), end(NAN), position(0.5), style(RangeBreakSloped) {};
			float start;
			float end;
			float position;
			RangeBreakStyle style;
		};

		//simple wrapper for QList<RangeBreaking> in order to get our macros working
		struct RangeBreaks {
			RangeBreaks() : lastChanged(-1) { RangeBreak b; list<<b;};
			QList<RangeBreak> list;
			int lastChanged;
		};

		void initDefault(Type=FourAxes);
		QIcon icon() const;
		QMenu* createContextMenu();
		void setRect(const QRectF&);
		QRectF plotRect();
		void setMouseMode(const MouseMode);
		MouseMode mouseMode() const;
		void navigate(NavigationOperation);

		virtual void save(QXmlStreamWriter*) const;
		virtual bool load(XmlStreamReader*);

		BASIC_D_ACCESSOR_DECL(bool, autoScaleX, AutoScaleX)
		BASIC_D_ACCESSOR_DECL(bool, autoScaleY, AutoScaleY)
		BASIC_D_ACCESSOR_DECL(float, xMin, XMin)
		BASIC_D_ACCESSOR_DECL(float, xMax, XMax)
		BASIC_D_ACCESSOR_DECL(float, yMin, YMin)
		BASIC_D_ACCESSOR_DECL(float, yMax, YMax)
		BASIC_D_ACCESSOR_DECL(CartesianPlot::Scale, xScale, XScale)
		BASIC_D_ACCESSOR_DECL(CartesianPlot::Scale, yScale, YScale)
		BASIC_D_ACCESSOR_DECL(bool, xRangeBreakingEnabled, XRangeBreakingEnabled)
		BASIC_D_ACCESSOR_DECL(bool, yRangeBreakingEnabled, YRangeBreakingEnabled)
		CLASS_D_ACCESSOR_DECL(RangeBreaks, xRangeBreaks, XRangeBreaks);
		CLASS_D_ACCESSOR_DECL(RangeBreaks, yRangeBreaks, YRangeBreaks);

		typedef CartesianPlot BaseClass;
		typedef CartesianPlotPrivate Private;

	private:
		void init();
		void initActions();
		void initMenus();

		CartesianPlotLegend* m_legend;
		float m_zoomFactor;

		QAction* visibilityAction;

		QAction* addCurveAction;
		QAction* addEquationCurveAction;
		QAction* addInterpolationCurveAction;
		QAction* addSmoothCurveAction;
		QAction* addFitCurveAction;
		QAction* addFourierFilterCurveAction;
		QAction* addFourierTransformCurveAction;
		QAction* addHorizontalAxisAction;
		QAction* addVerticalAxisAction;
 		QAction* addLegendAction;
		QAction* addCustomPointAction;

		QAction* scaleAutoXAction;
		QAction* scaleAutoYAction;
		QAction* scaleAutoAction;
		QAction* zoomInAction;
		QAction* zoomOutAction;
		QAction* zoomInXAction;
		QAction* zoomOutXAction;
		QAction* zoomInYAction;
		QAction* zoomOutYAction;
		QAction* shiftLeftXAction;
		QAction* shiftRightXAction;
		QAction* shiftUpYAction;
		QAction* shiftDownYAction;

		QMenu* addNewMenu;
		QMenu* zoomMenu;

		Q_DECLARE_PRIVATE(CartesianPlot)

	public slots:
		void addHorizontalAxis();
		void addVerticalAxis();
		XYCurve* addCurve();
		XYEquationCurve* addEquationCurve();
		XYInterpolationCurve* addInterpolationCurve();
		XYSmoothCurve* addSmoothCurve();
		XYFitCurve* addFitCurve();
		XYFourierFilterCurve* addFourierFilterCurve();
		XYFourierTransformCurve* addFourierTransformCurve();
		void addLegend();
		void addCustomPoint();
		void scaleAuto();
		void scaleAutoX();
		void scaleAutoY();
		void zoomIn();
		void zoomOut();
		void zoomInX();
		void zoomOutX();
		void zoomInY();
		void zoomOutY();
		void shiftLeftX();
		void shiftRightX();
		void shiftUpY();
		void shiftDownY();

	private slots:
		void updateLegend();
		void childAdded(const AbstractAspect*);
		void childRemoved(const AbstractAspect* parent, const AbstractAspect* before, const AbstractAspect* child);

		void dataChanged();
		void xDataChanged();
		void yDataChanged();
		void curveVisibilityChanged();

		//SLOTs for changes triggered via QActions in the context menu
		void visibilityChanged();

	protected:
		CartesianPlot(const QString &name, CartesianPlotPrivate *dd);

	signals:
		friend class CartesianPlotSetRectCmd;
		friend class CartesianPlotSetAutoScaleXCmd;
		friend class CartesianPlotSetXMinCmd;
		friend class CartesianPlotSetXMaxCmd;
		friend class CartesianPlotSetXScaleCmd;
		friend class CartesianPlotSetAutoScaleYCmd;
		friend class CartesianPlotSetYMinCmd;
		friend class CartesianPlotSetYMaxCmd;
		friend class CartesianPlotSetYScaleCmd;
		friend class CartesianPlotSetXRangeBreakingEnabledCmd;
		friend class CartesianPlotSetYRangeBreakingEnabledCmd;
		friend class CartesianPlotSetXRangeBreaksCmd;
		friend class CartesianPlotSetYRangeBreaksCmd;
		void rectChanged(QRectF&);
		void xAutoScaleChanged(bool);
		void xMinChanged(float);
		void xMaxChanged(float);
		void xScaleChanged(int);
		void yAutoScaleChanged(bool);
		void yMinChanged(float);
		void yMaxChanged(float);
		void yScaleChanged(int);
		void xRangeBreakingEnabledChanged(bool);
		void xRangeBreaksChanged(const CartesianPlot::RangeBreaks&);
		void yRangeBreakingEnabledChanged(bool);
		void yRangeBreaksChanged(const CartesianPlot::RangeBreaks&);
};

#endif
