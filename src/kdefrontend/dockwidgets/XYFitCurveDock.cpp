/***************************************************************************
    File             : XYFitCurveDock.cpp
    Project          : LabPlot
    --------------------------------------------------------------------
    Copyright        : (C) 2014-2016 Alexander Semke (alexander.semke@web.de)
    Description      : widget for editing properties of fit curves

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

#include "XYFitCurveDock.h"
#include "backend/core/AspectTreeModel.h"
#include "backend/core/Project.h"
#include "commonfrontend/widgets/TreeViewComboBox.h"
#include "kdefrontend/widgets/ConstantsWidget.h"
#include "kdefrontend/widgets/FunctionsWidget.h"
#include "kdefrontend/widgets/FitOptionsWidget.h"
#include "kdefrontend/widgets/FitParametersWidget.h"

#include <QMenu>
#include <QWidgetAction>

/*!
  \class XYFitCurveDock
  \brief  Provides a widget for editing the properties of the XYFitCurves
		(2D-curves defined by a fit model) currently selected in
		the project explorer.

  If more then one curves are set, the properties of the first column are shown.
  The changes of the properties are applied to all curves.
  The exclusions are the name, the comment and the datasets (columns) of
  the curves  - these properties can only be changed if there is only one single curve.

  \ingroup kdefrontend
*/

XYFitCurveDock::XYFitCurveDock(QWidget *parent)
	 : XYCurveDock(parent), cbXDataColumn(0), cbYDataColumn(0), cbWeightsColumn(0), m_fitCurve(0) {

	//remove the tab "Error bars"
	ui.tabWidget->removeTab(5);
}

/*!
 * 	// Tab "General"
 */
void XYFitCurveDock::setupGeneral() {
	QWidget* generalTab = new QWidget(ui.tabGeneral);
	uiGeneralTab.setupUi(generalTab);
	QGridLayout* gridLayout = dynamic_cast<QGridLayout*>(generalTab->layout());
	if (gridLayout) {
		gridLayout->setContentsMargins(2,2,2,2);
		gridLayout->setHorizontalSpacing(2);
		gridLayout->setVerticalSpacing(2);
	}

	cbXDataColumn = new TreeViewComboBox(generalTab);
	gridLayout->addWidget(cbXDataColumn, 4, 4, 1, 2);

	cbYDataColumn = new TreeViewComboBox(generalTab);
	gridLayout->addWidget(cbYDataColumn, 5, 4, 1, 2);

	cbWeightsColumn = new TreeViewComboBox(generalTab);
	gridLayout->addWidget(cbWeightsColumn, 6, 4, 1, 2);

	uiGeneralTab.cbModel->addItem(i18n("Polynomial"));
	uiGeneralTab.cbModel->addItem(i18n("Power"));
	uiGeneralTab.cbModel->addItem(i18n("Exponential"));
	uiGeneralTab.cbModel->addItem(i18n("Inverse Exponential"));
	uiGeneralTab.cbModel->addItem(i18n("Fourier"));
	uiGeneralTab.cbModel->addItem(i18n("Gaussian"));
	uiGeneralTab.cbModel->addItem(i18n("Lorentz (Cauchy)"));
	uiGeneralTab.cbModel->addItem(i18n("Maxwell-Boltzmann"));
	uiGeneralTab.cbModel->addItem(i18n("Sigmoid"));
	uiGeneralTab.cbModel->addItem(i18n("Custom"));

	uiGeneralTab.teEquation->setMaximumHeight(uiGeneralTab.leName->sizeHint().height()*2);

	uiGeneralTab.tbConstants->setIcon( KIcon("labplot-format-text-symbol") );
	uiGeneralTab.tbFunctions->setIcon( KIcon("preferences-desktop-font") );
	uiGeneralTab.pbRecalculate->setIcon(KIcon("run-build"));

	QHBoxLayout* layout = new QHBoxLayout(ui.tabGeneral);
	layout->setMargin(0);
	layout->addWidget(generalTab);

	//Slots
	connect( uiGeneralTab.leName, SIGNAL(returnPressed()), this, SLOT(nameChanged()) );
	connect( uiGeneralTab.leComment, SIGNAL(returnPressed()), this, SLOT(commentChanged()) );
	connect( uiGeneralTab.chkVisible, SIGNAL(clicked(bool)), this, SLOT(visibilityChanged(bool)) );

	connect( uiGeneralTab.cbModel, SIGNAL(currentIndexChanged(int)), this, SLOT(modelChanged(int)) );
	connect( uiGeneralTab.sbDegree, SIGNAL(valueChanged(int)), this, SLOT(updateModelEquation()) );
	connect( uiGeneralTab.teEquation, SIGNAL(expressionChanged()), this, SLOT(enableRecalculate()) );
	connect( uiGeneralTab.tbConstants, SIGNAL(clicked()), this, SLOT(showConstants()) );
	connect( uiGeneralTab.tbFunctions, SIGNAL(clicked()), this, SLOT(showFunctions()) );
	connect( uiGeneralTab.pbParameters, SIGNAL(clicked()), this, SLOT(showParameters()) );
	connect( uiGeneralTab.pbOptions, SIGNAL(clicked()), this, SLOT(showOptions()) );
	connect( uiGeneralTab.pbRecalculate, SIGNAL(clicked()), this, SLOT(recalculateClicked()) );
}

