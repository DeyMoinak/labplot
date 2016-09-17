/***************************************************************************
    File                 : nsl_interp.c
    Project              : LabPlot
    Description          : NSL interpolation functions
    --------------------------------------------------------------------
    Copyright            : (C) 2016 by Stefan Gerlach (stefan.gerlach@uni.kn)
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

#include <math.h>
#include <stdlib.h>
#include <float.h>
#include "nsl_interp.h"

const char* nsl_interp_type_name[] = { "linear", "polynomial", "cubic spline (natural)", "cubic spline (periodic)", 
	"Akima-spline (natural)", "Akima-spline (periodic)", "Steffen spline", "cosine", "exponential",
	"piecewise cubic Hermite (PCH)", "rational functions" };
const char* nsl_interp_pch_variant_name[] = { "finite differences", "Catmull-Rom", "cardinal", "Kochanek-Bartels (TCB)"};
const char* nsl_interp_evaluate_name[] = { "function", "derivative", "second derivative", "integral"};

int nsl_interp_derivative(double *x, double *y, unsigned int n) {
	double dy=0, oldy=0;
	unsigned int i;
	for (i=0; i<n; i++) {
		if (i == 0)
			dy = (y[1]-y[0])/(x[1]-x[0]);
		else if (i == n-1)
			y[i] = (y[i]-y[i-1])/(x[i]-x[i-1]);
		else
			dy = (y[i+1]-y[i-1])/(x[i+1]-x[i-1]);

		if (i != 0)
			y[i-1] = oldy;
		oldy = dy;
	}

	return 0;
}

int nsl_interp_second_derivative(double *x, double *y, unsigned int n) {
	double dx1, dx2, dy=0., oldy=0., oldoldy=0.;
	unsigned int i;
	for (i=0; i<n; i++) {
		/* see http://websrv.cs.umt.edu/isis/index.php/Finite_differencing:_Introduction */
		if (i == 0) {
			dx1 = x[1]-x[0];
			dx2 = x[2]-x[1];
			dy = 2.*(dx1*y[2]-(dx1+dx2)*y[1]+dx2*y[0])/(dx1*dx2*(dx1+dx2));
		}
		else if (i == n-1) {
			dx1 = x[i-1]-x[i-2];
			dx2 = x[i]-x[i-1];
			y[i] = 2.*(dx1*y[i]-(dx1+dx2)*y[i-1]+dx2*y[i-2])/(dx1*dx2*(dx1+dx2));
			y[i-2] = oldoldy;
		}
		else {
			dx1 = x[i]-x[i-1];
			dx2 = x[i+1]-x[i];
			dy = (dx1*y[i+1]-(dx1+dx2)*y[i]+dx2*y[i-1])/(dx1*dx2*(dx1+dx2));
		}

		/* set value (attention if i==n-2) */
		if (i != 0 && i != n-2)
			y[i-1] = oldy;
		if (i == n-2)
			oldoldy = oldy;

		oldy=dy;
	}
	
	return 0;
}

int nsl_interp_integral(double *x, double *y, unsigned int n) {
	double vold=0.;
	unsigned int i;
	for (i=0; i < n-1; i++) {
		/* trapezoidal rule */
		double v = (x[i+1]-x[i])*(y[i+1]+y[i])/2.;
		if (i == 0)
			y[i] = vold;
		else
			y[i] = y[i-1]+vold;
		vold = v;
	}
	y[n-1] = y[n-2] + vold;

	return 0;
}

int nsl_interp_ratint(double *x, double *y, int n, double xn, double *v, double *dv) {
	int i,j,a=0,b=n-1;
	while (b-a > 1) {       /* find interval using bisection */
		j = floor((a+b)/2.);
		if (x[j] > xn)
			b = j;
		else
			a = j;
	}

	int ns=a;	/* nearest index */
	if (fabs(xn-x[a]) > fabs(xn-x[b]))
		ns=b;

	if (xn == x[ns]) {      /* exact point */
		*v = y[ns];
		*dv = 0;
		return 1;
	}

	double *c = (double*)malloc(n*sizeof(double));
	double *d = (double*)malloc(n*sizeof(double));
	for (i=0; i < n; i++)
		c[i] = d[i] = y[i];

	*v = y[ns--];

	double t, dd;
	int m;
	for (m=1; m < n; m++) {
		for (i=0; i < n-m; i++) {
			t = (x[i]-xn)*d[i]/(x[i+m]-xn);
			dd = t-c[i+1];
			if (dd == 0.0) /* pole */
				dd += DBL_MIN;
			dd = (c[i+1]-d[i])/dd;
			d[i] = c[i+1]*dd;
			c[i] = t*dd;
		}

		*dv = (2*(ns+1) < (n-m) ? c[ns+1] : d[ns--]);
		*v += *dv;
	}

	free(c); free(d);

	return 0;
}
