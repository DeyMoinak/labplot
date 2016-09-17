/***************************************************************************
    File             : XYFourierTransformCurveDock.cpp
    Project          : LabPlot
    --------------------------------------------------------------------
    Copyright        : (C) 2016 Stefan Gerlach (stefan.gerlach@uni.kn)
    Description      : widget for editing properties of Fourier transform curves

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

#include "XYFourierTransformCurveDock.h"
#include "backend/core/AspectTreeModel.h"
#include "backend/core/Project.h"
#include "backend/worksheet/plots/cartesian/XYFourierTransformCurve.h"
#include "commonfrontend/widgets/TreeViewComboBox.h"

#include <QMenu>
#include <QWidgetAction>
#include <KMessageBox>

/*!
  \class XYFourierTransformCurveDock
 \brief  Provides a widget for editing the properties of the XYFourierTransformCurves
		(2D-curves defined by a Fourier transform) currently selected in
		the project explorer.

  If more then one curves are set, the properties of the first column are shown.
  The changes of the properties are applied to all curves.
  The exclusions are the name, the comment and the datasets (columns) of
  the curves  - these properties can only be changed if there is only one single curve.

  \ingroup kdefrontend
*/

XYFourierTransformCurveDock::XYFourierTransformCurveDock(QWidget *parent): 
	XYCurveDock(parent), cbXDataColumn(0), cbYDataColumn(0), m_transformCurve(0) {

	//remove the tab "Error bars"
	ui.tabWidget->removeTab(5);
}

/*!
 * 	// Tab "General"
 */
void XYFourierTransformCurveDock::setupGeneral() {
	QWidget* generalTab = new QWidget(ui.tabGeneral);
	uiGeneralTab.setupUi(generalTab);

	QGridLayout* gridLayout = dynamic_cast<QGridLayout*>(generalTab->layout());
	if (gridLayout) {
		gridLayout->setContentsMargins(2,2,2,2);
		gridLayout->setHorizontalSpacing(2);
		gridLayout->setVerticalSpacing(2);
	}

	cbXDataColumn = new TreeViewComboBox(generalTab);
	gridLayout->addWidget(cbXDataColumn, 4, 2, 1, 2);
	cbYDataColumn = new TreeViewComboBox(generalTab);
	gridLayout->addWidget(cbYDataColumn, 5, 2, 1, 2);

	for (int i=0; i < NSL_SF_WINDOW_TYPE_COUNT; i++)
		uiGeneralTab.cbWindowType->addItem(i18n(nsl_sf_window_type_name[i]));
	for (int i=0; i < NSL_DFT_RESULT_TYPE_COUNT; i++)
		uiGeneralTab.cbType->addItem(i18n(nsl_dft_result_type_name[i]));
	for (int i=0; i < NSL_DFT_XSCALE_COUNT; i++)
		uiGeneralTab.cbXScale->addItem(i18n(nsl_dft_xscale_name[i]));

	QHBoxLayout* layout = new QHBoxLayout(ui.tabGeneral);
	layout->setMargin(0);
	layout->addWidget(generalTab);

	//Slots
	connect( uiGeneralTab.leName, SIGNAL(returnPressed()), this, SLOT(nameChanged()) );
	connect( uiGeneralTab.leComment, SIGNAL(returnPressed()), this, SLOT(commentChanged()) );
	connect( uiGeneralTab.chkVisible, SIGNAL(clicked(bool)), this, SLOT(visibilityChanged(bool)) );

	connect( uiGeneralTab.cbWindowType, SIGNAL(currentIndexChanged(int)), this, SLOT(windowTypeChanged()) );
	connect( uiGeneralTab.cbType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged()) );
	connect( uiGeneralTab.cbTwoSided, SIGNAL(stateChanged(int)), this, SLOT(twoSidedChanged()) );
	connect( uiGeneralTab.cbShifted, SIGNAL(stateChanged(int)), this, SLOT(shiftedChanged()) );
	connect( uiGeneralTab.cbXScale, SIGNAL(currentIndexChanged(int)), this, SLOT(xScaleChanged()) );

//	connect( uiGeneralTab.pbOptions, SIGNAL(clicked()), this, SLOT(showOptions()) );
	connect( uiGeneralTab.pbRecalculate, SIGNAL(clicked()), this, SLOT(recalculateClicked()) );
}