void XYFitCurveDock::initGeneralTab() {
	//if there are more then one curve in the list, disable the tab "general"
	if (m_curvesList.size()==1) {
		uiGeneralTab.lName->setEnabled(true);
		uiGeneralTab.leName->setEnabled(true);
		uiGeneralTab.lComment->setEnabled(true);
		uiGeneralTab.leComment->setEnabled(true);

		uiGeneralTab.leName->setText(m_curve->name());
		uiGeneralTab.leComment->setText(m_curve->comment());
	} else {
		uiGeneralTab.lName->setEnabled(false);
		uiGeneralTab.leName->setEnabled(false);
		uiGeneralTab.lComment->setEnabled(false);
		uiGeneralTab.leComment->setEnabled(false);

		uiGeneralTab.leName->setText("");
		uiGeneralTab.leComment->setText("");
	}

	//show the properties of the first curve
	m_fitCurve = dynamic_cast<XYFitCurve*>(m_curve);
	Q_ASSERT(m_fitCurve);
	XYCurveDock::setModelIndexFromColumn(cbXDataColumn, m_fitCurve->xDataColumn());
	XYCurveDock::setModelIndexFromColumn(cbYDataColumn, m_fitCurve->yDataColumn());
	XYCurveDock::setModelIndexFromColumn(cbWeightsColumn, m_fitCurve->weightsColumn());

	uiGeneralTab.cbModel->setCurrentIndex(m_fitData.modelType);
	this->modelChanged(m_fitData.modelType);

	uiGeneralTab.sbDegree->setValue(m_fitData.degree);
	this->showFitResult();

	//enable the "recalculate"-button if the source data was changed since the last fit
	uiGeneralTab.pbRecalculate->setEnabled(m_fitCurve->isSourceDataChangedSinceLastFit());

	uiGeneralTab.chkVisible->setChecked( m_curve->isVisible() );

	//Slots
	connect(m_fitCurve, SIGNAL(aspectDescriptionChanged(const AbstractAspect*)), this, SLOT(curveDescriptionChanged(const AbstractAspect*)));
	connect(m_fitCurve, SIGNAL(xDataColumnChanged(const AbstractColumn*)), this, SLOT(curveXDataColumnChanged(const AbstractColumn*)));
	connect(m_fitCurve, SIGNAL(yDataColumnChanged(const AbstractColumn*)), this, SLOT(curveYDataColumnChanged(const AbstractColumn*)));
	connect(m_fitCurve, SIGNAL(weightsColumnChanged(const AbstractColumn*)), this, SLOT(curveWeightsColumnChanged(const AbstractColumn*)));
	connect(m_fitCurve, SIGNAL(fitDataChanged(XYFitCurve::FitData)), this, SLOT(curveFitDataChanged(XYFitCurve::FitData)));
	connect(m_fitCurve, SIGNAL(sourceDataChangedSinceLastFit()), this, SLOT(enableRecalculate()));
}

