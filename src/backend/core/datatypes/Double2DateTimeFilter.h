/***************************************************************************
    File                 : Double2DateTimeFilter.h
    Project              : AbstractColumn
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Knut Franke, Tilman Benkert
    Email (use @ for *)  : knut.franke*gmx.de, thzs@gmx.net
    Description          : Conversion filter double -> QDateTime, interpreting
                           the input numbers as (fractional) Julian days.

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
#ifndef DOUBLE2DATE_TIME_FILTER_H
#define DOUBLE2DATE_TIME_FILTER_H

#include "../AbstractSimpleFilter.h"
#include <QDateTime>
#include <cmath>

//! Conversion filter double -> QDateTime, interpreting the input numbers as (fractional) Julian days.
class Double2DateTimeFilter : public AbstractSimpleFilter
{
	Q_OBJECT

	public:
		virtual QDate dateAt(int row) const {
			if (!m_inputs.value(0)) return QDate();
			double inputValue = m_inputs.value(0)->valueAt(row);
			if (std::isnan(inputValue)) return QDate();
			return QDate::fromJulianDay(qRound(inputValue));
		}
		virtual QTime timeAt(int row) const {
			if (!m_inputs.value(0)) return QTime();
			double inputValue = m_inputs.value(0)->valueAt(row);
			if (std::isnan(inputValue)) return QTime();
			// we only want the digits behind the dot and
			// convert them from fraction of day to milliseconds
			return QTime(12,0,0,0).addMSecs(int( (inputValue - int(inputValue)) * 86400000.0 ));
		}
		virtual QDateTime dateTimeAt(int row) const {
			return QDateTime(dateAt(row), timeAt(row));
		}

		//! Return the data type of the column
		virtual AbstractColumn::ColumnMode columnMode() const { return AbstractColumn::DateTime; }

	protected:
		//! Using typed ports: only double inputs are accepted.
		virtual bool inputAcceptable(int, const AbstractColumn *source) {
			return source->columnMode() == AbstractColumn::Numeric;
		}
};

#endif // ifndef DOUBLE2DATE_TIME_FILTER_H

