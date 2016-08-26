/***************************************************************************
    File             : XYFourierFilterCurveDock.h
    Project          : LabPlot
    --------------------------------------------------------------------
    Copyright        : (C) 2016 Stefan Gerlach (stefan.gerlach@uni.kn)
    Description      : widget for editing properties of Fourier filter curves

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

#ifndef XYFOURIERFILTERCURVEDOCK_H
#define XYFOURIERFILTERCURVEDOCK_H

#include "kdefrontend/dockwidgets/XYCurveDock.h"
#include "backend/worksheet/plots/cartesian/XYFourierFilterCurve.h"
#include "ui_xyfourierfiltercurvedockgeneraltab.h"

class TreeViewComboBox;

class XYFourierFilterCurveDock: public XYCurveDock {
	Q_OBJECT

public:
	explicit XYFourierFilterCurveDock(QWidget *parent);
	void setCurves(QList<XYCurve*>);
	virtual void setupGeneral();

private:
	virtual void initGeneralTab();
	void showFilterResult();

	Ui::XYFourierFilterCurveDockGeneralTab uiGeneralTab;
	TreeViewComboBox* cbXDataColumn;
	TreeViewComboBox* cbYDataColumn;

	XYFourierFilterCurve* m_filterCurve;
	XYFourierFilterCurve::FilterData m_filterData;

protected:
	virtual void setModel();

private slots:
	//SLOTs for changes triggered in XYFourierFilterCurveDock
	//general tab
	void nameChanged();
	void commentChanged();
	void xDataColumnChanged(const QModelIndex&);
	void yDataColumnChanged(const QModelIndex&);
	void typeChanged();
	void formChanged();
	void orderChanged();
	void unitChanged();
	void unit2Changed();

//	void showOptions();
	void recalculateClicked();

	void enableRecalculate() const;

	//SLOTs for changes triggered in XYCurve
	//General-Tab
	void curveDescriptionChanged(const AbstractAspect*);
	void curveXDataColumnChanged(const AbstractColumn*);
	void curveYDataColumnChanged(const AbstractColumn*);
	void curveFilterDataChanged(const XYFourierFilterCurve::FilterData&);
	void dataChanged();

};

#endif
