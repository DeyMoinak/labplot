/***************************************************************************
    File                 : FunctionValuesDialog.cpp
    Project              : LabPlot
    Description          : Dialog for generating values from a mathematical function
    --------------------------------------------------------------------
    Copyright            : (C) 2014-2015 by Alexander Semke (alexander.semke@web.de)

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
#include "FunctionValuesDialog.h"
#include "backend/core/AspectTreeModel.h"
#include "backend/core/column/Column.h"
#include "backend/core/Project.h"
#include "backend/gsl/ExpressionParser.h"
#include "backend/lib/macros.h"
#include "backend/spreadsheet/Spreadsheet.h"
#include "commonfrontend/widgets/TreeViewComboBox.h"
#include "kdefrontend/widgets/ConstantsWidget.h"
#include "kdefrontend/widgets/FunctionsWidget.h"

#include <QMenu>
#include <QWidgetAction>

#include <cmath>

/*!
	\class FunctionValuesDialog
	\brief Dialog for generating values from a mathematical function.

	\ingroup kdefrontend
 */

FunctionValuesDialog::FunctionValuesDialog(Spreadsheet* s, QWidget* parent, Qt::WFlags fl) : KDialog(parent, fl), m_spreadsheet(s) {
	Q_ASSERT(s);
	setWindowTitle(i18n("Function values"));

	QFrame* mainWidget = new QFrame(this);
	ui.setupUi(mainWidget);
	setMainWidget(mainWidget);

	ui.tbConstants->setIcon( KIcon("labplot-format-text-symbol") );
	ui.tbFunctions->setIcon( KIcon("preferences-desktop-font") );

	ui.teEquation->setMaximumHeight(QLineEdit().sizeHint().height()*2);
	ui.teEquation->setFocus();

	m_topLevelClasses<<"Folder"<<"Workbook"<<"Spreadsheet"<<"FileDataSource"<<"Column";
	m_selectableClasses<<"Column";

#if __cplusplus < 201103L
	m_aspectTreeModel = std::auto_ptr<AspectTreeModel>(new AspectTreeModel(m_spreadsheet->project()));
#else
	m_aspectTreeModel = std::unique_ptr<AspectTreeModel>(new AspectTreeModel(m_spreadsheet->project()));
#endif
	m_aspectTreeModel->setSelectableAspects(m_selectableClasses);

	ui.bAddVariable->setIcon(KIcon("list-add"));
	ui.bAddVariable->setToolTip(i18n("Add new variable"));

	setButtons( KDialog::Ok | KDialog::Cancel );
	setButtonText(KDialog::Ok, i18n("&Generate"));
	setButtonToolTip(KDialog::Ok, i18n("Generate function values"));

	connect( ui.bAddVariable, SIGNAL(pressed()), this, SLOT(addVariable()) );
	connect( ui.teEquation, SIGNAL(expressionChanged()), this, SLOT(checkValues()) );
	connect( ui.tbConstants, SIGNAL(clicked()), this, SLOT(showConstants()) );
	connect( ui.tbFunctions, SIGNAL(clicked()), this, SLOT(showFunctions()) );
	connect(this, SIGNAL(okClicked()), this, SLOT(generate()));

	resize( QSize(300,0).expandedTo(minimumSize()) );
}

void FunctionValuesDialog::setColumns(QList<Column*> list) {
	m_columns = list;
	ui.teEquation->setPlainText(m_columns.first()->formula());

	const QStringList& variableNames = m_columns.first()->formulaVariableNames();
	if (!variableNames.size()) {
		//no formular was used for this column -> add the first variable "x"
		addVariable();
		m_variableNames[0]->setText("x");
	} else {
		//formula and variables are available
		const QStringList& columnPathes = m_columns.first()->formulaVariableColumnPathes();

		//add all available variables and select the corresponding columns
		const QList<AbstractAspect*> columns = m_spreadsheet->project()->children("Column", AbstractAspect::Recursive);
		for (int i=0; i<variableNames.size(); ++i) {
			addVariable();
			m_variableNames[i]->setText(variableNames.at(i));

			foreach (const AbstractAspect* aspect, columns) {
				if (aspect->path() == columnPathes.at(i)) {
					const AbstractColumn* column = dynamic_cast<const AbstractColumn*>(aspect);
					if (column)
						m_variableDataColumns[i]->setCurrentModelIndex(m_aspectTreeModel->modelIndexOfAspect(column));
					else
						m_variableDataColumns[i]->setCurrentModelIndex(QModelIndex());

					break;
				}
			}
		}
	}
}

