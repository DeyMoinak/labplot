/***************************************************************************
    File             : XYInterpolationCurveDock.cpp
    Project          : LabPlot
    --------------------------------------------------------------------
    Copyright        : (C) 2016 Stefan Gerlach (stefan.gerlach@uni.kn)
    Description      : widget for editing properties of interpolation curves

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

#include "XYInterpolationCurveDock.h"
#include "backend/core/AspectTreeModel.h"
#include "backend/core/Project.h"
#include "backend/worksheet/plots/cartesian/XYInterpolationCurve.h"
#include "commonfrontend/widgets/TreeViewComboBox.h"

#include <QMenu>
#include <QWidgetAction>
#include <QStandardItemModel>

extern "C" {
#include <gsl/gsl_interp.h>	// gsl_interp types
}
#include <cmath>        // isnan

/*!
  \class XYInterpolationCurveDock
 \brief  Provides a widget for editing the properties of the XYInterpolationCurves
		(2D-curves defined by an interpolation) currently selected in
		the project explorer.

  If more then one curves are set, the properties of the first column are shown.
  The changes of the properties are applied to all curves.
  The exclusions are the name, the comment and the datasets (columns) of
  the curves  - these properties can only be changed if there is only one single curve.

  \ingroup kdefrontend
*/

XYInterpolationCurveDock::XYInterpolationCurveDock(QWidget *parent): 
	XYCurveDock(parent), cbXDataColumn(0), cbYDataColumn(0), m_interpolationCurve(0), dataPoints(0) {

	//hide the line connection type
	ui.cbLineType->setDisabled(true);

	//remove the tab "Error bars"
	ui.tabWidget->removeTab(5);
}

/*!
 * 	// Tab "General"
 */
void XYInterpolationCurveDock::setupGeneral() {
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

	for (int i=0; i < NSL_INTERP_TYPE_COUNT; i++)
		uiGeneralTab.cbType->addItem(i18n(nsl_interp_type_name[i]));
#if GSL_MAJOR_VERSION < 2
	// disable Steffen spline item
	const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(uiGeneralTab.cbType->model());
	QStandardItem* item = model->item(nsl_interp_type_steffen);
	item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
#endif
	for (int i=0; i < NSL_INTERP_PCH_VARIANT_COUNT; i++)
		uiGeneralTab.cbVariant->addItem(i18n(nsl_interp_pch_variant_name[i]));
	for (int i=0; i < NSL_INTERP_EVALUATE_COUNT; i++)
		uiGeneralTab.cbEval->addItem(i18n(nsl_interp_evaluate_name[i]));

	uiGeneralTab.cbPointsMode->addItem(i18n("Auto (5x data points)"));
	uiGeneralTab.cbPointsMode->addItem(i18n("Multiple of data points"));
	uiGeneralTab.cbPointsMode->addItem(i18n("Custom"));

	uiGeneralTab.pbRecalculate->setIcon(KIcon("run-build"));

	QHBoxLayout* layout = new QHBoxLayout(ui.tabGeneral);
	layout->setMargin(0);
	layout->addWidget(generalTab);

	//Slots
	connect( uiGeneralTab.leName, SIGNAL(returnPressed()), this, SLOT(nameChanged()) );
	connect( uiGeneralTab.leComment, SIGNAL(returnPressed()), this, SLOT(commentChanged()) );
	connect( uiGeneralTab.chkVisible, SIGNAL(clicked(bool)), this, SLOT(visibilityChanged(bool)) );

	connect( uiGeneralTab.cbType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged()) );
	connect( uiGeneralTab.cbVariant, SIGNAL(currentIndexChanged(int)), this, SLOT(variantChanged()) );
	connect( uiGeneralTab.sbTension, SIGNAL(valueChanged(double)), this, SLOT(tensionChanged()) );
	connect( uiGeneralTab.sbContinuity, SIGNAL(valueChanged(double)), this, SLOT(continuityChanged()) );
	connect( uiGeneralTab.sbBias, SIGNAL(valueChanged(double)), this, SLOT(biasChanged()) );
	connect( uiGeneralTab.cbEval, SIGNAL(currentIndexChanged(int)), this, SLOT(evaluateChanged()) );
	connect( uiGeneralTab.sbPoints, SIGNAL(valueChanged(double)), this, SLOT(numberOfPointsChanged()) );
	connect( uiGeneralTab.cbPointsMode, SIGNAL(currentIndexChanged(int)), this, SLOT(pointsModeChanged()) );

	connect( uiGeneralTab.pbRecalculate, SIGNAL(clicked()), this, SLOT(recalculateClicked()) );
}

