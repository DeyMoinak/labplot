/***************************************************************************
    File                 : XYSmoothCurve.cpp
    Project              : LabPlot
    Description          : A xy-curve defined by a smooth
    --------------------------------------------------------------------
    Copyright            : (C) 2016 Stefan Gerlach (stefan.gerlach@uni.kn)

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

/*!
  \class XYSmoothCurve
  \brief A xy-curve defined by a smooth

  \ingroup worksheet
*/

#include "XYSmoothCurve.h"
#include "XYSmoothCurvePrivate.h"
#include "backend/core/column/Column.h"
#include "backend/lib/commandtemplates.h"

#include <KIcon>
#include <KLocale>
#include <QElapsedTimer>
#include <QThreadPool>
#ifndef NDEBUG
#include <QDebug>
#endif

extern "C" {
#include <gsl/gsl_math.h>	// gsl_pow_*
#include "backend/nsl/nsl_stats.h"
#include "backend/nsl/nsl_sf_kernel.h"
}

XYSmoothCurve::XYSmoothCurve(const QString& name)
		: XYCurve(name, new XYSmoothCurvePrivate(this)) {
	init();
}

XYSmoothCurve::XYSmoothCurve(const QString& name, XYSmoothCurvePrivate* dd)
		: XYCurve(name, dd) {
	init();
}


XYSmoothCurve::~XYSmoothCurve() {
	//no need to delete the d-pointer here - it inherits from QGraphicsItem
	//and is deleted during the cleanup in QGraphicsScene
}

void XYSmoothCurve::init() {
	Q_D(XYSmoothCurve);

	//TODO: read from the saved settings for XYSmoothCurve?
	d->lineType = XYCurve::Line;
	d->symbolsStyle = Symbol::NoSymbols;
}

void XYSmoothCurve::recalculate() {
	Q_D(XYSmoothCurve);
	d->recalculate();
}

/*!
	Returns an icon to be used in the project explorer.
*/
QIcon XYSmoothCurve::icon() const {
	return KIcon("labplot-xy-smooth-curve");
}

//##############################################################################
//##########################  getter methods  ##################################
//##############################################################################
BASIC_SHARED_D_READER_IMPL(XYSmoothCurve, const AbstractColumn*, xDataColumn, xDataColumn)
BASIC_SHARED_D_READER_IMPL(XYSmoothCurve, const AbstractColumn*, yDataColumn, yDataColumn)
const QString& XYSmoothCurve::xDataColumnPath() const { Q_D(const XYSmoothCurve); return d->xDataColumnPath; }
const QString& XYSmoothCurve::yDataColumnPath() const { Q_D(const XYSmoothCurve); return d->yDataColumnPath; }

BASIC_SHARED_D_READER_IMPL(XYSmoothCurve, XYSmoothCurve::SmoothData, smoothData, smoothData)

const XYSmoothCurve::SmoothResult& XYSmoothCurve::smoothResult() const {
	Q_D(const XYSmoothCurve);
	return d->smoothResult;
}

bool XYSmoothCurve::isSourceDataChangedSinceLastSmooth() const {
	Q_D(const XYSmoothCurve);
	return d->sourceDataChangedSinceLastSmooth;
}

//##############################################################################
//#################  setter methods and undo commands ##########################
//##############################################################################
STD_SETTER_CMD_IMPL_S(XYSmoothCurve, SetXDataColumn, const AbstractColumn*, xDataColumn)
void XYSmoothCurve::setXDataColumn(const AbstractColumn* column) {
	Q_D(XYSmoothCurve);
	if (column != d->xDataColumn) {
		exec(new XYSmoothCurveSetXDataColumnCmd(d, column, i18n("%1: assign x-data")));
		emit sourceDataChangedSinceLastSmooth();
		if (column) {
			connect(column, SIGNAL(dataChanged(const AbstractColumn*)), this, SLOT(handleSourceDataChanged()));
			//TODO disconnect on undo
		}
	}
}

STD_SETTER_CMD_IMPL_S(XYSmoothCurve, SetYDataColumn, const AbstractColumn*, yDataColumn)
void XYSmoothCurve::setYDataColumn(const AbstractColumn* column) {
	Q_D(XYSmoothCurve);
	if (column != d->yDataColumn) {
		exec(new XYSmoothCurveSetYDataColumnCmd(d, column, i18n("%1: assign y-data")));
		emit sourceDataChangedSinceLastSmooth();
		if (column) {
			connect(column, SIGNAL(dataChanged(const AbstractColumn*)), this, SLOT(handleSourceDataChanged()));
			//TODO disconnect on undo
		}
	}
}

STD_SETTER_CMD_IMPL_F_S(XYSmoothCurve, SetSmoothData, XYSmoothCurve::SmoothData, smoothData, recalculate);
void XYSmoothCurve::setSmoothData(const XYSmoothCurve::SmoothData& smoothData) {
	Q_D(XYSmoothCurve);
	exec(new XYSmoothCurveSetSmoothDataCmd(d, smoothData, i18n("%1: set options and perform the smooth")));
}