void XYFitCurveDock::setModel() {
	QList<const char*>  list;
	list<<"Folder"<<"Workbook"<<"Spreadsheet"<<"FileDataSource"<<"Column"<<"Datapicker";
	cbXDataColumn->setTopLevelClasses(list);
	cbYDataColumn->setTopLevelClasses(list);
	cbWeightsColumn->setTopLevelClasses(list);

	list.clear();
	list<<"Column";
	cbXDataColumn->setSelectableClasses(list);
	cbYDataColumn->setSelectableClasses(list);
	cbWeightsColumn->setSelectableClasses(list);

	cbXDataColumn->setModel(m_aspectTreeModel);
	cbYDataColumn->setModel(m_aspectTreeModel);
	cbWeightsColumn->setModel(m_aspectTreeModel);

	connect( cbXDataColumn, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(xDataColumnChanged(QModelIndex)) );
	connect( cbYDataColumn, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(yDataColumnChanged(QModelIndex)) );
	connect( cbWeightsColumn, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(weightsColumnChanged(QModelIndex)) );
	XYCurveDock::setModel();
}

/*!
  sets the curves. The properties of the curves in the list \c list can be edited in this widget.
*/
void XYFitCurveDock::setCurves(QList<XYCurve*> list) {
	m_initializing=true;
	m_curvesList=list;
	m_curve=list.first();
	m_fitCurve = dynamic_cast<XYFitCurve*>(m_curve);
	Q_ASSERT(m_fitCurve);
	m_aspectTreeModel = new AspectTreeModel(m_curve->project());
	this->setModel();
	m_fitData = m_fitCurve->fitData();
	initGeneralTab();
	initTabs();
	m_initializing=false;
}

//*************************************************************
//**** SLOTs for changes triggered in XYFitCurveDock *****
//*************************************************************
void XYFitCurveDock::nameChanged() {
	if (m_initializing)
		return;

	m_curve->setName(uiGeneralTab.leName->text());
}

void XYFitCurveDock::commentChanged() {
	if (m_initializing)
		return;

	m_curve->setComment(uiGeneralTab.leComment->text());
}

void XYFitCurveDock::xDataColumnChanged(const QModelIndex& index) {
	if (m_initializing)
		return;

	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = 0;
	if (aspect) {
		column = dynamic_cast<AbstractColumn*>(aspect);
		Q_ASSERT(column);
	}

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYFitCurve*>(curve)->setXDataColumn(column);
}

void XYFitCurveDock::yDataColumnChanged(const QModelIndex& index) {
	if (m_initializing)
		return;

	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = 0;
	if (aspect) {
		column = dynamic_cast<AbstractColumn*>(aspect);
		Q_ASSERT(column);
	}

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYFitCurve*>(curve)->setYDataColumn(column);
}

void XYFitCurveDock::weightsColumnChanged(const QModelIndex& index) {
	if (m_initializing)
		return;

	AbstractAspect* aspect = static_cast<AbstractAspect*>(index.internalPointer());
	AbstractColumn* column = 0;
	if (aspect) {
		column = dynamic_cast<AbstractColumn*>(aspect);
		Q_ASSERT(column);
	}

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYFitCurve*>(curve)->setWeightsColumn(column);
}

void XYFitCurveDock::modelChanged(int index) {
	XYFitCurve::ModelType type = (XYFitCurve::ModelType)index;
	bool custom = (type==XYFitCurve::Custom);
	uiGeneralTab.teEquation->setReadOnly(!custom);
	uiGeneralTab.tbFunctions->setVisible(custom);
	uiGeneralTab.tbConstants->setVisible(custom);

	if (type == XYFitCurve::Polynomial) {
		uiGeneralTab.lDegree->setVisible(true);
		uiGeneralTab.sbDegree->setVisible(true);
		uiGeneralTab.sbDegree->setMaximum(10);
		uiGeneralTab.sbDegree->setValue(1);
	} else if (type == XYFitCurve::Power) {
		uiGeneralTab.lDegree->setVisible(true);
		uiGeneralTab.sbDegree->setVisible(true);
		uiGeneralTab.sbDegree->setMaximum(2);
		uiGeneralTab.sbDegree->setValue(1);
	} else if (type == XYFitCurve::Exponential) {
		uiGeneralTab.lDegree->setVisible(true);
		uiGeneralTab.sbDegree->setVisible(true);
		uiGeneralTab.sbDegree->setMaximum(3);
		uiGeneralTab.sbDegree->setValue(1);
	} else if (type == XYFitCurve::Fourier) {
		uiGeneralTab.lDegree->setVisible(true);
		uiGeneralTab.sbDegree->setVisible(true);
		uiGeneralTab.sbDegree->setMaximum(10);
		uiGeneralTab.sbDegree->setValue(1);
	} else if (type == XYFitCurve::Gaussian) {
		uiGeneralTab.lDegree->setVisible(true);
		uiGeneralTab.sbDegree->setVisible(true);
		uiGeneralTab.sbDegree->setMaximum(10);
		uiGeneralTab.sbDegree->setValue(1);
	} else if (type == XYFitCurve::Lorentz
		|| type == XYFitCurve::Maxwell
		|| type == XYFitCurve::Inverse_Exponential
		|| type == XYFitCurve::Sigmoid
		|| type == XYFitCurve::Custom) {
		uiGeneralTab.lDegree->setVisible(false);
		uiGeneralTab.sbDegree->setVisible(false);
	}

	this->updateModelEquation();
}

