/*
    File                 : SpreadsheetModel.cpp
    Project              : LabPlot
    Description          : Model for the access to a Spreadsheet
    --------------------------------------------------------------------
    SPDX-FileCopyrightText: 2007 Tilman Benkert <thzs@gmx.net>
    SPDX-FileCopyrightText: 2009 Knut Franke <knut.franke@gmx.de>
    SPDX-FileCopyrightText: 2013-2021 Alexander Semke <alexander.semke@web.de>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "backend/spreadsheet/Spreadsheet.h"
#include "backend/spreadsheet/SpreadsheetModel.h"
#include "backend/core/datatypes/Double2StringFilter.h"
#include "backend/lib/macros.h"
#include "backend/lib/trace.h"

#include <QBrush>
#include <QIcon>
#include <QPalette>

#include <KLocalizedString>

/*!
	\class SpreadsheetModel
	\brief  Model for the access to a Spreadsheet

	This is a model in the sense of Qt4 model/view framework which is used
	to access a Spreadsheet object from any of Qt4s view classes, typically a QTableView.
	Its main purposes are translating Spreadsheet signals into QAbstractItemModel signals
	and translating calls to the QAbstractItemModel read/write API into calls
	in the public API of Spreadsheet. In many cases a pointer to the addressed column
	is obtained by calling Spreadsheet::column() and the manipulation is done using the
	public API of column.

	\ingroup backend
*/
SpreadsheetModel::SpreadsheetModel(Spreadsheet* spreadsheet) : QAbstractItemModel(nullptr),
	m_spreadsheet(spreadsheet),
	m_rowCount(spreadsheet->rowCount()),
	m_columnCount(spreadsheet->columnCount()) {

	updateVerticalHeader();
	updateHorizontalHeader();

	connect(m_spreadsheet, &Spreadsheet::aspectAdded, this, &SpreadsheetModel::handleAspectAdded);
	connect(m_spreadsheet, &Spreadsheet::aspectAboutToBeRemoved, this, &SpreadsheetModel::handleAspectAboutToBeRemoved);
	connect(m_spreadsheet, &Spreadsheet::aspectRemoved, this, &SpreadsheetModel::handleAspectRemoved);
	connect(m_spreadsheet, &Spreadsheet::aspectDescriptionChanged, this, &SpreadsheetModel::handleDescriptionChange);

	for (int i = 0; i < spreadsheet->columnCount(); ++i) {
		beginInsertColumns(QModelIndex(), i, i);
		handleAspectAdded(spreadsheet->column(i));
	}

	m_spreadsheet->setModel(this);
}

void SpreadsheetModel::suppressSignals(bool value) {
	m_suppressSignals = value;

	//update the headers after all the data was added to the model
	//and we start listening to signals again
	if (!m_suppressSignals) {
		m_rowCount = m_spreadsheet->rowCount();
		m_columnCount = m_spreadsheet->columnCount();
		m_spreadsheet->emitColumnCountChanged();
		updateVerticalHeader();
		updateHorizontalHeader();
		beginResetModel();
		endResetModel();
	}
}

