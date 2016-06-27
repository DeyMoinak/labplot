/***************************************************************************
    File                 : HistogramPrivate.h
    Project              : LabPlot
    Description          : Private members of Histogram
    --------------------------------------------------------------------
    Copyright            : (C) 2016 Anu Mittal (anu22mittal@gmail.com)
    
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

#ifndef HISTOGRAMPRIVATE_H
#define HISTOGRAMPRIVATE_H

#include <vector>

class HistogramPrivate: public QGraphicsItem {
  public:
		explicit HistogramPrivate(Histogram *owner);

		QString name() const;
		virtual QRectF boundingRect() const;
		QPainterPath shape() const;

		bool m_printing;
		bool m_hovered;
		bool m_suppressRecalc;
		bool m_suppressRetransform;
		QPixmap m_pixmap;
		QImage m_hoverEffectImage;
		QImage m_selectionEffectImage;
		bool m_hoverEffectImageIsDirty;
		bool m_selectionEffectImageIsDirty;

		void retransform();
		void updateValues();
		void updateFilling();
		bool swapVisible(bool on);
		void recalcShapeAndBoundingRect();
		void drawSymbols(QPainter*);
		void drawValues(QPainter*);
		void drawFilling(QPainter*);
		void draw(QPainter*);
		void updatePixmap();
		bool autoScaleX, autoScaleY;

		virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget* widget = 0);

		//data columns
		const AbstractColumn* xColumn;
		const AbstractColumn* yColumn;
		QString xColumnPath;
		QString yColumnPath;

		//values
		Histogram::ValuesType valuesType;
		const AbstractColumn* valuesColumn;
		QString valuesColumnPath;
		Histogram::ValuesPosition valuesPosition;
		qreal valuesDistance;
		qreal valuesRotationAngle;
		qreal valuesOpacity;
		QString valuesPrefix;
		QString valuesSuffix;
		QFont valuesFont;
		QColor valuesColor;

		//filling
		Histogram::FillingPosition fillingPosition;
		PlotArea::BackgroundType fillingType;
		PlotArea::BackgroundColorStyle fillingColorStyle;
		PlotArea::BackgroundImageStyle fillingImageStyle;
		Qt::BrushStyle fillingBrushStyle;
		QColor fillingFirstColor;
		QColor fillingSecondColor;
		QString fillingFileName;
		qreal fillingOpacity;

		QPainterPath valuesPath;
		QRectF boundingRectangle;
		QPainterPath curveShape;
		QList<QLineF> lines;
		QList<QPointF> symbolPointsLogical;	//points in logical coordinates
		QList<QPointF> symbolPointsScene;	//points in scene coordinates
		std::vector<bool> visiblePoints;	//vector of the size of symbolPointsLogical with true of false for the points currently visible or not in the plot
		QList<QPointF> valuesPoints;
		std::vector<bool> connectedPointsLogical;  //vector of the size of symbolPointsLogical with true for points connected with the consecutive point and
											       //false otherwise (don't connect because of a gap (NAN) in-between)
		QList<QString> valuesStrings;
		QList<QPolygonF> fillPolygons;
		
		//cached values of minimum and maximum for all visible curves
		bool curvesXMinMaxIsDirty, curvesYMinMaxIsDirty;
		double curvesXMin, curvesXMax, curvesYMin, curvesYMax;
		
		float xMin, xMax, yMin, yMax;
		float xMinPrev, xMaxPrev, yMinPrev, yMaxPrev;
		bool autoScaleHistogramX, autoScaleHistogramY;
		
		Histogram* const q;
		

	private:
        void contextMenuEvent(QGraphicsSceneContextMenuEvent*);
		virtual void hoverEnterEvent(QGraphicsSceneHoverEvent*);
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
		QImage blurred(const QImage& image, const QRect& rect, int radius, bool alphaOnly);
};

#endif