void XYFitCurveDock::updateModelEquation() {
	QStringList vars; //variables/parameters that are known in ExpressionTextEdit teEquation
	vars << "x";
	QString eq;
	m_fitData.modelType = (XYFitCurve::ModelType)uiGeneralTab.cbModel->currentIndex();
	int num = uiGeneralTab.sbDegree->value();

	if (m_fitData.modelType!=XYFitCurve::Custom)
		m_fitData.paramNames.clear();

	if (m_fitData.modelType == XYFitCurve::Polynomial) {
		eq = "c0 + c1*x";
		m_fitData.model = eq;
		vars << "c0" << "c1";
		m_fitData.paramNames << "c0" << "c1";
		if (num==2) {
			eq += " + c2*x^2";
			m_fitData.model += " + c2*x^2";
			vars << "c2";
			m_fitData.paramNames << "c2";
		} else if (num>2) {
			QString numStr = QString::number(num);
			eq += " + ... + c" + numStr + "*x^" + numStr;
			vars << "c" + numStr << "...";
			for (int i=2; i<=num; ++i) {
				numStr = QString::number(i);
				m_fitData.model += "+c" + numStr + "*x^" + numStr;
				m_fitData.paramNames << "c"+numStr;
			}
		}
	} else if (m_fitData.modelType == XYFitCurve::Power) {
		if (num==1) {
			eq = "a*x^b";
			vars << "a" << "b";
			m_fitData.paramNames << "a" << "b";
		} else {
			eq = "a + b*x^c";
			vars << "a" << "b" << "c";
			m_fitData.paramNames << "a" << "b" << "c";
		}
		m_fitData.model = eq;
	} else if (m_fitData.modelType == XYFitCurve::Exponential) {
		eq = "a*exp(b*x)";
		vars << "a" << "b";
		m_fitData.paramNames << "a" << "b";
		if (num==2) {
			eq += " + c*exp(d*x)";
			vars << "c" << "d";
			m_fitData.paramNames << "c" << "d";
		} else if (num==3) {
			eq += " + c*exp(d*x) + e*exp(f*x)";
			vars << "c" << "d" << "e" << "f";
			m_fitData.paramNames << "c" << "d" << "e" << "f";
		}
		m_fitData.model = eq;
	} else if (m_fitData.modelType == XYFitCurve::Inverse_Exponential) {
		eq = "a*(1-exp(b*x))+c";
		m_fitData.model = eq;
		vars << "a" << "b" << "c";
		m_fitData.paramNames << "a" << "b" << "c";
	} else if (m_fitData.modelType == XYFitCurve::Fourier) {
		eq = "a0 + (a1*cos(w*x) + b1*sin(w*x))";
		m_fitData.model = eq;
		vars << "w" << "a0" << "a1" << "b1";
		m_fitData.paramNames << "w" << "a0" << "a1" << "b1";
		if (num==2) {
			eq += " + (a2*cos(2*w*x) + b2*sin(2*w*x))";
			m_fitData.model += " + (a2*cos(2*w*x) + b2*sin(2*w*x))";
			vars << "a2" << "b2";
			m_fitData.paramNames << "a2" << "b2";
		} else if (num>2) {
			QString numStr = QString::number(num);
			eq += " + ... + (a" + numStr + "*cos(" + numStr + "*w*x) + b" + numStr + "*sin(" + numStr + "*w*x))";
			vars << "a"+numStr << "b"+numStr << "...";
			for (int i=2; i<=num; ++i) {
				numStr = QString::number(i);
				m_fitData.model += "+ (a" + numStr + "*cos(" + numStr + "*w*x) + b" + numStr + "*sin(" + numStr + "*w*x))";
				m_fitData.paramNames << "a"+numStr << "b"+numStr;
			}
		}
	} else if (m_fitData.modelType == XYFitCurve::Gaussian) {
		eq = "a1*exp(-((x-b1)/c1)^2)";
		m_fitData.model = eq;
		vars << "a1" << "b1" << "c1";
		m_fitData.paramNames << "a1" << "b1" << "c1";
		if (num==2) {
			eq += " + a2*exp(-((x-b2)/c2)^2)";
			m_fitData.model += " + a2*exp(-((x-b2)/c2)^2)";
			vars << "a2" << "b2" << "c2";
			m_fitData.paramNames << "a2" << "b2" << "c2";
		} else if (num==3) {
			eq += " + a2*exp(-((x-b2)/c2)^2) + a3*exp(-((x-b3)/c3)^2)";
			m_fitData.model += " + a2*exp(-((x-b2)/c2)^2) + a3*exp(-((x-b3)/c3)^2)";
			vars << "a2" << "b2" << "c2" << "a3" << "b3" << "c3";
			m_fitData.paramNames << "a2" << "b2" << "c2" << "a3" << "b3" << "c3";
		} else if (num>3) {
			QString numStr = QString::number(num);
			eq += " + a2*exp(-((x-b2)/c2)^2) + ... + a" + numStr + "*exp(-((x-b" + numStr + ")/c" + numStr + ")^2)";
			vars << "a2" << "b2" << "c2" << "a"+numStr << "b"+numStr << "c"+numStr << "...";
			for (int i=2; i<=num; ++i) {
				numStr = QString::number(i);
				m_fitData.model += "+ a" + numStr + "*exp(-((x-b" + numStr + ")/c" + numStr + ")^2)";
				m_fitData.paramNames << "a"+numStr << "b"+numStr << "c"+numStr;
			}
		}
	} else if (m_fitData.modelType == XYFitCurve::Lorentz) {
		eq = "1/pi*s/(s^2+(x-t)^2)";
		m_fitData.model = eq;
		vars << "s" << "t";
		m_fitData.paramNames << "s" << "t";
	} else if (m_fitData.modelType == XYFitCurve::Maxwell) {
		eq = "sqrt(2/pi)*x^2*exp(-x^2/(2*a^2))/a^3";
		m_fitData.model = eq;
		vars << "a";
		m_fitData.paramNames << "a";
	} else if (m_fitData.modelType == XYFitCurve::Sigmoid) {
		eq = "a/(1+exp(-b*(x-c)))";
		m_fitData.model = eq;
		vars << "a" << "b" << "c";
		m_fitData.paramNames << "a" << "b" << "c";
	} else if (m_fitData.modelType == XYFitCurve::Custom) {
		//use the equation of the last selected predefined model or of the last available custom model
		eq = m_fitData.model;
		vars << m_fitData.paramNames;
	}

	//resize the vector for the start values and set the elements to 1.0
	//in case a custom model is used, do nothing, we take over the previous values
	//when initializing, don't do anything - we use start values already
	//available - unless there're no values available
	if (m_fitData.modelType!=XYFitCurve::Custom &&
	        !(m_initializing && m_fitData.paramNames.size() == m_fitData.paramStartValues.size())) {
		m_fitData.paramStartValues.resize(m_fitData.paramNames.size());
		for (int i=0; i<m_fitData.paramStartValues.size(); ++i)
			m_fitData.paramStartValues[i] = 1.0;
	}

	uiGeneralTab.teEquation->setVariables(vars);
	uiGeneralTab.teEquation->setText(eq);
}

