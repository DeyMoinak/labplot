/*
    File                 : XYDifferentiationCurve.cpp
    Project              : LabPlot
    Description          : A xy-curve defined by an differentiation
    --------------------------------------------------------------------
    SPDX-FileCopyrightText: 2016 Stefan Gerlach <stefan.gerlach@uni.kn>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


/*!
  \class XYDifferentiationCurve
  \brief A xy-curve defined by an differentiation

  \ingroup worksheet
*/

#include "XYDifferentiationCurve.h"
#include "XYDifferentiationCurvePrivate.h"
#include "CartesianCoordinateSystem.h"
#include "backend/core/column/Column.h"
#include "backend/lib/commandtemplates.h"
#include "backend/lib/macros.h"
#include "backend/lib/XmlStreamReader.h"

extern "C" {
#include <gsl/gsl_errno.h>
}

#include <KLocalizedString>
#include <QIcon>
#include <QElapsedTimer>
#include <QThreadPool>

XYDifferentiationCurve::XYDifferentiationCurve(const QString& name)
	: XYAnalysisCurve(name, new XYDifferentiationCurvePrivate(this), AspectType::XYDifferentiationCurve) {
}

XYDifferentiationCurve::XYDifferentiationCurve(const QString& name, XYDifferentiationCurvePrivate* dd)
	: XYAnalysisCurve(name, dd, AspectType::XYDifferentiationCurve) {
}

//no need to delete the d-pointer here - it inherits from QGraphicsItem
//and is deleted during the cleanup in QGraphicsScene
XYDifferentiationCurve::~XYDifferentiationCurve() = default;

void XYDifferentiationCurve::recalculate() {
	Q_D(XYDifferentiationCurve);
	d->recalculate();
}

/*!
	Returns an icon to be used in the project explorer.
*/
QIcon XYDifferentiationCurve::icon() const {
	return QIcon::fromTheme("labplot-xy-curve");
}

//##############################################################################
//##########################  getter methods  ##################################
//##############################################################################
BASIC_SHARED_D_READER_IMPL(XYDifferentiationCurve, XYDifferentiationCurve::DifferentiationData, differentiationData, differentiationData)

const XYDifferentiationCurve::DifferentiationResult& XYDifferentiationCurve::differentiationResult() const {
	Q_D(const XYDifferentiationCurve);
	return d->differentiationResult;
}

//##############################################################################
//#################  setter methods and undo commands ##########################
//##############################################################################
STD_SETTER_CMD_IMPL_F_S(XYDifferentiationCurve, SetDifferentiationData, XYDifferentiationCurve::DifferentiationData, differentiationData, recalculate)
void XYDifferentiationCurve::setDifferentiationData(const XYDifferentiationCurve::DifferentiationData& differentiationData) {
	Q_D(XYDifferentiationCurve);
	exec(new XYDifferentiationCurveSetDifferentiationDataCmd(d, differentiationData, ki18n("%1: set options and perform the differentiation")));
}

//##############################################################################
//######################### Private implementation #############################
//##############################################################################
XYDifferentiationCurvePrivate::XYDifferentiationCurvePrivate(XYDifferentiationCurve* owner) : XYAnalysisCurvePrivate(owner), q(owner)  {
}

//no need to delete xColumn and yColumn, they are deleted
//when the parent aspect is removed
XYDifferentiationCurvePrivate::~XYDifferentiationCurvePrivate() = default;

