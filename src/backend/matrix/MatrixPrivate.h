/***************************************************************************
    File                 : MatrixPrivate.h
    Project              : LabPlot
    Description          : Private members of Matrix.
    --------------------------------------------------------------------
    Copyright            : (C) 2008-2009 Tilman Benkert (thzs@gmx.net)
    Copyright            : (C) 2015 Alexander Semke (alexander.semke@web.de)
    Copyright            : (C) 2017 Stefan Gerlach (stefan.gerlach@uni.kn)

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

#ifndef MATRIXPRIVATE_H
#define MATRIXPRIVATE_H

template <class T> class QVector;

class MatrixPrivate {
public:
	explicit MatrixPrivate(Matrix*, AbstractColumn::ColumnMode);

	void insertColumns(int before, int count);
	void removeColumns(int first, int count);
	void insertRows(int before, int count);
	void removeRows(int first, int count);

	QString name() const { return q->name(); }

	// get value of cell at row/col (must be defined in header)
	template <typename T> T cell(int row, int col) const {
		Q_ASSERT(row >= 0 && row < rowCount);
		Q_ASSERT(col >= 0 && col < columnCount);

		return (static_cast<QVector<QVector<T>>*>(data))->at(col).at(row);
	}

	// Set value of cell at row/col (must be defined in header)
	template <typename T> void setCell(int row, int col, T value) {
		Q_ASSERT(row >= 0 && row < rowCount);
		Q_ASSERT(col >= 0 && col < columnCount);

		static_cast<QVector<QVector<T>>*>(data)->operator[](col)[row] = value;

		if (!suppressDataChange)
			emit q->dataChanged(row, col, row, col);
	}

	template <typename T> QVector<T> columnCells(int col, int first_row, int last_row);
	template <typename T> QVector<T> rowCells(int row, int first_column, int last_column);
	template <typename T> void setColumnCells(int col, int first_row, int last_row, const QVector<T>& values) {
		Q_ASSERT(first_row >= 0 && first_row < rowCount);
		Q_ASSERT(last_row >= 0 && last_row < rowCount);
		Q_ASSERT(values.count() > last_row - first_row);

		if (first_row == 0 && last_row == rowCount-1) {
			static_cast<QVector<QVector<T>>*>(data)->operator[](col) = values;
			static_cast<QVector<QVector<T>>*>(data)->operator[](col).resize(rowCount);  // values may be larger
			if (!suppressDataChange)
				emit q->dataChanged(first_row, col, last_row, col);
			return;
		}

		for (int i = first_row; i <= last_row; i++)
			static_cast<QVector<QVector<T>>*>(data)->operator[](col)[i] = values.at(i-first_row);

		if (!suppressDataChange)
			emit q->dataChanged(first_row, col, last_row, col);
	}
	template <typename T> void setRowCells(int row, int first_column, int last_column, const QVector<T>& values) {
		Q_ASSERT(first_column >= 0 && first_column < columnCount);
		Q_ASSERT(last_column >= 0 && last_column < columnCount);
		Q_ASSERT(values.count() > last_column - first_column);

		for (int i = first_column; i <= last_column; i++)
			static_cast<QVector<QVector<T>>*>(data)->operator[](i)[row] = values.at(i-first_column);
		if (!suppressDataChange)
			emit q->dataChanged(row, first_column, row, last_column);
	}

	void clearColumn(int col);

	void setRowHeight(int row, int height) { rowHeights[row] = height; }
	void setColumnWidth(int col, int width) { columnWidths[col] = width; }
	int rowHeight(int row) const { return rowHeights.at(row); }
	int columnWidth(int col) const { return columnWidths.at(col); }

	void updateViewHeader();
	void emitDataChanged(int top, int left, int bottom, int right) { emit q->dataChanged(top, left, bottom, right); }

	Matrix* q;
	void* data;
	AbstractColumn::ColumnMode mode;	// mode (data type) of values

	int rowCount;
	int columnCount;
	QVector<int> rowHeights;	//!< Row widths
	QVector<int> columnWidths;	//!< Columns widths
	Matrix::HeaderFormat headerFormat;

	char numericFormat;			//!< Format code for displaying numbers
	int precision;				//!< Number of significant digits
	double xStart, xEnd;
	double yStart, yEnd;
	QString formula;			//!<formula used to calculate the cells
	bool suppressDataChange;
};

#endif