void XYFitCurveDock::showConstants() {
	QMenu menu;
	ConstantsWidget constants(&menu);

	connect(&constants, SIGNAL(constantSelected(QString)), this, SLOT(insertConstant(QString)));
	connect(&constants, SIGNAL(constantSelected(QString)), &menu, SLOT(close()));
	connect(&constants, SIGNAL(canceled()), &menu, SLOT(close()));

	QWidgetAction* widgetAction = new QWidgetAction(this);
	widgetAction->setDefaultWidget(&constants);
	menu.addAction(widgetAction);

	QPoint pos(-menu.sizeHint().width()+uiGeneralTab.tbConstants->width(),-menu.sizeHint().height());
	menu.exec(uiGeneralTab.tbConstants->mapToGlobal(pos));
}

void XYFitCurveDock::showFunctions() {
	QMenu menu;
	FunctionsWidget functions(&menu);
	connect(&functions, SIGNAL(functionSelected(QString)), this, SLOT(insertFunction(QString)));
	connect(&functions, SIGNAL(functionSelected(QString)), &menu, SLOT(close()));
	connect(&functions, SIGNAL(canceled()), &menu, SLOT(close()));

	QWidgetAction* widgetAction = new QWidgetAction(this);
	widgetAction->setDefaultWidget(&functions);
	menu.addAction(widgetAction);

	QPoint pos(-menu.sizeHint().width()+uiGeneralTab.tbFunctions->width(),-menu.sizeHint().height());
	menu.exec(uiGeneralTab.tbFunctions->mapToGlobal(pos));
}

