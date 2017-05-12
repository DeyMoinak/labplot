/***************************************************************************
    File             : XYFitCurveDock.h
    Project          : LabPlot
    --------------------------------------------------------------------
    Copyright        : (C) 2014 Alexander Semke (alexander.semke@web.de)
    Description      : widget for editing properties of equation curves

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

#ifndef XYFITCURVEDOCK_H
#define XYFITCURVEDOCK_H

#include "kdefrontend/dockwidgets/XYCurveDock.h"
#include "backend/worksheet/plots/cartesian/XYFitCurve.h"
#include "ui_xyfitcurvedockgeneraltab.h"

class TreeViewComboBox;

class XYFitCurveDock: public XYCurveDock {
	Q_OBJECT

public:
	explicit XYFitCurveDock(QWidget *parent);
	void setCurves(QList<XYCurve*>);
	virtual void setupGeneral();

private:
	virtual void initGeneralTab();
	void showFitResultSummary(const XYFitCurve::FitResult& fitResult);
	void showFitResultLog(const XYFitCurve::FitResult& fitResult);
	void showFitResult();

	Ui::XYFitCurveDockGeneralTab uiGeneralTab;
	TreeViewComboBox* cbXDataColumn;
	TreeViewComboBox* cbYDataColumn;
	TreeViewComboBox* cbWeightsColumn;

	XYFitCurve* m_fitCurve;
	XYFitCurve::FitData m_fitData;
	QList<double> parameters;
	QList<double> parameterValues;

protected:
	virtual void setModel();

private slots:
	//SLOTs for changes triggered in XYFitCurveDock
	//general tab
	void nameChanged();
	void commentChanged();
	void categoryChanged(int);
	void modelChanged(int);
	void xDataColumnChanged(const QModelIndex&);
	void yDataColumnChanged(const QModelIndex&);
	void weightsColumnChanged(const QModelIndex&);
	void autoRangeChanged();
	void xRangeMinChanged();
	void xRangeMaxChanged();

	void showConstants();
	void showFunctions();
	void updateParameterList();
	void showParameters();
	void parametersChanged();
	void showOptions();
	void insertFunction(const QString&);
	void insertConstant(const QString&);
	void recalculateClicked();
	void updateModelEquation();
	void enableRecalculate() const;

	//SLOTs for changes triggered in XYCurve
	//General-Tab
	void curveDescriptionChanged(const AbstractAspect*);
	void curveXDataColumnChanged(const AbstractColumn*);
	void curveYDataColumnChanged(const AbstractColumn*);
	void curveWeightsColumnChanged(const AbstractColumn*);
	void curveFitDataChanged(const XYFitCurve::FitData&);
	void dataChanged();
};

#endif
