/***************************************************************************
    File             : XYDataReductionCurveDock.cpp
    Project          : LabPlot
    --------------------------------------------------------------------
    Copyright        : (C) 2016 Stefan Gerlach (stefan.gerlach@uni.kn)
    Description      : widget for editing properties of data reduction curves

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

#include "XYDataReductionCurveDock.h"
#include "backend/core/AspectTreeModel.h"
#include "backend/core/Project.h"
#include "backend/worksheet/plots/cartesian/XYDataReductionCurve.h"
#include "commonfrontend/widgets/TreeViewComboBox.h"

#include <QMenu>
#include <QWidgetAction>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QProgressBar>
#ifndef NDEBUG
#include <QDebug>
#endif

#include <cmath>        // isnan

/*!
  \class XYDataReductionCurveDock
 \brief  Provides a widget for editing the properties of the XYDataReductionCurves
		(2D-curves defined by an data reduction) currently selected in
		the project explorer.

  If more then one curves are set, the properties of the first column are shown.
  The changes of the properties are applied to all curves.
  The exclusions are the name, the comment and the datasets (columns) of
  the curves  - these properties can only be changed if there is only one single curve.

  \ingroup kdefrontend
*/

XYDataReductionCurveDock::XYDataReductionCurveDock(QWidget *parent, QStatusBar *sb):
	XYCurveDock(parent), statusBar(sb), cbXDataColumn(0), cbYDataColumn(0), m_dataReductionCurve(0) {

	//hide the line connection type
	ui.cbLineType->setDisabled(true);

	//remove the tab "Error bars"
	ui.tabWidget->removeTab(5);
}

/*!
 * 	// Tab "General"
 */
void XYDataReductionCurveDock::setupGeneral() {
	QWidget* generalTab = new QWidget(ui.tabGeneral);
	uiGeneralTab.setupUi(generalTab);

	QGridLayout* gridLayout = dynamic_cast<QGridLayout*>(generalTab->layout());
	if (gridLayout) {
		gridLayout->setContentsMargins(2,2,2,2);
		gridLayout->setHorizontalSpacing(2);
		gridLayout->setVerticalSpacing(2);
	}

	cbXDataColumn = new TreeViewComboBox(generalTab);
	gridLayout->addWidget(cbXDataColumn, 4, 3, 1, 2);
	cbYDataColumn = new TreeViewComboBox(generalTab);
	gridLayout->addWidget(cbYDataColumn, 5, 3, 1, 2);

	for (int i=0; i < NSL_GEOM_LINESIM_TYPE_COUNT; i++)
		uiGeneralTab.cbType->addItem(i18n(nsl_geom_linesim_type_name[i]));
	uiGeneralTab.cbType->setItemData(nsl_geom_linesim_type_visvalingam_whyatt, i18n("This method is much slower than any other"), Qt::ToolTipRole);

	uiGeneralTab.pbRecalculate->setIcon(QIcon::fromTheme("run-build"));

	QHBoxLayout* layout = new QHBoxLayout(ui.tabGeneral);
	layout->setMargin(0);
	layout->addWidget(generalTab);

	//Slots
	connect( uiGeneralTab.leName, SIGNAL(returnPressed()), this, SLOT(nameChanged()) );
	connect( uiGeneralTab.leComment, SIGNAL(returnPressed()), this, SLOT(commentChanged()) );
	connect( uiGeneralTab.chkVisible, SIGNAL(clicked(bool)), this, SLOT(visibilityChanged(bool)) );
	connect( uiGeneralTab.cbAutoRange, SIGNAL(clicked(bool)), this, SLOT(autoRangeChanged()) );
	connect( uiGeneralTab.sbMin, SIGNAL(valueChanged(double)), this, SLOT(xRangeMinChanged()) );
	connect( uiGeneralTab.sbMax, SIGNAL(valueChanged(double)), this, SLOT(xRangeMaxChanged()) );

	connect( uiGeneralTab.cbType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged()) );
	connect( uiGeneralTab.chkAuto, SIGNAL(clicked(bool)), this, SLOT(autoToleranceChanged()) );
	connect( uiGeneralTab.sbTolerance, SIGNAL(valueChanged(double)), this, SLOT(toleranceChanged()) );
	connect( uiGeneralTab.chkAuto2, SIGNAL(clicked(bool)), this, SLOT(autoTolerance2Changed()) );
	connect( uiGeneralTab.sbTolerance2, SIGNAL(valueChanged(double)), this, SLOT(tolerance2Changed()) );

	connect( uiGeneralTab.pbRecalculate, SIGNAL(clicked()), this, SLOT(recalculateClicked()) );
}

