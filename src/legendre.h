// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Distributed under the terms of the BSD license (see the LICENSE
// file for the exact terms).
// Email: hermes1d@googlegroups.com, home page: http://hpfem.org/

#ifndef SHAPESET_LEGENDRE_H_
#define SHAPESET_LEGENDRE_H_

#include <math.h>

#include "common.h"

extern double leg_norm_const(int n);
extern void fill_legendre_array(double x, 
                                double val_array[MAX_P+1],
                                double der_array[MAX_P+1]);
extern double calc_legendre_val(double x, int n);
extern double calc_legendre_der(double x, int n);

//extern shape_fn_t legendre_fn_tab_1d[];
//extern shape_fn_t legendre_der_tab_1d[];

extern int legendre_order_1d[];

#endif /* SHAPESET_LOBATTO_H_ */
