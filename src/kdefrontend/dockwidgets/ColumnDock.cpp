/***************************************************************************
    File                 : ColumnDock.cpp
    Project              : LabPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2011-2017 by Alexander Semke (alexander.semke@web.de)
    Copyright            : (C) 2013-2017 by Stefan Gerlach (stefan.gerlach@uni.kn)
    Description          : widget for column properties

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

#include "ColumnDock.h"

#include "backend/core/AbstractFilter.h"
#include "backend/core/datatypes/SimpleCopyThroughFilter.h"
#include "backend/core/datatypes/Double2StringFilter.h"
#include "backend/core/datatypes/String2DoubleFilter.h"
#include "backend/core/datatypes/DateTime2StringFilter.h"
#include "backend/core/datatypes/String2DateTimeFilter.h"
#include "backend/datasources/FileDataSource.h"
#include "backend/spreadsheet/Spreadsheet.h"

#include <KLocalizedString>

/*!
  \class ColumnDock
  \brief Provides a widget for editing the properties of the spreadsheet columns currently selected in the project explorer.

  \ingroup kdefrontend
*/

ColumnDock::ColumnDock(QWidget* parent) : QWidget(parent), m_column(0), m_initializing(false) {
	ui.setupUi(this);

	connect(ui.leName, SIGNAL(returnPressed()), this, SLOT(nameChanged()));
	connect(ui.leComment, SIGNAL(returnPressed()), this, SLOT(commentChanged()));
	connect(ui.cbType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui.cbFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(formatChanged(int)));
	connect(ui.sbPrecision, SIGNAL(valueChanged(int)), this, SLOT(precisionChanged(int)) );
	connect(ui.cbPlotDesignation, SIGNAL(currentIndexChanged(int)), this, SLOT(plotDesignationChanged(int)));

	retranslateUi();
}

void ColumnDock::setColumns(QList<Column*> list) {
	m_initializing = true;
	m_columnsList = list;
	m_column = list.first();

	//check whether we have non-editable columns (e.g. columns for residuals calculated in XYFitCurve)
	bool nonEditable = false;
	for (auto* col: m_columnsList) {
		Spreadsheet* s = dynamic_cast<Spreadsheet*>(col->parentAspect());
		if (s) {
			if (dynamic_cast<FileDataSource*>(s)) {
				nonEditable = true;
				break;
			}
		} else {
			nonEditable = true;
			break;
		}
	}

	if (list.size() == 1) {
		//names and comments of non-editable columns in a file data source can be changed.
		if (!nonEditable && dynamic_cast<FileDataSource*>(m_column->parentAspect()) != 0) {
			ui.leName->setEnabled(false);
			ui.leComment->setEnabled(false);
		} else {
			ui.leName->setEnabled(true);
			ui.leComment->setEnabled(true);
		}

		ui.leName->setText(m_column->name());
		ui.leComment->setText(m_column->comment());
	} else {
		ui.leName->setEnabled(false);
		ui.leComment->setEnabled(false);

		ui.leName->setText("");
		ui.leComment->setText("");
	}

	//show the properties of the first column
	AbstractColumn::ColumnMode columnMode = m_column->columnMode();
	ui.cbType->setCurrentIndex(ui.cbType->findData((int)columnMode));

	//disable widgets if we have at least one non-editable column
	ui.cbType->setEnabled(!nonEditable);
	ui.lFormat->setVisible(!nonEditable);
	ui.cbFormat->setVisible(!nonEditable);
	ui.lPrecision->setVisible(!nonEditable);
	ui.sbPrecision->setVisible(!nonEditable);
	ui.lPlotDesignation->setVisible(!nonEditable);
	ui.cbPlotDesignation->setVisible(!nonEditable);
	if (nonEditable) {
		m_initializing = false;
		return;
	}

	this->updateFormatWidgets(columnMode);

	switch(columnMode) {
	case AbstractColumn::Numeric: {
			Double2StringFilter* filter = static_cast<Double2StringFilter*>(m_column->outputFilter());
			ui.cbFormat->setCurrentIndex(ui.cbFormat->findData(filter->numericFormat()));
			ui.sbPrecision->setValue(filter->numDigits());
			break;
		}
	case AbstractColumn::Month:
	case AbstractColumn::Day:
	case AbstractColumn::DateTime: {
			DateTime2StringFilter* filter = static_cast<DateTime2StringFilter*>(m_column->outputFilter());
			DEBUG("	set column format: " << filter->format().toStdString());
			ui.cbFormat->setCurrentIndex(ui.cbFormat->findData(filter->format()));
			break;
		}
	case AbstractColumn::Integer:	// nothing to set
	case AbstractColumn::Text:
		break;
	}

	ui.cbPlotDesignation->setCurrentIndex( int(m_column->plotDesignation()) );

	// slots
	connect(m_column, SIGNAL(aspectDescriptionChanged(const AbstractAspect*)),this, SLOT(columnDescriptionChanged(const AbstractAspect*)));
	connect(m_column->outputFilter(), SIGNAL(formatChanged()),this, SLOT(columnFormatChanged()));
	connect(m_column->outputFilter(), SIGNAL(digitsChanged()),this, SLOT(columnPrecisionChanged()));
	connect(m_column, SIGNAL(plotDesignationChanged(const AbstractColumn*)),this, SLOT(columnPlotDesignationChanged(const AbstractColumn*)));

	m_initializing = false;
}

