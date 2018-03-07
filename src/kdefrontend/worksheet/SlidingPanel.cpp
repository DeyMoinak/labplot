/***************************************************************************
File                 : SlidingPanel.cpp
Project              : LabPlot
Description          : Sliding panel shown in the presenter widget
--------------------------------------------------------------------
Copyright            : (C) 2016 by Fabian Kristof (fkristofszabolcs@gmail.com)
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
#include "SlidingPanel.h"

#include <QLabel>
#include <QPushButton>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QApplication>
#include <KLocale>

SlidingPanel::SlidingPanel(QWidget *parent, const QString &worksheetName) : QFrame(parent) {
	setAttribute(Qt::WA_DeleteOnClose);

	m_worksheetName = new QLabel(worksheetName);
	QFont nameFont;
	nameFont.setPointSize(20);
	nameFont.setBold(true);
	m_worksheetName->setFont(nameFont);

	m_quitPresentingMode = new QPushButton(i18n("Quit presentation"));
	m_quitPresentingMode->setIcon(QIcon::fromTheme(QLatin1String("window-close")));

	QHBoxLayout* hlayout = new QHBoxLayout;
	hlayout->addWidget(m_worksheetName);
	QSpacerItem* spacer = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
	hlayout->addItem(spacer);
	hlayout->addWidget(m_quitPresentingMode);
	setLayout(hlayout);

	QPalette pal(palette());
	pal.setColor(QPalette::Background, Qt::gray);
	setAutoFillBackground(true);
	setPalette(pal);

	move(0, 0);
	raise();
	show();
}

SlidingPanel::~SlidingPanel() {
	delete m_worksheetName;
	delete m_quitPresentingMode;
}

void SlidingPanel::movePanel(qreal value) {
	move(0, -height() + static_cast<int>(value * height()) );
	raise();
}

QPushButton* SlidingPanel::quitButton() const {
	return m_quitPresentingMode;
}

QSize SlidingPanel::sizeHint() const {
	QSize sh;
	QDesktopWidget* const dw = QApplication::desktop();
	const int primaryScreenIdx = dw->primaryScreen();
	const QRect& screenSize = dw->availableGeometry(primaryScreenIdx);
	sh.setWidth(screenSize.width());

	//for the height use 1.5 times the height of the font used in the label (20 points) in pixels
	QFont font;
	font.setPointSize(20);
	const QFontMetrics fm(font);
	sh.setHeight(1.5*fm.ascent());

	return sh;
}