/*!
	check the user input and enables/disables the Ok-button depending on the correctness of the input
 */
void FunctionValuesDialog::checkValues() {
	//check whether the formulr syntax is correct
	if (!ui.teEquation->isValid()) {
		enableButton(KDialog::Ok, false);
		return;
	}

	//check whether for the variables where a name was provided also a column was selected.
	for (int i=0; i<m_variableDataColumns.size(); ++i) {
		if (m_variableNames.at(i)->text().simplified().isEmpty())
			continue;

		TreeViewComboBox* cb = m_variableDataColumns.at(i);
		AbstractAspect* aspect = static_cast<AbstractAspect*>(cb->currentModelIndex().internalPointer());
		if (!aspect) {
			enableButton(KDialog::Ok, false);
			return;
		}
	}

	enableButton(KDialog::Ok, true);
}

void FunctionValuesDialog::showConstants() {
	QMenu menu;
	ConstantsWidget constants(&menu);
	connect(&constants, SIGNAL(constantSelected(QString)), this, SLOT(insertConstant(QString)));
	connect(&constants, SIGNAL(constantSelected(QString)), &menu, SLOT(close()));
	connect(&constants, SIGNAL(canceled()), &menu, SLOT(close()));

	QWidgetAction* widgetAction = new QWidgetAction(this);
	widgetAction->setDefaultWidget(&constants);
	menu.addAction(widgetAction);

	QPoint pos(-menu.sizeHint().width()+ui.tbConstants->width(),-menu.sizeHint().height());
	menu.exec(ui.tbConstants->mapToGlobal(pos));
}

void FunctionValuesDialog::showFunctions() {
	QMenu menu;
	FunctionsWidget functions(&menu);
	connect(&functions, SIGNAL(functionSelected(QString)), this, SLOT(insertFunction(QString)));
	connect(&functions, SIGNAL(functionSelected(QString)), &menu, SLOT(close()));
	connect(&functions, SIGNAL(canceled()), &menu, SLOT(close()));

	QWidgetAction* widgetAction = new QWidgetAction(this);
	widgetAction->setDefaultWidget(&functions);
	menu.addAction(widgetAction);

	QPoint pos(-menu.sizeHint().width()+ui.tbFunctions->width(),-menu.sizeHint().height());
	menu.exec(ui.tbFunctions->mapToGlobal(pos));
}

void FunctionValuesDialog::insertFunction(const QString& str) {
	ui.teEquation->insertPlainText(str + "(x)");
}

void FunctionValuesDialog::insertConstant(const QString& str) {
	ui.teEquation->insertPlainText(str);
}

void FunctionValuesDialog::addVariable() {
	QGridLayout* layout = dynamic_cast<QGridLayout*>(ui.frameVariables->layout());
	int row = m_variableNames.size();

	//text field for the variable name
	QLineEdit* le = new QLineEdit();
	le->setMaximumWidth(30);
	connect(le, SIGNAL(textChanged(QString)), this, SLOT(variableNameChanged()));
	layout->addWidget(le, row, 0, 1, 1);
	m_variableNames<<le;

	//label for the "="-sign
	QLabel* l = new QLabel("=");
	layout->addWidget(l, row, 1, 1, 1);
	m_variableLabels<<l;

	//combo box for the data column
	TreeViewComboBox* cb = new TreeViewComboBox();
	cb->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
	connect( cb, SIGNAL(currentModelIndexChanged(QModelIndex)), this, SLOT(checkValues()) );
	layout->addWidget(cb, row, 2, 1, 1);
	m_variableDataColumns<<cb;

	cb->setTopLevelClasses(m_topLevelClasses);
	cb->setSelectableClasses(m_selectableClasses);
	cb->setModel(m_aspectTreeModel.get());
	cb->setCurrentModelIndex(m_aspectTreeModel->modelIndexOfAspect(m_spreadsheet->column(0)));

	//move the add-button to the next row
	layout->removeWidget(ui.bAddVariable);
	layout->addWidget(ui.bAddVariable, row+1,3, 1, 1);

	//add delete-button for the just added variable
	if (row!=0) {
		QToolButton* b = new QToolButton();
		b->setIcon(KIcon("list-remove"));
		b->setToolTip(i18n("Delete variable"));
		layout->addWidget(b, row, 3, 1, 1);
		m_variableDeleteButtons<<b;
		connect(b, SIGNAL(pressed()), this, SLOT(deleteVariable()));
	}

	//TODO: adjust the tab-ordering after new widgets were added
}

