/***************************************************************************
    File             : XYSmoothCurveDock.h
    Project          : LabPlot
    --------------------------------------------------------------------
    Copyright        : (C) 2016 Stefan Gerlach (stefan.gerlach@uni.kn)
    Description      : widget for editing properties of smooth curves

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

#ifndef XYSMOOTHCURVEDOCK_H
#define XYSMOOTHCURVEDOCK_H

#include "kdefrontend/dockwidgets/XYCurveDock.h"
#include "backend/worksheet/plots/cartesian/XYSmoothCurve.h"
#include "ui_xysmoothcurvedockgeneraltab.h"

class TreeViewComboBox;

class XYSmoothCurveDock: public XYCurveDock {
	Q_OBJECT

public:
	explicit XYSmoothCurveDock(QWidget *parent);
	void setCurves(QList<XYCurve*>);
	virtual void setupGeneral();

private:
	virtual void initGeneralTab();
	void showSmoothResult();

	Ui::XYSmoothCurveDockGeneralTab uiGeneralTab;
	TreeViewComboBox* cbXDataColumn;
	TreeViewComboBox* cbYDataColumn;

	XYSmoothCurve* m_smoothCurve;
	XYSmoothCurve::SmoothData m_smoothData;

protected:
	virtual void setModel();

private slots:
	//SLOTs for changes triggered in XYSmoothCurveDock
	//general tab
	void nameChanged();
	void commentChanged();
	void xDataColumnChanged(const QModelIndex&);
	void yDataColumnChanged(const QModelIndex&);
	void typeChanged();
	void pointsChanged();
	void weightChanged();
	void percentileChanged();
	void orderChanged();
	void modeChanged();
	void valueChanged();

//	void showOptions();
	void recalculateClicked();

	void enableRecalculate() const;

	//SLOTs for changes triggered in XYCurve
	//General-Tab
	void curveDescriptionChanged(const AbstractAspect*);
	void curveXDataColumnChanged(const AbstractColumn*);
	void curveYDataColumnChanged(const AbstractColumn*);
	void curveSmoothDataChanged(const XYSmoothCurve::SmoothData&);
	void dataChanged();

};

#endif
