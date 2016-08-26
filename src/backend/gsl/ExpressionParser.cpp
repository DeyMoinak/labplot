/***************************************************************************
    File             : ExpressionParser.cpp
    Project          : LabPlot
    --------------------------------------------------------------------
    Copyright        : (C) 2014 Alexander Semke (alexander.semke@web.de)
    Copyright        : (C) 2014 Stefan Gerlach (stefan.gerlach@uni.kn)
    Description      : C++ wrapper for the bison generated parser.

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

#include "backend/gsl/ExpressionParser.h"
#include "backend/gsl/parser_extern.h"
#include "backend/gsl/parser_struct.h"

#include <klocale.h>

extern "C" {
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_const_num.h>
}

ExpressionParser* ExpressionParser::instance = NULL;

ExpressionParser::ExpressionParser(){
	init_table();
	initFunctions();
	initConstants();
}

void ExpressionParser::initFunctions() {
	//functions
	for (int i = 0; _functions[i].name != 0; i++)
		m_functions << _functions[i].name;

	m_functionsGroups << i18n("Standard Mathematical functions");
	m_functionsGroups << i18n("GSL Mathematical functions");
	//http://www.gnu.org/software/gsl/manual/html_node/Special-Functions.html
	m_functionsGroups << i18n("Airy Functions and Derivatives");
	m_functionsGroups << i18n("Bessel Functions");
	m_functionsGroups << i18n("Clausen Functions");
	m_functionsGroups << i18n("Coulomb Functions");
// 	m_functionsGroups << i18n("Coupling Coefficients");
	m_functionsGroups << i18n("Dawson Function");
	m_functionsGroups << i18n("Debye Functions");
	m_functionsGroups << i18n("Dilogarithm");
//	m_functionsGroups << i18n("Elementary Operations");
	m_functionsGroups << i18n("Elliptic Integrals");
//	m_functionsGroups << i18n("Elliptic Functions (Jacobi)");
	m_functionsGroups << i18n("Error Functions");
	m_functionsGroups << i18n("Exponential Functions");
	m_functionsGroups << i18n("Exponential Integrals");
	m_functionsGroups << i18n("Fermi-Dirac Function");
	m_functionsGroups << i18n("Gamma and Beta Functions");
	m_functionsGroups << i18n("Gegenbauer Functions");
	m_functionsGroups << i18n("Hypergeometric Functions");
	m_functionsGroups << i18n("Laguerre Functions");
	m_functionsGroups << i18n("Lambert W Functions");
	m_functionsGroups << i18n("Legendre Functions and Spherical Harmonics");
	m_functionsGroups << i18n("Logarithm and Related Functions");
//	m_functionsGroups << i18n("Mathieu Functions");
	m_functionsGroups << i18n("Power Function");
	m_functionsGroups << i18n("Psi (Digamma) Function");
	m_functionsGroups << i18n("Synchrotron Functions");
	m_functionsGroups << i18n("Transport Functions");
	m_functionsGroups << i18n("Trigonometric Functions");
	m_functionsGroups << i18n("Zeta Functions");
	// GSL random distribution functions
	m_functionsGroups << i18n("Gaussian Distribution");
	m_functionsGroups << i18n("Exponential Distribution");
	m_functionsGroups << i18n("Laplace Distribution");
	m_functionsGroups << i18n("Exponential Power Distribution");
	m_functionsGroups << i18n("Cauchy Distribution");
	m_functionsGroups << i18n("Rayleigh Distribution");
	m_functionsGroups << i18n("Landau Distribution");
	m_functionsGroups << i18n("Gamma Distribution");
	m_functionsGroups << i18n("Flat (Uniform) Distribution");
	m_functionsGroups << i18n("Lognormal Distribution");
	m_functionsGroups << i18n("Chi-squared Distribution");
	m_functionsGroups << i18n("F-distribution");
	m_functionsGroups << i18n("t-distribution");
	m_functionsGroups << i18n("Beta Distribution");
	m_functionsGroups << i18n("Logistic Distribution");
	m_functionsGroups << i18n("Pareto Distribution");
	m_functionsGroups << i18n("Weibull Distribution");
	m_functionsGroups << i18n("Gumbel Distribution");
	m_functionsGroups << i18n("Poisson Distribution");
	m_functionsGroups << i18n("Bernoulli Distribution");
	m_functionsGroups << i18n("Binomial Distribution");
	m_functionsGroups << i18n("Pascal Distribution");
	m_functionsGroups << i18n("Geometric Distribution");
	m_functionsGroups << i18n("Hypergeometric Distribution");
	m_functionsGroups << i18n("Logarithmic Distribution");

	// Standard mathematical functions
	m_functionsNames << i18n("pseudo-random integer [0,RAND_MAX]");
	m_functionsNames << i18n("nonlinear additive feedback rng [0,RAND_MAX]");
	m_functionsNames << i18n("nonlinear additive feedback rng [0,1]");
	m_functionsNames << i18n("Arc cosine");
	m_functionsNames << i18n("Inverse hyperbolic cosine");
	m_functionsNames << i18n("Principal value of the arc sine");
	m_functionsNames << i18n("Inverse hyperbolic sine");
	m_functionsNames << i18n("Principal value of the arc tangent");
	m_functionsNames << i18n("atan() using sign");
	m_functionsNames << i18n("Inverse hyperbolic tangent");

	m_functionsNames << i18n("Cube root");
	m_functionsNames << i18n("Smallest integral value not less");
	m_functionsNames << i18n("Hyperbolic cosine");
	m_functionsNames << i18n("Absolute value");
	m_functionsNames << i18n("x * 2^y");
	m_functionsNames << i18n("Base 10 logarithm");
	m_functionsNames << i18n("Extract the exponent");
	m_functionsNames << i18n("Power function [x^y]");
	m_functionsNames << i18n("Round to an integer value");
	m_functionsNames << i18n("Round to the nearest integer");
	m_functionsNames << i18n("Hyperbolic sine");
	m_functionsNames << i18n("Nonnegative square root");

	m_functionsNames << i18n("Tangent");
	m_functionsNames << i18n("Hyperbolic tangent");
	m_functionsNames << i18n("Round to the nearest integer");
//	m_functionsNames << i18n("Bessel function of first kind, orders 0");
//	m_functionsNames << i18n("Bessel function of first kind, orders 1");
//	m_functionsNames << i18n("Bessel function of first kind, orders n");
//	m_functionsNames << i18n("Bessel function of second kind, orders 0");
//	m_functionsNames << i18n("Bessel function of second kind, orders 1");
//	m_functionsNames << i18n("Bessel function of second kind, orders n");
//	m_functionsNames << i18n("Error function");
//	m_functionsNames << i18n("Complementary error function");
//	m_functionsNames << i18n("Exponential function");
//	m_functionsNames << i18n("exp() - 1");
//	m_functionsNames << i18n("Logarithm of the gamma function");
//	m_functionsNames << i18n("Natural logarithm of the Gamma function (absolute value)");
//	m_functionsNames << i18n("(True) Gamma function");
//	m_functionsNames << i18n("Natural logarithm");
//	m_functionsNames << i18n("Natural logarithm");
//	m_functionsNames << i18n("log (1 + x)");
//	m_functionsNames << i18n("Cosine");
//	m_functionsNames << i18n("Length of the hypotenuse sqrt(x*x+y*y)");
//	m_functionsNames << i18n("Sine");

	for(int i=0;i<25;i++)
		m_functionsGroupIndex << 0;

	// GSL mathematical functions
	m_functionsNames << i18n("log(1+x)");
	m_functionsNames << i18n("exp(x)-1");
	m_functionsNames << i18n("sqrt(x^2+y^2)");
	m_functionsNames << i18n("sqrt(x^2+y^2+z^2)");
	m_functionsNames << i18n("arccosh(x)");
	m_functionsNames << i18n("arcsinh(x)");
	m_functionsNames << i18n("arctanh(x)");
	m_functionsNames << i18n("x * 2^e");
	m_functionsNames << i18n("x^n");
	m_functionsNames << i18n("x^2");

	m_functionsNames << i18n("x^3");
	m_functionsNames << i18n("x^4");
	m_functionsNames << i18n("x^5");
	m_functionsNames << i18n("x^6");
	m_functionsNames << i18n("x^7");
	m_functionsNames << i18n("x^8");
	m_functionsNames << i18n("x^9");

	for(int i=0;i<17;i++)
		m_functionsGroupIndex << 1;


	// Airy Functions and Derivatives
	m_functionsNames << i18n("Airy function of the first kind");
	m_functionsNames << i18n("Airy function of the second kind");
	m_functionsNames << i18n("Scaled Airy function of the first kind");
	m_functionsNames << i18n("Scaled Airy function of the second kind");
	m_functionsNames << i18n("Airy function derivative of the first kind");
	m_functionsNames << i18n("Airy function derivative of the second kind");
	m_functionsNames << i18n("Scaled Airy function derivative of the first kind");
	m_functionsNames << i18n("Scaled Airy function derivative of the second kind");
	m_functionsNames << i18n("n-th zero of the Airy function of the first kind");
	m_functionsNames << i18n("n-th zero of the Airy function of the second kind");
	m_functionsNames << i18n("n-th zero of the Airy function derivative of the first kind");
	m_functionsNames << i18n("n-th zero of the Airy function derivative of the second kind");

	for(int i=0;i<12;i++)
		m_functionsGroupIndex << 2;

	// Bessel Functions
	m_functionsNames << i18n("Regular cylindrical Bessel function of zeroth order");
	m_functionsNames << i18n("Regular cylindrical Bessel function of first order");
	m_functionsNames << i18n("Regular cylindrical Bessel function of order n");
	m_functionsNames << i18n("Irregular cylindrical Bessel function of zeroth order");
	m_functionsNames << i18n("Irregular cylindrical Bessel function of first order");
	m_functionsNames << i18n("Irregular cylindrical Bessel function of order n");
	m_functionsNames << i18n("Regular modified cylindrical Bessel function of zeroth order");
	m_functionsNames << i18n("Regular modified cylindrical Bessel function of first order");
	m_functionsNames << i18n("Regular modified cylindrical Bessel function of order n");
	m_functionsNames << i18n("Scaled regular modified cylindrical Bessel function of zeroth order exp(-|x|) I0(x)");

	m_functionsNames << i18n("Scaled regular modified cylindrical Bessel function of first order exp(-|x|) I1(x)");
	m_functionsNames << i18n("Scaled regular modified cylindrical Bessel function of order n exp(-|x|) In(x)");
	m_functionsNames << i18n("Irregular modified cylindrical Bessel function of zeroth order");
	m_functionsNames << i18n("Irregular modified cylindrical Bessel function of first order");
	m_functionsNames << i18n("Irregular modified cylindrical Bessel function of order n");
	m_functionsNames << i18n("Scaled irregular modified cylindrical Bessel function of zeroth order exp(x) K0(x)");
	m_functionsNames << i18n("Scaled irregular modified cylindrical Bessel function of first order exp(x) K1(x)");
	m_functionsNames << i18n("Scaled irregular modified cylindrical Bessel function of order n exp(x) Kn(x)");
	m_functionsNames << i18n("Regular spherical Bessel function of zeroth order");
	m_functionsNames << i18n("Regular spherical Bessel function of first order");

	m_functionsNames << i18n("Regular spherical Bessel function of second order");
	m_functionsNames << i18n("Regular spherical Bessel function of order l");
	m_functionsNames << i18n("Irregular spherical Bessel function of zeroth order");
	m_functionsNames << i18n("Irregular spherical Bessel function of first order");
	m_functionsNames << i18n("Irregular spherical Bessel function of second order");
	m_functionsNames << i18n("Irregular spherical Bessel function of order l");
	m_functionsNames << i18n("Scaled regular modified spherical Bessel function of zeroth order, exp(-|x|) i0(x)");
	m_functionsNames << i18n("Scaled regular modified spherical Bessel function of first order, exp(-|x|) i1(x)");
	m_functionsNames << i18n("Scaled regular modified spherical Bessel function of second order, exp(-|x|) i2(x)");
	m_functionsNames << i18n("Scaled regular modified spherical Bessel function of order l, exp(-|x|) il(x)");

	m_functionsNames << i18n("Scaled irregular modified spherical Bessel function of zeroth order, exp(x) k0(x)");
	m_functionsNames << i18n("Scaled irregular modified spherical Bessel function of first order, exp(-|x|) k1(x)");
	m_functionsNames << i18n("Scaled irregular modified spherical Bessel function of second order, exp(-|x|) k2(x)");
	m_functionsNames << i18n("Scaled irregular modified spherical Bessel function of order l, exp(-|x|) kl(x)");
	m_functionsNames << i18n("Regular cylindrical Bessel function of fractional order");
	m_functionsNames << i18n("Irregular cylindrical Bessel function of fractional order");
	m_functionsNames << i18n("Regular modified Bessel function of fractional order");
	m_functionsNames << i18n("Scaled regular modified Bessel function of fractional order");
	m_functionsNames << i18n("Irregular modified Bessel function of fractional order");
	m_functionsNames << i18n("Logarithm of irregular modified Bessel function of fractional order");

	m_functionsNames << i18n("Scaled irregular modified Bessel function of fractional order");
	m_functionsNames << i18n("n-th positive zero of the Bessel function J0");
	m_functionsNames << i18n("n-th positive zero of the Bessel function J1");
	m_functionsNames << i18n("n-th positive zero of the Bessel function Jnu");

	for(int i=0;i<44;i++)
		m_functionsGroupIndex << 3;

	// Clausen Functions
	m_functionsNames << i18n("Clausen function");
	m_functionsGroupIndex << 4;

	// Coulomb Functions
	m_functionsNames << i18n("Lowest-order normalized hydrogenic bound state radial wavefunction");
	m_functionsNames << i18n("n-th normalized hydrogenic bound state radial wavefunction");

	for(int i=0;i<2;i++)
		m_functionsGroupIndex << 5;

	// Dawson Function
	m_functionsNames << i18n("Dawson integral");
	m_functionsGroupIndex << 6;

	// Debye Functions
	m_functionsNames << i18n("First-order Debye function");
	m_functionsNames << i18n("Second-order Debye function");
	m_functionsNames << i18n("Third-order Debye function");
	m_functionsNames << i18n("Fourth-order Debye function");
	m_functionsNames << i18n("Fifth-order Debye function");
	m_functionsNames << i18n("Sixth-order Debye function");

	for(int i=0;i<6;i++)
		m_functionsGroupIndex << 7;

	// Dilogarithm
        m_functionsNames << i18n("Dilogarithm for a real argument");
	m_functionsGroupIndex << 8;

	// Elliptic Integrals
        m_functionsNames << i18n("Legendre form of complete elliptic integral K");
        m_functionsNames << i18n("Legendre form of complete elliptic integral E");
        m_functionsNames << i18n("Legendre form of complete elliptic integral Pi");
        m_functionsNames << i18n("Legendre form of incomplete elliptic integral F");
        m_functionsNames << i18n("Legendre form of incomplete elliptic integral E");
        m_functionsNames << i18n("Legendre form of incomplete elliptic integral P");
        m_functionsNames << i18n("Legendre form of incomplete elliptic integral D");
        m_functionsNames << i18n("Carlson form of incomplete elliptic integral RC");
        m_functionsNames << i18n("Carlson form of incomplete elliptic integral RD");
        m_functionsNames << i18n("Carlson form of incomplete elliptic integral RF");
        m_functionsNames << i18n("Carlson form of incomplete elliptic integral RJ");

	for(int i=0;i<11;i++)
		m_functionsGroupIndex << 9;

	// Error Functions
        m_functionsNames << i18n("Error function");
        m_functionsNames << i18n("Complementary error function");
        m_functionsNames << i18n("Logarithm of complementary error function");
        m_functionsNames << i18n("Gaussian probability density function Z");
        m_functionsNames << i18n("Upper tail of the Gaussian probability function Q");
        m_functionsNames << i18n("Hazard function for the normal distribution Z/Q");

	for(int i=0;i<6;i++)
		m_functionsGroupIndex << 10;

	// Exponential Functions
        m_functionsNames << i18n("Exponential function");
        m_functionsNames << i18n("exponentiate x and multiply by y");
        m_functionsNames << i18n("exp(x) - 1");
        m_functionsNames << i18n("(exp(x)-1)/x");
        m_functionsNames << i18n("2(exp(x)-1-x)/x^2");
        m_functionsNames << i18n("n-relative exponential");

	for(int i=0;i<6;i++)
		m_functionsGroupIndex << 11;

	// Exponential Integrals
        m_functionsNames << i18n("Exponential integral");
        m_functionsNames << i18n("Second order exponential integral");
        m_functionsNames << i18n("Exponential integral of order n");
        m_functionsNames << i18n("Exponential integral Ei");
        m_functionsNames << i18n("Hyperbolic integral Shi");
        m_functionsNames << i18n("Hyperbolic integral Chi");
        m_functionsNames << i18n("Third-order exponential integral");
        m_functionsNames << i18n("Sine integral");
        m_functionsNames << i18n("Cosine integral");
        m_functionsNames << i18n("Arctangent integral");

	for(int i=0;i<10;i++)
		m_functionsGroupIndex << 12;

	// Fermi-Dirac Function
        m_functionsNames << i18n("Complete Fermi-Dirac integral with index -1");
        m_functionsNames << i18n("Complete Fermi-Dirac integral with index 0");
        m_functionsNames << i18n("Complete Fermi-Dirac integral with index 1");
        m_functionsNames << i18n("Complete Fermi-Dirac integral with index 2");
        m_functionsNames << i18n("Complete Fermi-Dirac integral with integer index j");
        m_functionsNames << i18n("Complete Fermi-Dirac integral with index -1/2");
        m_functionsNames << i18n("Complete Fermi-Dirac integral with index 1/2");
        m_functionsNames << i18n("Complete Fermi-Dirac integral with index 3/2");
        m_functionsNames << i18n("Incomplete Fermi-Dirac integral with index zero");

	for(int i=0;i<9;i++)
		m_functionsGroupIndex << 13;

	// Gamma and Beta Functions
        m_functionsNames << i18n("Gamma function");
        m_functionsNames << i18n("Gamma function");
        m_functionsNames << i18n("Logarithm of the gamma function");
        m_functionsNames << i18n("Logarithm of the gamma function");
        m_functionsNames << i18n("Regulated gamma function");
        m_functionsNames << i18n("Reciprocal of the gamma function");
        m_functionsNames << i18n("Factorial n!");
        m_functionsNames << i18n("Double factorial n!!");
        m_functionsNames << i18n("Logarithm of the factorial");
        m_functionsNames << i18n("Logarithm of the double factorial");

        m_functionsNames << i18n("Combinatorial factor");
        m_functionsNames << i18n("Logarithm of the combinatorial factor");
        m_functionsNames << i18n("Taylor coefficient");
        m_functionsNames << i18n("Pochhammer symbol");
        m_functionsNames << i18n("Logarithm of the Pochhammer symbol");
        m_functionsNames << i18n("Relative Pochhammer symbol");
        m_functionsNames << i18n("Unnormalized incomplete gamma function");
        m_functionsNames << i18n("Normalized incomplete gamma function");
        m_functionsNames << i18n("Complementary normalized incomplete gamma function");
        m_functionsNames << i18n("Beta function");

        m_functionsNames << i18n("Logarithm of the beta function");
        m_functionsNames << i18n("Normalized incomplete beta function");

	for(int i=0;i<22;i++)
		m_functionsGroupIndex << 14;

	// Gegenbauer Functions
        m_functionsNames << i18n("Gegenbauer polynomial C_1");
        m_functionsNames << i18n("Gegenbauer polynomial C_2");
        m_functionsNames << i18n("Gegenbauer polynomial C_3");
        m_functionsNames << i18n("Gegenbauer polynomial C_n");

	for(int i=0;i<4;i++)
		m_functionsGroupIndex << 15;

	// Hypergeometric Functions
        m_functionsNames << i18n("Hypergeometric function 0F1");
        m_functionsNames << i18n("Confluent hypergeometric function 1F1 for integer parameters");
        m_functionsNames << i18n("Confluent hypergeometric function 1F1 for general parameters");
        m_functionsNames << i18n("Confluent hypergeometric function U for integer parameters");
        m_functionsNames << i18n("Confluent hypergeometric function U");
        m_functionsNames << i18n("Gauss hypergeometric function 2F1");
        m_functionsNames << i18n("Gauss hypergeometric function 2F1 with complex parameters");
        m_functionsNames << i18n("Renormalized Gauss hypergeometric function 2F1");
        m_functionsNames << i18n("Renormalized Gauss hypergeometric function 2F1 with complex parameters");
        m_functionsNames << i18n("Hypergeometric function 2F0");

	for(int i=0;i<10;i++)
		m_functionsGroupIndex << 16;

	// Laguerre Functions
        m_functionsNames << i18n("generalized Laguerre polynomials L_1");
        m_functionsNames << i18n("generalized Laguerre polynomials L_2");
        m_functionsNames << i18n("generalized Laguerre polynomials L_3");

	for(int i=0;i<3;i++)
		m_functionsGroupIndex << 17;

	// Lambert W Functions
        m_functionsNames << i18n("Principal branch of the Lambert W function");
        m_functionsNames << i18n("Secondary real-valued branch of the Lambert W function");

	for(int i=0;i<2;i++)
		m_functionsGroupIndex << 18;

	// Legendre Functions and Spherical Harmonics
        m_functionsNames << i18n("Legendre polynomial P_1");
        m_functionsNames << i18n("Legendre polynomial P_2");
        m_functionsNames << i18n("Legendre polynomial P_3");
        m_functionsNames << i18n("Legendre polynomial P_l");
        m_functionsNames << i18n("Legendre function Q_0");
        m_functionsNames << i18n("Legendre function Q_1");
        m_functionsNames << i18n("Legendre function Q_l");
        m_functionsNames << i18n("Associated Legendre polynomial");
        m_functionsNames << i18n("Normalized associated Legendre polynomial");
        m_functionsNames << i18n("Irregular spherical conical function P^1/2");

        m_functionsNames << i18n("Regular spherical conical function P^(-1/2)");
        m_functionsNames << i18n("Conical function P^0");
        m_functionsNames << i18n("Conical function P^1");
        m_functionsNames << i18n("Regular spherical conical function P^(-1/2-l)");
        m_functionsNames << i18n("Regular cylindrical conical function P^(-m)");
        m_functionsNames << i18n("Zeroth radial eigenfunction of the Laplacian on the 3-dimensional hyperbolic space");
        m_functionsNames << i18n("First radial eigenfunction of the Laplacian on the 3-dimensional hyperbolic space");
        m_functionsNames << i18n("l-th radial eigenfunction of the Laplacian on the 3-dimensional hyperbolic space");

	for(int i=0;i<18;i++)
		m_functionsGroupIndex << 19;

	// Logarithm and Related Functions
        m_functionsNames << i18n("Logarithm");
        m_functionsNames << i18n("Logarithm of the magnitude");
        m_functionsNames << i18n("log(1+x)");
        m_functionsNames << i18n("log(1+x) - x");

	for(int i=0;i<4;i++)
		m_functionsGroupIndex << 20;

	// Power Function
        m_functionsNames << i18n("x^n for integer n with an error estimate");
	m_functionsGroupIndex << 21;

	// Psi (Digamma) Function
        m_functionsNames << i18n("Digamma function for positive integer n");
        m_functionsNames << i18n("Digamma function");
        m_functionsNames << i18n("Real part of the digamma function on the line 1+i y");
        m_functionsNames << i18n("Trigamma function psi' for positive integer n");
        m_functionsNames << i18n("Trigamma function psi'");
        m_functionsNames << i18n("Polygamma function psi^(n)");

	for(int i=0;i<6;i++)
		m_functionsGroupIndex << 22;

	// Synchrotron Functions
        m_functionsNames << i18n("First synchrotron function");
        m_functionsNames << i18n("Second synchrotron function");

	for(int i=0;i<2;i++)
		m_functionsGroupIndex << 23;

	// Transport Functions
        m_functionsNames << i18n("Transport function");
        m_functionsNames << i18n("Transport function");
        m_functionsNames << i18n("Transport function");
        m_functionsNames << i18n("Transport function");

	for(int i=0;i<4;i++)
		m_functionsGroupIndex << 24;

	// Trigonometric Functions
        m_functionsNames << i18n("sine");
        m_functionsNames << i18n("cosine");
        m_functionsNames << i18n("hypotenuse function");
        m_functionsNames << i18n("sin(x)/x");
        m_functionsNames << i18n("log(sinh(x))");
        m_functionsNames << i18n("log(cosh(x))");
        m_functionsNames << i18n("restrict to [-pi,pi]");
        m_functionsNames << i18n("restrict to [0,2 pi]");

	for(int i=0;i<8;i++)
		m_functionsGroupIndex << 25;

	// Zeta Functions
        m_functionsNames << i18n("Riemann zeta function for integer n");
        m_functionsNames << i18n("Riemann zeta function");
        m_functionsNames << i18n("zeta(n)-1 for integer n");
        m_functionsNames << i18n("zeta(x)-1");
        m_functionsNames << i18n("Hurwitz zeta function");
        m_functionsNames << i18n("Eta function for integer n");
        m_functionsNames << i18n("Eta function");

	for(int i=0;i<7;i++)
		m_functionsGroupIndex << 26;

	// GSL Random Number Distributions: see http://www.gnu.org/software/gsl/manual/html_node/Random-Number-Distributions.html
	// Gaussian Distribution
        m_functionsNames << i18n("Probability density for a Gaussian distribution");
        m_functionsNames << i18n("Probability density for a unit Gaussian distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");
        m_functionsNames << i18n("Cumulative unit distribution function P");
        m_functionsNames << i18n("Cumulative unit distribution function Q");
        m_functionsNames << i18n("Inverse cumulative unit distribution function P");
        m_functionsNames << i18n("Inverse cumulative unit distribution function Q");

        m_functionsNames << i18n("Probability density for Gaussian tail distribution");
        m_functionsNames << i18n("Probability density for unit Gaussian tail distribution");
        m_functionsNames << i18n("Probability density for a bivariate Gaussian distribution");

	for(int i=0;i<13;i++)
		m_functionsGroupIndex << 27;

	// Exponential Distribution
        m_functionsNames << i18n("Probability density for an exponential distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 28;

	// Laplace Distribution
        m_functionsNames << i18n("Probability density for a Laplace distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 29;

	// Exponential Power Distribution
	m_functionsNames << i18n("Probability density for an exponential power distribution");
        m_functionsNames << i18n("cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");

	for(int i=0;i<3;i++)
		m_functionsGroupIndex << 30;

	// Cauchy Distribution
        m_functionsNames << i18n("Probability density for a Cauchy distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 31;

	// Rayleigh Distribution
        m_functionsNames << i18n("Probability density for a Rayleigh distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");
        m_functionsNames << i18n("Probability density for a Rayleigh tail distribution");

	for(int i=0;i<6;i++)
		m_functionsGroupIndex << 32;

	// Landau Distribution
        m_functionsNames << i18n("Probability density for a Landau distribution");
	m_functionsGroupIndex << 33;

	// Gamma Distribution
        m_functionsNames << i18n("Probability density for a gamma distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 34;

	// Flat (Uniform) Distribution
        m_functionsNames << i18n("Probability density for a uniform distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 35;

	// Lognormal Distribution
        m_functionsNames << i18n("Probability density for a lognormal distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 36;

	// Chi-squared Distribution
        m_functionsNames << i18n("Probability density for a chi squared distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 37;

	// F-distribution
        m_functionsNames << i18n("Probability density for a F-distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 38;

	// t-distribution
        m_functionsNames << i18n("Probability density for a t-distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 39;

	// Beta Distribution
        m_functionsNames << i18n("Probability density for a beta distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 40;

	// Logistic Distribution
        m_functionsNames << i18n("Probability density for a logistic distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 41;

	// Pareto Distribution
        m_functionsNames << i18n("Probability density for a Pareto distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 42;

	// Weibull Distribution
        m_functionsNames << i18n("Probability density for a Weibull distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<5;i++)
		m_functionsGroupIndex << 43;

	// Gumbel Distribution
        m_functionsNames << i18n("Probability density for a Type-1 Gumbel distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");
        m_functionsNames << i18n("Probability density for a Type-2 Gumbel distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Inverse cumulative distribution function P");
        m_functionsNames << i18n("Inverse cumulative distribution function Q");

	for(int i=0;i<10;i++)
		m_functionsGroupIndex << 44;

	// Poisson Distribution
        m_functionsNames << i18n("Probability density for a Poisson distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");

	for(int i=0;i<3;i++)
		m_functionsGroupIndex << 45;

	// Bernoulli Distribution
        m_functionsNames << i18n("Probability density for a Bernoulli distribution");
	m_functionsGroupIndex << 46;

	// Binomial Distribution
        m_functionsNames << i18n("Probability density for a binomial distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");
        m_functionsNames << i18n("Probability density for a negative binomial distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");

	for(int i=0;i<6;i++)
		m_functionsGroupIndex << 47;

	// Pascal Distribution
        m_functionsNames << i18n("Probability density for a Pascal distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");

	for(int i=0;i<3;i++)
		m_functionsGroupIndex << 48;

	// Geometric Distribution
        m_functionsNames << i18n("Probability density for a geometric distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");

	for(int i=0;i<3;i++)
		m_functionsGroupIndex << 49;

	// Hypergeometric Distribution
        m_functionsNames << i18n("Probability density for a hypergeometric distribution");
        m_functionsNames << i18n("Cumulative distribution function P");
        m_functionsNames << i18n("Cumulative distribution function Q");

	for(int i=0;i<3;i++)
		m_functionsGroupIndex << 50;

	// Logarithmic Distribution
        m_functionsNames << i18n("Probability density for a logarithmic distribution");
	m_functionsGroupIndex << 51;
}

//TODO: decide whether we want to have i18n here in the backend part of the code
void ExpressionParser::initConstants() {
	for (int i = 0; _constants[i].name != 0; i++)
		m_constants << _constants[i].name;

	//groups
	m_constantsGroups << i18n("Mathematical constants");
	m_constantsGroups << i18n("Fundamental constants");
	m_constantsGroups << i18n("Astronomy and Astrophysics");
	m_constantsGroups << i18n("Atomic and Nuclear Physics");
	m_constantsGroups << i18n("Measurement of Time");
	m_constantsGroups << i18n("Imperial Units");
	m_constantsGroups << i18n("Speed and Nautical Units");
	m_constantsGroups << i18n("Printers Units");
	m_constantsGroups << i18n("Volume, Area and Length");
	m_constantsGroups << i18n("Mass and Weight");
	m_constantsGroups << i18n("Thermal Energy and Power");
	m_constantsGroups << i18n("Pressure");
	m_constantsGroups << i18n("Viscosity");
	m_constantsGroups << i18n("Light and Illumination");
	m_constantsGroups << i18n("Radioactivity");
	m_constantsGroups << i18n("Force and Energy");

	//Mathematical constants
	m_constantsNames << i18n("Euler constant");
	m_constantsValues << QString::number(M_E,'g',15); m_constantsUnits << "";
	m_constantsNames << i18n("Pi");
	m_constantsValues << QString::number(M_PI,'g',15); m_constantsUnits << "";

	for(int i=0;i<2;i++)
		m_constantsGroupIndex << 0;

	//Fundamental constants
	m_constantsNames << i18n("Speed of light");
	m_constantsValues << QString::number(GSL_CONST_MKSA_SPEED_OF_LIGHT); m_constantsUnits << "m / s";
	m_constantsNames << i18n("Vacuum permeability");
	m_constantsValues << QString::number(GSL_CONST_MKSA_VACUUM_PERMEABILITY); m_constantsUnits << "kg m / A^2 s^2";
	m_constantsNames << i18n("Vacuum permittivity");
	m_constantsValues << QString::number(GSL_CONST_MKSA_VACUUM_PERMITTIVITY); m_constantsUnits << "A^2 s^4 / kg m^3";
	m_constantsNames << i18n("Planck constant");
	m_constantsValues << QString::number(GSL_CONST_MKSA_PLANCKS_CONSTANT_H); m_constantsUnits << "kg m^2 / s";
	m_constantsNames << i18n("Reduced Planck constant");
	m_constantsValues << QString::number(GSL_CONST_MKSA_PLANCKS_CONSTANT_HBAR); m_constantsUnits << "kg m^2 / s";
	m_constantsNames << i18n("Avogadro constant");
	m_constantsValues << QString::number(GSL_CONST_NUM_AVOGADRO); m_constantsUnits << "1 / mol";
	m_constantsNames << i18n("Faraday");
	m_constantsValues << QString::number(GSL_CONST_MKSA_FARADAY); m_constantsUnits << "A s / mol";
	m_constantsNames << i18n("Boltzmann constant");
	m_constantsValues << QString::number(GSL_CONST_MKSA_BOLTZMANN); m_constantsUnits << "kg m^2 / K s^2";
	m_constantsNames << i18n("Molar gas");
	m_constantsValues << QString::number(GSL_CONST_MKSA_MOLAR_GAS); m_constantsUnits << "kg m^2 / K mol s^2";
	m_constantsNames << i18n("Standard gas volume");
	m_constantsValues << QString::number(GSL_CONST_MKSA_STANDARD_GAS_VOLUME); m_constantsUnits << "m^3 / mol";
	m_constantsNames << i18n("Stefan-Boltzmann constant");
	m_constantsValues << QString::number(GSL_CONST_MKSA_STEFAN_BOLTZMANN_CONSTANT); m_constantsUnits << "kg / K^4 s^3";
	m_constantsNames << i18n("Gauss");
	m_constantsValues << QString::number(GSL_CONST_MKSA_GAUSS); m_constantsUnits << "kg / A s^2";

	for(int i=0;i<12;i++)
		m_constantsGroupIndex << 1;

	// Astronomy and Astrophysics
	m_constantsNames << i18n("Astronomical unit");
	m_constantsValues << QString::number(GSL_CONST_MKSA_ASTRONOMICAL_UNIT); m_constantsUnits << "m";
	m_constantsNames << i18n("Gravitational constant");
	m_constantsValues << QString::number(GSL_CONST_MKSA_GRAVITATIONAL_CONSTANT); m_constantsUnits << "m^3 / kg s^2";
	m_constantsNames << i18n("Light year");
	m_constantsValues << QString::number(GSL_CONST_MKSA_LIGHT_YEAR); m_constantsUnits << "m";
	m_constantsNames << i18n("Parsec");
	m_constantsValues << QString::number(GSL_CONST_MKSA_PARSEC); m_constantsUnits << "m";
	m_constantsNames << i18n("Gravitational acceleration");
	m_constantsValues << QString::number(GSL_CONST_MKSA_GRAV_ACCEL); m_constantsUnits << "m / s^2";
	m_constantsNames << i18n("Solar mass");
	m_constantsValues << QString::number(GSL_CONST_MKSA_SOLAR_MASS); m_constantsUnits << "kg";

	for(int i=0;i<6;i++)
		m_constantsGroupIndex << 2;

	// Atomic and Nuclear Physics;
	m_constantsNames << i18n("Charge of the electron");
	m_constantsValues << QString::number(GSL_CONST_MKSA_ELECTRON_CHARGE); m_constantsUnits << "A s";
	m_constantsNames << i18n("Energy of 1 electron volt");
	m_constantsValues << QString::number(GSL_CONST_MKSA_ELECTRON_VOLT); m_constantsUnits << "kg m^2 / s^2";
	m_constantsNames << i18n("Unified atomic mass");
	m_constantsValues << QString::number(GSL_CONST_MKSA_UNIFIED_ATOMIC_MASS); m_constantsUnits << "kg";
	m_constantsNames << i18n("Mass of the electron");
	m_constantsValues << QString::number(GSL_CONST_MKSA_MASS_ELECTRON); m_constantsUnits << "kg";
	m_constantsNames << i18n("Mass of the muon");
	m_constantsValues << QString::number(GSL_CONST_MKSA_MASS_MUON); m_constantsUnits << "kg";
	m_constantsNames << i18n("Mass of the proton");
	m_constantsValues << QString::number(GSL_CONST_MKSA_MASS_PROTON); m_constantsUnits << "kg";
	m_constantsNames << i18n("Mass of the neutron");
	m_constantsValues << QString::number(GSL_CONST_MKSA_MASS_NEUTRON); m_constantsUnits << "kg";
	m_constantsNames << i18n("Electromagnetic fine structure constant");
	m_constantsValues << QString::number(GSL_CONST_NUM_FINE_STRUCTURE); m_constantsUnits << "";
	m_constantsNames << i18n("Rydberg constant");
	m_constantsValues << QString::number(GSL_CONST_MKSA_RYDBERG); m_constantsUnits << "kg m^2 / s^2";
	m_constantsNames << i18n("Bohr radius");
	m_constantsValues << QString::number(GSL_CONST_MKSA_BOHR_RADIUS); m_constantsUnits << "m";
	m_constantsNames << i18n("Length of 1 angstrom");
	m_constantsValues << QString::number(GSL_CONST_MKSA_ANGSTROM); m_constantsUnits << "m";
	m_constantsNames << i18n("Area of 1 barn");
	m_constantsValues << QString::number(GSL_CONST_MKSA_BARN); m_constantsUnits << "m^2";
	m_constantsNames << i18n("Bohr Magneton");
	m_constantsValues << QString::number(GSL_CONST_MKSA_BOHR_MAGNETON); m_constantsUnits << "A m^2";
	m_constantsNames << i18n("Nuclear Magneton");
	m_constantsValues << QString::number(GSL_CONST_MKSA_NUCLEAR_MAGNETON); m_constantsUnits << "A m^2";
	m_constantsNames << i18n("Magnetic moment of the electron [absolute value]");
	m_constantsValues << QString::number(GSL_CONST_MKSA_ELECTRON_MAGNETIC_MOMENT); m_constantsUnits << "A m^2";
	m_constantsNames << i18n("Magnetic moment of the proton");
	m_constantsValues << QString::number(GSL_CONST_MKSA_PROTON_MAGNETIC_MOMENT); m_constantsUnits << "A m^2";
	m_constantsNames << i18n("Thomson cross section");
	m_constantsValues << QString::number(GSL_CONST_MKSA_THOMSON_CROSS_SECTION); m_constantsUnits << "m^2";
	m_constantsNames << i18n("Electric dipole moment of 1 Debye");
	m_constantsValues << QString::number(GSL_CONST_MKSA_DEBYE); m_constantsUnits << "A s^2 / m^2";

	for(int i=0;i<18;i++)
		m_constantsGroupIndex << 3;

	// Measurement of Time
	m_constantsNames << i18n("Number of seconds in 1 minute");
	m_constantsValues << QString::number(GSL_CONST_MKSA_MINUTE); m_constantsUnits << "s";
	m_constantsNames << i18n("Number of seconds in 1 hour");
	m_constantsValues << QString::number(GSL_CONST_MKSA_HOUR); m_constantsUnits << "s";
	m_constantsNames << i18n("Number of seconds in 1 day");
	m_constantsValues << QString::number(GSL_CONST_MKSA_DAY); m_constantsUnits << "s";
	m_constantsNames << i18n("Number of seconds in 1 week");
	m_constantsValues << QString::number(GSL_CONST_MKSA_WEEK); m_constantsUnits << "s";

	for(int i=0;i<4;i++)
		m_constantsGroupIndex << 4;

	// Imperial Units
	m_constantsNames << i18n("Length of 1 inch");
	m_constantsValues << QString::number(GSL_CONST_MKSA_INCH); m_constantsUnits << "m";
	m_constantsNames << i18n("Length of 1 foot");
	m_constantsValues << QString::number(GSL_CONST_MKSA_FOOT); m_constantsUnits << "m";
	m_constantsNames << i18n("Length of 1 yard");
	m_constantsValues << QString::number(GSL_CONST_MKSA_YARD); m_constantsUnits << "m";
	m_constantsNames << i18n("Length of 1 mile");
	m_constantsValues << QString::number(GSL_CONST_MKSA_MILE); m_constantsUnits << "m";
	m_constantsNames << i18n("Length of 1/1000th of an inch");
	m_constantsValues << QString::number(GSL_CONST_MKSA_MIL); m_constantsUnits << "m";

	for(int i=0;i<5;i++)
		m_constantsGroupIndex << 5;

	// Speed and Nautical Units
	m_constantsNames << i18n("Speed of 1 kilometer per hour");
	m_constantsValues << QString::number(GSL_CONST_MKSA_KILOMETERS_PER_HOUR); m_constantsUnits << "m / s";
	m_constantsNames << i18n("Speed of 1 mile per hour");
	m_constantsValues << QString::number(GSL_CONST_MKSA_MILES_PER_HOUR); m_constantsUnits << "m / s";
	m_constantsNames << i18n("Length of 1 nautical mile");
	m_constantsValues << QString::number(GSL_CONST_MKSA_NAUTICAL_MILE); m_constantsUnits << "m";
	m_constantsNames << i18n("Length of 1 fathom");
	m_constantsValues << QString::number(GSL_CONST_MKSA_FATHOM); m_constantsUnits << "m";
	m_constantsNames << i18n("Speed of 1 knot");
	m_constantsValues << QString::number(GSL_CONST_MKSA_KNOT); m_constantsUnits << "m / s";

	for(int i=0;i<5;i++)
		m_constantsGroupIndex << 6;

	// Printers Units
	m_constantsNames << i18n("length of 1 printer's point [1/72 inch]");
	m_constantsValues << QString::number(GSL_CONST_MKSA_POINT); m_constantsUnits << "m";
	m_constantsNames << i18n("length of 1 TeX point [1/72.27 inch]");
	m_constantsValues << QString::number(GSL_CONST_MKSA_TEXPOINT); m_constantsUnits << "m";

	for(int i=0;i<2;i++)
		m_constantsGroupIndex << 7;

	// Volume, Area and Length
	m_constantsNames << i18n("Length of 1 micron");
	m_constantsValues << QString::number(GSL_CONST_MKSA_MICRON); m_constantsUnits << "m";
	m_constantsNames << i18n("Area of 1 hectare");
	m_constantsValues << QString::number(GSL_CONST_MKSA_HECTARE); m_constantsUnits << "m^2";
	m_constantsNames << i18n("Area of 1 acre");
	m_constantsValues << QString::number(GSL_CONST_MKSA_ACRE); m_constantsUnits << "m^2";
	m_constantsNames << i18n("Volume of 1 liter");
	m_constantsValues << QString::number(GSL_CONST_MKSA_LITER); m_constantsUnits << "m^3";
	m_constantsNames << i18n("Volume of 1 US gallon");
	m_constantsValues << QString::number(GSL_CONST_MKSA_US_GALLON); m_constantsUnits << "m^3";
	m_constantsNames << i18n("Volume of 1 Canadian gallon");
	m_constantsValues << QString::number(GSL_CONST_MKSA_CANADIAN_GALLON); m_constantsUnits << "m^3";
	m_constantsNames << i18n("Volume of 1 UK gallon");
	m_constantsValues << QString::number(GSL_CONST_MKSA_UK_GALLON); m_constantsUnits << "m^3";
	m_constantsNames << i18n("Volume of 1 quart");
	m_constantsValues << QString::number(GSL_CONST_MKSA_QUART); m_constantsUnits << "m^3";
	m_constantsNames << i18n("Volume of 1 pint");
	m_constantsValues << QString::number(GSL_CONST_MKSA_PINT); m_constantsUnits << "m^3";

	for(int i=0;i<9;i++)
		m_constantsGroupIndex << 8;

	// Mass and Weight
	m_constantsNames << i18n("Mass of 1 pound");
	m_constantsValues << QString::number(GSL_CONST_MKSA_POUND_MASS); m_constantsUnits << "kg";
	m_constantsNames << i18n("Mass of 1 ounce");
	m_constantsValues << QString::number(GSL_CONST_MKSA_OUNCE_MASS); m_constantsUnits << "kg";
	m_constantsNames << i18n("Mass of 1 ton");
	m_constantsValues << QString::number(GSL_CONST_MKSA_TON); m_constantsUnits << "kg";
	m_constantsNames << i18n("Mass of 1 metric ton [1000 kg]");
	m_constantsValues << QString::number(GSL_CONST_MKSA_METRIC_TON); m_constantsUnits << "kg";
	m_constantsNames << i18n("Mass of 1 UK ton");
	m_constantsValues << QString::number(GSL_CONST_MKSA_UK_TON); m_constantsUnits << "kg";
	m_constantsNames << i18n("Mass of 1 troy ounce");
	m_constantsValues << QString::number(GSL_CONST_MKSA_TROY_OUNCE); m_constantsUnits << "kg";
	m_constantsNames << i18n("Mass of 1 carat");
	m_constantsValues << QString::number(GSL_CONST_MKSA_CARAT); m_constantsUnits << "kg";
	m_constantsNames << i18n("Force of 1 gram weight");
	m_constantsValues << QString::number(GSL_CONST_MKSA_GRAM_FORCE); m_constantsUnits << "kg m / s^2";
	m_constantsNames << i18n("Force of 1 pound weight");
	m_constantsValues << QString::number(GSL_CONST_MKSA_POUND_FORCE); m_constantsUnits << "kg m / s^2";
	m_constantsNames << i18n("Force of 1 kilopound weight");
	m_constantsValues << QString::number(GSL_CONST_MKSA_KILOPOUND_FORCE); m_constantsUnits << "kg m / s^2";
	m_constantsNames << i18n("Force of 1 poundal");
	m_constantsValues << QString::number(GSL_CONST_MKSA_POUNDAL); m_constantsUnits << "kg m / s^2";

	for(int i=0;i<11;i++)
		m_constantsGroupIndex << 9;

	// Thermal Energy and Power
	m_constantsNames << i18n("Energy of 1 calorie");
	m_constantsValues << QString::number(GSL_CONST_MKSA_CALORIE); m_constantsUnits << "kg m^2 / s^2";
	m_constantsNames << i18n("Energy of 1 British Thermal Unit");
	m_constantsValues << QString::number(GSL_CONST_MKSA_BTU); m_constantsUnits << "kg m^2 / s^2";
	m_constantsNames << i18n("Energy of 1 Therm");
	m_constantsValues << QString::number(GSL_CONST_MKSA_THERM); m_constantsUnits << "kg m^2 / s^2";
	m_constantsNames << i18n("Power of 1 horsepower");
	m_constantsValues << QString::number(GSL_CONST_MKSA_HORSEPOWER); m_constantsUnits << "kg m^2 / s^3";

	for(int i=0;i<4;i++)
		m_constantsGroupIndex << 10;

	// Pressure
	m_constantsNames << i18n("Pressure of 1 bar");
	m_constantsValues << QString::number(GSL_CONST_MKSA_BAR); m_constantsUnits << "kg / m s^2";
	m_constantsNames << i18n("Pressure of 1 standard atmosphere");
	m_constantsValues << QString::number(GSL_CONST_MKSA_STD_ATMOSPHERE); m_constantsUnits << "kg / m s^2";
	m_constantsNames << i18n("Pressure of 1 torr");
	m_constantsValues << QString::number(GSL_CONST_MKSA_TORR); m_constantsUnits << "kg / m s^2";
	m_constantsNames << i18n("Pressure of 1 meter of mercury");
	m_constantsValues << QString::number(GSL_CONST_MKSA_METER_OF_MERCURY); m_constantsUnits << "kg / m s^2";
	m_constantsNames << i18n("Pressure of 1 inch of mercury");
	m_constantsValues << QString::number(GSL_CONST_MKSA_INCH_OF_MERCURY); m_constantsUnits << "kg / m s^2";
	m_constantsNames << i18n("Pressure of 1 inch of water");
	m_constantsValues << QString::number(GSL_CONST_MKSA_INCH_OF_WATER); m_constantsUnits << "kg / m s^2";
	m_constantsNames << i18n("Pressure of 1 pound per square inch");
	m_constantsValues << QString::number(GSL_CONST_MKSA_PSI); m_constantsUnits << "kg / m s^2";

	for(int i=0;i<7;i++)
		m_constantsGroupIndex << 11;

	// Viscosity
	m_constantsNames << i18n("Dynamic viscosity of 1 poise");
	m_constantsValues << QString::number(GSL_CONST_MKSA_POISE); m_constantsUnits << "kg / m s";
	m_constantsNames << i18n("Kinematic viscosity of 1 stokes");
	m_constantsValues << QString::number(GSL_CONST_MKSA_STOKES); m_constantsUnits << "m^2 / s";

	for(int i=0;i<2;i++)
		m_constantsGroupIndex << 12;

	// Light and Illumination
	m_constantsNames << i18n("Luminance of 1 stilb");
	m_constantsValues << QString::number(GSL_CONST_MKSA_STILB); m_constantsUnits << "cd / m^2";
	m_constantsNames << i18n("Luminous flux of 1 lumen");
	m_constantsValues << QString::number(GSL_CONST_MKSA_LUMEN); m_constantsUnits << "cd sr";
	m_constantsNames << i18n("Illuminance of 1 lux");
	m_constantsValues << QString::number(GSL_CONST_MKSA_LUX); m_constantsUnits << "cd sr / m^2";
	m_constantsNames << i18n("Illuminance of 1 phot");
	m_constantsValues << QString::number(GSL_CONST_MKSA_PHOT); m_constantsUnits << "cd sr / m^2";
	m_constantsNames << i18n("Illuminance of 1 footcandle");
	m_constantsValues << QString::number(GSL_CONST_MKSA_FOOTCANDLE); m_constantsUnits << "cd sr / m^2";
	m_constantsNames << i18n("Luminance of 1 lambert");
	m_constantsValues << QString::number(GSL_CONST_MKSA_LAMBERT); m_constantsUnits << "cd sr / m^2";
	m_constantsNames << i18n("Luminance of 1 footlambert");
	m_constantsValues << QString::number(GSL_CONST_MKSA_FOOTLAMBERT); m_constantsUnits << "cd sr / m^2";

	for(int i=0;i<7;i++)
		m_constantsGroupIndex << 13;

	// Radioactivity
	m_constantsNames << i18n("Activity of 1 curie");
	m_constantsValues << QString::number(GSL_CONST_MKSA_CURIE); m_constantsUnits << "1 / s";
	m_constantsNames << i18n("Exposure of 1 roentgen");
	m_constantsValues << QString::number(GSL_CONST_MKSA_ROENTGEN); m_constantsUnits << "A s / kg";
	m_constantsNames << i18n("Absorbed dose of 1 rad");
	m_constantsValues << QString::number(GSL_CONST_MKSA_RAD); m_constantsUnits << "m^2 / s^2";

	for(int i=0;i<3;i++)
		m_constantsGroupIndex << 14;

	// Force and Energy
	m_constantsNames << i18n("SI unit of force");
	m_constantsValues << QString::number(GSL_CONST_MKSA_NEWTON); m_constantsUnits << "kg m / s^2";
	m_constantsNames << i18n("Force of 1 Dyne");
	m_constantsValues << QString::number(GSL_CONST_MKSA_DYNE); m_constantsUnits << "kg m / s^2";
	m_constantsNames << i18n("SI unit of energy");
	m_constantsValues << QString::number(GSL_CONST_MKSA_JOULE); m_constantsUnits << "kg m^2 / s^2";
	m_constantsNames << i18n("Energy 1 erg");
	m_constantsValues << QString::number(GSL_CONST_MKSA_ERG); m_constantsUnits << "kg m^2 / s^2";

	for(int i=0;i<4;i++)
		m_constantsGroupIndex << 15;
}

ExpressionParser::~ExpressionParser(){
	delete_table();
}

ExpressionParser* ExpressionParser::getInstance(){
	if (!instance)
		instance = new ExpressionParser();

	return instance;
}

const QStringList& ExpressionParser::functions() {
	return m_functions;
}


const QStringList& ExpressionParser::functionsGroups() {
	return m_functionsGroups;
}

const QStringList& ExpressionParser::functionsNames() {
	return m_functionsNames;
}

const QVector<int>& ExpressionParser::functionsGroupIndices() {
	return m_functionsGroupIndex;
}

const QStringList& ExpressionParser::constants() {
	return m_constants;
}

const QStringList& ExpressionParser::constantsGroups() {
	return m_constantsGroups;
}

const QStringList& ExpressionParser::constantsNames() {
	return m_constantsNames;
}

const QStringList& ExpressionParser::constantsValues() {
	return m_constantsValues;
}

const QStringList& ExpressionParser::constantsUnits() {
	return m_constantsUnits;
}

const QVector<int>& ExpressionParser::constantsGroupIndices() {
	return m_constantsGroupIndex;
}

bool ExpressionParser::isValid(const QString& expr, const QStringList& vars){
	for (int i=0; i<vars.size(); ++i)
		assign_variable(vars.at(i).toLocal8Bit().data(), 0);

	QByteArray funcba = expr.toLocal8Bit();
	const char* data = funcba.data();
	gsl_set_error_handler_off();
	parse(data);
	return !(parse_errors()>0);
}

bool ExpressionParser::evaluateCartesian(const QString& expr, const QString& min, const QString& max,
										 int count, QVector<double>* xVector, QVector<double>* yVector,
										 const QStringList& paramNames, const QVector<double>& paramValues) {
	double xMin = parse( min.toLocal8Bit().data() );
	double xMax = parse( max.toLocal8Bit().data() );
	double step = (xMax-xMin)/(double)(count-1);
	QByteArray funcba = expr.toLocal8Bit();
	const char* func = funcba.data();
	double x, y;
	char xVar[] = "x";
	gsl_set_error_handler_off();

	for (int i=0; i<paramNames.size(); ++i)
		assign_variable(paramNames.at(i).toLocal8Bit().data(), paramValues.at(i));

	for(int i = 0;i < count; i++) {
		x = xMin + step*i;
		assign_variable(xVar,x);
		y = parse(func);

		if(parse_errors()>0)
			return false;

		(*xVector)[i] = x;
		if (isfinite(y))
			(*yVector)[i] = y;
		else
			(*yVector)[i] = NAN;
	}

	return true;
}

bool ExpressionParser::evaluateCartesian(const QString& expr, const QString& min, const QString& max,
										 int count, QVector<double>* xVector, QVector<double>* yVector) {
	double xMin = parse( min.toLocal8Bit().data() );
	double xMax = parse( max.toLocal8Bit().data() );
	double step = (xMax-xMin)/(double)(count-1);
	QByteArray funcba = expr.toLocal8Bit();
	const char* func = funcba.data();
	double x, y;
	char xVar[] = "x";
	gsl_set_error_handler_off();

	for(int i = 0;i < count; i++) {
		x = xMin + step*i;
		assign_variable(xVar,x);
		y = parse(func);

		if(parse_errors()>0)
			return false;

		(*xVector)[i] = x;
		if (isfinite(y))
			(*yVector)[i] = y;
		else
			(*yVector)[i] = NAN;
	}

	return true;
}

bool ExpressionParser::evaluateCartesian(const QString& expr, QVector<double>* xVector, QVector<double>* yVector) {
	QByteArray funcba = expr.toLocal8Bit();
	const char* func = funcba.data();
	double x, y;
	char xVar[] = "x";
	gsl_set_error_handler_off();

	for(int i = 0; i < xVector->count(); i++) {
		x = xVector->at(i);
		assign_variable(xVar,x);
		y = parse(func);

		if(parse_errors()>0)
			return false;

		if (isfinite(y))
			(*yVector)[i] = y;
		else
			(*yVector)[i] = NAN;
	}

	return true;
}


/*!
	evaluates multivariate function y=f(x_1, x_2, ...).
	Variable names (x_1, x_2, ...) are stored in \c vars.
	Data is stored in \c dataVectors.
 */