void XYDataReductionCurveDock::initGeneralTab() {
	//if there are more then one curve in the list, disable the tab "general"
	if (m_curvesList.size()==1) {
		uiGeneralTab.lName->setEnabled(true);
		uiGeneralTab.leName->setEnabled(true);
		uiGeneralTab.lComment->setEnabled(true);
		uiGeneralTab.leComment->setEnabled(true);

		uiGeneralTab.leName->setText(m_curve->name());
		uiGeneralTab.leComment->setText(m_curve->comment());
	}else {
		uiGeneralTab.lName->setEnabled(false);
		uiGeneralTab.leName->setEnabled(false);
		uiGeneralTab.lComment->setEnabled(false);
		uiGeneralTab.leComment->setEnabled(false);

		uiGeneralTab.leName->setText("");
		uiGeneralTab.leComment->setText("");
	}

	//show the properties of the first curve
	m_dataReductionCurve = dynamic_cast<XYDataReductionCurve*>(m_curve);
	Q_ASSERT(m_dataReductionCurve);
	XYCurveDock::setModelIndexFromColumn(cbXDataColumn, m_dataReductionCurve->xDataColumn());
	XYCurveDock::setModelIndexFromColumn(cbYDataColumn, m_dataReductionCurve->yDataColumn());
	uiGeneralTab.cbAutoRange->setChecked(m_dataReductionData.autoRange);
	uiGeneralTab.sbMin->setValue(m_dataReductionData.xRange.front());
	uiGeneralTab.sbMax->setValue(m_dataReductionData.xRange.back());
	this->autoRangeChanged();
	// update list of selectable types
	xDataColumnChanged(cbXDataColumn->currentModelIndex());

	uiGeneralTab.cbType->setCurrentIndex(m_dataReductionData.type);
	this->typeChanged();
	uiGeneralTab.chkAuto->setChecked(m_dataReductionData.autoTolerance);
	this->autoToleranceChanged();
	uiGeneralTab.sbTolerance->setValue(m_dataReductionData.tolerance);
	this->toleranceChanged();
	uiGeneralTab.chkAuto2->setChecked(m_dataReductionData.autoTolerance2);
	this->autoTolerance2Changed();
	uiGeneralTab.sbTolerance2->setValue(m_dataReductionData.tolerance2);
	this->tolerance2Changed();

	this->showDataReductionResult();

	//enable the "recalculate"-button if the source data was changed since the last dataReduction
	uiGeneralTab.pbRecalculate->setEnabled(m_dataReductionCurve->isSourceDataChangedSinceLastDataReduction());

	uiGeneralTab.chkVisible->setChecked( m_curve->isVisible() );

	//Slots
	connect(m_dataReductionCurve, SIGNAL(aspectDescriptionChanged(const AbstractAspect*)), this, SLOT(curveDescriptionChanged(const AbstractAspect*)));
	connect(m_dataReductionCurve, SIGNAL(xDataColumnChanged(const AbstractColumn*)), this, SLOT(curveXDataColumnChanged(const AbstractColumn*)));
	connect(m_dataReductionCurve, SIGNAL(yDataColumnChanged(const AbstractColumn*)), this, SLOT(curveYDataColumnChanged(const AbstractColumn*)));
	connect(m_dataReductionCurve, SIGNAL(dataReductionDataChanged(XYDataReductionCurve::DataReductionData)), this, SLOT(curveDataReductionDataChanged(XYDataReductionCurve::DataReductionData)));
	connect(m_dataReductionCurve, SIGNAL(sourceDataChangedSinceLastDataReduction()), this, SLOT(enableRecalculate()));
}

