/***************************************************************************
    File                 : CantorWorksheet.cpp
    Project              : LabPlot
    Description          : Aspect providing a Cantor Worksheets for Multiple backends
    --------------------------------------------------------------------
    Copyright            : (C) 2015 Garvit Khatri (garvitdelhi@gmail.com)
    Copyright            : (C) 2016 by Alexander Semke (alexander.semke@web.de)

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
#include "CantorWorksheet.h"
#include "VariableParser.h"
#include "backend/core/column/Column.h"
#include "backend/core/column/ColumnPrivate.h"
#include "backend/core/Project.h"
#include "commonfrontend/cantorWorksheet/CantorWorksheetView.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KParts/ReadWritePart>

#include <QAction>
#include <QModelIndex>
#include <QDebug>

#include "cantor/cantor_part.h"
#include <cantor/panelpluginhandler.h>
#include <cantor/panelplugin.h>
#include <cantor/worksheetaccess.h>

CantorWorksheet::CantorWorksheet(AbstractScriptingEngine* engine, const QString &name, bool loading)
		: AbstractPart(name),
		scripted(engine),
		m_backendName(name),
		m_session(0),
		m_part(0),
		m_variableModel(0),
		m_worksheetAccess(0) {

	if(!loading)
		init();
}

/*!
	initializes Cantor's part and plugins
*/
bool CantorWorksheet::init(QByteArray* content) {
	KPluginFactory* factory = KPluginLoader(QLatin1String("libcantorpart")).factory();
	if (factory) {
		m_part = factory->create<KParts::ReadWritePart>(this, QVariantList() << m_backendName << QLatin1String("--noprogress"));
		if (!m_part) {
			qDebug() << "Could not create the Cantor Part.";
			return false;
		}
		m_worksheetAccess = m_part->findChild<Cantor::WorksheetAccessInterface*>(Cantor::WorksheetAccessInterface::Name);

		//load worksheet content if available
		if(content)
			m_worksheetAccess->loadWorksheetFromByteArray(content);

		connect(m_worksheetAccess, SIGNAL(sessionChanged()), this, SLOT(sessionChanged()));

		//Cantor's session
		m_session = m_worksheetAccess->session();
		connect(m_session, SIGNAL(statusChanged(Cantor::Session::Status)), this, SIGNAL(statusChanged(Cantor::Session::Status)));

		//variable model
		m_variableModel = m_session->variableModel();
		connect(m_variableModel, SIGNAL(rowsInserted(const QModelIndex, int, int)), this, SLOT(rowsInserted(const QModelIndex, int, int)));
		connect(m_variableModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex, int, int)), this, SLOT(rowsAboutToBeRemoved(const QModelIndex, int, int)));
		connect(m_variableModel, SIGNAL(modelReset()), this, SLOT(modelReset()));

		//available plugins
		Cantor::PanelPluginHandler* handler = m_part->findChild<Cantor::PanelPluginHandler*>(QLatin1String("PanelPluginHandler"));
		if(!handler) {
			KMessageBox::error(view(), i18n("no PanelPluginHandle found for the Cantor Part."));
			return false;
		}
		m_plugins = handler->plugins();
	}
	else {
		//we can only get to this here if we open a project having Cantor content and Cantor plugins were not found.
		//return false here, a proper error message will be created in load() and propagated further.
		return false;
	}
	return true;
}

//SLots
void CantorWorksheet::rowsInserted(const QModelIndex& parent, int first, int last) {
	Q_UNUSED(parent)
	for(int i = first; i <= last; ++i) {
		const QString name = m_variableModel->data(m_variableModel->index(first, 0)).toString();
		const QString value = m_variableModel->data(m_variableModel->index(first, 1)).toString();
		VariableParser* parser = new VariableParser(m_backendName, value);
		if(parser->isParsed()) {
			Column* col = child<Column>(name);
			if (col) {
				col->replaceValues(0, parser->values());
			} else {
				col = new Column(name, parser->values());
				col->setUndoAware(false);
				addChild(col);

				//TODO: Cantor currently ignores the order of variables in the worksheets
				//and adds new variables at the last position in the model.
				//Fix this in Cantor and switch to insertChildBefore here later.
				//insertChildBefore(col, child<Column>(i));
			}
		} else {
			//the already existing variable doesn't contain any numerical values -> remove it
			Column* col = child<Column>(name);
			if (col)
				removeChild(col);
		}

		delete(parser);
	}

	project()->setChanged(true);
}

void CantorWorksheet::sessionChanged() {
	//TODO: signal is never emitted in Cantor
	project()->setChanged(true);
}

void CantorWorksheet::modelReset() {
	for(int i = 0; i < childCount<Column>(); ++i)
		child<Column>(i)->remove();
}

