/*
    File                 : RandomValuesDialog.cpp
    Project              : LabPlot
    Description          : Dialog for generating non-uniformly distributed random numbers
    --------------------------------------------------------------------
    SPDX-FileCopyrightText: 2014-2019 Alexander Semke <alexander.semke@web.de>
    SPDX-FileCopyrightText: 2016-2021 Stefan Gerlach <stefan.gerlach@uni.kn>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kdefrontend/GuiTools.h"
#include "RandomValuesDialog.h"
#include "backend/core/column/Column.h"
#include "backend/lib/macros.h"
#include "backend/spreadsheet/Spreadsheet.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QPushButton>
#include <QStandardPaths>
#include <QWindow>

#include <KLocalizedString>
#include <KWindowConfig>

extern "C" {
#include "backend/nsl/nsl_sf_stats.h"
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
}

/*!
	\class RandomValuesDialog
	\brief Dialog for generating non-uniform random numbers.

	\ingroup kdefrontend
 */

RandomValuesDialog::RandomValuesDialog(Spreadsheet* s, QWidget* parent) : QDialog(parent), m_spreadsheet(s) {
	setWindowTitle(i18nc("@title:window", "Random Values"));

	QWidget* mainWidget = new QWidget(this);
	ui.setupUi(mainWidget);
	auto* layout = new QVBoxLayout(this);

	auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	m_okButton = buttonBox->button(QDialogButtonBox::Ok);
	m_okButton->setDefault(true);
	m_okButton->setToolTip(i18n("Generate random values according to the selected distribution"));
	m_okButton->setText(i18n("&Generate"));

	connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &RandomValuesDialog::close);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &RandomValuesDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &RandomValuesDialog::reject);

	layout->addWidget(mainWidget);
	layout->addWidget(buttonBox);
	setLayout(layout);
	setAttribute(Qt::WA_DeleteOnClose);
	QVector<QPair<QString, int>> distros;
	for (int i = 0; i < NSL_SF_STATS_DISTRIBUTION_RNG_COUNT; i++)
		distros << QPair<QString, int>(i18n(nsl_sf_stats_distribution_name[i]), i);
	std::sort(std::begin(distros), std::end(distros));
	for (const auto& d : distros)
		ui.cbDistribution->addItem(d.first, d.second);
	const int defaultDist = (int)nsl_sf_stats_gaussian;	// default dist

	//use white background in the preview label
	QPalette p;
	p.setColor(QPalette::Window, Qt::white);
	ui.lFuncPic->setAutoFillBackground(true);
	ui.lFuncPic->setPalette(p);
	ui.lFuncPic->setScaledContents(false);

	ui.leParameter1->setClearButtonEnabled(true);
	ui.leParameter2->setClearButtonEnabled(true);
	ui.leParameter3->setClearButtonEnabled(true);

	ui.leParameter1->setValidator( new QDoubleValidator(ui.leParameter1) );
	ui.leParameter2->setValidator( new QDoubleValidator(ui.leParameter2) );
	ui.leParameter3->setValidator( new QDoubleValidator(ui.leParameter3) );

	connect(ui.cbDistribution, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &RandomValuesDialog::distributionChanged);
	connect(ui.leParameter1, &QLineEdit::textChanged, this, &RandomValuesDialog::checkValues);
	connect(ui.leParameter2, &QLineEdit::textChanged, this, &RandomValuesDialog::checkValues);
	connect(ui.leParameter3, &QLineEdit::textChanged, this, &RandomValuesDialog::checkValues);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &RandomValuesDialog::generate);

	//restore saved settings if available
	create(); // ensure there's a window created
	const KConfigGroup conf(KSharedConfig::openConfig(), "RandomValuesDialog");
	if (conf.exists()) {
		const int dist = conf.readEntry("Distribution", defaultDist);
		ui.cbDistribution->setCurrentIndex(ui.cbDistribution->findData(dist));
		if (ui.cbDistribution->currentIndex() == 0)	// if index=0 no signal is emitted above, call this slot directly
			this->distributionChanged();
		SET_NUMBER_LOCALE
		// read parameter or set values for default dist
		ui.leParameter1->setText(numberLocale.toString(conf.readEntry("Parameter1", 0.0)));
		ui.leParameter2->setText(numberLocale.toString(conf.readEntry("Parameter2", 1.0)));
		ui.leParameter3->setText(numberLocale.toString(conf.readEntry("Parameter3", 1.0)));

		KWindowConfig::restoreWindowSize(windowHandle(), conf);
		resize(windowHandle()->size()); // workaround for QTBUG-40584
	} else {
		ui.cbDistribution->setCurrentIndex(ui.cbDistribution->findData(defaultDist));

		resize( QSize(400, 0).expandedTo(minimumSize()) );
	}
}