void XYDataReductionCurveDock::setModel() {
	QList<const char*>  list;
	list<<"Folder"<<"Workbook"<<"Datapicker"<<"DatapickerCurve"<<"Spreadsheet"
		<<"FileDataSource"<<"Column"<<"Worksheet"<<"CartesianPlot"<<"XYFitCurve";
	cbXDataColumn->setTopLevelClasses(list);
	cbYDataColumn->setTopLevelClasses(list);

 	list.clear();
	list<<"Column";
	cbXDataColumn->setSelectableClasses(list);
	cbYDataColumn->setSelectableClasses(list);

	connect( cbXDataColumn, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(xDataColumnChanged(QModelIndex)) );
	connect( cbYDataColumn, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(yDataColumnChanged(QModelIndex)) );

	cbXDataColumn->setModel(m_aspectTreeModel);
	cbYDataColumn->setModel(m_aspectTreeModel);

	XYCurveDock::setModel();
}

/*!
  sets the curves. The properties of the curves in the list \c list can be edited in this widget.
*/
void XYDataReductionCurveDock::setCurves(QList<XYCurve*> list) {
	m_initializing=true;
	m_curvesList=list;
	m_curve=list.first();
	m_dataReductionCurve = dynamic_cast<XYDataReductionCurve*>(m_curve);
	Q_ASSERT(m_dataReductionCurve);
	m_aspectTreeModel = new AspectTreeModel(m_curve->project());
	this->setModel();
	m_dataReductionData = m_dataReductionCurve->dataReductionData();
	initGeneralTab();
	initTabs();
	m_initializing=false;

	//hide the "skip gaps" option after the curves were set
	ui.lLineSkipGaps->hide();
	ui.chkLineSkipGaps->hide();
}

//*************************************************************
//**** SLOTs for changes triggered in XYFitCurveDock *****
//*************************************************************
void XYDataReductionCurveDock::nameChanged() {
	if (m_initializing)
		return;

	m_curve->setName(uiGeneralTab.leName->text());
}

void XYDataReductionCurveDock::commentChanged() {
	if (m_initializing)
		return;

	m_curve->setComment(uiGeneralTab.leComment->text());
}

