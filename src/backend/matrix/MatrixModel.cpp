/***************************************************************************
    File                 : MatrixModel.cpp
    Project              : LabPlot
    Description          : Matrix data model
    --------------------------------------------------------------------
    Copyright            : (C) 2015-2016 Alexander Semke (alexander.semke@web.de)
    Copyright            : (C) 2008-2009 Tilman Benkert (thzs@gmx.net)

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

#include "backend/matrix/MatrixModel.h"
#include "backend/matrix/Matrix.h"
#include <QBrush>
#include <QLocale>

/*!
  \class MatrixModel
  \brief Model for the access to data of a Matrix-object.

	This is a model in the sense of Qt4 model/view framework which is used
	to access a Matrix object from any of Qt4s view classes, typically a QMatrixView.
	Its main purposes are translating Matrix signals into QAbstractItemModel signals
	and translating calls to the QAbstractItemModel read/write API into calls
	in the public API of Matrix.

	\ingroup backend
*/
MatrixModel::MatrixModel(Matrix* matrix) : QAbstractItemModel(0), m_matrix(matrix), m_suppressDataChangedSignal(false) {
	connect(m_matrix, SIGNAL(columnsAboutToBeInserted(int,int)),
			this, SLOT(handleColumnsAboutToBeInserted(int,int)));
	connect(m_matrix, SIGNAL(columnsInserted(int,int)),
			this, SLOT(handleColumnsInserted(int,int)));
	connect(m_matrix, SIGNAL(columnsAboutToBeRemoved(int,int)),
			this, SLOT(handleColumnsAboutToBeRemoved(int,int)));
	connect(m_matrix, SIGNAL(columnsRemoved(int,int)),
			this, SLOT(handleColumnsRemoved(int,int)));
	connect(m_matrix, SIGNAL(rowsAboutToBeInserted(int,int)),
			this, SLOT(handleRowsAboutToBeInserted(int,int)));
	connect(m_matrix, SIGNAL(rowsInserted(int,int)),
			this, SLOT(handleRowsInserted(int,int)));
	connect(m_matrix, SIGNAL(rowsAboutToBeRemoved(int,int)),
			this, SLOT(handleRowsAboutToBeRemoved(int,int)));
	connect(m_matrix, SIGNAL(rowsRemoved(int,int)),
			this, SLOT(handleRowsRemoved(int,int)));
	connect(m_matrix, SIGNAL(dataChanged(int,int,int,int)),
			this, SLOT(handleDataChanged(int,int,int,int)));
	connect(m_matrix, SIGNAL(coordinatesChanged()),
			this, SLOT(handleCoordinatesChanged()));
	connect(m_matrix, SIGNAL(numericFormatChanged(char)), this, SLOT(handleFormatChanged()));
	connect(m_matrix, SIGNAL(precisionChanged(int)), this, SLOT(handleFormatChanged()));
}

void MatrixModel::setSuppressDataChangedSignal(bool b) {
	m_suppressDataChangedSignal = b;
}

void MatrixModel::setChanged() {
	emit changed();
}

