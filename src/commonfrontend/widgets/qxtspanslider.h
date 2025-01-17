/*
 * SPDX-FileCopyrightText: 2006-2011 the LibQxt project <http://libqxt.org, foundation@libqxt.org>
 * SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef QXTSPANSLIDER_H
#define QXTSPANSLIDER_H

#include <QSlider>
#include "qxtnamespace.h"
#include "qxtglobal.h"

class QxtSpanSliderPrivate;

class QxtSpanSlider : public QSlider {
	Q_OBJECT
	QXT_DECLARE_PRIVATE(QxtSpanSlider)
	Q_PROPERTY(int lowerValue READ lowerValue WRITE setLowerValue)
	Q_PROPERTY(int upperValue READ upperValue WRITE setUpperValue)
	Q_PROPERTY(int lowerPosition READ lowerPosition WRITE setLowerPosition)
	Q_PROPERTY(int upperPosition READ upperPosition WRITE setUpperPosition)
	Q_PROPERTY(HandleMovementMode handleMovementMode READ handleMovementMode WRITE setHandleMovementMode)
	Q_ENUMS(HandleMovementMode)

public:
	explicit QxtSpanSlider(QWidget* parent = nullptr);
	explicit QxtSpanSlider(Qt::Orientation orientation, QWidget* parent = nullptr);
	~QxtSpanSlider() override;

	enum HandleMovementMode {FreeMovement, NoCrossing, NoOverlapping};
	enum SpanHandle {NoHandle, LowerHandle, UpperHandle};

	HandleMovementMode handleMovementMode() const;
	void setHandleMovementMode(HandleMovementMode);

	int lowerValue() const;
	int upperValue() const;

	int lowerPosition() const;
	int upperPosition() const;

public Q_SLOTS:
	void setLowerValue(int lower);
	void setUpperValue(int upper);
	void setSpan(int lower, int upper);

	void setLowerPosition(int lower);
	void setUpperPosition(int upper);

Q_SIGNALS:
	void spanChanged(int lower, int upper);
	void lowerValueChanged(int lower);
	void upperValueChanged(int upper);

	void lowerPositionChanged(int lower);
	void upperPositionChanged(int upper);

	void sliderPressed(QxtSpanSlider::SpanHandle);

protected:
	void keyPressEvent(QKeyEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseMoveEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void paintEvent(QPaintEvent*) override;
};

#endif // QXTSPANSLIDER_H