/*!
  depending on the currently selected column type (column mode) updates the widgets for the column format,
  shows/hides the allowed widgets, fills the corresponding combobox with the possible entries.
  Called when the type (column mode) is changed.
*/
void ColumnDock::updateFormatWidgets(const AbstractColumn::ColumnMode columnMode) {
	ui.cbFormat->clear();

	switch (columnMode) {
	case AbstractColumn::Numeric:
		ui.cbFormat->addItem(i18n("Decimal"), QVariant('f'));
		ui.cbFormat->addItem(i18n("Scientific (e)"), QVariant('e'));
		ui.cbFormat->addItem(i18n("Scientific (E)"), QVariant('E'));
		ui.cbFormat->addItem(i18n("Automatic (g)"), QVariant('g'));
		ui.cbFormat->addItem(i18n("Automatic (G)"), QVariant('G'));
		break;
	case AbstractColumn::Month:
		ui.cbFormat->addItem(i18n("Number without leading zero"), QVariant("M"));
		ui.cbFormat->addItem(i18n("Number with leading zero"), QVariant("MM"));
		ui.cbFormat->addItem(i18n("Abbreviated month name"), QVariant("MMM"));
		ui.cbFormat->addItem(i18n("Full month name"), QVariant("MMMM"));
		break;
	case AbstractColumn::Day:
		ui.cbFormat->addItem(i18n("Number without leading zero"), QVariant("d"));
		ui.cbFormat->addItem(i18n("Number with leading zero"), QVariant("dd"));
		ui.cbFormat->addItem(i18n("Abbreviated day name"), QVariant("ddd"));
		ui.cbFormat->addItem(i18n("Full day name"), QVariant("dddd"));
		break;
	case AbstractColumn::DateTime:
			for (const auto& s: AbstractColumn::dateTimeFormats())
				ui.cbFormat->addItem(s, QVariant(s));
			break;
	case AbstractColumn::Integer:
	case AbstractColumn::Text:
		break;
	}

	if (columnMode == AbstractColumn::Numeric) {
		ui.lPrecision->show();
		ui.sbPrecision->show();
	} else {
		ui.lPrecision->hide();
		ui.sbPrecision->hide();
	}

	if (columnMode == AbstractColumn::Text || columnMode == AbstractColumn::Integer) {
		ui.lFormat->hide();
		ui.cbFormat->hide();
	} else {
		ui.lFormat->show();
		ui.cbFormat->show();
	}

	if (columnMode == AbstractColumn::DateTime) {
		ui.cbFormat->setEditable(true);
		ui.cbFormat->setCurrentItem("yyyy-MM-dd hh:mm:ss.zzz");
	} else {
		ui.cbFormat->setEditable(false);
		ui.cbFormat->setCurrentIndex(0);
	}
}