Qt::ItemFlags MatrixModel::flags(const QModelIndex& index) const {
	if (index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	else
		return Qt::ItemIsEnabled;
}

QVariant MatrixModel::data(const QModelIndex& index, int role) const {
	if( !index.isValid() )
		return QVariant();

	int row = index.row();
	int col = index.column();

	switch(role) {
		case Qt::ToolTipRole:
		case Qt::EditRole:
		case Qt::DisplayRole:
			return QVariant(m_matrix->text(row, col));
		case Qt::BackgroundRole:
			return QVariant(QBrush(QColor(192,255,255)));
	}

	return QVariant();
}

QVariant MatrixModel::headerData(int section, Qt::Orientation orientation, int role) const {
	QString result;
	Matrix::HeaderFormat headerFormat = m_matrix->headerFormat();
	switch(orientation) {
		case Qt::Horizontal:
			switch(role) {
				case Qt::DisplayRole:
				case Qt::ToolTipRole:
					if (headerFormat==Matrix::HeaderRowsColumns) {
						result = QString::number(section+1);
					} else if (headerFormat == Matrix::HeaderValues) {
						double diff = m_matrix->xEnd() - m_matrix->xStart();
						double step = 0.0;
						if (m_matrix->columnCount() > 1)
							step = diff/double(m_matrix->columnCount()-1);
						result = QLocale().toString(m_matrix->xStart()+double(section)*step,
								m_matrix->numericFormat(), m_matrix->precision());
					} else {
						result = QString::number(section+1) + QLatin1String(" (");
						double diff = m_matrix->xEnd() - m_matrix->xStart();
						double step = 0.0;
						if (m_matrix->columnCount() > 1)
							step = diff/double(m_matrix->columnCount()-1);
						result += QLocale().toString(m_matrix->xStart()+double(section)*step,
								m_matrix->numericFormat(), m_matrix->precision());

						result += ')';
					}
					return QVariant(result);
			}
			break;
		case Qt::Vertical:
			switch(role) {
				case Qt::DisplayRole:
				case Qt::ToolTipRole:
					if (headerFormat==Matrix::HeaderRowsColumns) {
						result = QString::number(section+1);
					} else if (headerFormat==Matrix::HeaderValues) {
						double diff = m_matrix->yEnd() - m_matrix->yStart();
						double step = 0.0;
						if (m_matrix->rowCount() > 1)
							step = diff/double(m_matrix->rowCount()-1);
						// TODO: implement decent double == 0 check
// 						if (diff < 1e-10)
// 							result += QLocale().toString(m_matrix->yStart(),
// 									m_matrix->numericFormat(), m_matrix->displayedDigits());
						result += QLocale().toString(m_matrix->yStart()+double(section)*step,
								m_matrix->numericFormat(), m_matrix->precision());
					} else {
						result = QString::number(section+1) + QString(" (");
						double diff = m_matrix->yEnd() - m_matrix->yStart();
						double step = 0.0;
						if (m_matrix->rowCount() > 1)
							step = diff/double(m_matrix->rowCount()-1);

						result += QLocale().toString(m_matrix->yStart()+double(section)*step,
									m_matrix->numericFormat(), m_matrix->precision());
						result += ')';
					}
					return QVariant(result);
			}
	}
	return QVariant();
}

int MatrixModel::rowCount(const QModelIndex& parent) const {
	Q_UNUSED(parent)
	return m_matrix->rowCount();
}

int MatrixModel::columnCount(const QModelIndex& parent) const {
	Q_UNUSED(parent)
	return m_matrix->columnCount();
}

bool MatrixModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid())
		return false;

	int row = index.row();
	int column = index.column();

	if(role ==  Qt::EditRole) {
		m_matrix->setCell(row, column, value.toDouble());
		if (!m_suppressDataChangedSignal) emit changed();
		return true;
	}
	return false;
}

QModelIndex MatrixModel::index(int row, int column, const QModelIndex& parent) const {
	Q_UNUSED(parent)
	return createIndex(row, column);
}

QModelIndex MatrixModel::parent(const QModelIndex& child) const {
	Q_UNUSED(child)
    return QModelIndex();
}

void MatrixModel::updateHeader() {
	emit headerDataChanged(Qt::Horizontal, 0, m_matrix->columnCount());
	emit headerDataChanged(Qt::Vertical, 0, m_matrix->rowCount());
}

void MatrixModel::handleColumnsAboutToBeInserted(int before, int count) {
	beginInsertColumns(QModelIndex(), before, before+count-1);
}

void MatrixModel::handleColumnsInserted(int first, int count) {
	Q_UNUSED(first)
	Q_UNUSED(count)
	endInsertColumns();
	if (!m_suppressDataChangedSignal) emit changed();
}

void MatrixModel::handleColumnsAboutToBeRemoved(int first, int count) {
	beginRemoveColumns(QModelIndex(), first, first+count-1);
}

void MatrixModel::handleColumnsRemoved(int first, int count) {
	Q_UNUSED(first)
	Q_UNUSED(count)
	endRemoveColumns();
	if (!m_suppressDataChangedSignal) emit changed();
}

void MatrixModel::handleRowsAboutToBeInserted(int before, int count) {
	beginInsertRows(QModelIndex(), before, before+count-1);
}

void MatrixModel::handleRowsInserted(int first, int count) {
	Q_UNUSED(first)
	Q_UNUSED(count)
	endInsertRows();
	if (!m_suppressDataChangedSignal) emit changed();
}

void MatrixModel::handleRowsAboutToBeRemoved(int first, int count) {
	beginRemoveRows(QModelIndex(), first, first+count-1);
}

void MatrixModel::handleRowsRemoved(int first, int count) {
	Q_UNUSED(first)
	Q_UNUSED(count)
	endRemoveRows();
	if (!m_suppressDataChangedSignal) emit changed();
}

void MatrixModel::handleDataChanged(int top, int left, int bottom, int right) {
	emit dataChanged(index(top, left), index(bottom, right));
	if (!m_suppressDataChangedSignal) emit changed();
}

void MatrixModel::handleCoordinatesChanged() {
	emit headerDataChanged(Qt::Horizontal, 0, columnCount()-1);
	emit headerDataChanged(Qt::Vertical, 0, rowCount()-1);
}

void MatrixModel::handleFormatChanged() {
	handleCoordinatesChanged();
	handleDataChanged(0, 0, rowCount()-1, columnCount()-1);
}