void XYDataReductionCurveDock::updateTolerance() {
	const AbstractColumn *xDataColumn = dynamic_cast<XYDataReductionCurve*>(m_curve)->xDataColumn();
	const AbstractColumn *yDataColumn = dynamic_cast<XYDataReductionCurve*>(m_curve)->yDataColumn();
	if(xDataColumn == 0 || yDataColumn == 0)
			return;

	//copy all valid data points for calculating tolerance to temporary vectors
	QVector<double> xdataVector;
	QVector<double> ydataVector;
	const double xmin = m_dataReductionData.xRange.front();
	const double xmax = m_dataReductionData.xRange.back();
	for (int row=0; row<xDataColumn->rowCount(); ++row) {
		//only copy those data where _all_ values (for x and y, if given) are valid
		if (!std::isnan(xDataColumn->valueAt(row)) && !std::isnan(yDataColumn->valueAt(row))
			&& !xDataColumn->isMasked(row) && !yDataColumn->isMasked(row)) {
				// only when inside given range
				if (xDataColumn->valueAt(row) >= xmin && xDataColumn->valueAt(row) <= xmax) {
					xdataVector.append(xDataColumn->valueAt(row));
					ydataVector.append(yDataColumn->valueAt(row));
				}
		}
	}

	if(xdataVector.size() > 1) {
		uiGeneralTab.cbType->setEnabled(true);
	} else {
		uiGeneralTab.cbType->setEnabled(false);
		return;
	}
#ifndef NDEBUG
	qDebug()<<"automatic tolerance:";
	qDebug()<<"clip_diag_perpoint ="<<nsl_geom_linesim_clip_diag_perpoint(xdataVector.data(), ydataVector.data(), xdataVector.size());
	qDebug()<<"clip_area_perpoint ="<<nsl_geom_linesim_clip_area_perpoint(xdataVector.data(), ydataVector.data(), xdataVector.size());
	qDebug()<<"avg_dist_perpoint ="<<nsl_geom_linesim_avg_dist_perpoint(xdataVector.data(), ydataVector.data(), xdataVector.size());
#endif

	nsl_geom_linesim_type type = (nsl_geom_linesim_type)uiGeneralTab.cbType->currentIndex();
	if (type == nsl_geom_linesim_type_raddist || type == nsl_geom_linesim_type_opheim)
		m_dataReductionData.tolerance = 10.*nsl_geom_linesim_clip_diag_perpoint(xdataVector.data(), ydataVector.data(), xdataVector.size());
	else if (type == nsl_geom_linesim_type_visvalingam_whyatt)
		m_dataReductionData.tolerance = 0.1*nsl_geom_linesim_clip_area_perpoint(xdataVector.data(), ydataVector.data(), xdataVector.size());
	else if (type == nsl_geom_linesim_type_douglas_peucker_variant)
		m_dataReductionData.tolerance = xdataVector.size()/10.;	// reduction to 10%
	else 
		m_dataReductionData.tolerance = 2.*nsl_geom_linesim_avg_dist_perpoint(xdataVector.data(), ydataVector.data(), xdataVector.size());
		//m_dataReductionData.tolerance = nsl_geom_linesim_clip_diag_perpoint(xdataVector.data(), ydataVector.data(), xdataVector.size());
	uiGeneralTab.sbTolerance->setValue(m_dataReductionData.tolerance);
}

void XYDataReductionCurveDock::updateTolerance2() {
	nsl_geom_linesim_type type = (nsl_geom_linesim_type)uiGeneralTab.cbType->currentIndex();

	if (type == nsl_geom_linesim_type_perpdist)
		uiGeneralTab.sbTolerance2->setValue(10);
	else if (type == nsl_geom_linesim_type_opheim)
		uiGeneralTab.sbTolerance2->setValue(5*uiGeneralTab.sbTolerance->value());
	else if (type == nsl_geom_linesim_type_lang)
		uiGeneralTab.sbTolerance2->setValue(10);
}

void XYDataReductionCurveDock::xDataColumnChanged(const QModelIndex& index) {
	if (m_initializing)
		return;

	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = 0;
	if (aspect) {
		column = dynamic_cast<AbstractColumn*>(aspect);
		Q_ASSERT(column);
	}

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYDataReductionCurve*>(curve)->setXDataColumn(column);

	if (column != 0 && uiGeneralTab.cbAutoRange->isChecked()) {
		uiGeneralTab.sbMin->setValue(column->minimum());
		uiGeneralTab.sbMax->setValue(column->maximum());
	}

	updateTolerance();
	updateTolerance2();
}

void XYDataReductionCurveDock::yDataColumnChanged(const QModelIndex& index) {
	if (m_initializing)
		return;

	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = 0;
	if (aspect) {
		column = dynamic_cast<AbstractColumn*>(aspect);
		Q_ASSERT(column);
	}

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYDataReductionCurve*>(curve)->setYDataColumn(column);

	updateTolerance();
	updateTolerance2();
}