RandomValuesDialog::~RandomValuesDialog() {
	//save current settings
	KConfigGroup conf(KSharedConfig::openConfig(), "RandomValuesDialog");
	// saving enum value to be consistent
	conf.writeEntry("Distribution", ui.cbDistribution->itemData(ui.cbDistribution->currentIndex()).toInt());
	SET_NUMBER_LOCALE
	conf.writeEntry("Parameter1", numberLocale.toDouble(ui.leParameter1->text()));
	conf.writeEntry("Parameter2", numberLocale.toDouble(ui.leParameter2->text()));
	conf.writeEntry("Parameter3", numberLocale.toDouble(ui.leParameter3->text()));

	KWindowConfig::saveWindowSize(windowHandle(), conf);
}

void RandomValuesDialog::setColumns(const QVector<Column*>& columns) {
	m_columns = columns;
}

void RandomValuesDialog::distributionChanged(int index) {
	DEBUG(Q_FUNC_INFO << ", index = " << index)
	const nsl_sf_stats_distribution dist = (nsl_sf_stats_distribution)ui.cbDistribution->itemData(index).toInt();
	DEBUG(Q_FUNC_INFO << ", dist = " << nsl_sf_stats_distribution_name[(int)dist])

	//  default settings (used by most distributions)
	ui.lParameter1->show();
	ui.leParameter1->show();
	ui.lParameter2->show();
	ui.leParameter2->show();
	ui.lParameter3->hide();
	ui.leParameter3->hide();
	ui.lFunc->setText("p(x) =");

	SET_NUMBER_LOCALE
	switch (dist) {
	case nsl_sf_stats_gaussian:
		ui.lParameter1->setText(UTF8_QSTRING("μ ="));
		ui.lParameter2->setText(UTF8_QSTRING("σ ="));
		ui.leParameter1->setText(numberLocale.toString(0.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_gaussian_tail:
		ui.lParameter3->show();
		ui.leParameter3->show();
		ui.lParameter1->setText(UTF8_QSTRING("μ ="));
		ui.lParameter2->setText(UTF8_QSTRING("σ ="));
		ui.lParameter3->setText("a =");
		ui.leParameter1->setText(numberLocale.toString(0.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		ui.leParameter3->setText(numberLocale.toString(0.0));
		break;
	case nsl_sf_stats_exponential:
		ui.lParameter1->setText(UTF8_QSTRING("λ ="));
		ui.lParameter2->setText(UTF8_QSTRING("μ ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(0.0));
		break;
	case nsl_sf_stats_laplace:
		ui.lParameter1->setText(UTF8_QSTRING("μ ="));
		ui.lParameter2->setText(UTF8_QSTRING("σ ="));
		ui.leParameter1->setText(numberLocale.toString(0.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_exponential_power:
		ui.lParameter3->show();
		ui.leParameter3->show();
		ui.lParameter1->setText(UTF8_QSTRING("μ ="));
		ui.lParameter2->setText(UTF8_QSTRING("σ ="));
		ui.lParameter3->setText("b =");
		ui.leParameter1->setText(numberLocale.toString(0.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		ui.leParameter3->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_cauchy_lorentz:
		ui.lParameter1->setText(UTF8_QSTRING("γ ="));
		ui.lParameter2->setText(UTF8_QSTRING("μ ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(0.0));
		break;
	case nsl_sf_stats_rayleigh:
		ui.lParameter2->hide();
		ui.leParameter2->hide();
		ui.lParameter1->setText(UTF8_QSTRING("σ ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_rayleigh_tail:
		ui.lParameter1->setText(UTF8_QSTRING("μ ="));
		ui.lParameter2->setText(UTF8_QSTRING("σ ="));
		ui.leParameter1->setText(numberLocale.toString(0.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_landau:
		ui.lParameter1->hide();
		ui.leParameter1->hide();
		ui.lParameter2->hide();
		ui.leParameter2->hide();
		break;
	case nsl_sf_stats_levy_alpha_stable:
		ui.lParameter1->setText("c =");
		ui.lParameter2->setText(UTF8_QSTRING("α ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_levy_skew_alpha_stable:
		ui.lParameter3->show();
		ui.leParameter3->show();
		ui.lParameter1->setText(UTF8_QSTRING("c ="));
		ui.lParameter2->setText(UTF8_QSTRING("α ="));
		ui.lParameter3->setText(UTF8_QSTRING("β ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		ui.leParameter3->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_flat:
		ui.lParameter1->setText("a =");
		ui.lParameter2->setText("b =");
		ui.leParameter1->setText(numberLocale.toString(0.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_gamma:
		ui.lParameter1->setText(UTF8_QSTRING("θ ="));
		ui.lParameter2->setText("k =");
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_weibull:
		ui.lParameter3->show();
		ui.leParameter3->show();
		ui.lParameter1->setText("k =");
		ui.lParameter2->setText(UTF8_QSTRING("λ ="));
		ui.lParameter3->setText(UTF8_QSTRING("μ ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		ui.leParameter3->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_beta:
		ui.lParameter1->setText("a =");
		ui.lParameter2->setText("b =");
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_gumbel1:
		ui.lParameter3->show();
		ui.leParameter3->show();
		ui.lParameter1->setText(UTF8_QSTRING("σ ="));
		ui.lParameter2->setText(UTF8_QSTRING("β ="));
		ui.lParameter3->setText(UTF8_QSTRING("μ ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		ui.leParameter3->setText(numberLocale.toString(0.0));
		break;
	case nsl_sf_stats_gumbel2:
		ui.lParameter3->show();
		ui.leParameter3->show();
		ui.lParameter1->setText("a =");
		ui.lParameter2->setText("b =");
		ui.lParameter3->setText(UTF8_QSTRING("μ ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		ui.leParameter3->setText(numberLocale.toString(0.0));
		break;
	case nsl_sf_stats_pareto:
		ui.lParameter1->setText("a =");
		ui.lParameter2->setText("b =");
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(0.0));
		break;
	case nsl_sf_stats_lognormal:
		ui.lParameter1->setText(UTF8_QSTRING("μ ="));
		ui.lParameter2->setText(UTF8_QSTRING("σ ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_chi_squared:
		ui.lParameter2->hide();
		ui.leParameter2->hide();
		ui.lParameter1->setText("n =");
		ui.leParameter1->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_fdist:
		ui.lParameter1->setText(UTF8_QSTRING("ν₁ ="));
		ui.lParameter2->setText(UTF8_QSTRING("ν₂ ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(1.0));
		break;
	case nsl_sf_stats_tdist:
		ui.lParameter2->hide();
		ui.leParameter2->hide();
		ui.lParameter1->setText(UTF8_QSTRING("ν ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter1->setText("1.0");
		break;
	case nsl_sf_stats_logistic:
		ui.lParameter1->setText(UTF8_QSTRING("σ ="));
		ui.lParameter2->setText(UTF8_QSTRING("μ ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(0.0));
		break;
	case nsl_sf_stats_poisson:
		ui.lParameter2->hide();
		ui.leParameter2->hide();
		ui.lFunc->setText("p(k) =");
		ui.lParameter1->setText(UTF8_QSTRING("λ ="));
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter1->setText("1.0");
		break;
	case nsl_sf_stats_bernoulli:
	case nsl_sf_stats_geometric:
	case nsl_sf_stats_logarithmic:
		ui.lParameter2->hide();
		ui.leParameter2->hide();
		if (dist == nsl_sf_stats_bernoulli)
			ui.lFunc->setText(QString());
		else
			ui.lFunc->setText("p(k) =");
		ui.lParameter1->setText("p =");
		ui.leParameter1->setText(numberLocale.toString(0.5));
		break;
	case nsl_sf_stats_binomial:
	case nsl_sf_stats_negative_binomial:
	case nsl_sf_stats_pascal:
		ui.lFunc->setText("p(k) =");
		ui.lParameter1->setText("p =");
		ui.lParameter2->setText("n =");
		ui.leParameter1->setText(numberLocale.toString(0.5));
		ui.leParameter2->setText(numberLocale.toString(100));
		break;
	case nsl_sf_stats_hypergeometric:
		ui.lParameter3->show();
		ui.leParameter3->show();
		ui.lFunc->setText("p(k) =");
		ui.lParameter1->setText("n1 =");
		ui.lParameter2->setText("n2 =");
		ui.lParameter3->setText("t =");
		ui.leParameter1->setText(numberLocale.toString(1.0));
		ui.leParameter2->setText(numberLocale.toString(2.0));
		ui.leParameter3->setText(numberLocale.toString(3.0));
		break;
	case nsl_sf_stats_maxwell_boltzmann:	// additional non-GSL distros
	case nsl_sf_stats_sech:
	case nsl_sf_stats_levy:
	case nsl_sf_stats_frechet:
		break;
	}

	QString file = QStandardPaths::locate( QStandardPaths::AppDataLocation,
			"pics/gsl_distributions/" + QString(nsl_sf_stats_distribution_pic_name[dist]) + QLatin1String(".pdf") );
	QImage image = GuiTools::importPDFFile(file);

	if (image.isNull()) {
		ui.lFunc->hide();
		ui.lFuncPic->hide();
	} else {
		ui.lFuncPic->setPixmap(QPixmap::fromImage(image));
		ui.lFuncPic->show();
	}
}

void RandomValuesDialog::checkValues() {
	if (ui.leParameter1->text().simplified().isEmpty()) {
		m_okButton->setEnabled(false);
		return;
	}

	if (ui.leParameter2->isVisible() && ui.leParameter2->text().simplified().isEmpty()) {
		m_okButton->setEnabled(false);
		return;
	}

	if (ui.leParameter3->isVisible() && ui.leParameter3->text().simplified().isEmpty()) {
		m_okButton->setEnabled(false);
		return;
	}

	m_okButton->setEnabled(true);
	return;
}

void RandomValuesDialog::generate() {
	Q_ASSERT(m_spreadsheet);

	//create a generator chosen by the environment variable GSL_RNG_TYPE
	gsl_rng_env_setup();
	const gsl_rng_type* T = gsl_rng_default;
	gsl_rng* r = gsl_rng_alloc(T);
	gsl_rng_set(r, QDateTime::currentMSecsSinceEpoch());

	WAIT_CURSOR;
	for (auto* col : m_columns)
		col->setSuppressDataChangedSignal(true);

	m_spreadsheet->beginMacro(i18np("%1: fill column with non-uniform random numbers",
					"%1: fill columns with non-uniform random numbers",
					m_spreadsheet->name(), m_columns.size()));

	const int index = ui.cbDistribution->currentIndex();
	const nsl_sf_stats_distribution dist = (nsl_sf_stats_distribution)ui.cbDistribution->itemData(index).toInt();
	DEBUG(Q_FUNC_INFO << ", random number distribution: " << nsl_sf_stats_distribution_name[dist]);

	const int rows = m_spreadsheet->rowCount();
	QVector<double> data(rows);
	QVector<int> data_int(rows);
	QVector<qint64> data_bigint(rows);

	switch (dist) {
	case nsl_sf_stats_gaussian: {
		double mu{0.0}, sigma{1.0};
		SET_DOUBLE_FROM_LE(mu, ui.leParameter1)
		SET_DOUBLE_FROM_LE(sigma, ui.leParameter2)
		DEBUG(Q_FUNC_INFO << ", mu = " << mu << ", sigma = " << sigma);
		for (auto* col : m_columns) {
			auto mode = col->columnMode();
			if (mode == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_gaussian(r, sigma) + mu;
				col->replaceValues(0, data);
			} else if (mode == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_gaussian(r, sigma) + mu);
				col->replaceInteger(0, data_int);
			} else if (mode == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_gaussian(r, sigma) + mu);
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_gaussian_tail: {
		double mu{0.0}, sigma{1.0}, a{0.0};
		SET_DOUBLE_FROM_LE(mu, ui.leParameter1)
		SET_DOUBLE_FROM_LE(sigma, ui.leParameter2)
		SET_DOUBLE_FROM_LE(a, ui.leParameter3)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_gaussian_tail(r, a, sigma) + mu;
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_gaussian_tail(r, a, sigma) + mu);
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_gaussian_tail(r, a, sigma) + mu);
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_exponential: {
		double l{1.0}, mu{0.0};
		SET_DOUBLE_FROM_LE(l, ui.leParameter1)
		SET_DOUBLE_FROM_LE(mu, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				//GSL uses the inverse for exp. distrib.
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_exponential(r, 1./l) + mu;
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_exponential(r, 1./l) + mu);
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_exponential(r, 1./l) + mu);
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_laplace: {
		double mu{0.0}, s{1.0};
		SET_DOUBLE_FROM_LE(mu, ui.leParameter1)
		SET_DOUBLE_FROM_LE(s, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_laplace(r, s) + mu;
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_laplace(r, s) + mu);
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_laplace(r, s) + mu);
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_exponential_power: {
		double mu{0.0}, a{1.0}, b{1.0};
		SET_DOUBLE_FROM_LE(mu, ui.leParameter1)
		SET_DOUBLE_FROM_LE(a, ui.leParameter2)
		SET_DOUBLE_FROM_LE(b, ui.leParameter3)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_exppow(r, a, b) + mu;
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_exppow(r, a, b) + mu);
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_exppow(r, a, b) + mu);
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_cauchy_lorentz: {
		double gamma{1.0}, mu{0.0};
		SET_DOUBLE_FROM_LE(gamma, ui.leParameter1)
		SET_DOUBLE_FROM_LE(mu, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_cauchy(r, gamma) + mu;
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_cauchy(r, gamma) + mu);
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_cauchy(r, gamma) + mu);
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_rayleigh: {
		double s{1.0};
		SET_DOUBLE_FROM_LE(s, ui.leParameter1)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_rayleigh(r, s);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_rayleigh(r, s));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_rayleigh(r, s));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_rayleigh_tail: {
		double mu{0.0}, sigma{1.0};
		SET_DOUBLE_FROM_LE(mu, ui.leParameter1)
		SET_DOUBLE_FROM_LE(sigma, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_rayleigh_tail(r, mu, sigma);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_rayleigh_tail(r, mu, sigma));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_rayleigh_tail(r, mu, sigma));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_landau:
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_landau(r);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_landau(r));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_landau(r));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	case nsl_sf_stats_levy_alpha_stable: {
		double c{1.0}, alpha{1.0};
		SET_DOUBLE_FROM_LE(c, ui.leParameter1)
		SET_DOUBLE_FROM_LE(alpha, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_levy(r, c, alpha);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_levy(r, c, alpha));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_levy(r, c, alpha));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_levy_skew_alpha_stable: {
		double c{1.0}, alpha{1.0}, beta{1.0};
		SET_DOUBLE_FROM_LE(c, ui.leParameter1)
		SET_DOUBLE_FROM_LE(alpha, ui.leParameter2)
		SET_DOUBLE_FROM_LE(beta, ui.leParameter3)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_levy_skew(r, c, alpha, beta);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_levy_skew(r, c, alpha, beta));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_levy_skew(r, c, alpha, beta));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_flat: {
		double a{0.0}, b{1.0};
		SET_DOUBLE_FROM_LE(a, ui.leParameter1)
		SET_DOUBLE_FROM_LE(b, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_flat(r, a, b);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_flat(r, a, b));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_flat(r, a, b));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_gamma: {
		double a{1.0}, b{1.0};
		SET_DOUBLE_FROM_LE(a, ui.leParameter1)
		SET_DOUBLE_FROM_LE(b, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_gamma(r, a, b);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_gamma(r, a, b));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_gamma(r, a, b));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_lognormal: {
		double s{1.0}, mu{1.0};
		SET_DOUBLE_FROM_LE(mu, ui.leParameter1)
		SET_DOUBLE_FROM_LE(s, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_lognormal(r, mu, s);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_lognormal(r, mu, s));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_lognormal(r, mu, s));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_chi_squared: {
		double n{1.0};
		SET_DOUBLE_FROM_LE(n, ui.leParameter1)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_chisq(r, n);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_chisq(r, n));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_chisq(r, n));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_fdist: {
		double nu1{1.0}, nu2{1.0};
		SET_DOUBLE_FROM_LE(nu1, ui.leParameter1)
		SET_DOUBLE_FROM_LE(nu2, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_fdist(r, nu1, nu2);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_fdist(r, nu1, nu2));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_fdist(r, nu1, nu2));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_tdist: {
		double nu{1.0};
		SET_DOUBLE_FROM_LE(nu, ui.leParameter1)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_tdist(r, nu);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_tdist(r, nu));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_tdist(r, nu));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_beta: {
		double a{1.0}, b{1.0};
		SET_DOUBLE_FROM_LE(a, ui.leParameter1)
		SET_DOUBLE_FROM_LE(b, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_beta(r, a, b);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_beta(r, a, b));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_beta(r, a, b));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_logistic: {
		double s{1.0}, mu{0.0};
		SET_DOUBLE_FROM_LE(s, ui.leParameter1)
		SET_DOUBLE_FROM_LE(mu, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_logistic(r, s) + mu;
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_logistic(r, s) + mu);
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_logistic(r, s) + mu);
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_pareto: {
		double a{1.0}, b{0.0};
		SET_DOUBLE_FROM_LE(a, ui.leParameter1)
		SET_DOUBLE_FROM_LE(b, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_pareto(r, a, b);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_pareto(r, a, b));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_pareto(r, a, b));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_weibull: {
		double k{1.0}, l{1.0}, mu{1.0};
		SET_DOUBLE_FROM_LE(k, ui.leParameter1)
		SET_DOUBLE_FROM_LE(l, ui.leParameter2)
		SET_DOUBLE_FROM_LE(mu, ui.leParameter3)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_weibull(r, l, k) + mu;
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_weibull(r, l, k) + mu);
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_weibull(r, l, k) + mu);
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_gumbel1: {
		double s{1.0}, b{1.0}, mu{0.0};
		SET_DOUBLE_FROM_LE(s, ui.leParameter1)
		SET_DOUBLE_FROM_LE(b, ui.leParameter2)
		SET_DOUBLE_FROM_LE(mu, ui.leParameter3)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_gumbel1(r, 1./s, b) + mu;
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_gumbel1(r, 1./s, b) + mu);
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_gumbel1(r, 1./s, b) + mu);
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_gumbel2: {
		double a{1.0}, b{1.0}, mu{0.0};
		SET_DOUBLE_FROM_LE(a, ui.leParameter1)
		SET_DOUBLE_FROM_LE(b, ui.leParameter2)
		SET_DOUBLE_FROM_LE(mu, ui.leParameter3)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_gumbel2(r, a, b) + mu;
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_gumbel2(r, a, b) + mu);
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_gumbel2(r, a, b) + mu);
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_poisson: {
		double l{1.0};
		SET_DOUBLE_FROM_LE(l, ui.leParameter1)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_poisson(r, l);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_poisson(r, l));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_poisson(r, l));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_bernoulli: {
		double p{1.0};
		SET_DOUBLE_FROM_LE(p, ui.leParameter1)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_bernoulli(r, p);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_bernoulli(r, p));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_bernoulli(r, p));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_binomial: {
		double p{0.5}, n{100.};
		SET_DOUBLE_FROM_LE(p, ui.leParameter1)
		SET_DOUBLE_FROM_LE(n, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_binomial(r, p, n);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_binomial(r, p, n));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_binomial(r, p, n));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_negative_binomial: {
		double p{0.5}, n{100.};
		SET_DOUBLE_FROM_LE(p, ui.leParameter1)
		SET_DOUBLE_FROM_LE(n, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_negative_binomial(r, p, n);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_negative_binomial(r, p, n));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_negative_binomial(r, p, n));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_pascal: {
		double p{0.5}, n{100.};
		SET_DOUBLE_FROM_LE(p, ui.leParameter1)
		SET_DOUBLE_FROM_LE(n, ui.leParameter2)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_pascal(r, p, n);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_pascal(r, p, n));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_pascal(r, p, n));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_geometric: {
		double p{0.5};
		SET_DOUBLE_FROM_LE(p, ui.leParameter1)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_geometric(r, p);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_geometric(r, p));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_geometric(r, p));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_hypergeometric: {
		double n1{1.0}, n2{2.0}, t{3.0};
		SET_DOUBLE_FROM_LE(n1, ui.leParameter1)
		SET_DOUBLE_FROM_LE(n2, ui.leParameter2)
		SET_DOUBLE_FROM_LE(t, ui.leParameter3)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_hypergeometric(r, n1, n2, t);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_hypergeometric(r, n1, n2, t));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_hypergeometric(r, n1, n2, t));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	case nsl_sf_stats_logarithmic: {
		double p{0.5};
		SET_DOUBLE_FROM_LE(p, ui.leParameter1)
		for (auto* col : m_columns) {
			if (col->columnMode() == AbstractColumn::ColumnMode::Double) {
				for (int i = 0; i < rows; ++i)
					data[i] = gsl_ran_logarithmic(r, p);
				col->replaceValues(0, data);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::Integer) {
				for (int i = 0; i < rows; ++i)
					data_int[i] = (int)round(gsl_ran_logarithmic(r, p));
				col->replaceInteger(0, data_int);
			} else if (col->columnMode() == AbstractColumn::ColumnMode::BigInt) {
				for (int i = 0; i < rows; ++i)
					data_bigint[i] = (qint64)round(gsl_ran_logarithmic(r, p));
				col->replaceBigInt(0, data_bigint);
			}
		}
		break;
	}
	// additional non-GSL distributions not needed
	case nsl_sf_stats_maxwell_boltzmann:
	case nsl_sf_stats_sech:
	case nsl_sf_stats_levy:
	case nsl_sf_stats_frechet:
		break;
	}

	for (auto* col : m_columns) {
		col->setSuppressDataChangedSignal(false);
		col->setChanged();
	}
	m_spreadsheet->endMacro();
	RESET_CURSOR;

	gsl_rng_free(r);
}