// ...
// see XYFitCurvePrivate
void XYDifferentiationCurvePrivate::recalculate() {
	QElapsedTimer timer;
	timer.start();

	//create differentiation result columns if not available yet, clear them otherwise
	if (!xColumn) {
		xColumn = new Column("x", AbstractColumn::ColumnMode::Double);
		yColumn = new Column("y", AbstractColumn::ColumnMode::Double);
		xVector = static_cast<QVector<double>* >(xColumn->data());
		yVector = static_cast<QVector<double>* >(yColumn->data());

		xColumn->setHidden(true);
		q->addChild(xColumn);
		yColumn->setHidden(true);
		q->addChild(yColumn);

		q->setUndoAware(false);
		q->setXColumn(xColumn);
		q->setYColumn(yColumn);
		q->setUndoAware(true);
	} else {
		xVector->clear();
		yVector->clear();
	}

	// clear the previous result
	differentiationResult = XYDifferentiationCurve::DifferentiationResult();

	//determine the data source columns
	const AbstractColumn* tmpXDataColumn = nullptr;
	const AbstractColumn* tmpYDataColumn = nullptr;
	if (dataSourceType == XYAnalysisCurve::DataSourceType::Spreadsheet) {
		//spreadsheet columns as data source
		tmpXDataColumn = xDataColumn;
		tmpYDataColumn = yDataColumn;
	} else {
		//curve columns as data source
		tmpXDataColumn = dataSourceCurve->xColumn();
		tmpYDataColumn = dataSourceCurve->yColumn();
	}

	if (!tmpXDataColumn || !tmpYDataColumn) {
		Q_EMIT q->dataChanged();
		sourceDataChangedSinceLastRecalc = false;
		return;
	}

	//copy all valid data point for the differentiation to temporary vectors
	QVector<double> xdataVector;
	QVector<double> ydataVector;

	double xmin;
	double xmax;
	if (differentiationData.autoRange) {
		xmin = tmpXDataColumn->minimum();
		xmax = tmpXDataColumn->maximum();
	} else {
		xmin = differentiationData.xRange.first();
		xmax = differentiationData.xRange.last();
	}

	XYAnalysisCurve::copyData(xdataVector, ydataVector, tmpXDataColumn, tmpYDataColumn, xmin, xmax);

	//number of data points to differentiate
	const size_t n = (size_t)xdataVector.size();
	if (n < 3) {
		differentiationResult.available = true;
		differentiationResult.valid = false;
		differentiationResult.status = i18n("Not enough data points available.");
		recalcLogicalPoints();
		Q_EMIT q->dataChanged();
		sourceDataChangedSinceLastRecalc = false;
		return;
	}

	double* xdata = xdataVector.data();
	double* ydata = ydataVector.data();

	// differentiation settings
	const nsl_diff_deriv_order_type derivOrder = differentiationData.derivOrder;
	const int accOrder = differentiationData.accOrder;

	DEBUG(nsl_diff_deriv_order_name[derivOrder] << "derivative");
	DEBUG("accuracy order:" << accOrder);

///////////////////////////////////////////////////////////
	int status = 0;

	switch (derivOrder) {
	case nsl_diff_deriv_order_first:
		status = nsl_diff_first_deriv(xdata, ydata, n, accOrder);
		break;
	case nsl_diff_deriv_order_second:
		status = nsl_diff_second_deriv(xdata, ydata, n, accOrder);
		break;
	case nsl_diff_deriv_order_third:
		status = nsl_diff_third_deriv(xdata, ydata, n, accOrder);
		break;
	case nsl_diff_deriv_order_fourth:
		status = nsl_diff_fourth_deriv(xdata, ydata, n, accOrder);
		break;
	case nsl_diff_deriv_order_fifth:
		status = nsl_diff_fifth_deriv(xdata, ydata, n, accOrder);
		break;
	case nsl_diff_deriv_order_sixth:
		status = nsl_diff_sixth_deriv(xdata, ydata, n, accOrder);
		break;
	}

	xVector->resize((int)n);
	yVector->resize((int)n);
	memcpy(xVector->data(), xdata, n * sizeof(double));
	memcpy(yVector->data(), ydata, n * sizeof(double));
///////////////////////////////////////////////////////////

	//write the result
	differentiationResult.available = true;
	differentiationResult.valid = true;
	differentiationResult.status = QString::number(status);
	differentiationResult.elapsedTime = timer.elapsed();

	//redraw the curve
	recalcLogicalPoints();
	Q_EMIT q->dataChanged();
	sourceDataChangedSinceLastRecalc = false;
}

