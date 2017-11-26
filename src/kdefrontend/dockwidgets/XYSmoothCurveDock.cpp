/***************************************************************************
    File             : XYSmoothCurveDock.cpp
    Project          : LabPlot
    --------------------------------------------------------------------
    Copyright        : (C) 2016 Stefan Gerlach (stefan.gerlach@uni.kn)
    Copyright        : (C) 2017 Alexander Semke (alexander.semke@web.de)
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

#include "XYSmoothCurveDock.h"
#include "backend/core/AspectTreeModel.h"
#include "backend/core/Project.h"
#include "backend/worksheet/plots/cartesian/XYSmoothCurve.h"
#include "commonfrontend/widgets/TreeViewComboBox.h"

#include <QMenu>
#include <QWidgetAction>
#include <QStandardItemModel>

/*!
  \class XYSmoothCurveDock
 \brief  Provides a widget for editing the properties of the XYSmoothCurves
		(2D-curves defined by an smooth) currently selected in
		the project explorer.

  If more then one curves are set, the properties of the first column are shown.
  The changes of the properties are applied to all curves.
  The exclusions are the name, the comment and the datasets (columns) of
  the curves  - these properties can only be changed if there is only one single curve.

  \ingroup kdefrontend
*/

XYSmoothCurveDock::XYSmoothCurveDock(QWidget* parent) : XYCurveDock(parent),
	cbDataSourceCurve(nullptr),
	cbXDataColumn(nullptr),
	cbYDataColumn(nullptr),
	m_smoothCurve(nullptr) {

	//hide the line connection type
	ui.cbLineType->setDisabled(true);

	//remove the tab "Error bars"
	ui.tabWidget->removeTab(5);
}

/*!
 * 	// Tab "General"
 */
void XYSmoothCurveDock::setupGeneral() {
#ifndef NDEBUG
	qDebug()<<"XYSmoothCurveDock::setupGeneral()";
#endif
	QWidget* generalTab = new QWidget(ui.tabGeneral);
	uiGeneralTab.setupUi(generalTab);

	QGridLayout* gridLayout = dynamic_cast<QGridLayout*>(generalTab->layout());
	if (gridLayout) {
		gridLayout->setContentsMargins(2,2,2,2);
		gridLayout->setHorizontalSpacing(2);
		gridLayout->setVerticalSpacing(2);
	}

	uiGeneralTab.cbDataSourceType->addItem(i18n("Spreadsheet"));
	uiGeneralTab.cbDataSourceType->addItem(i18n("XY-Curve"));

	cbDataSourceCurve = new TreeViewComboBox(generalTab);
	gridLayout->addWidget(cbDataSourceCurve, 5, 2, 1, 2);
	cbXDataColumn = new TreeViewComboBox(generalTab);
	gridLayout->addWidget(cbXDataColumn, 6, 2, 1, 2);
	cbYDataColumn = new TreeViewComboBox(generalTab);
	gridLayout->addWidget(cbYDataColumn, 7, 2, 1, 2);

	for (int i=0; i < NSL_SMOOTH_TYPE_COUNT; i++)
		uiGeneralTab.cbType->addItem(i18n(nsl_smooth_type_name[i]));

	for (int i=0; i < NSL_SMOOTH_WEIGHT_TYPE_COUNT; i++)
		uiGeneralTab.cbWeight->addItem(i18n(nsl_smooth_weight_type_name[i]));

	for (int i=0; i < NSL_SMOOTH_PAD_MODE_COUNT; i++)
		uiGeneralTab.cbMode->addItem(i18n(nsl_smooth_pad_mode_name[i]));

	uiGeneralTab.pbRecalculate->setIcon(QIcon::fromTheme("run-build"));

	QHBoxLayout* layout = new QHBoxLayout(ui.tabGeneral);
	layout->setMargin(0);
	layout->addWidget(generalTab);

	//Slots
	connect( uiGeneralTab.leName, SIGNAL(returnPressed()), this, SLOT(nameChanged()) );
	connect( uiGeneralTab.leComment, SIGNAL(returnPressed()), this, SLOT(commentChanged()) );
	connect( uiGeneralTab.chkVisible, SIGNAL(clicked(bool)), this, SLOT(visibilityChanged(bool)) );
	connect( uiGeneralTab.cbDataSourceType, SIGNAL(currentIndexChanged(int)), this, SLOT(dataSourceTypeChanged(int)) );
	connect( uiGeneralTab.cbAutoRange, SIGNAL(clicked(bool)), this, SLOT(autoRangeChanged()) );
	connect( uiGeneralTab.sbMin, SIGNAL(valueChanged(double)), this, SLOT(xRangeMinChanged()) );
	connect( uiGeneralTab.sbMax, SIGNAL(valueChanged(double)), this, SLOT(xRangeMaxChanged()) );
	connect( uiGeneralTab.cbType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged()) );
	connect( uiGeneralTab.sbPoints, SIGNAL(valueChanged(int)), this, SLOT(pointsChanged()) );
	connect( uiGeneralTab.cbWeight, SIGNAL(currentIndexChanged(int)), this, SLOT(weightChanged()) );
	connect( uiGeneralTab.sbPercentile, SIGNAL(valueChanged(double)), this, SLOT(percentileChanged()) );
	connect( uiGeneralTab.sbOrder, SIGNAL(valueChanged(int)), this, SLOT(orderChanged()) );
	connect( uiGeneralTab.cbMode, SIGNAL(currentIndexChanged(int)), this, SLOT(modeChanged()) );
	connect( uiGeneralTab.sbLeftValue, SIGNAL(valueChanged(double)), this, SLOT(valueChanged()) );
	connect( uiGeneralTab.sbRightValue, SIGNAL(valueChanged(double)), this, SLOT(valueChanged()) );
	connect( uiGeneralTab.pbRecalculate, SIGNAL(clicked()), this, SLOT(recalculateClicked()) );

	connect( cbDataSourceCurve, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(dataSourceCurveChanged(QModelIndex)) );
	connect( cbXDataColumn, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(xDataColumnChanged(QModelIndex)) );
	connect( cbYDataColumn, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(yDataColumnChanged(QModelIndex)) );
}