//##############################################################################
//################################## SLOTS ####################################
//##############################################################################
void XYSmoothCurve::handleSourceDataChanged() {
	Q_D(XYSmoothCurve);
	d->sourceDataChangedSinceLastSmooth = true;
	emit sourceDataChangedSinceLastSmooth();
}
//##############################################################################
//######################### Private implementation #############################
//##############################################################################
XYSmoothCurvePrivate::XYSmoothCurvePrivate(XYSmoothCurve* owner) : XYCurvePrivate(owner),
	xDataColumn(0), yDataColumn(0), 
	xColumn(0), yColumn(0), 
	xVector(0), yVector(0), 
	sourceDataChangedSinceLastSmooth(false),
	q(owner)  {

}

XYSmoothCurvePrivate::~XYSmoothCurvePrivate() {
	//no need to delete xColumn and yColumn, they are deleted
	//when the parent aspect is removed
}

// ...
// see XYFitCurvePrivate

void XYSmoothCurvePrivate::recalculate() {
	QElapsedTimer timer;
	timer.start();

	//create smooth result columns if not available yet, clear them otherwise
	if (!xColumn) {
		xColumn = new Column("x", AbstractColumn::Numeric);
		yColumn = new Column("y", AbstractColumn::Numeric);
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
	smoothResult = XYSmoothCurve::SmoothResult();

	if (!xDataColumn || !yDataColumn) {
		emit (q->dataChanged());
		sourceDataChangedSinceLastSmooth = false;
		return;
	}

	//check column sizes
	if (xDataColumn->rowCount()!=yDataColumn->rowCount()) {
		smoothResult.available = true;
		smoothResult.valid = false;
		smoothResult.status = i18n("Number of x and y data points must be equal.");
		emit (q->dataChanged());
		sourceDataChangedSinceLastSmooth = false;
		return;
	}

	//copy all valid data point for the smooth to temporary vectors
	QVector<double> xdataVector;
	QVector<double> ydataVector;
	for (int row=0; row<xDataColumn->rowCount(); ++row) {
		//only copy those data where _all_ values (for x and y, if given) are valid
		if (!std::isnan(xDataColumn->valueAt(row)) && !std::isnan(yDataColumn->valueAt(row))
			&& !xDataColumn->isMasked(row) && !yDataColumn->isMasked(row)) {

			xdataVector.append(xDataColumn->valueAt(row));
			ydataVector.append(yDataColumn->valueAt(row));
		}
	}

	//number of data points to smooth
	const unsigned int n = ydataVector.size();
	if (n < 2) {
		smoothResult.available = true;
		smoothResult.valid = false;
		smoothResult.status = i18n("Not enough data points available.");
		emit (q->dataChanged());
		sourceDataChangedSinceLastSmooth = false;
		return;
	}

	double* xdata = xdataVector.data();
	double* ydata = ydataVector.data();

	// smooth settings
	const nsl_smooth_type type = smoothData.type;
	const unsigned int points = smoothData.points;
	const nsl_smooth_weight_type weight = smoothData.weight;
	const double percentile = smoothData.percentile;
	const unsigned int order = smoothData.order;
	const nsl_smooth_pad_mode mode = smoothData.mode;
	const double lvalue = smoothData.lvalue;
	const double rvalue = smoothData.rvalue;
#ifndef NDEBUG
	qDebug()<<"type:"<<nsl_smooth_type_name[type];
	qDebug()<<"points ="<<points;
	qDebug()<<"weight:"<<nsl_smooth_weight_type_name[weight];
	qDebug()<<"percentile ="<<percentile;
	qDebug()<<"order ="<<order;
	qDebug()<<"mode ="<<nsl_smooth_pad_mode_name[mode];
	qDebug()<<"const. values ="<<lvalue<<rvalue;
#endif
///////////////////////////////////////////////////////////
	int status=0;

	switch (type) {
	case nsl_smooth_type_moving_average:
		status = nsl_smooth_moving_average(ydata, n, points, weight, mode);
		break;
	case nsl_smooth_type_moving_average_lagged:
		status = nsl_smooth_moving_average_lagged(ydata, n, points, weight, mode);
		break;
	case nsl_smooth_type_percentile:
		status = nsl_smooth_percentile(ydata, n, points, percentile, mode);
		break;
	case nsl_smooth_type_savitzky_golay:
		if (mode == nsl_smooth_pad_constant)
			nsl_smooth_pad_constant_set(lvalue, rvalue);
		status = nsl_smooth_savgol(ydata, n, points, order, mode);
		break;
	}

	xVector->resize(n);
	yVector->resize(n);
	memcpy(xVector->data(), xdata, n*sizeof(double));
	memcpy(yVector->data(), ydata, n*sizeof(double));

///////////////////////////////////////////////////////////

	//write the result
	smoothResult.available = true;
	smoothResult.valid = true;
	smoothResult.status = QString::number(status);
	smoothResult.elapsedTime = timer.elapsed();

	//redraw the curve
	emit (q->dataChanged());
	sourceDataChangedSinceLastSmooth = false;
}

//##############################################################################
//##################  Serialization/Deserialization  ###########################
//##############################################################################
//! Save as XML
void XYSmoothCurve::save(QXmlStreamWriter* writer) const{
	Q_D(const XYSmoothCurve);

	writer->writeStartElement("xySmoothCurve");

	//write xy-curve information
	XYCurve::save(writer);

	//write xy-smooth-curve specific information
	// smooth data
	writer->writeStartElement("smoothData");
	WRITE_COLUMN(d->xDataColumn, xDataColumn);
	WRITE_COLUMN(d->yDataColumn, yDataColumn);
	writer->writeAttribute( "type", QString::number(d->smoothData.type) );
	writer->writeAttribute( "points", QString::number(d->smoothData.points) );
	writer->writeAttribute( "weight", QString::number(d->smoothData.weight) );
	writer->writeAttribute( "percentile", QString::number(d->smoothData.percentile) );
	writer->writeAttribute( "order", QString::number(d->smoothData.order) );
	writer->writeAttribute( "mode", QString::number(d->smoothData.mode) );
	writer->writeAttribute( "lvalue", QString::number(d->smoothData.lvalue) );
	writer->writeAttribute( "rvalue", QString::number(d->smoothData.rvalue) );
	writer->writeEndElement();// smoothData

	// smooth results (generated columns)
	writer->writeStartElement("smoothResult");
	writer->writeAttribute( "available", QString::number(d->smoothResult.available) );
	writer->writeAttribute( "valid", QString::number(d->smoothResult.valid) );
	writer->writeAttribute( "status", d->smoothResult.status );
	writer->writeAttribute( "time", QString::number(d->smoothResult.elapsedTime) );

	//save calculated columns if available
	if (d->xColumn) {
		d->xColumn->save(writer);
		d->yColumn->save(writer);
	}
	writer->writeEndElement(); //"smoothResult"

	writer->writeEndElement(); //"xySmoothCurve"
}

//! Load from XML
bool XYSmoothCurve::load(XmlStreamReader* reader) {
	Q_D(XYSmoothCurve);

	if (!reader->isStartElement() || reader->name() != "xySmoothCurve") {
		reader->raiseError(i18n("no xy smooth curve element found"));
		return false;
	}

	QString attributeWarning = i18n("Attribute '%1' missing or empty, default value is used");
	QXmlStreamAttributes attribs;
	QString str;

	while (!reader->atEnd()) {
		reader->readNext();
		if (reader->isEndElement() && reader->name() == "xySmoothCurve")
			break;

		if (!reader->isStartElement())
			continue;

		if (reader->name() == "xyCurve") {
			if ( !XYCurve::load(reader) )
				return false;
		} else if (reader->name() == "smoothData") {
			attribs = reader->attributes();

			READ_COLUMN(xDataColumn);
			READ_COLUMN(yDataColumn);

			str = attribs.value("type").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'type'"));
			else
				d->smoothData.type = (nsl_smooth_type) str.toInt();

			str = attribs.value("points").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'points'"));
			else
				d->smoothData.points = str.toInt();

			str = attribs.value("weight").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'weight'"));
			else
				d->smoothData.weight = (nsl_smooth_weight_type) str.toInt();

			str = attribs.value("percentile").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'percentile'"));
			else
				d->smoothData.percentile = str.toDouble();

			str = attribs.value("order").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'order'"));
			else
				d->smoothData.order = str.toInt();

			str = attribs.value("mode").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'mode'"));
			else
				d->smoothData.mode = (nsl_smooth_pad_mode) str.toInt();

			str = attribs.value("lvalue").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'lvalue'"));
			else
				d->smoothData.lvalue = str.toDouble();

			str = attribs.value("rvalue").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'rvalue'"));
			else
				d->smoothData.rvalue = str.toDouble();
		} else if (reader->name() == "smoothResult") {

			attribs = reader->attributes();

			str = attribs.value("available").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'available'"));
			else
				d->smoothResult.available = str.toInt();

			str = attribs.value("valid").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'valid'"));
			else
				d->smoothResult.valid = str.toInt();
			
			str = attribs.value("status").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'status'"));
			else
				d->smoothResult.status = str;

			str = attribs.value("time").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'time'"));
			else
				d->smoothResult.elapsedTime = str.toInt();
		} else if (reader->name() == "column") {
			Column* column = new Column("", AbstractColumn::Numeric);
			if (!column->load(reader)) {
				delete column;
				return false;
			}
			if (column->name()=="x")
				d->xColumn = column;
			else if (column->name()=="y")
				d->yColumn = column;
		}
	}

	// wait for data to be read before using the pointers
	QThreadPool::globalInstance()->waitForDone();

	if (d->xColumn && d->yColumn) {
		d->xColumn->setHidden(true);
		addChild(d->xColumn);

		d->yColumn->setHidden(true);
		addChild(d->yColumn);

		d->xVector = static_cast<QVector<double>* >(d->xColumn->data());
		d->yVector = static_cast<QVector<double>* >(d->yColumn->data());

		setUndoAware(false);
		XYCurve::d_ptr->xColumn = d->xColumn;
		XYCurve::d_ptr->yColumn = d->yColumn;
		setUndoAware(true);
	}

	return true;
}