//##############################################################################
//##################  Serialization/Deserialization  ###########################
//##############################################################################
//! Save as XML
void XYDifferentiationCurve::save(QXmlStreamWriter* writer) const{
	Q_D(const XYDifferentiationCurve);

	writer->writeStartElement("xyDifferentiationCurve");

	//write the base class
	XYAnalysisCurve::save(writer);

	//write xy-differentiation-curve specific information
	// differentiation data
	writer->writeStartElement("differentiationData");
	writer->writeAttribute( "derivOrder", QString::number(d->differentiationData.derivOrder) );
	writer->writeAttribute( "accOrder", QString::number(d->differentiationData.accOrder) );
	writer->writeAttribute( "autoRange", QString::number(d->differentiationData.autoRange) );
	writer->writeAttribute( "xRangeMin", QString::number(d->differentiationData.xRange.first()) );
	writer->writeAttribute( "xRangeMax", QString::number(d->differentiationData.xRange.last()) );
	writer->writeEndElement();// differentiationData

	// differentiation results (generated columns)
	writer->writeStartElement("differentiationResult");
	writer->writeAttribute( "available", QString::number(d->differentiationResult.available) );
	writer->writeAttribute( "valid", QString::number(d->differentiationResult.valid) );
	writer->writeAttribute( "status", d->differentiationResult.status );
	writer->writeAttribute( "time", QString::number(d->differentiationResult.elapsedTime) );

	//save calculated columns if available
	if (saveCalculations() && d->xColumn) {
		d->xColumn->save(writer);
		d->yColumn->save(writer);
	}
	writer->writeEndElement(); //"differentiationResult"

	writer->writeEndElement(); //"xyDifferentiationCurve"
}

//! Load from XML
bool XYDifferentiationCurve::load(XmlStreamReader* reader, bool preview) {
	Q_D(XYDifferentiationCurve);

	KLocalizedString attributeWarning = ki18n("Attribute '%1' missing or empty, default value is used");
	QXmlStreamAttributes attribs;
	QString str;

	while (!reader->atEnd()) {
		reader->readNext();
		if (reader->isEndElement() && reader->name() == "xyDifferentiationCurve")
			break;

		if (!reader->isStartElement())
			continue;

		if (reader->name() == "xyAnalysisCurve") {
			if ( !XYAnalysisCurve::load(reader, preview) )
				return false;
		} else if (!preview && reader->name() == "differentiationData") {
			attribs = reader->attributes();
			READ_INT_VALUE("autoRange", differentiationData.autoRange, bool);
			READ_DOUBLE_VALUE("xRangeMin", differentiationData.xRange.first());
			READ_DOUBLE_VALUE("xRangeMax", differentiationData.xRange.last());
			READ_INT_VALUE("derivOrder", differentiationData.derivOrder, nsl_diff_deriv_order_type);
			READ_INT_VALUE("accOrder", differentiationData.accOrder, int);
		} else if (!preview && reader->name() == "differentiationResult") {
			attribs = reader->attributes();
			READ_INT_VALUE("available", differentiationResult.available, int);
			READ_INT_VALUE("valid", differentiationResult.valid, int);
			READ_STRING_VALUE("status", differentiationResult.status);
			READ_INT_VALUE("time", differentiationResult.elapsedTime, int);
		} else if (reader->name() == "column") {
			Column* column = new Column(QString(), AbstractColumn::ColumnMode::Double);
			if (!column->load(reader, preview)) {
				delete column;
				return false;
			}
			if (column->name() == "x")
				d->xColumn = column;
			else if (column->name() == "y")
				d->yColumn = column;
		}
	}

	if (preview)
		return true;

	// wait for data to be read before using the pointers
	QThreadPool::globalInstance()->waitForDone();

	if (d->xColumn && d->yColumn) {
		d->xColumn->setHidden(true);
		addChild(d->xColumn);

		d->yColumn->setHidden(true);
		addChild(d->yColumn);

		d->xVector = static_cast<QVector<double>* >(d->xColumn->data());
		d->yVector = static_cast<QVector<double>* >(d->yColumn->data());

		static_cast<XYCurvePrivate*>(d_ptr)->xColumn = d->xColumn;
		static_cast<XYCurvePrivate*>(d_ptr)->yColumn = d->yColumn;

		recalcLogicalPoints();
	}

	return true;
}