void XYDataReductionCurveDock::autoRangeChanged() {
	bool autoRange = uiGeneralTab.cbAutoRange->isChecked();
	m_dataReductionData.autoRange = autoRange;

	if (autoRange) {
		uiGeneralTab.lMin->setEnabled(false);
		uiGeneralTab.sbMin->setEnabled(false);
		uiGeneralTab.lMax->setEnabled(false);
		uiGeneralTab.sbMax->setEnabled(false);
		m_dataReductionCurve = dynamic_cast<XYDataReductionCurve*>(m_curve);
		Q_ASSERT(m_dataReductionCurve);
		if (m_dataReductionCurve->xDataColumn()) {
			uiGeneralTab.sbMin->setValue(m_dataReductionCurve->xDataColumn()->minimum());
			uiGeneralTab.sbMax->setValue(m_dataReductionCurve->xDataColumn()->maximum());
		}
	} else {
		uiGeneralTab.lMin->setEnabled(true);
		uiGeneralTab.sbMin->setEnabled(true);
		uiGeneralTab.lMax->setEnabled(true);
		uiGeneralTab.sbMax->setEnabled(true);
	}

}
void XYDataReductionCurveDock::xRangeMinChanged() {
	double xMin = uiGeneralTab.sbMin->value();

	m_dataReductionData.xRange.front() = xMin;
	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYDataReductionCurveDock::xRangeMaxChanged() {
	double xMax = uiGeneralTab.sbMax->value();

	m_dataReductionData.xRange.back() = xMax;
	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYDataReductionCurveDock::typeChanged() {
	nsl_geom_linesim_type type = (nsl_geom_linesim_type)uiGeneralTab.cbType->currentIndex();
	m_dataReductionData.type = type;

	switch (type) {
	case nsl_geom_linesim_type_douglas_peucker:
	case nsl_geom_linesim_type_raddist:
	case nsl_geom_linesim_type_interp:
	case nsl_geom_linesim_type_reumann_witkam:
		uiGeneralTab.lOption->setText(i18n("Tolerance (distance)"));
		uiGeneralTab.sbTolerance->setDecimals(6);
		uiGeneralTab.sbTolerance->setMinimum(0);
		uiGeneralTab.sbTolerance->setSingleStep(0.01);
		uiGeneralTab.lOption2->hide();
		uiGeneralTab.chkAuto2->hide();
		uiGeneralTab.sbTolerance2->hide();
		if (uiGeneralTab.chkAuto->isChecked())
			updateTolerance();
		break;
	case nsl_geom_linesim_type_douglas_peucker_variant:
		uiGeneralTab.lOption->setText(i18n("Number of points"));
		uiGeneralTab.sbTolerance->setDecimals(0);
		uiGeneralTab.sbTolerance->setMinimum(2);
		uiGeneralTab.sbTolerance->setSingleStep(1);
		uiGeneralTab.lOption2->hide();
		uiGeneralTab.chkAuto2->hide();
		uiGeneralTab.sbTolerance2->hide();
		if (uiGeneralTab.chkAuto->isChecked())
			updateTolerance();
		break;
	case nsl_geom_linesim_type_nthpoint:
		uiGeneralTab.lOption->setText(i18n("Step size"));
		uiGeneralTab.sbTolerance->setValue(10);
		uiGeneralTab.sbTolerance->setDecimals(0);
		uiGeneralTab.sbTolerance->setMinimum(1);
		uiGeneralTab.sbTolerance->setSingleStep(1);
		uiGeneralTab.lOption2->hide();
		uiGeneralTab.chkAuto2->hide();
		uiGeneralTab.sbTolerance2->hide();
		break;
	case nsl_geom_linesim_type_perpdist:	// repeat option
		uiGeneralTab.lOption->setText(i18n("Tolerance (distance)"));
		uiGeneralTab.sbTolerance->setDecimals(6);
		uiGeneralTab.sbTolerance->setMinimum(0);
		uiGeneralTab.sbTolerance->setSingleStep(0.01);
		uiGeneralTab.sbTolerance2->show();
		uiGeneralTab.lOption2->show();
		uiGeneralTab.chkAuto2->show();
		uiGeneralTab.lOption2->setText(i18n("Repeats"));
		uiGeneralTab.sbTolerance2->setDecimals(0);
		uiGeneralTab.sbTolerance2->setMinimum(1);
		uiGeneralTab.sbTolerance2->setSingleStep(1);
		if (uiGeneralTab.chkAuto->isChecked())
			updateTolerance();
		if (uiGeneralTab.chkAuto2->isChecked())
			updateTolerance2();
		break;
	case nsl_geom_linesim_type_visvalingam_whyatt:
		uiGeneralTab.lOption->setText(i18n("Tolerance (area)"));
		uiGeneralTab.sbTolerance->setDecimals(6);
		uiGeneralTab.sbTolerance->setMinimum(0);
		uiGeneralTab.sbTolerance->setSingleStep(0.01);
		uiGeneralTab.lOption2->hide();
		uiGeneralTab.chkAuto2->hide();
		uiGeneralTab.sbTolerance2->hide();
		if (uiGeneralTab.chkAuto->isChecked())
			updateTolerance();
		break;
	case nsl_geom_linesim_type_opheim:	// min/max tol options
		uiGeneralTab.lOption->setText(i18n(" Min. Tolerance"));
		uiGeneralTab.sbTolerance->setDecimals(6);
		uiGeneralTab.sbTolerance->setMinimum(0);
		uiGeneralTab.sbTolerance->setSingleStep(0.01);
		uiGeneralTab.lOption2->setText(i18n("Max. Tolerance"));
		uiGeneralTab.lOption2->show();
		uiGeneralTab.chkAuto2->show();
		uiGeneralTab.sbTolerance2->show();
		uiGeneralTab.sbTolerance2->setDecimals(6);
		uiGeneralTab.sbTolerance2->setMinimum(0);
		uiGeneralTab.sbTolerance2->setSingleStep(0.01);
		if (uiGeneralTab.chkAuto->isChecked())
			updateTolerance();
		if (uiGeneralTab.chkAuto2->isChecked())
			updateTolerance2();
		break;
	case nsl_geom_linesim_type_lang:	// distance/region
		uiGeneralTab.lOption->setText(i18n("Tolerance (distance)"));
		uiGeneralTab.sbTolerance->setDecimals(6);
		uiGeneralTab.sbTolerance->setMinimum(0);
		uiGeneralTab.sbTolerance->setSingleStep(0.01);
		uiGeneralTab.lOption2->setText(i18n("Search region"));
		uiGeneralTab.lOption2->show();
		uiGeneralTab.chkAuto2->show();
		uiGeneralTab.sbTolerance2->show();
		uiGeneralTab.sbTolerance2->setDecimals(0);
		uiGeneralTab.sbTolerance2->setMinimum(1);
		uiGeneralTab.sbTolerance2->setSingleStep(1);
		if (uiGeneralTab.chkAuto->isChecked())
			updateTolerance();
		if (uiGeneralTab.chkAuto2->isChecked())
			updateTolerance2();
		break;
	}

	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYDataReductionCurveDock::autoToleranceChanged() {
	bool autoTolerance = (bool)uiGeneralTab.chkAuto->isChecked();
	m_dataReductionData.autoTolerance = autoTolerance;

	if (autoTolerance) {
		uiGeneralTab.sbTolerance->setEnabled(false);
		updateTolerance();
	} else {
		uiGeneralTab.sbTolerance->setEnabled(true);
	}
}

void XYDataReductionCurveDock::toleranceChanged() {
	m_dataReductionData.tolerance = uiGeneralTab.sbTolerance->value();

	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYDataReductionCurveDock::autoTolerance2Changed() {
	bool autoTolerance2 = (bool)uiGeneralTab.chkAuto2->isChecked();
	m_dataReductionData.autoTolerance2 = autoTolerance2;

	if (autoTolerance2) {
		uiGeneralTab.sbTolerance2->setEnabled(false);
		updateTolerance2();
	} else {
		uiGeneralTab.sbTolerance2->setEnabled(true);
	}
}

void XYDataReductionCurveDock::tolerance2Changed() {
	m_dataReductionData.tolerance2 = uiGeneralTab.sbTolerance2->value();

	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYDataReductionCurveDock::recalculateClicked() {
        //show a progress bar in the status bar
        QProgressBar* progressBar = new QProgressBar();
        progressBar->setMinimum(0);
        progressBar->setMaximum(100);
	connect(m_curve, SIGNAL(completed(int)), progressBar, SLOT(setValue(int)));
        statusBar->clearMessage();
        statusBar->addWidget(progressBar, 1);
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	foreach (XYCurve* curve, m_curvesList)
		dynamic_cast<XYDataReductionCurve*>(curve)->setDataReductionData(m_dataReductionData);

        QApplication::restoreOverrideCursor();
        statusBar->removeWidget(progressBar);

	uiGeneralTab.pbRecalculate->setEnabled(false);
}

void XYDataReductionCurveDock::enableRecalculate() const {
	if (m_initializing)
		return;

	//no dataReductioning possible without the x- and y-data
	AbstractAspect* aspectX = static_cast<AbstractAspect*>(cbXDataColumn->currentModelIndex().internalPointer());
	AbstractAspect* aspectY = static_cast<AbstractAspect*>(cbYDataColumn->currentModelIndex().internalPointer());
	bool data = (aspectX!=0 && aspectY!=0);

	uiGeneralTab.pbRecalculate->setEnabled(data);
}

/*!
 * show the result and details of the dataReduction
 */
void XYDataReductionCurveDock::showDataReductionResult() {
	const XYDataReductionCurve::DataReductionResult& dataReductionResult = m_dataReductionCurve->dataReductionResult();
	if (!dataReductionResult.available) {
		uiGeneralTab.teResult->clear();
		return;
	}

	//const XYDataReductionCurve::DataReductionData& dataReductionData = m_dataReductionCurve->dataReductionData();
	QString str = i18n("status:") + ' ' + dataReductionResult.status + "<br>";

	if (!dataReductionResult.valid) {
		uiGeneralTab.teResult->setText(str);
		return; //result is not valid, there was an error which is shown in the status-string, nothing to show more.
	}

	if (dataReductionResult.elapsedTime>1000)
		str += i18n("calculation time: %1 s").arg(QString::number(dataReductionResult.elapsedTime/1000)) + "<br>";
	else
		str += i18n("calculation time: %1 ms").arg(QString::number(dataReductionResult.elapsedTime)) + "<br>";

 	str += "<br>";

	str += i18n("number of points: %1").arg(QString::number(dataReductionResult.npoints)) + "<br>";
	str += i18n("positional squared error: %1").arg(QString::number(dataReductionResult.posError)) + "<br>";
	str += i18n("area error: %1").arg(QString::number(dataReductionResult.areaError)) + "<br>";

	uiGeneralTab.teResult->setText(str);
}

//*************************************************************
//*********** SLOTs for changes triggered in XYCurve **********
//*************************************************************
//General-Tab
void XYDataReductionCurveDock::curveDescriptionChanged(const AbstractAspect* aspect) {
	if (m_curve != aspect)
		return;

	m_initializing = true;
	if (aspect->name() != uiGeneralTab.leName->text()) {
		uiGeneralTab.leName->setText(aspect->name());
	} else if (aspect->comment() != uiGeneralTab.leComment->text()) {
		uiGeneralTab.leComment->setText(aspect->comment());
	}
	m_initializing = false;
}

void XYDataReductionCurveDock::curveXDataColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromColumn(cbXDataColumn, column);
	m_initializing = false;
}

void XYDataReductionCurveDock::curveYDataColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromColumn(cbYDataColumn, column);
	m_initializing = false;
}

void XYDataReductionCurveDock::curveDataReductionDataChanged(const XYDataReductionCurve::DataReductionData& data) {
	m_initializing = true;
	m_dataReductionData = data;
	//uiGeneralTab.cbType->setCurrentIndex(m_dataReductionData.type);
	//this->typeChanged();

	this->showDataReductionResult();
	m_initializing = false;
}

void XYDataReductionCurveDock::dataChanged() {
	this->enableRecalculate();
}