//*************************************************************
//******** SLOTs for changes triggered in ColumnDock **********
//*************************************************************
void ColumnDock::retranslateUi() {
	m_initializing = true;

	ui.cbType->clear();
	ui.cbType->addItem(i18n("Numeric"), QVariant(int(AbstractColumn::Numeric)));
	ui.cbType->addItem(i18n("Integer"), QVariant(int(AbstractColumn::Integer)));
	ui.cbType->addItem(i18n("Text"), QVariant(int(AbstractColumn::Text)));
	ui.cbType->addItem(i18n("Month names"), QVariant(int(AbstractColumn::Month)));
	ui.cbType->addItem(i18n("Day names"), QVariant(int(AbstractColumn::Day)));
	ui.cbType->addItem(i18n("Date and time"), QVariant(int(AbstractColumn::DateTime)));

	ui.cbPlotDesignation->clear();
	ui.cbPlotDesignation->addItem(i18n("none"));
	ui.cbPlotDesignation->addItem(i18n("X"));
	ui.cbPlotDesignation->addItem(i18n("Y"));
	ui.cbPlotDesignation->addItem(i18n("Z"));
	ui.cbPlotDesignation->addItem(i18n("X-error"));
	ui.cbPlotDesignation->addItem(i18n("X-error -"));
	ui.cbPlotDesignation->addItem(i18n("X-error +"));
	ui.cbPlotDesignation->addItem(i18n("Y-error"));
	ui.cbPlotDesignation->addItem(i18n("Y-error -"));
	ui.cbPlotDesignation->addItem(i18n("Y-error +"));

	m_initializing = false;
}

void ColumnDock::nameChanged() {
	if (m_initializing)
		return;

	m_columnsList.first()->setName(ui.leName->text());
}

void ColumnDock::commentChanged() {
	if (m_initializing)
		return;

	m_columnsList.first()->setComment(ui.leComment->text());
}

/*!
  called when the type (column mode - numeric, text etc.) of the column was changed.
*/
void ColumnDock::typeChanged(int index) {
	DEBUG("ColumnDock::typeChanged()");
	if (m_initializing)
		return;

	AbstractColumn::ColumnMode columnMode = (AbstractColumn::ColumnMode)ui.cbType->itemData(index).toInt();
	int format_index = ui.cbFormat->currentIndex();

	m_initializing = true;
	this->updateFormatWidgets(columnMode);
	m_initializing = false;

	switch(columnMode) {
	case AbstractColumn::Numeric: {
		int digits = ui.sbPrecision->value();
		for (auto* col: m_columnsList) {
			col->beginMacro(i18n("%1: change column type", col->name()));
			col->setColumnMode(columnMode);
			Double2StringFilter* filter = static_cast<Double2StringFilter*>(col->outputFilter());
			filter->setNumericFormat(ui.cbFormat->itemData(format_index).toChar().toLatin1());
			filter->setNumDigits(digits);
			col->endMacro();
		}
		break;
	}
	case AbstractColumn::Integer:
	case AbstractColumn::Text:
		for (auto* col: m_columnsList)
			col->setColumnMode(columnMode);
		break;
	case AbstractColumn::Month:
	case AbstractColumn::Day:
		for (auto* col: m_columnsList) {
			col->beginMacro(i18n("%1: change column type", col->name()));
			// the format is saved as item data
			QString format = ui.cbFormat->itemData(ui.cbFormat->currentIndex()).toString();
			col->setColumnMode(columnMode);
			DateTime2StringFilter* filter = static_cast<DateTime2StringFilter*>(col->outputFilter());
			filter->setFormat(format);
			col->endMacro();
		}
		break;
	case AbstractColumn::DateTime:
		for (auto* col: m_columnsList) {
			col->beginMacro(i18n("%1: change column type", col->name()));
			// the format is the current text
			QString format = ui.cbFormat->currentText();
			col->setColumnMode(columnMode);
			DateTime2StringFilter* filter = static_cast<DateTime2StringFilter*>(col->outputFilter());
			filter->setFormat(format);
			col->endMacro();
		}
		break;
	}
	DEBUG("ColumnDock::typeChanged() DONE");
}