Qt::ItemFlags SpreadsheetModel::flags(const QModelIndex& index) const {
	if (index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	else
		return Qt::ItemIsEnabled;
}

void SpreadsheetModel::setSearchText(const QString& text) {
	m_searchText = text;
}

QModelIndex SpreadsheetModel::index(const QString& text) const {
	const int colCount = m_spreadsheet->columnCount();
	const int rowCount = m_spreadsheet->rowCount();
	for (int col = 0; col < colCount; ++col) {
		auto* column = m_spreadsheet->column(col)->asStringColumn();
		for (int row = 0; row < rowCount; ++row) {
			if (column->textAt(row).indexOf(text) != -1)
				return createIndex(row, col);
		}
	}

	return createIndex(-1, -1);
}

QVariant SpreadsheetModel::data(const QModelIndex& index, int role) const {
	if ( !index.isValid() )
		return QVariant();

	const int row = index.row();
	const int col = index.column();
	const Column* col_ptr = m_spreadsheet->column(col);

	if (!col_ptr)
		return QVariant();

	switch (role) {
	case Qt::ToolTipRole:
		if (col_ptr->isValid(row)) {
			if (col_ptr->isMasked(row))
				return QVariant(i18n("%1, masked (ignored in all operations)", col_ptr->asStringColumn()->textAt(row)));
			else
				return QVariant(col_ptr->asStringColumn()->textAt(row));
		} else {
			if (col_ptr->isMasked(row))
				return QVariant(i18n("invalid cell, masked (ignored in all operations)"));
			else
				return QVariant(i18n("invalid cell (ignored in all operations)"));
		}
	case Qt::EditRole:
		if (col_ptr->columnMode() == AbstractColumn::ColumnMode::Double) {
			double value = col_ptr->valueAt(row);
			if (std::isnan(value))
				return QVariant("-");
			else if (std::isinf(value))
				return QVariant(QLatin1String("inf"));
			else
				return QVariant(col_ptr->asStringColumn()->textAt(row));
		}

		if (col_ptr->isValid(row))
			return QVariant(col_ptr->asStringColumn()->textAt(row));

		//m_formula_mode is not used at the moment
		//if (m_formula_mode)
		//	return QVariant(col_ptr->formula(row));

		break;
	case Qt::DisplayRole:
		if (col_ptr->columnMode() == AbstractColumn::ColumnMode::Double) {
			double value = col_ptr->valueAt(row);
			if (std::isnan(value))
				return QVariant("-");
			else if (std::isinf(value))
				return QVariant(UTF8_QSTRING("∞"));
			else
				return QVariant(col_ptr->asStringColumn()->textAt(row));
		}

		if (!col_ptr->isValid(row))
			return QVariant("-");

		//m_formula_mode is not used at the moment
		//if (m_formula_mode)
		//	return QVariant(col_ptr->formula(row));

		return QVariant(col_ptr->asStringColumn()->textAt(row));
	case Qt::ForegroundRole:
		if (!col_ptr->isValid(row))
			return QVariant(QBrush(Qt::red));
		return color(col_ptr, row, AbstractColumn::Formatting::Foreground);
	case Qt::BackgroundRole:
		if (m_searchText.isEmpty())
			return color(col_ptr, row, AbstractColumn::Formatting::Background);
		else {
			if (col_ptr->asStringColumn()->textAt(row).indexOf(m_searchText) == -1)
				return color(col_ptr, row, AbstractColumn::Formatting::Background);
			else
				return QVariant(QApplication::palette().color(QPalette::Highlight));
		}
	case static_cast<int>(CustomDataRole::MaskingRole):
		return QVariant(col_ptr->isMasked(row));
	case static_cast<int>(CustomDataRole::FormulaRole):
		return QVariant(col_ptr->formula(row));
	case Qt::DecorationRole:
		return color(col_ptr, row, AbstractColumn::Formatting::Icon);
// 		if (m_formula_mode)
// 			return QIcon(QPixmap(":/equals.png")); //TODO
	}

	return QVariant();
}

QVariant SpreadsheetModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if ( (orientation == Qt::Horizontal && section > m_columnCount-1)
		|| (orientation == Qt::Vertical && section > m_rowCount-1) )
		return QVariant();

	switch (orientation) {
	case Qt::Horizontal:
		switch (role) {
		case Qt::DisplayRole:
		case Qt::ToolTipRole:
		case Qt::EditRole:
			return m_horizontal_header_data.at(section);
		case Qt::DecorationRole:
			return m_spreadsheet->child<Column>(section)->icon();
		case static_cast<int>(CustomDataRole::CommentRole):
			return m_spreadsheet->child<Column>(section)->comment();
		}
		break;
	case Qt::Vertical:
		switch (role) {
		case Qt::DisplayRole:
		case Qt::ToolTipRole:
			return m_vertical_header_data.at(section);
		}
	}

	return QVariant();
}

int SpreadsheetModel::rowCount(const QModelIndex& /*parent*/) const {
	return m_rowCount;
}

int SpreadsheetModel::columnCount(const QModelIndex& /*parent*/) const {
	return m_columnCount;
}

bool SpreadsheetModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid())
		return false;

	int row = index.row();
	Column* column = m_spreadsheet->column(index.column());

	SET_NUMBER_LOCALE
	//DEBUG("SpreadsheetModel::setData() value = " << STDSTRING(value.toString()))

	//don't do anything if no new value was provided
	if (column->columnMode() == AbstractColumn::ColumnMode::Double) {
		bool ok;
		double new_value = numberLocale.toDouble(value.toString(), &ok);
		if (ok) {
			if (column->valueAt(row) == new_value)
				return false;
		} else {
			//an empty (non-numeric value) was provided
			if (std::isnan(column->valueAt(row)))
				return false;
		}
	} else {
		if (column->asStringColumn()->textAt(row) == value.toString())
			return false;
	}

	switch (role) {
	case Qt::EditRole:
		// remark: the validity of the cell is determined by the input filter
		if (m_formula_mode)
			column->setFormula(row, value.toString());
		else
			column->asStringColumn()->setTextAt(row, value.toString());
		return true;
	case static_cast<int>(CustomDataRole::MaskingRole):
		m_spreadsheet->column(index.column())->setMasked(row, value.toBool());
		return true;
	case static_cast<int>(CustomDataRole::FormulaRole):
		m_spreadsheet->column(index.column())->setFormula(row, value.toString());
		return true;
	}

	return false;
}