void XYInterpolationCurveDock::initGeneralTab() {
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
	m_interpolationCurve = dynamic_cast<XYInterpolationCurve*>(m_curve);
	Q_ASSERT(m_interpolationCurve);
	XYCurveDock::setModelIndexFromColumn(cbXDataColumn, m_interpolationCurve->xDataColumn());
	XYCurveDock::setModelIndexFromColumn(cbYDataColumn, m_interpolationCurve->yDataColumn());
	// update list of selectable types
	xDataColumnChanged(cbXDataColumn->currentModelIndex());

	uiGeneralTab.cbType->setCurrentIndex(m_interpolationData.type);
	this->typeChanged();
	uiGeneralTab.cbVariant->setCurrentIndex(m_interpolationData.variant);
	this->variantChanged();
	uiGeneralTab.sbTension->setValue(m_interpolationData.tension);
	uiGeneralTab.sbContinuity->setValue(m_interpolationData.continuity);
	uiGeneralTab.sbBias->setValue(m_interpolationData.bias);
	uiGeneralTab.cbEval->setCurrentIndex(m_interpolationData.evaluate);

	if (m_interpolationData.pointsMode == XYInterpolationCurve::Multiple)
		uiGeneralTab.sbPoints->setValue(m_interpolationData.npoints/5.);
	else
		uiGeneralTab.sbPoints->setValue(m_interpolationData.npoints);
	uiGeneralTab.cbPointsMode->setCurrentIndex(m_interpolationData.pointsMode);

	this->showInterpolationResult();

	//enable the "recalculate"-button if the source data was changed since the last interpolation
	uiGeneralTab.pbRecalculate->setEnabled(m_interpolationCurve->isSourceDataChangedSinceLastInterpolation());

	uiGeneralTab.chkVisible->setChecked( m_curve->isVisible() );

	//Slots
	connect(m_interpolationCurve, SIGNAL(aspectDescriptionChanged(const AbstractAspect*)), this, SLOT(curveDescriptionChanged(const AbstractAspect*)));
	connect(m_interpolationCurve, SIGNAL(xDataColumnChanged(const AbstractColumn*)), this, SLOT(curveXDataColumnChanged(const AbstractColumn*)));
	connect(m_interpolationCurve, SIGNAL(yDataColumnChanged(const AbstractColumn*)), this, SLOT(curveYDataColumnChanged(const AbstractColumn*)));
	connect(m_interpolationCurve, SIGNAL(interpolationDataChanged(XYInterpolationCurve::InterpolationData)), this, SLOT(curveInterpolationDataChanged(XYInterpolationCurve::InterpolationData)));
	connect(m_interpolationCurve, SIGNAL(sourceDataChangedSinceLastInterpolation()), this, SLOT(enableRecalculate()));
}