/*!
  called when the format for the current type (column mode) was changed.
*/
void ColumnDock::formatChanged(int index) {
	DEBUG("ColumnDock::formatChanged()");
	if (m_initializing)
		return;

	AbstractColumn::ColumnMode mode = (AbstractColumn::ColumnMode)ui.cbType->itemData(ui.cbType->currentIndex()).toInt();
	int format_index = index;

	switch(mode) {
	case AbstractColumn::Numeric: {
		for (auto* col: m_columnsList) {
			Double2StringFilter* filter = static_cast<Double2StringFilter*>(col->outputFilter());
			filter->setNumericFormat(ui.cbFormat->itemData(format_index).toChar().toLatin1());
		}
		break;
	}
	case AbstractColumn::Integer:
	case AbstractColumn::Text:
		break;
	case AbstractColumn::Month:
	case AbstractColumn::Day:
	case AbstractColumn::DateTime: {
		QString format = ui.cbFormat->itemData(ui.cbFormat->currentIndex()).toString();
		for (auto* col: m_columnsList) {
			DateTime2StringFilter* filter = static_cast<DateTime2StringFilter*>(col->outputFilter());
			filter->setFormat(format);
		}
		break;
	}
	}
	DEBUG("ColumnDock::formatChanged() DONE");
}

void ColumnDock::precisionChanged(int digits) {
	if (m_initializing)
		return;

	for (auto* col: m_columnsList) {
		Double2StringFilter* filter = static_cast<Double2StringFilter*>(col->outputFilter());
		filter->setNumDigits(digits);
	}
}

void ColumnDock::plotDesignationChanged(int index) {
	if (m_initializing)
		return;

	AbstractColumn::PlotDesignation pd = AbstractColumn::PlotDesignation(index);
	for (auto* col: m_columnsList)
		col->setPlotDesignation(pd);
}

//*************************************************************
//********* SLOTs for changes triggered in Column *************
//*************************************************************
void ColumnDock::columnDescriptionChanged(const AbstractAspect* aspect) {
	if (m_column != aspect)
		return;

	m_initializing = true;
	if (aspect->name() != ui.leName->text())
		ui.leName->setText(aspect->name());
	else if (aspect->comment() != ui.leComment->text())
		ui.leComment->setText(aspect->comment());
	m_initializing = false;
}

void ColumnDock::columnFormatChanged() {
	DEBUG("ColumnDock::columnFormatChanged()");
	m_initializing = true;
	AbstractColumn::ColumnMode columnMode = m_column->columnMode();
	switch(columnMode) {
	case AbstractColumn::Numeric: {
		Double2StringFilter* filter = static_cast<Double2StringFilter*>(m_column->outputFilter());
		ui.cbFormat->setCurrentIndex(ui.cbFormat->findData(filter->numericFormat()));
		break;
	}
	case AbstractColumn::Integer:
	case AbstractColumn::Text:
		break;
	case AbstractColumn::Month:
	case AbstractColumn::Day:
	case AbstractColumn::DateTime: {
		DateTime2StringFilter* filter = static_cast<DateTime2StringFilter*>(m_column->outputFilter());
		ui.cbFormat->setCurrentIndex(ui.cbFormat->findData(filter->format()));
		break;
	}
	}
	m_initializing = false;
}

void ColumnDock::columnPrecisionChanged() {
	m_initializing = true;
	Double2StringFilter* filter = static_cast<Double2StringFilter*>(m_column->outputFilter());
	ui.sbPrecision->setValue(filter->numDigits());
	m_initializing = false;
}

void ColumnDock::columnPlotDesignationChanged(const AbstractColumn* col) {
	m_initializing = true;
	ui.cbPlotDesignation->setCurrentIndex( int(col->plotDesignation()) );
	m_initializing = false;
}
