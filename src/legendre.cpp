// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Distributed under the terms of the BSD license (see the LICENSE
// file for the exact terms).
// Email: hermes1d@googlegroups.com, home page: http://hpfem.org/

#include "legendre.h"

double legendre_val_ref_tab[MAX_QUAD_ORDER][MAX_QUAD_PTS_NUM][MAX_P + 1];
double legendre_der_ref_tab[MAX_QUAD_ORDER][MAX_QUAD_PTS_NUM][MAX_P + 1];

int legendre_order_1d[] = {
0,
 1, 2, 3, 4, 5, 6, 7, 8, 9,10,
11,12,13,14,15,16,17,18,19,20,
21,22,23,24,25,26,27,28,29,30,
31,32,33,34,35,36,37,38,39,40,
41,42,43,44,45,46,47,48,49,50,
51,52,53,54,55,56,57,58,59,60,
61,62,63,64,65,66,67,68,69,70,
71,72,73,74,75,76,77,78,79,80,
81,82,83,84,85,86,87,88,89,90,
91,92,93,94,95,96,97,98,99,100,
};

// Non-normalized Legendre polynomials divided by this constant 
// become orthonormal in the L2(-1, 1) product
double leg_norm_const_ref(int n)
{
  return sqrt(2./(2.*n + 1));
} 

// Fills an array of length MAX_P + 1 with Legendre polynomials 
// and their derivatives at point 'x'. The polynomials are 
// normalized in the inner product L^2(-1,1)
extern void fill_legendre_array_ref(double x, 
                                double val_array[MAX_P+1],
                                double der_array[MAX_P+1]) {
    int max_fns_num = MAX_P + 1;
    // first fill the array with unnormed Legendre 
    // polynomials using the recursive formula
    val_array[0] = 1.;
    der_array[0] = 0;
    val_array[1] = x;
    der_array[1] = 1.;
    for (int i=1; i < max_fns_num; i++) {
      val_array[i+1]  = (2*i+1)*x*val_array[i] - i*val_array[i-1]; 
      val_array[i+1] /= i+1; 
      der_array[i+1]  = (2*i+1)*(val_array[i] + x*der_array[i]) 
                        - i*der_array[i-1]; 
      der_array[i+1] /= i+1; 
    }
    // normalization
    for (int i=0; i < max_fns_num; i++) {
      val_array[i] /= leg_norm_const_ref(i);
      der_array[i] /= leg_norm_const_ref(i);
    }
}

// FIXME - this function is inefficient, 
// it fills the whole array
extern double legendre_val_ref(double x, int n) 
{
    // first fill the array with unnormed Legendre 
    // polynomials using the recursive formula
    double val_array[MAX_P + 1];
    double der_array[MAX_P + 1];
    fill_legendre_array_ref(x, val_array, der_array);
    return val_array[n];
}

// FIXME - this function is inefficient,
// it fills the whole array
extern double legendre_der_ref(double x, int n) 
{
    // first fill the array with unnormed Legendre 
    // polynomials using the recursive formula
    double val_array[MAX_P + 1];
    double der_array[MAX_P + 1];
    fill_legendre_array_ref(x, val_array, der_array);
    return der_array[n];
}

void precalculate_legendre_1d() 
{
  fprintf(stderr, "Precalculating Legendre polynomials...");
  fflush(stderr);
  // erasing
  for (int quad_order=0; quad_order < MAX_QUAD_ORDER; quad_order++) {
    for (int point_id=0; point_id < MAX_QUAD_PTS_NUM; point_id++) {
      for (int poly_deg=0; poly_deg < MAX_P + 1; poly_deg++) {
        legendre_val_ref_tab[quad_order][point_id][poly_deg] = 0;
      }
    }
  }

  for (int quad_order=0; quad_order < MAX_QUAD_ORDER; quad_order++) {
    int pts_num = g_quad_1d_std.get_num_points(quad_order);
    double2 *ref_tab = g_quad_1d_std.get_points(quad_order);
    for (int point_id=0; point_id < pts_num; point_id++) {
      double x_ref = ref_tab[point_id][0];
      fill_legendre_array_ref(x_ref, legendre_val_ref_tab[quad_order][point_id],
                          legendre_der_ref_tab[quad_order][point_id]);
    }
  }
  fprintf(stderr, "done.\n");

}
