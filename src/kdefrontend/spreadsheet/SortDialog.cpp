/***************************************************************************
    File                 : SortDialog.h
    Project              : LabPlot
    Description          : Sorting options dialog
    --------------------------------------------------------------------
    Copyright            : (C) 2011 by Alexander Semke (alexander.semke@web.de)

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
#include "SortDialog.h"

#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QLayout>
#include <KLocale>
#include <QIcon>
#include <QDialog>
#include <QDialogButtonBox>


/*!
	\class SortDialog
	\brief Dialog for sorting the columns in a spreadsheet.

	\ingroup kdefrontend
 */

SortDialog::SortDialog( QWidget* parent, Qt::WFlags fl ) : QDialog( parent, fl ){

	setWindowIcon(QIcon::fromTheme("view-sort-ascending"));
	setWindowTitle(i18n("Sort columns"));
	setSizeGripEnabled(true);

	QGroupBox* widget = new QGroupBox(i18n("Options"));
	QGridLayout* layout = new QGridLayout(widget);
	layout->setSpacing(4);
	layout->setContentsMargins(4,4,4,4);

	layout->addWidget( new QLabel( i18n("Order")), 0, 0 );
	cbOrdering = new QComboBox();
	cbOrdering->addItem(QIcon::fromTheme("view-sort-ascending"), i18n("Ascending"));
	cbOrdering->addItem(QIcon::fromTheme("view-sort-descending"), i18n("Descending"));
	layout->addWidget(cbOrdering, 0, 1 );
	
	lblType = new QLabel(i18n("Sort columns"));
	layout->addWidget( lblType, 1, 0 );
	cbType = new QComboBox();
	cbType->addItem(i18n("Separately"));
	cbType->addItem(i18n("Together"));
	layout->addWidget(cbType, 1, 1 );
	cbType->setCurrentIndex(Together);

	lblColumns = new QLabel(i18n("Leading column"));
	layout->addWidget( lblColumns, 2, 0 );
	cbColumns = new QComboBox();
	layout->addWidget(cbColumns, 2, 1);
	layout->setRowStretch(3, 1);


	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                         | QDialogButtonBox::Cancel);

	buttonBox->button(QDialogButtonBox::Ok)->setText(i18n("Sort"));

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(sort()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

	layout->addWidget(buttonBox);

	setLayout(layout);

	connect( cbType, SIGNAL(currentIndexChanged(int)), this, SLOT(changeType(int)));

	this->resize(400,0);
}

void SortDialog::sort(){
	Column* leading;
	if(cbType->currentIndex() == Together) 
		leading = m_columns_list.at(cbColumns->currentIndex());
	else
		leading = 0;
	
	emit sort(leading, m_columns_list, cbOrdering->currentIndex() == Ascending );
	
	accepted();
}

void SortDialog::setColumnsList(QList<Column*> list){
	m_columns_list = list;

	for(int i=0; i<list.size(); i++)
		cbColumns->addItem( list.at(i)->name() );

	cbColumns->setCurrentIndex(0);
	
	if (list.size() == 1){
		lblType->hide();
		cbType->hide();
		lblColumns->hide();
		cbColumns->hide();
	}
}

void SortDialog::changeType(int Type){
	if(Type == Together)
		cbColumns->setEnabled(true);
	else
		cbColumns->setEnabled(false);
}