void XYFourierTransformCurveDock::initGeneralTab() {
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
	m_transformCurve = dynamic_cast<XYFourierTransformCurve*>(m_curve);
	Q_ASSERT(m_transformCurve);
	XYCurveDock::setModelIndexFromColumn(cbXDataColumn, m_transformCurve->xDataColumn());
	XYCurveDock::setModelIndexFromColumn(cbYDataColumn, m_transformCurve->yDataColumn());

	uiGeneralTab.cbWindowType->setCurrentIndex(m_transformData.windowType);
	this->windowTypeChanged();
	uiGeneralTab.cbType->setCurrentIndex(m_transformData.type);
	this->typeChanged();
	uiGeneralTab.cbTwoSided->setChecked(m_transformData.twoSided);
	this->twoSidedChanged();	// show/hide shifted check box
	uiGeneralTab.cbShifted->setChecked(m_transformData.shifted);
	this->shiftedChanged();
	uiGeneralTab.cbXScale->setCurrentIndex(m_transformData.xScale);
	this->xScaleChanged();
	this->showTransformResult();

	//enable the "recalculate"-button if the source data was changed since the last transform
	uiGeneralTab.pbRecalculate->setEnabled(m_transformCurve->isSourceDataChangedSinceLastTransform());

	uiGeneralTab.chkVisible->setChecked( m_curve->isVisible() );

	//Slots
	connect(m_transformCurve, SIGNAL(aspectDescriptionChanged(const AbstractAspect*)), this, SLOT(curveDescriptionChanged(const AbstractAspect*)));
	connect(m_transformCurve, SIGNAL(xDataColumnChanged(const AbstractColumn*)), this, SLOT(curveXDataColumnChanged(const AbstractColumn*)));
	connect(m_transformCurve, SIGNAL(yDataColumnChanged(const AbstractColumn*)), this, SLOT(curveYDataColumnChanged(const AbstractColumn*)));
	connect(m_transformCurve, SIGNAL(transformDataChanged(XYFourierTransformCurve::TransformData)), this, SLOT(curveTransformDataChanged(XYFourierTransformCurve::TransformData)));
	connect(m_transformCurve, SIGNAL(sourceDataChangedSinceLastTransform()), this, SLOT(enableRecalculate()));
}

void XYFourierTransformCurveDock::setModel() {
	QList<const char*>  list;
	list<<"Folder"<<"Workbook"<<"Datapicker"<<"DatapickerCurve"<<"Spreadsheet"
		<<"FileDataSource"<<"Column"<<"Worksheet"<<"CartesianPlot"<<"XYFitCurve";
	cbXDataColumn->setTopLevelClasses(list);
	cbYDataColumn->setTopLevelClasses(list);

 	list.clear();
	list<<"Column";
	cbXDataColumn->setSelectableClasses(list);
	cbYDataColumn->setSelectableClasses(list);

	cbXDataColumn->setModel(m_aspectTreeModel);
	cbYDataColumn->setModel(m_aspectTreeModel);

	connect( cbXDataColumn, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(xDataColumnChanged(QModelIndex)) );
	connect( cbYDataColumn, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(yDataColumnChanged(QModelIndex)) );
	XYCurveDock::setModel();
}

/*!
  sets the curves. The properties of the curves in the list \c list can be edited in this widget.
*/
void XYFourierTransformCurveDock::setCurves(QList<XYCurve*> list) {
	m_initializing=true;
	m_curvesList=list;
	m_curve=list.first();
	m_transformCurve = dynamic_cast<XYFourierTransformCurve*>(m_curve);
	Q_ASSERT(m_transformCurve);
	m_aspectTreeModel = new AspectTreeModel(m_curve->project());
	this->setModel();
	m_transformData = m_transformCurve->transformData();
	initGeneralTab();
	initTabs();
	m_initializing=false;
}

//*************************************************************
//**** SLOTs for changes triggered in XYFitCurveDock *****
//*************************************************************
void XYFourierTransformCurveDock::nameChanged() {
	if (m_initializing)
		return;

	m_curve->setName(uiGeneralTab.leName->text());
}

void XYFourierTransformCurveDock::commentChanged() {
	if (m_initializing)
		return;

	m_curve->setComment(uiGeneralTab.leComment->text());
}

void XYFourierTransformCurveDock::xDataColumnChanged(const QModelIndex& index) {
	if (m_initializing)
		return;

	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = 0;
	if (aspect) {
		column = dynamic_cast<AbstractColumn*>(aspect);
		Q_ASSERT(column);
	}

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYFourierTransformCurve*>(curve)->setXDataColumn(column);
}