bool ExpressionParser::evaluateCartesian(const QString& expr, const QStringList& vars, const QVector<QVector<double>*>& xVectors, QVector<double>* yVector) {
	Q_ASSERT(vars.size()==xVectors.size());
	QByteArray funcba = expr.toLocal8Bit();
	const char* func = funcba.data();
	double y, varValue;
	QString varName;

	gsl_set_error_handler_off();

	bool stop = false;
	for(int i = 0; i < yVector->size(); i++) {
		//stop iterating over i if one of the x-vectors has no elements anymore.
		for (int n=0; n<xVectors.size(); ++n) {
			if (i==xVectors.at(n)->size()) {
				stop = true;
				break;
			}
		}
		if (stop)
			break;

		for (int n=0; n<vars.size(); ++n) {
			varName = vars.at(n);
			varValue = xVectors.at(n)->at(i);
			assign_variable(varName.toLocal8Bit().data(), varValue);
		}

		y = parse(func);

		if(parse_errors()>0)
			return false;

		if (isfinite(y))
			(*yVector)[i] = y;
		else
			(*yVector)[i] = NAN;
	}

	return true;
}

bool ExpressionParser::evaluatePolar(const QString& expr, const QString& min, const QString& max,
										 int count, QVector<double>* xVector, QVector<double>* yVector) {
	double minValue = parse( min.toLocal8Bit().data() );
	double maxValue = parse( max.toLocal8Bit().data() );
	double step = (maxValue-minValue)/(double)(count-1);
	QByteArray funcba = expr.toLocal8Bit();
	const char* func = funcba.data();
	double r, phi;
	char var[] = "phi";
	gsl_set_error_handler_off();

	for(int i = 0;i < count; i++) {
		phi = minValue + step*i;
		assign_variable(var,phi);
		r = parse(func);
		if(parse_errors()>0)
			return false;

		if (isfinite(r)) {
			(*xVector)[i] = r*cos(phi);
			(*yVector)[i] = r*sin(phi);
		} else {
			(*xVector)[i] = NAN;
			(*yVector)[i] = NAN;
		}
	}

	return true;
}

bool ExpressionParser::evaluateParametric(const QString& expr1, const QString& expr2, const QString& min, const QString& max,
										 int count, QVector<double>* xVector, QVector<double>* yVector) {
	double minValue = parse( min.toLocal8Bit().data() );
	double maxValue = parse( max.toLocal8Bit().data() );
	double step = (maxValue-minValue)/(double)(count-1);
	QByteArray xfuncba = expr1.toLocal8Bit();
	const char* xFunc = xfuncba.data();
	QByteArray yfuncba = expr2.toLocal8Bit();
	const char* yFunc = yfuncba.data();
	double x, y, t;
	char var[] = "t";
	gsl_set_error_handler_off();

	for(int i = 0;i < count; i++) {
		t = minValue + step*i;
		assign_variable(var,t);
		x = parse(xFunc);
		if(parse_errors()>0)
			return false;

		if (isfinite(x))
			(*xVector)[i] = x;
		else
			(*xVector)[i] = NAN;

		y = parse(yFunc);
		if(parse_errors()>0)
			return false;

		if (isfinite(y))
			(*yVector)[i] = y;
		else
			(*yVector)[i] = NAN;
	}

	return true;
}