void CantorWorksheet::rowsAboutToBeRemoved(const QModelIndex & parent, int first, int last) {
	Q_UNUSED(parent);
	Q_UNUSED(first);
	Q_UNUSED(last);
	//TODO: Cantor removes rows from the model even when the variable was changed only.
	//We don't want this behaviour since this removes the columns from the datasource in the curve.
	//We need to fix/change this in Cantor.
	return;

// 	Q_UNUSED(parent)
// 	for(int i = first; i <= last; ++i) {
// 		const QString name = m_variableModel->data(m_variableModel->index(first, 0)).toString();
// 		Column* column = child<Column>(name);
// 		if(column)
// 			column->remove();
// 	}
}

QList<Cantor::PanelPlugin*> CantorWorksheet::getPlugins(){
	return m_plugins;
}

KParts::ReadWritePart* CantorWorksheet::part() {
	return m_part;
}

QIcon CantorWorksheet::icon() const {
	if(m_session)
		return QIcon::fromTheme(m_session->backend()->icon());
	return QIcon();
}

QWidget* CantorWorksheet::view() const {
	if (!m_view) {
		m_view = new CantorWorksheetView(const_cast<CantorWorksheet*>(this));
		m_view->setBaseSize(1500, 1500);
		// 	connect(m_view, SIGNAL(statusInfo(QString)), this, SIGNAL(statusInfo(QString)));
	}
	return m_view;
}

//! Return a new context menu.
/**
 * The caller takes ownership of the menu.
 */
QMenu* CantorWorksheet::createContextMenu() {
	QMenu* menu = AbstractPart::createContextMenu();
	Q_ASSERT(menu);
	emit requestProjectContextMenu(menu);
	return menu;
}

QString CantorWorksheet::backendName() {
	return this->m_backendName;
}

//TODO
bool CantorWorksheet::exportView() const {
	return false;
}

bool CantorWorksheet::printView() {
	m_part->action("file_print")->trigger();
	return true;
}

bool CantorWorksheet::printPreview() const {
	m_part->action("file_print_preview")->trigger();
	return true;
}

//##############################################################################
//##################  Serialization/Deserialization  ###########################
//##############################################################################

//! Save as XML
void CantorWorksheet::save(QXmlStreamWriter* writer) const{
	writer->writeStartElement("cantorWorksheet");
	writeBasicAttributes(writer);
	writeCommentElement(writer);

	//general
	writer->writeStartElement( "general" );
	writer->writeAttribute( "backend_name", m_backendName);
	//TODO: save worksheet settings
	writer->writeEndElement();

	//save the content of Cantor's worksheet
	QByteArray content = m_worksheetAccess->saveWorksheetToByteArray();
	writer->writeStartElement("worksheet");
	writer->writeAttribute("content", content.toBase64());
	writer->writeEndElement();

	//save columns(variables)
	for (auto* col : children<Column>(IncludeHidden))
		col->save(writer);

	writer->writeEndElement(); // close "cantorWorksheet" section
}

//! Load from XML
bool CantorWorksheet::load(XmlStreamReader* reader, bool preview) {
	if(!reader->isStartElement() || reader->name() != "cantorWorksheet"){
		reader->raiseError(i18n("no Cantor worksheet element found"));
		return false;
	}

	if (!readBasicAttributes(reader))
		return false;

	QString attributeWarning = i18n("Attribute '%1' missing or empty, default value is used");
	QXmlStreamAttributes attribs;
	QString str;
	bool rc = false;

	while (!reader->atEnd()){
		reader->readNext();
		if (reader->isEndElement() && reader->name() == "cantorWorksheet")
			break;

		if (!reader->isStartElement())
			continue;

		if (reader->name() == "comment"){
			if (!readCommentElement(reader))
				return false;
		} else if (!preview && reader->name() == "general"){
			attribs = reader->attributes();

			m_backendName = attribs.value("backend_name").toString().trimmed();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'backend_name'"));
		} else if (!preview && reader->name() == "worksheet"){
			attribs = reader->attributes();

			str = attribs.value("content").toString().trimmed();
			if(str.isEmpty())
				reader->raiseWarning(attributeWarning.arg("'content'"));

			QByteArray content = QByteArray::fromBase64(str.toAscii());
			rc = init(&content);
			if  (!rc) {
				QString msg = i18n("This project has Cantor content but no Cantor plugins were found. Please check your installation. The project will be closed.");
				reader->raiseError(msg);
				return false;
			}
		} else if(!preview && reader->name() == "column") {
			Column* column = new Column("");
			column->setUndoAware(false);
			if (!column->load(reader, preview)) {
				delete column;
				return false;
			}
			addChild(column);
		} else { // unknown element
			reader->raiseWarning(i18n("unknown element '%1'", reader->name().toString()));
			if (!reader->skipToEndElement()) return false;
		}
	}

	return true;
}