QModelIndex SpreadsheetModel::index(int row, int column, const QModelIndex& /*parent*/) const {
	return createIndex(row, column);
}

QModelIndex SpreadsheetModel::parent(const QModelIndex& /*child*/) const {
	return QModelIndex{};
}

bool SpreadsheetModel::hasChildren(const QModelIndex& /*parent*/) const {
	return false;
}

void SpreadsheetModel::handleAspectAdded(const AbstractAspect* aspect) {
	//PERFTRACE(Q_FUNC_INFO);

	const Column* col = dynamic_cast<const Column*>(aspect);
	if (!col || aspect->parentAspect() != m_spreadsheet)
		return;

	connect(col, &Column::plotDesignationChanged, this, &SpreadsheetModel::handlePlotDesignationChange);
	connect(col, &Column::modeChanged, this, &SpreadsheetModel::handleDataChange);
	connect(col, &Column::dataChanged, this, &SpreadsheetModel::handleDataChange);
	connect(col, &Column::formatChanged, this, &SpreadsheetModel::handleDataChange);
	connect(col, &Column::modeChanged, this, &SpreadsheetModel::handleModeChange);
	connect(col, &Column::rowsInserted, this, &SpreadsheetModel::handleRowsInserted);
	connect(col, &Column::rowsRemoved, this, &SpreadsheetModel::handleRowsRemoved);
	connect(col, &Column::maskingChanged, this, &SpreadsheetModel::handleDataChange);
	connect(col->outputFilter(), &AbstractSimpleFilter::digitsChanged, this, &SpreadsheetModel::handleDigitsChange);

	if (!m_suppressSignals) {
		beginResetModel();
		updateVerticalHeader();
		updateHorizontalHeader();
		endResetModel();

		int index = m_spreadsheet->indexOfChild<AbstractAspect>(aspect);
		m_columnCount = m_spreadsheet->columnCount();
		m_spreadsheet->emitColumnCountChanged();
		Q_EMIT headerDataChanged(Qt::Horizontal, index, m_columnCount - 1);
	}
}

void SpreadsheetModel::handleAspectAboutToBeRemoved(const AbstractAspect* aspect) {
	if (m_suppressSignals)
		return;

	const Column* col = dynamic_cast<const Column*>(aspect);
	if (!col || aspect->parentAspect() != m_spreadsheet)
		return;

	beginResetModel();
	disconnect(col, nullptr, this, nullptr);
}

void SpreadsheetModel::handleAspectRemoved(const AbstractAspect* parent, const AbstractAspect* /*before*/, const AbstractAspect* child) {
	const Column* col = dynamic_cast<const Column*>(child);
	if (!col || parent != m_spreadsheet)
		return;

	updateVerticalHeader();
	updateHorizontalHeader();

	m_columnCount = m_spreadsheet->columnCount();
	m_spreadsheet->emitColumnCountChanged();

	endResetModel();
}

void SpreadsheetModel::handleDescriptionChange(const AbstractAspect* aspect) {
	if (m_suppressSignals)
		return;

	const Column* col = dynamic_cast<const Column*>(aspect);
	if (!col || aspect->parentAspect() != m_spreadsheet)
		return;

	if (!m_suppressSignals) {
		updateHorizontalHeader();
		int index = m_spreadsheet->indexOfChild<Column>(col);
		Q_EMIT headerDataChanged(Qt::Horizontal, index, index);
	}
}

void SpreadsheetModel::handleModeChange(const AbstractColumn* col) {
	if (m_suppressSignals)
		return;

	updateHorizontalHeader();
	int index = m_spreadsheet->indexOfChild<Column>(col);
	Q_EMIT headerDataChanged(Qt::Horizontal, index, index);
	handleDataChange(col);

	//output filter was changed after the mode change, update the signal-slot connection
	disconnect(nullptr, SIGNAL(digitsChanged()), this, SLOT(handledigitsChange()));
	connect(static_cast<const Column*>(col)->outputFilter(), &AbstractSimpleFilter::digitsChanged, this, &SpreadsheetModel::handleDigitsChange);
}

void SpreadsheetModel::handleDigitsChange() {
	if (m_suppressSignals)
		return;

	const auto* filter = dynamic_cast<const Double2StringFilter*>(QObject::sender());
	if (!filter)
		return;

	const AbstractColumn* col = filter->output(0);
	handleDataChange(col);
}