void XYFourierTransformCurveDock::yDataColumnChanged(const QModelIndex& index) {
	if (m_initializing)
		return;

	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = 0;
	if (aspect) {
		column = dynamic_cast<AbstractColumn*>(aspect);
		Q_ASSERT(column);
	}

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYFourierTransformCurve*>(curve)->setYDataColumn(column);
}

void XYFourierTransformCurveDock::windowTypeChanged() {
	nsl_sf_window_type windowType = (nsl_sf_window_type)uiGeneralTab.cbWindowType->currentIndex();
	m_transformData.windowType = windowType;

	enableRecalculate();
}

void XYFourierTransformCurveDock::typeChanged() {
	nsl_dft_result_type type = (nsl_dft_result_type)uiGeneralTab.cbType->currentIndex();
	m_transformData.type = type;

	enableRecalculate();
}

void XYFourierTransformCurveDock::twoSidedChanged() {
	bool twoSided = uiGeneralTab.cbTwoSided->isChecked();
	m_transformData.twoSided = twoSided;

	if (twoSided)
		uiGeneralTab.cbShifted->setEnabled(true);
	else {
		uiGeneralTab.cbShifted->setEnabled(false);
		uiGeneralTab.cbShifted->setChecked(false);
	}

	enableRecalculate();
}

void XYFourierTransformCurveDock::shiftedChanged() {
	bool shifted = uiGeneralTab.cbShifted->isChecked();
	m_transformData.shifted = shifted;

	enableRecalculate();
}

void XYFourierTransformCurveDock::xScaleChanged() {
	nsl_dft_xscale xScale = (nsl_dft_xscale)uiGeneralTab.cbXScale->currentIndex();
	m_transformData.xScale = xScale;

	enableRecalculate();
}

void XYFourierTransformCurveDock::recalculateClicked() {

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYFourierTransformCurve*>(curve)->setTransformData(m_transformData);

	uiGeneralTab.pbRecalculate->setEnabled(false);
	QApplication::restoreOverrideCursor();
}

void XYFourierTransformCurveDock::enableRecalculate() const {
	if (m_initializing)
		return;

	//no transforming possible without the x- and y-data
	AbstractAspect* aspectX = static_cast<AbstractAspect*>(cbXDataColumn->currentModelIndex().internalPointer());
	AbstractAspect* aspectY = static_cast<AbstractAspect*>(cbYDataColumn->currentModelIndex().internalPointer());
	bool data = (aspectX!=0 && aspectY!=0);

	uiGeneralTab.pbRecalculate->setEnabled(data);
}

/*!
 * show the result and details of the transform
 */
void XYFourierTransformCurveDock::showTransformResult() {
	const XYFourierTransformCurve::TransformResult& transformResult = m_transformCurve->transformResult();
	if (!transformResult.available) {
		uiGeneralTab.teResult->clear();
		return;
	}

	//const XYFourierTransformCurve::TransformData& transformData = m_transformCurve->transformData();
	QString str = i18n("status:") + ' ' + transformResult.status + "<br>";

	if (!transformResult.valid) {
		uiGeneralTab.teResult->setText(str);
		return; //result is not valid, there was an error which is shown in the status-string, nothing to show more.
	}

	if (transformResult.elapsedTime>1000)
		str += i18n("calculation time: %1 s").arg(QString::number(transformResult.elapsedTime/1000)) + "<br>";
	else
		str += i18n("calculation time: %1 ms").arg(QString::number(transformResult.elapsedTime)) + "<br>";

 	str += "<br><br>";

	uiGeneralTab.teResult->setText(str);
}

//*************************************************************
//*********** SLOTs for changes triggered in XYCurve **********
//*************************************************************
//General-Tab
void XYFourierTransformCurveDock::curveDescriptionChanged(const AbstractAspect* aspect) {
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

void XYFourierTransformCurveDock::curveXDataColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromColumn(cbXDataColumn, column);
	m_initializing = false;
}

void XYFourierTransformCurveDock::curveYDataColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromColumn(cbYDataColumn, column);
	m_initializing = false;
}

void XYFourierTransformCurveDock::curveTransformDataChanged(const XYFourierTransformCurve::TransformData& data) {
	m_initializing = true;
	m_transformData = data;
	uiGeneralTab.cbType->setCurrentIndex(m_transformData.type);
	this->typeChanged();

	this->showTransformResult();
	m_initializing = false;
}

void XYFourierTransformCurveDock::dataChanged() {
	this->enableRecalculate();
}