void XYSmoothCurveDock::initGeneralTab() {
#ifndef NDEBUG
	qDebug()<<"XYSmoothCurveDock::initGeneralTab()";
#endif
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
	m_smoothCurve = dynamic_cast<XYSmoothCurve*>(m_curve);

	uiGeneralTab.cbDataSourceType->setCurrentIndex(m_smoothCurve->dataSourceType());
	this->dataSourceTypeChanged(uiGeneralTab.cbDataSourceType->currentIndex());
	XYCurveDock::setModelIndexFromAspect(cbDataSourceCurve, m_smoothCurve->dataSourceCurve());
	XYCurveDock::setModelIndexFromAspect(cbXDataColumn, m_smoothCurve->xDataColumn());
	XYCurveDock::setModelIndexFromAspect(cbYDataColumn, m_smoothCurve->yDataColumn());
	uiGeneralTab.cbAutoRange->setChecked(m_smoothData.autoRange);
	uiGeneralTab.sbMin->setValue(m_smoothData.xRange.first());
	uiGeneralTab.sbMax->setValue(m_smoothData.xRange.last());
	this->autoRangeChanged();
	// update list of selectable types
	xDataColumnChanged(cbXDataColumn->currentModelIndex());

	uiGeneralTab.cbType->setCurrentIndex(m_smoothData.type);
	typeChanged();	// needed, when type does not change
	uiGeneralTab.sbPoints->setValue((int)m_smoothData.points);
	uiGeneralTab.cbWeight->setCurrentIndex(m_smoothData.weight);
	uiGeneralTab.sbPercentile->setValue(m_smoothData.percentile);
	uiGeneralTab.sbOrder->setValue((int)m_smoothData.order);
	uiGeneralTab.cbMode->setCurrentIndex(m_smoothData.mode);
	modeChanged();	// needed, when mode does not change
	uiGeneralTab.sbLeftValue->setValue(m_smoothData.lvalue);
	uiGeneralTab.sbRightValue->setValue(m_smoothData.rvalue);
	valueChanged();
	this->showSmoothResult();

	uiGeneralTab.chkVisible->setChecked( m_curve->isVisible() );

	//Slots
	connect(m_smoothCurve, SIGNAL(aspectDescriptionChanged(const AbstractAspect*)), this, SLOT(curveDescriptionChanged(const AbstractAspect*)));
	connect(m_smoothCurve, SIGNAL(dataSourceTypeChanged(XYAnalysisCurve::DataSourceType)), this, SLOT(curveDataSourceTypeChanged(XYAnalysisCurve::DataSourceType)));
	connect(m_smoothCurve, SIGNAL(dataSourceCurveChanged(const XYCurve*)), this, SLOT(curveDataSourceCurveChanged(const XYCurve*)));
	connect(m_smoothCurve, SIGNAL(xDataColumnChanged(const AbstractColumn*)), this, SLOT(curveXDataColumnChanged(const AbstractColumn*)));
	connect(m_smoothCurve, SIGNAL(yDataColumnChanged(const AbstractColumn*)), this, SLOT(curveYDataColumnChanged(const AbstractColumn*)));
	connect(m_smoothCurve, SIGNAL(smoothDataChanged(XYSmoothCurve::SmoothData)), this, SLOT(curveSmoothDataChanged(XYSmoothCurve::SmoothData)));
	connect(m_smoothCurve, SIGNAL(sourceDataChanged()), this, SLOT(enableRecalculate()));
}