void XYFitCurveDock::showParameters() {
	QMenu menu;
	FitParametersWidget w(&menu, &m_fitData);
	connect(&w, SIGNAL(finished()), &menu, SLOT(close()));
	connect(&w, SIGNAL(parametersChanged()), this, SLOT(parametersChanged()));

	QWidgetAction* widgetAction = new QWidgetAction(this);
	widgetAction->setDefaultWidget(&w);
	menu.addAction(widgetAction);

	QPoint pos(-menu.sizeHint().width()+uiGeneralTab.pbParameters->width(),-menu.sizeHint().height());
	menu.exec(uiGeneralTab.pbParameters->mapToGlobal(pos));
}

/*!
 * called when parameter names and/or start values for the custom model were changed
 */
void XYFitCurveDock::parametersChanged() {
	//parameter names were (probably) changed -> set the new names in EquationTextEdit
	uiGeneralTab.teEquation->setVariables(m_fitData.paramNames);
	enableRecalculate();
}

void XYFitCurveDock::showOptions() {
	QMenu menu;
	FitOptionsWidget w(&menu, &m_fitData);
	connect(&w, SIGNAL(finished()), &menu, SLOT(close()));
	connect(&w, SIGNAL(optionsChanged()), this, SLOT(enableRecalculate()));

	QWidgetAction* widgetAction = new QWidgetAction(this);
	widgetAction->setDefaultWidget(&w);
	menu.addAction(widgetAction);

	QPoint pos(-menu.sizeHint().width()+uiGeneralTab.pbParameters->width(),-menu.sizeHint().height());
	menu.exec(uiGeneralTab.pbOptions->mapToGlobal(pos));
}

void XYFitCurveDock::insertFunction(const QString& str) {
	uiGeneralTab.teEquation->insertPlainText(str + "(x)");
}

void XYFitCurveDock::insertConstant(const QString& str) {
	uiGeneralTab.teEquation->insertPlainText(str);
}

void XYFitCurveDock::recalculateClicked() {
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m_fitData.degree = uiGeneralTab.sbDegree->value();
	if (m_fitData.modelType==XYFitCurve::Custom) {
		m_fitData.model = uiGeneralTab.teEquation->toPlainText();
	}

	foreach(XYCurve* curve, m_curvesList)
		dynamic_cast<XYFitCurve*>(curve)->setFitData(m_fitData);

	this->showFitResult();
	uiGeneralTab.pbRecalculate->setEnabled(false);
	QApplication::restoreOverrideCursor();
}

void XYFitCurveDock::enableRecalculate() const {
	if (m_initializing)
		return;

	//no fitting possible without the x- and y-data
	AbstractAspect* aspectX = static_cast<AbstractAspect*>(cbXDataColumn->currentModelIndex().internalPointer());
	AbstractAspect* aspectY = static_cast<AbstractAspect*>(cbYDataColumn->currentModelIndex().internalPointer());
	bool data = (aspectX!=0 && aspectY!=0);

	XYFitCurve::ModelType type = (XYFitCurve::ModelType)uiGeneralTab.cbModel->currentIndex();
	if (type==XYFitCurve::Custom)
		uiGeneralTab.pbRecalculate->setEnabled( data && uiGeneralTab.teEquation->isValid() );
	else
		uiGeneralTab.pbRecalculate->setEnabled(data);
}