void XYInterpolationCurveDock::setModel() {
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
void XYInterpolationCurveDock::setCurves(QList<XYCurve*> list) {
	m_initializing=true;
	m_curvesList=list;
	m_curve=list.first();
	m_interpolationCurve = dynamic_cast<XYInterpolationCurve*>(m_curve);
	Q_ASSERT(m_interpolationCurve);
	m_aspectTreeModel = new AspectTreeModel(m_curve->project());
	this->setModel();
	m_interpolationData = m_interpolationCurve->interpolationData();
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
void XYInterpolationCurveDock::nameChanged() {
	if (m_initializing)
		return;

	m_curve->setName(uiGeneralTab.leName->text());
}

void XYInterpolationCurveDock::commentChanged() {
	if (m_initializing)
		return;

	m_curve->setComment(uiGeneralTab.leComment->text());
}

void XYInterpolationCurveDock::xDataColumnChanged(const QModelIndex& index) {
	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = 0;
	if (aspect) {
		column = dynamic_cast<AbstractColumn*>(aspect);
		Q_ASSERT(column);
	}

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYInterpolationCurve*>(curve)->setXDataColumn(column);

	// disable types that need more data points
	if (column != 0) {
		unsigned int n=0;
		for (int row=0; row < column->rowCount(); row++)
			if (!std::isnan(column->valueAt(row)) && !column->isMasked(row)) 
				n++;
		dataPoints = n;
		if(m_interpolationData.pointsMode == XYInterpolationCurve::Auto)
			pointsModeChanged();

		const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(uiGeneralTab.cbType->model());
		QStandardItem* item = model->item(nsl_interp_type_polynomial);
		if (dataPoints < gsl_interp_type_min_size(gsl_interp_polynomial) || dataPoints > 100) {	// not good for many points
			item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
			if (uiGeneralTab.cbType->currentIndex() == nsl_interp_type_polynomial)
				uiGeneralTab.cbType->setCurrentIndex(0);
		}
		else
			item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

		item = model->item(nsl_interp_type_cspline);
		if (dataPoints < gsl_interp_type_min_size(gsl_interp_cspline)) {
			item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
			if (uiGeneralTab.cbType->currentIndex() == nsl_interp_type_cspline)
				uiGeneralTab.cbType->setCurrentIndex(0);
		}
		else
			item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

		item = model->item(nsl_interp_type_cspline_periodic);
		if (dataPoints < gsl_interp_type_min_size(gsl_interp_cspline_periodic)) {
			item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
			if (uiGeneralTab.cbType->currentIndex() == nsl_interp_type_cspline_periodic)
				uiGeneralTab.cbType->setCurrentIndex(0);
		}
		else
			item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

		item = model->item(nsl_interp_type_akima);
		if (dataPoints < gsl_interp_type_min_size(gsl_interp_akima)) {
			item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
			if (uiGeneralTab.cbType->currentIndex() == nsl_interp_type_akima)
				uiGeneralTab.cbType->setCurrentIndex(0);
		}
		else
			item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

		item = model->item(nsl_interp_type_akima_periodic);
		if (dataPoints < gsl_interp_type_min_size(gsl_interp_akima_periodic)) {
			item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
			if (uiGeneralTab.cbType->currentIndex() == nsl_interp_type_akima_periodic)
				uiGeneralTab.cbType->setCurrentIndex(0);
		}
		else
			item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

#if GSL_MAJOR_VERSION >= 2
		item = model->item(nsl_interp_type_steffen);
		if (dataPoints < gsl_interp_type_min_size(gsl_interp_steffen)) {
			item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
			if (uiGeneralTab.cbType->currentIndex() == nsl_interp_type_steffen)
				uiGeneralTab.cbType->setCurrentIndex(0);
		}
		else
			item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
#endif
		// own types work with 2 or more data points
	}
}

void XYInterpolationCurveDock::yDataColumnChanged(const QModelIndex& index) {
	if (m_initializing)
		return;

	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = 0;
	if (aspect) {
		column = dynamic_cast<AbstractColumn*>(aspect);
		Q_ASSERT(column);
	}

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYInterpolationCurve*>(curve)->setYDataColumn(column);
}

void XYInterpolationCurveDock::typeChanged() {
	nsl_interp_type type = (nsl_interp_type)uiGeneralTab.cbType->currentIndex();
	m_interpolationData.type = type;

	switch (type) {
	case nsl_interp_type_pch:
		uiGeneralTab.lVariant->show();
		uiGeneralTab.cbVariant->show();
		break;
	default:
		uiGeneralTab.lVariant->hide();
		uiGeneralTab.cbVariant->hide();
		uiGeneralTab.cbVariant->setCurrentIndex(nsl_interp_pch_variant_finite_difference);
		uiGeneralTab.lParameter->hide();
		uiGeneralTab.lTension->hide();
		uiGeneralTab.sbTension->hide();
		uiGeneralTab.lContinuity->hide();
		uiGeneralTab.sbContinuity->hide();
		uiGeneralTab.lBias->hide();
		uiGeneralTab.sbBias->hide();
	}

	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYInterpolationCurveDock::variantChanged() {
	nsl_interp_pch_variant variant = (nsl_interp_pch_variant)uiGeneralTab.cbVariant->currentIndex();
	m_interpolationData.variant = variant;

	switch (variant) {
	case nsl_interp_pch_variant_finite_difference:
		uiGeneralTab.lParameter->hide();
                uiGeneralTab.lTension->hide();
                uiGeneralTab.sbTension->hide();
                uiGeneralTab.lContinuity->hide();
                uiGeneralTab.sbContinuity->hide();
                uiGeneralTab.lBias->hide();
                uiGeneralTab.sbBias->hide();
		break;
	case nsl_interp_pch_variant_catmull_rom:
		uiGeneralTab.lParameter->show();
                uiGeneralTab.lTension->show();
                uiGeneralTab.sbTension->show();
                uiGeneralTab.sbTension->setEnabled(false);
                uiGeneralTab.sbTension->setValue(0.0);
                uiGeneralTab.lContinuity->hide();
                uiGeneralTab.sbContinuity->hide();
                uiGeneralTab.lBias->hide();
                uiGeneralTab.sbBias->hide();
		break;
	case nsl_interp_pch_variant_cardinal:
		uiGeneralTab.lParameter->show();
                uiGeneralTab.lTension->show();
                uiGeneralTab.sbTension->show();
                uiGeneralTab.sbTension->setEnabled(true);
                uiGeneralTab.lContinuity->hide();
                uiGeneralTab.sbContinuity->hide();
                uiGeneralTab.lBias->hide();
                uiGeneralTab.sbBias->hide();
		break;
	case nsl_interp_pch_variant_kochanek_bartels:
		uiGeneralTab.lParameter->show();
                uiGeneralTab.lTension->show();
                uiGeneralTab.sbTension->show();
                uiGeneralTab.sbTension->setEnabled(true);
                uiGeneralTab.lContinuity->show();
                uiGeneralTab.sbContinuity->show();
                uiGeneralTab.lBias->show();
                uiGeneralTab.sbBias->show();
		break;
	}

	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYInterpolationCurveDock::tensionChanged() {
	m_interpolationData.tension = uiGeneralTab.sbTension->value();

	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYInterpolationCurveDock::continuityChanged() {
	m_interpolationData.continuity = uiGeneralTab.sbContinuity->value();

	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYInterpolationCurveDock::biasChanged() {
	m_interpolationData.bias = uiGeneralTab.sbBias->value();

	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYInterpolationCurveDock::evaluateChanged() {
	m_interpolationData.evaluate = (nsl_interp_evaluate)uiGeneralTab.cbEval->currentIndex();


	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYInterpolationCurveDock::pointsModeChanged() {
	XYInterpolationCurve::PointsMode mode = (XYInterpolationCurve::PointsMode)uiGeneralTab.cbPointsMode->currentIndex();

	switch (mode) {
	case XYInterpolationCurve::Auto:
		uiGeneralTab.sbPoints->setEnabled(false);
		uiGeneralTab.sbPoints->setDecimals(0);
		uiGeneralTab.sbPoints->setSingleStep(1.0);
		uiGeneralTab.sbPoints->setValue(5*dataPoints);
		break;
	case XYInterpolationCurve::Multiple:
		uiGeneralTab.sbPoints->setEnabled(true);
		if(m_interpolationData.pointsMode != XYInterpolationCurve::Multiple && dataPoints > 0) {
			uiGeneralTab.sbPoints->setDecimals(2);
			uiGeneralTab.sbPoints->setValue(uiGeneralTab.sbPoints->value()/(double)dataPoints);
			uiGeneralTab.sbPoints->setSingleStep(0.01);
		}
		break;
	case XYInterpolationCurve::Custom:
		uiGeneralTab.sbPoints->setEnabled(true);
		if(m_interpolationData.pointsMode == XYInterpolationCurve::Multiple) {
			uiGeneralTab.sbPoints->setDecimals(0);
			uiGeneralTab.sbPoints->setSingleStep(1.0);
			uiGeneralTab.sbPoints->setValue(uiGeneralTab.sbPoints->value()*dataPoints);
		}
		break;
	}

	m_interpolationData.pointsMode = mode;
}

void XYInterpolationCurveDock::numberOfPointsChanged() {
	if(uiGeneralTab.cbPointsMode->currentIndex() == XYInterpolationCurve::Multiple)
		m_interpolationData.npoints = uiGeneralTab.sbPoints->value()*dataPoints;
	else
		m_interpolationData.npoints = uiGeneralTab.sbPoints->value();

	// warn if points is smaller than data points
	QPalette palette = uiGeneralTab.sbPoints->palette();
	if(m_interpolationData.npoints < dataPoints)
		palette.setColor(QPalette::Text, Qt::red);
	else
		palette.setColor(QPalette::Text, Qt::black);
	uiGeneralTab.sbPoints->setPalette(palette);

	enableRecalculate();
}

void XYInterpolationCurveDock::recalculateClicked() {
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYInterpolationCurve*>(curve)->setInterpolationData(m_interpolationData);

	uiGeneralTab.pbRecalculate->setEnabled(false);
	QApplication::restoreOverrideCursor();
}

void XYInterpolationCurveDock::enableRecalculate() const {
	if (m_initializing)
		return;

	//no interpolationing possible without the x- and y-data
	AbstractAspect* aspectX = static_cast<AbstractAspect*>(cbXDataColumn->currentModelIndex().internalPointer());
	AbstractAspect* aspectY = static_cast<AbstractAspect*>(cbYDataColumn->currentModelIndex().internalPointer());
	bool data = (aspectX!=0 && aspectY!=0);

	uiGeneralTab.pbRecalculate->setEnabled(data);
}

/*!
 * show the result and details of the interpolation
 */
void XYInterpolationCurveDock::showInterpolationResult() {
	const XYInterpolationCurve::InterpolationResult& interpolationResult = m_interpolationCurve->interpolationResult();
	if (!interpolationResult.available) {
		uiGeneralTab.teResult->clear();
		return;
	}

	//const XYInterpolationCurve::InterpolationData& interpolationData = m_interpolationCurve->interpolationData();
	QString str = i18n("status:") + ' ' + interpolationResult.status + "<br>";

	if (!interpolationResult.valid) {
		uiGeneralTab.teResult->setText(str);
		return; //result is not valid, there was an error which is shown in the status-string, nothing to show more.
	}

	if (interpolationResult.elapsedTime>1000)
		str += i18n("calculation time: %1 s").arg(QString::number(interpolationResult.elapsedTime/1000)) + "<br>";
	else
		str += i18n("calculation time: %1 ms").arg(QString::number(interpolationResult.elapsedTime)) + "<br>";

 	str += "<br><br>";

	uiGeneralTab.teResult->setText(str);
}

//*************************************************************
//*********** SLOTs for changes triggered in XYCurve **********
//*************************************************************
//General-Tab
void XYInterpolationCurveDock::curveDescriptionChanged(const AbstractAspect* aspect) {
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

void XYInterpolationCurveDock::curveXDataColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromColumn(cbXDataColumn, column);
	m_initializing = false;
}

void XYInterpolationCurveDock::curveYDataColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromColumn(cbYDataColumn, column);
	m_initializing = false;
}

void XYInterpolationCurveDock::curveInterpolationDataChanged(const XYInterpolationCurve::InterpolationData& data) {
	m_initializing = true;
	m_interpolationData = data;
	uiGeneralTab.cbType->setCurrentIndex(m_interpolationData.type);
	this->typeChanged();

	this->showInterpolationResult();
	m_initializing = false;
}

void XYInterpolationCurveDock::dataChanged() {
	this->enableRecalculate();
}