void SpreadsheetModel::handlePlotDesignationChange(const AbstractColumn* col) {
	if (m_suppressSignals)
		return;

	updateHorizontalHeader();
	int index = m_spreadsheet->indexOfChild<Column>(col);
	Q_EMIT headerDataChanged(Qt::Horizontal, index, m_columnCount-1);
}

void SpreadsheetModel::handleDataChange(const AbstractColumn* col) {
	if (m_suppressSignals)
		return;

	int i = m_spreadsheet->indexOfChild<Column>(col);
	Q_EMIT dataChanged(index(0, i), index(m_rowCount-1, i));
}

void SpreadsheetModel::handleRowsInserted(const AbstractColumn* col, int /*before*/, int /*count*/) {
	if (m_suppressSignals)
		return;

	int i = m_spreadsheet->indexOfChild<Column>(col);
	m_rowCount = col->rowCount();
	Q_EMIT dataChanged(index(0, i), index(m_rowCount-1, i));
	updateVerticalHeader();
	m_spreadsheet->emitRowCountChanged();
}

void SpreadsheetModel::handleRowsRemoved(const AbstractColumn* col, int /*first*/, int /*count*/) {
	if (m_suppressSignals)
		return;

	int i = m_spreadsheet->indexOfChild<Column>(col);
	m_rowCount = col->rowCount();
	Q_EMIT dataChanged(index(0, i), index(m_rowCount-1, i));
	updateVerticalHeader();
	m_spreadsheet->emitRowCountChanged();
}

void SpreadsheetModel::updateVerticalHeader() {
	int old_rows = m_vertical_header_data.size();
	int new_rows = m_rowCount;

	if (new_rows > old_rows) {
		beginInsertRows(QModelIndex(), old_rows, new_rows-1);

		for (int i = old_rows+1; i <= new_rows; i++)
			m_vertical_header_data << i;

		endInsertRows();
	} else if (new_rows < old_rows) {
		beginRemoveRows(QModelIndex(), new_rows, old_rows-1);

		while (m_vertical_header_data.size() > new_rows)
			m_vertical_header_data.removeLast();

		endRemoveRows();
	}
}

void SpreadsheetModel::updateHorizontalHeader() {
	int column_count = m_spreadsheet->childCount<Column>();

	while (m_horizontal_header_data.size() < column_count)
		m_horizontal_header_data << QString();

	while (m_horizontal_header_data.size() > column_count)
		m_horizontal_header_data.removeLast();

	KConfigGroup group = KSharedConfig::openConfig()->group("Settings_Spreadsheet");
	bool showColumnType = group.readEntry(QLatin1String("ShowColumnType"), true);
	bool showPlotDesignation = group.readEntry(QLatin1String("ShowPlotDesignation"), true);

	for (int i = 0; i < column_count; i++) {
		Column* col = m_spreadsheet->child<Column>(i);
		QString header = col->name();

		if (showColumnType)
			header += QLatin1String(" {") + col->columnModeString() + QLatin1Char('}');

		if (showPlotDesignation) {
			if (col->plotDesignation() != AbstractColumn::PlotDesignation::NoDesignation)
				header += QLatin1String(" ") + col->plotDesignationString();
		}
		m_horizontal_header_data.replace(i, header);
	}
}

Column* SpreadsheetModel::column(int index) {
	return m_spreadsheet->column(index);
}

void SpreadsheetModel::activateFormulaMode(bool on) {
	if (m_formula_mode == on) return;

	m_formula_mode = on;
	if (m_rowCount > 0 && m_columnCount > 0)
		Q_EMIT dataChanged(index(0,0), index(m_rowCount - 1, m_columnCount - 1));
}

bool SpreadsheetModel::formulaModeActive() const {
	return m_formula_mode;
}

QVariant SpreadsheetModel::color(const AbstractColumn* column, int row, AbstractColumn::Formatting type) const {
	if (!column->isNumeric()
		|| !column->isValid(row)
		|| !column->hasHeatmapFormat())
		return QVariant();

	const auto& format = column->heatmapFormat();
	if (format.type != type || format.colors.isEmpty())
		return QVariant();

	double value = column->valueAt(row);
	double range = (format.max - format.min)/format.colors.count();
	int index = 0;
	for (int i = 0; i < format.colors.count(); ++i) {
		if (value <=  format.min + (i+1)*range) {
			index = i;
			break;
		}
	}

	return QVariant(QColor(format.colors.at(index)));
}