void FunctionValuesDialog::deleteVariable() {
	QObject* ob=QObject::sender();
	int index = m_variableDeleteButtons.indexOf(qobject_cast<QToolButton*>(ob)) ;

	delete m_variableNames.takeAt(index+1);
	delete m_variableLabels.takeAt(index+1);
	delete m_variableDataColumns.takeAt(index+1);
	delete m_variableDeleteButtons.takeAt(index);

	variableNameChanged();
	checkValues();

	//adjust the layout
	resize( QSize(width(),0).expandedTo(minimumSize()) );

	//TODO: adjust the tab-ordering after some widgets were deleted
}

void FunctionValuesDialog::variableNameChanged() {
	QStringList vars;
	QString text;
	for (int i=0; i<m_variableNames.size(); ++i) {
		QString name = m_variableNames.at(i)->text().simplified();
		if (!name.isEmpty()) {
			vars<<name;

			if (text.isEmpty()) {
				text += name;
			} else {
				text += ", " + name;
			}
		}
	}

	if (!text.isEmpty())
		text = "f(" + text + ')';
	else
		text = 'f';

	ui.lFunction->setText(text);
	ui.teEquation->setVariables(vars);
}

void FunctionValuesDialog::generate() {
	Q_ASSERT(m_spreadsheet);

	WAIT_CURSOR;
	m_spreadsheet->beginMacro(i18np("%1: fill column with function values",
									"%1: fill columns with function values",
									m_spreadsheet->name(),
									m_columns.size()));

	//determine variable names and the data vectors of the specified columns
	QStringList variableNames;
	QStringList columnPathes;
	QVector<QVector<double>*> xVectors;
	QVector<Column*> xColumns;
	int maxRowCount = m_spreadsheet->rowCount();
	for (int i=0; i<m_variableNames.size(); ++i) {
		variableNames << m_variableNames.at(i)->text().simplified();

		AbstractAspect* aspect = static_cast<AbstractAspect*>(m_variableDataColumns.at(i)->currentModelIndex().internalPointer());
		Q_ASSERT(aspect);
		Column* column = dynamic_cast<Column*>(aspect);
		Q_ASSERT(column);
		columnPathes << column->path();
		xColumns << column;
		xVectors << static_cast<QVector<double>* >(column->data());

		if (column->rowCount()>maxRowCount)
			maxRowCount = column->rowCount();
	}

	//resize the spreadsheet if one of the data vectors from other spreadsheet(s) has more elements then the current spreadsheet.
	if (m_spreadsheet->rowCount()<maxRowCount)
		m_spreadsheet->setRowCount(maxRowCount);

	//create new vector for storing the calculated values
	//the vectors with the variable data can be smaller then the result vector. So, not all values in the result vector might get initialized.
	//->"clean" the result vector first
	QVector<double> new_data(maxRowCount);
	for (int i=0; i<new_data.size(); ++i)
		new_data[i] = NAN;

	//evaluate the expression for f(x_1, x_2, ...) and write the calculated values into a new vector.
	ExpressionParser* parser = ExpressionParser::getInstance();
	const QString& expression = ui.teEquation->toPlainText();
	parser->evaluateCartesian(expression, variableNames, xVectors, &new_data);

	//set the new values and store the expression, variable names and the used data columns
	foreach(Column* col, m_columns) {
		col->setFormula(expression, variableNames, columnPathes);
		col->replaceValues(0, new_data);
	}

	m_spreadsheet->endMacro();
	RESET_CURSOR;
}