/*!
 * show the result and details of fit
 */
void XYFitCurveDock::showFitResult() {
	const XYFitCurve::FitResult& fitResult = m_fitCurve->fitResult();
	if (!fitResult.available) {
		uiGeneralTab.teResult->clear();
		return;
	}

	const XYFitCurve::FitData& fitData = m_fitCurve->fitData();
	QString str = i18n("status:") + ' ' + fitResult.status + "<br>";

	if (!fitResult.valid) {
		uiGeneralTab.teResult->setText(str);
		return; //result is not valid, there was an error which is shown in the status-string, nothing to show more.
	}

	str += i18n("iterations:") + ' ' + QString::number(fitResult.iterations) + "<br>";
	if (fitResult.elapsedTime>1000)
		str += i18n("calculation time: %1 s", fitResult.elapsedTime/1000) + "<br>";
	else
		str += i18n("calculation time: %1 ms", fitResult.elapsedTime) + "<br>";

	str += i18n("degrees of freedom:") + ' ' + QString::number(fitResult.dof) + "<br><br>";

	str += "<b>" +i18n("Parameters:") + "</b>";
	for (int i=0; i<fitResult.paramValues.size(); i++) {
		str += "<br>" + fitData.paramNames.at(i) + QString(" = ") + QString::number(fitResult.paramValues.at(i))
		       + QString::fromUtf8("\u2213") + QString::number(fitResult.errorValues.at(i));
	}

	str += "<br><br><b>" + i18n("Goodness of fit:") + "</b><br>";
	str += i18n("sum of squared errors:") + ' ' + QString::number(fitResult.sse) + "<br>";
	str += i18n("mean squared error:") + ' ' + QString::number(fitResult.mse) + "<br>";
	str += i18n("root-mean squared error:") + ' ' + QString::number(fitResult.rmse) + "<br>";
	str += i18n("mean absolute error:") + ' ' + QString::number(fitResult.mae) + "<br>";

	if (fitResult.dof!=0) {
		str += i18n("residual mean square:") + ' ' + QString::number(fitResult.rms) + "<br>";
		str += i18n("residual standard deviation:") + ' ' + QString::number(fitResult.rsd) + "<br>";
	}

	str += i18n("coefficient of determination (R²):") + ' ' + QString::number(fitResult.rsquared) + "<br>";
	str += i18n("adj. coefficient of determination (R²):") + ' ' + QString::number(fitResult.rsquaredAdj) + "<br>";
// 	str += "<br><br>";
//
// 	QStringList iterations = fitResult.solverOutput.split(';');
// 	for (int i=0; i<iterations.size(); ++i)
// 		str += "<br>" + iterations.at(i);

	uiGeneralTab.teResult->setText(str);
}

//*************************************************************
//*********** SLOTs for changes triggered in XYCurve **********
//*************************************************************
//General-Tab
void XYFitCurveDock::curveDescriptionChanged(const AbstractAspect* aspect) {
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

void XYFitCurveDock::curveXDataColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromColumn(cbXDataColumn, column);
	m_initializing = false;
}

void XYFitCurveDock::curveYDataColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromColumn(cbYDataColumn, column);
	m_initializing = false;
}

void XYFitCurveDock::curveWeightsColumnChanged(const AbstractColumn* column) {
	m_initializing = true;
	XYCurveDock::setModelIndexFromColumn(cbWeightsColumn, column);
	m_initializing = false;
}

void XYFitCurveDock::curveFitDataChanged(const XYFitCurve::FitData& data) {
	m_initializing = true;
	m_fitData = data;
	uiGeneralTab.cbModel->setCurrentIndex(m_fitData.modelType);
	this->modelChanged(m_fitData.modelType);
	if (m_fitData.modelType == XYFitCurve::Custom)
		uiGeneralTab.teEquation->setPlainText(m_fitData.model);

	uiGeneralTab.sbDegree->setValue(m_fitData.degree);
	this->showFitResult();
	m_initializing = false;
}

void XYFitCurveDock::dataChanged() {
	this->enableRecalculate();
}