void XYSmoothCurveDock::setModel() {
	QList<const char*>  list;
	list<<"Folder"<<"Datapicker"<<"Worksheet"<<"CartesianPlot"<<"XYCurve";
	cbDataSourceCurve->setTopLevelClasses(list);

	QList<const AbstractAspect*> hiddenAspects;
	for (auto* curve : m_curvesList)
		hiddenAspects << curve;
	cbDataSourceCurve->setHiddenAspects(hiddenAspects);

	list.clear();
	list<<"Folder"<<"Workbook"<<"Datapicker"<<"DatapickerCurve"<<"Spreadsheet"
		<<"FileDataSource"<<"Column"<<"Worksheet"<<"CartesianPlot"<<"XYFitCurve"<<"CantorWorksheet";
	cbXDataColumn->setTopLevelClasses(list);
	cbYDataColumn->setTopLevelClasses(list);

	cbXDataColumn->setModel(m_aspectTreeModel);
	cbYDataColumn->setModel(m_aspectTreeModel);

	XYCurveDock::setModel();
}

/*!
  sets the curves. The properties of the curves in the list \c list can be edited in this widget.
*/
void XYSmoothCurveDock::setCurves(QList<XYCurve*> list) {
#ifndef NDEBUG
	qDebug()<<"XYSmoothCurveDock::setCurves()";
#endif
	m_initializing=true;
	m_curvesList=list;
	m_curve=list.first();
	m_smoothCurve = dynamic_cast<XYSmoothCurve*>(m_curve);
	m_aspectTreeModel = new AspectTreeModel(m_curve->project());
	this->setModel();
	m_smoothData = m_smoothCurve->smoothData();
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
void XYSmoothCurveDock::nameChanged() {
	if (m_initializing)
		return;

	m_curve->setName(uiGeneralTab.leName->text());
}

void XYSmoothCurveDock::commentChanged() {
	if (m_initializing)
		return;

	m_curve->setComment(uiGeneralTab.leComment->text());
}

void XYSmoothCurveDock::dataSourceTypeChanged(int index) {
	XYAnalysisCurve::DataSourceType type = (XYAnalysisCurve::DataSourceType)index;
	if (type == XYAnalysisCurve::DataSourceSpreadsheet) {
		uiGeneralTab.lDataSourceCurve->hide();
		cbDataSourceCurve->hide();
		uiGeneralTab.lXColumn->show();
		cbXDataColumn->show();
		uiGeneralTab.lYColumn->show();
		cbYDataColumn->show();
	} else {
		uiGeneralTab.lDataSourceCurve->show();
		cbDataSourceCurve->show();
		uiGeneralTab.lXColumn->hide();
		cbXDataColumn->hide();
		uiGeneralTab.lYColumn->hide();
		cbYDataColumn->hide();
	}

	if (m_initializing)
		return;

	for (auto* curve : m_curvesList)
		dynamic_cast<XYSmoothCurve*>(curve)->setDataSourceType(type);
}

void XYSmoothCurveDock::dataSourceCurveChanged(const QModelIndex& index) {
	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	XYCurve* dataSourceCurve = dynamic_cast<XYCurve*>(aspect);

	if (m_initializing)
		return;

	for (auto* curve : m_curvesList)
		dynamic_cast<XYSmoothCurve*>(curve)->setDataSourceCurve(dataSourceCurve);
}

void XYSmoothCurveDock::xDataColumnChanged(const QModelIndex& index) {
	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = dynamic_cast<AbstractColumn*>(aspect);

	for (auto* curve : m_curvesList)
		dynamic_cast<XYSmoothCurve*>(curve)->setXDataColumn(column);

	// disable types that need more data points
	if (column != nullptr) {
		if (uiGeneralTab.cbAutoRange->isChecked()) {
			uiGeneralTab.sbMin->setValue(column->minimum());
			uiGeneralTab.sbMax->setValue(column->maximum());
		}

		unsigned int n = 0;
		for (int row = 0; row < column->rowCount(); row++)
			if (!std::isnan(column->valueAt(row)) && !column->isMasked(row))
				n++;

		// set maximum of sbPoints to number of columns
		uiGeneralTab.sbPoints->setMaximum((int)n);
	}

}

void XYSmoothCurveDock::yDataColumnChanged(const QModelIndex& index) {
	if (m_initializing)
		return;

	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = dynamic_cast<AbstractColumn*>(aspect);

	for (auto* curve : m_curvesList)
		dynamic_cast<XYSmoothCurve*>(curve)->setYDataColumn(column);
}

void XYSmoothCurveDock::autoRangeChanged() {
	bool autoRange = uiGeneralTab.cbAutoRange->isChecked();
	m_smoothData.autoRange = autoRange;

	if (autoRange) {
		uiGeneralTab.lMin->setEnabled(false);
		uiGeneralTab.sbMin->setEnabled(false);
		uiGeneralTab.lMax->setEnabled(false);
		uiGeneralTab.sbMax->setEnabled(false);

		const AbstractColumn* xDataColumn = 0;
		if (m_smoothCurve->dataSourceType() == XYAnalysisCurve::DataSourceSpreadsheet)
			xDataColumn = m_smoothCurve->xDataColumn();
		else {
			if (m_smoothCurve->dataSourceCurve())
				xDataColumn = m_smoothCurve->dataSourceCurve()->xColumn();
		}

		if (xDataColumn) {
			uiGeneralTab.sbMin->setValue(xDataColumn->minimum());
			uiGeneralTab.sbMax->setValue(xDataColumn->maximum());
		}
	} else {
		uiGeneralTab.lMin->setEnabled(true);
		uiGeneralTab.sbMin->setEnabled(true);
		uiGeneralTab.lMax->setEnabled(true);
		uiGeneralTab.sbMax->setEnabled(true);
	}

}
void XYSmoothCurveDock::xRangeMinChanged() {
	double xMin = uiGeneralTab.sbMin->value();

	m_smoothData.xRange.first() = xMin;
	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYSmoothCurveDock::xRangeMaxChanged() {
	double xMax = uiGeneralTab.sbMax->value();

	m_smoothData.xRange.last() = xMax;
	uiGeneralTab.pbRecalculate->setEnabled(true);
}

void XYSmoothCurveDock::typeChanged() {
	nsl_smooth_type type = (nsl_smooth_type)uiGeneralTab.cbType->currentIndex();
	m_smoothData.type = type;

	const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(uiGeneralTab.cbMode->model());
	QStandardItem* pad_interp_item = model->item(nsl_smooth_pad_interp);
	if (type == nsl_smooth_type_moving_average || type == nsl_smooth_type_moving_average_lagged) {
		uiGeneralTab.lWeight->show();
		uiGeneralTab.cbWeight->show();
		// disable interp pad model for MA and MAL
		pad_interp_item->setFlags(pad_interp_item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
	} else {
		uiGeneralTab.lWeight->hide();
		uiGeneralTab.cbWeight->hide();
		pad_interp_item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
	}

	if (type == nsl_smooth_type_moving_average_lagged) {
		uiGeneralTab.sbPoints->setSingleStep(1);
		uiGeneralTab.sbPoints->setMinimum(2);
		uiGeneralTab.lRightValue->hide();
		uiGeneralTab.sbRightValue->hide();
	} else {
		uiGeneralTab.sbPoints->setSingleStep(2);
		uiGeneralTab.sbPoints->setMinimum(3);
		if (m_smoothData.mode == nsl_smooth_pad_constant) {
			uiGeneralTab.lRightValue->show();
			uiGeneralTab.sbRightValue->show();
		}
	}

	if (type == nsl_smooth_type_percentile) {
		uiGeneralTab.lPercentile->show();
		uiGeneralTab.sbPercentile->show();
		// disable interp pad model for MA and MAL
		pad_interp_item->setFlags(pad_interp_item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
	} else {
		uiGeneralTab.lPercentile->hide();
		uiGeneralTab.sbPercentile->hide();
	}

	if (type == nsl_smooth_type_savitzky_golay) {
		uiGeneralTab.lOrder->show();
		uiGeneralTab.sbOrder->show();
	} else {
		uiGeneralTab.lOrder->hide();
		uiGeneralTab.sbOrder->hide();
	}


	enableRecalculate();
}

void XYSmoothCurveDock::pointsChanged() {
	m_smoothData.points = (unsigned int)uiGeneralTab.sbPoints->value();

	// set maximum order
	uiGeneralTab.sbOrder->setMaximum((int)m_smoothData.points - 1);

	enableRecalculate();
}

void XYSmoothCurveDock::weightChanged() {
	m_smoothData.weight = (nsl_smooth_weight_type)uiGeneralTab.cbWeight->currentIndex();

	enableRecalculate();
}

void XYSmoothCurveDock::percentileChanged() {
	m_smoothData.percentile = uiGeneralTab.sbPercentile->value();

	enableRecalculate();
}

void XYSmoothCurveDock::orderChanged() {
	m_smoothData.order = (unsigned int)uiGeneralTab.sbOrder->value();

	enableRecalculate();
}

void XYSmoothCurveDock::modeChanged() {
	m_smoothData.mode = (nsl_smooth_pad_mode)(uiGeneralTab.cbMode->currentIndex());

	if (m_smoothData.mode == nsl_smooth_pad_constant) {
		uiGeneralTab.lLeftValue->show();
		uiGeneralTab.sbLeftValue->show();
		if (m_smoothData.type == nsl_smooth_type_moving_average_lagged) {
			uiGeneralTab.lRightValue->hide();
			uiGeneralTab.sbRightValue->hide();
		} else {
			uiGeneralTab.lRightValue->show();
			uiGeneralTab.sbRightValue->show();
		}
	} else {
		uiGeneralTab.lLeftValue->hide();
		uiGeneralTab.sbLeftValue->hide();
		uiGeneralTab.lRightValue->hide();
		uiGeneralTab.sbRightValue->hide();
	}

	enableRecalculate();
}

void XYSmoothCurveDock::valueChanged() {
	m_smoothData.lvalue = uiGeneralTab.sbLeftValue->value();
	m_smoothData.rvalue = uiGeneralTab.sbRightValue->value();

	enableRecalculate();
}

void XYSmoothCurveDock::recalculateClicked() {
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	for (auto* curve : m_curvesList)
		dynamic_cast<XYSmoothCurve*>(curve)->setSmoothData(m_smoothData);

	uiGeneralTab.pbRecalculate->setEnabled(false);
	emit info(i18n("Smoothing status: ") +  m_smoothCurve->smoothResult().status);
	QApplication::restoreOverrideCursor();
}

void XYSmoothCurveDock::enableRecalculate() const {
	if (m_initializing)
		return;

	//no smoothing possible without the x- and y-data
	bool hasSourceData = false;
	if (m_smoothCurve->dataSourceType() == XYAnalysisCurve::DataSourceSpreadsheet) {
		AbstractAspect* aspectX = static_cast<AbstractAspect*>(cbXDataColumn->currentModelIndex().internalPointer());
		AbstractAspect* aspectY = static_cast<AbstractAspect*>(cbYDataColumn->currentModelIndex().internalPointer());
		hasSourceData = (aspectX!=0 && aspectY!=0);
	} else {
		 hasSourceData = (m_smoothCurve->dataSourceCurve() != NULL);
	}

	uiGeneralTab.pbRecalculate->setEnabled(hasSourceData);
}

/*!
 * show the result and details of the smooth
 */
void XYSmoothCurveDock::showSmoothResult() {
	const XYSmoothCurve::SmoothResult& smoothResult = m_smoothCurve->smoothResult();
	if (!smoothResult.available) {
		uiGeneralTab.teResult->clear();
		return;
	}

	//const XYSmoothCurve::SmoothData& smoothData = m_smoothCurve->smoothData();
	QString str = i18n("status:") + ' ' + smoothResult.status + "<br>";

	if (!smoothResult.valid) {
		uiGeneralTab.teResult->setText(str);
		return; //result is not valid, there was an error which is shown in the status-string, nothing to show more.
	}

	if (smoothResult.elapsedTime>1000)
		str += i18n("calculation time: %1 s").arg(QString::number(smoothResult.elapsedTime/1000)) + "<br>";
	else
		str += i18n("calculation time: %1 ms").arg(QString::number(smoothResult.elapsedTime)) + "<br>";

 	str += "<br><br>";

	uiGeneralTab.teResult->setText(str);

	//enable the "recalculate"-button if the source data was changed since the last smooth
	uiGeneralTab.pbRecalculate->setEnabled(m_smoothCurve->isSourceDataChangedSinceLastRecalc());
}

//*************************************************************
//*********** SLOTs for changes triggered in XYCurve **********
//*************************************************************
//General-Tab
void XYSmoothCurveDock::curveDescriptionChanged(const AbstractAspect* aspect) {
	if (m_curve != aspect)
		return;

	m_initializing = true;
	if (aspect->name() != uiGeneralTab.leName->text())
		uiGeneralTab.leName->setText(aspect->name());
	else if (aspect->comment() != uiGeneralTab.leComment->text())
		uiGeneralTab.leComment->setText(aspect->comment());
	m_initializing = false;
}

void XYSmoothCurveDock::curveDataSourceTypeChanged(XYAnalysisCurve::DataSourceType type) {
	m_initializing = true;
	uiGeneralTab.cbDataSourceType->setCurrentIndex(type);
	m_initializing = false;
}

void XYSmoothCurveDock::curveDataSourceCurveChanged(const XYCurve* curve) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromAspect(cbDataSourceCurve, curve);
	m_initializing = false;
}

void XYSmoothCurveDock::curveXDataColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromAspect(cbXDataColumn, column);
	m_initializing = false;
}

void XYSmoothCurveDock::curveYDataColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromAspect(cbYDataColumn, column);
	m_initializing = false;
}

void XYSmoothCurveDock::curveSmoothDataChanged(const XYSmoothCurve::SmoothData& smoothData) {
	m_initializing = true;
	m_smoothData = smoothData;
	uiGeneralTab.cbType->setCurrentIndex(m_smoothData.type);

	this->showSmoothResult();
	m_initializing = false;
}

void XYSmoothCurveDock::dataChanged() {
	this->enableRecalculate();
}
