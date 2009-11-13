// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Distributed under the terms of the BSD license (see the LICENSE
// file for the exact terms).
// Email: hermes1d@googlegroups.com, home page: http://hpfem.org/

#include "transforms.h"

// transform values from (-1, 0) to (-1, 1)
#define map_left(x) (2*x+1)
// transform values from (0, 1) to (-1, 1)
#define map_right(x) (2*x-1)

// returns values of Legendre polynomials transformed
// and normalized to be orthonormal on (-1,0)
double legendre_left(int i, double x) {
  return sqrt(2)*legendre_fn_tab_1d[i](map_left(x));
}

// returns values of Legendre polynomials on (-1,1)
double legendre(int i, double x) {
  return legendre_fn_tab_1d[i](x);
}

// returns values of Legendre polynomials transformed
// and normalized to be orthonormal on (0,1)
double legendre_right(int i, double x) {
  return sqrt(2)*legendre_fn_tab_1d[i](map_right(x));
}

double calc_elem_L2_norm_squared(Element *e, double *y_prev, 
                        double bc_left_dir_values[MAX_EQN_NUM],
			double bc_right_dir_values[MAX_EQN_NUM]) 
{
  int n_eq = e->dof_size;
  double phys_x[MAX_PTS_NUM];         // quad points in physical element
  double phys_weights[MAX_PTS_NUM];   // quad weights in physical element
  // values of solution for all solution components
  double phys_val[MAX_EQN_NUM][MAX_PTS_NUM]; 
  double phys_der[MAX_EQN_NUM][MAX_PTS_NUM]; // not used

  // integration order
  int order = 2*e->p;
  int pts_num = 0;

  // create Gauss quadrature in 'e'
  create_phys_element_quadrature(e->x1, e->x2, order, phys_x, phys_weights,
                            &pts_num); 

  // evaluate solution and its derivative 
  // at all quadrature points in 'e', for every solution component
  e->get_solution(phys_x, pts_num, phys_val, phys_der, y_prev,
                  bc_left_dir_values, bc_right_dir_values);

  // integrate square over (-1, 1)
  double norm_squared[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) {
    norm_squared[c] = 0;
    for (int i=0; i<pts_num; i++) {
      double val = phys_val[c][i];
      norm_squared[c] += val * val * phys_weights[i];
    }
  }

  double elem_norm_squared = 0;
  for (int c=0; c<n_eq; c++)  
    elem_norm_squared += norm_squared[c];
  return elem_norm_squared;
}

double calc_elem_L2_error_squared_p(Element *e, Element *e_ref, 
                        double *y_prev, double *y_prev_ref, 
                        double bc_left_dir_values[MAX_EQN_NUM],
			double bc_right_dir_values[MAX_EQN_NUM]) 
{
  // create Gauss quadrature on 'e'
  int order = 2*e_ref->p;
  int pts_num;
  double phys_x[MAX_PTS_NUM];          // quad points
  double phys_weights[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e->x1, e->x2, order, phys_x, phys_weights, &pts_num); 

  // get coarse mesh solution values and derivatives
  double phys_u[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx[MAX_EQN_NUM][MAX_PTS_NUM];
  e->get_solution(phys_x, pts_num, phys_u, phys_dudx, y_prev, 
                  bc_left_dir_values, bc_right_dir_values); 

  // get fine mesh solution values and derivatives
  double phys_u_ref[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_ref[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref->get_solution(phys_x, pts_num, phys_u_ref, phys_dudx_ref, y_prev_ref, 
                  bc_left_dir_values, bc_right_dir_values); 

  // integrate over 'e'
  double norm_squared[MAX_EQN_NUM];
  int n_eq = e->dof_size;
  for (int c=0; c<n_eq; c++) {
    norm_squared[c] = 0;
    for (int i=0; i<pts_num; i++) {
      double diff = phys_u_ref[c][i] - phys_u[c][i];
      norm_squared[c] += diff*diff*phys_weights[i];
    }
  }

  double err_squared = 0;
  for (int c=0; c<n_eq; c++)  
    err_squared += norm_squared[c];
  return err_squared;
}

double calc_elem_L2_error_squared_hp(Element *e, 
				   Element *e_ref_left, Element *e_ref_right,
                                   double *y_prev, double *y_prev_ref, 
                                   double bc_left_dir_values[MAX_EQN_NUM],
			           double bc_right_dir_values[MAX_EQN_NUM]) 
{
  // create Gauss quadrature on 'e_ref_left'
  int order_left = 2*e_ref_left->p;
  int pts_num_left;
  double phys_x_left[MAX_PTS_NUM];          // quad points
  double phys_weights_left[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e_ref_left->x1, e_ref_left->x2, 
                                 order_left, phys_x_left, phys_weights_left, &pts_num_left); 

  // get coarse mesh solution values and derivatives on 'e_ref_left'
  double phys_u_left[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_left[MAX_EQN_NUM][MAX_PTS_NUM];
  e->get_solution(phys_x_left, pts_num_left, phys_u_left, phys_dudx_left, y_prev, 
                  bc_left_dir_values, bc_right_dir_values); 

  // get fine mesh solution values and derivatives on 'e_ref_left'
  double phys_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_left->get_solution(phys_x_left, pts_num_left, phys_u_ref_left, phys_dudx_ref_left, y_prev_ref, 
                  bc_left_dir_values, bc_right_dir_values); 

  // integrate over 'e_ref_left'
  double norm_squared_left[MAX_EQN_NUM];
  int n_eq = e->dof_size;
  for (int c=0; c<n_eq; c++) {
    norm_squared_left[c] = 0;
    for (int i=0; i<pts_num_left; i++) {
      double diff = phys_u_ref_left[c][i] - phys_u_left[c][i];
      norm_squared_left[c] += diff*diff*phys_weights_left[i];
    }
  }

  // create Gauss quadrature on 'e_ref_right'
  int order_right = 2*e_ref_right->p;
  int pts_num_right;
  double phys_x_right[MAX_PTS_NUM];          // quad points
  double phys_weights_right[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e_ref_right->x1, e_ref_right->x2, 
                                 order_right, phys_x_right, phys_weights_right, &pts_num_right); 

  // get coarse mesh solution values and derivatives on 'e_ref_right'
  double phys_u_right[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_right[MAX_EQN_NUM][MAX_PTS_NUM];
  e->get_solution(phys_x_right, pts_num_right, phys_u_right, phys_dudx_right, y_prev, 
                  bc_left_dir_values, bc_right_dir_values); 

  // get fine mesh solution values and derivatives on 'e_ref_right'
  double phys_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_right->get_solution(phys_x_right, pts_num_right, phys_u_ref_right, phys_dudx_ref_right, 
                  y_prev_ref, bc_left_dir_values, bc_right_dir_values); 

  // integrate over 'e_ref_right'
  double norm_squared_right[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) {
    norm_squared_right[c] = 0;
    for (int i=0; i<pts_num_right; i++) {
      double diff = phys_u_ref_right[c][i] - phys_u_right[c][i];
      norm_squared_right[c] += diff*diff*phys_weights_right[i];
    }
  }

  double err_squared = 0;
  for (int c=0; c<n_eq; c++) {
    err_squared += norm_squared_left[c] + norm_squared_right[c];
  }
  return err_squared;
}

double calc_elem_L2_errors_squared(Mesh* mesh, Mesh* mesh_ref, 
				   double* y_prev, double* y_prev_ref, 
				   double *err_squared_array)
{
  double err_total_squared = 0;
  Iterator *I = new Iterator(mesh);
  Iterator *I_ref = new Iterator(mesh_ref);

  // simultaneous traversal of 'mesh' and 'mesh_ref'
  Element *e;
  while ((e = I->next_active_element()) != NULL) {
    Element *e_ref = I_ref->next_active_element();
    double err_squared;
    if (e->level == e_ref->level) { // element 'e' was not refined in space
                                    // for reference solution
      err_squared = calc_elem_L2_error_squared_p(e, e_ref, y_prev, y_prev_ref, 
                                         mesh->bc_left_dir_values,
			                 mesh->bc_right_dir_values);
    }
    else { // element 'e' was refined in space for reference solution
      Element* e_ref_left = e_ref;
      Element* e_ref_right = I_ref->next_active_element();
      err_squared = calc_elem_L2_error_squared_hp(e, e_ref_left, e_ref_right, 
                                          y_prev, y_prev_ref, 
                                          mesh->bc_left_dir_values,
			                  mesh->bc_right_dir_values);
    }
    err_squared_array[e->id] = err_squared;
    err_total_squared += err_squared;
  }
  return sqrt(err_total_squared);
}

double calc_solution_L2_norm(Mesh* mesh, double* y_prev)
{
  double norm_squared = 0;
  Iterator *I = new Iterator(mesh);

  // traverse 'mesh' and calculate squared solution L2 norm 
  // in every element 
  Element *e;
  while ((e = I->next_active_element()) != NULL) {
    norm_squared += calc_elem_L2_norm_squared(e, y_prev,
                                         mesh->bc_left_dir_values,
			                 mesh->bc_right_dir_values);
  }
  return sqrt(norm_squared);
}

/* qsort int comparison function */
int int_cmp(const void *a, const void *b)
{
    const double *ia = (const double *)a; // casting pointer types
    const double *ib = (const double *)b;
    return ia[0] - ib[0] < 0; 
	/* double comparison: returns negative if b[0] < a[0] 
	and positive if a[0] < b[0] */
}

// sorting elements according to err_squared_array[]. After this,
// id_array[] will contain indices of sirted elements, and also 
// err_squared_array[] will be sorted. 
void sort_element_errors(int n, double *err_squared_array, int *id_array) 
{
    double array[MAX_ELEM_NUM][2];
    for (int i=0; i<n; i++) {
      array[i][0] = err_squared_array[i];
      array[i][1] = id_array[i];
    }

    qsort(array, n, 2*sizeof(double), int_cmp);

    for (int i=0; i<n; i++) {
      err_squared_array[i] = array[i][0];
      id_array[i] = array[i][1];
    }
}

// Assumes that reference solution is defined on two half-elements 'e_ref_left'
// and 'e_ref_right'. The reference solution is projected onto the space of 
// (discontinuous) polynomials of degree 'p_left' on 'e_ref_left'
// and degree 'p_right' on 'e_ref_right'
double check_refin_coarse_hp_fine_hp(Element *e, Element *e_ref_left, Element *e_ref_right, 
                                   double *y_prev_ref, int p_left, int p_right, 
                                   double bc_left_dir_values[MAX_EQN_NUM],
		                   double bc_right_dir_values[MAX_EQN_NUM])
{
  int n_eq = e->dof_size;

  // First in 'e_ref_left': 
  // Calculate L2 projection of the reference solution on 
  // transformed Legendre polynomials of degree 'p_left'

  // create Gauss quadrature on 'e_ref_left'
  int order_left = 2*max(e_ref_left->p, p_left);
  int pts_num_left;
  double phys_x_left[MAX_PTS_NUM];          // quad points
  double phys_weights_left[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e_ref_left->x1, e_ref_left->x2, 
                                 order_left, phys_x_left, phys_weights_left, &pts_num_left); 

  // get fine mesh solution values and derivatives on 'e_ref_left'
  double phys_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_left->get_solution(phys_x_left, pts_num_left, phys_u_ref_left, phys_dudx_ref_left, y_prev_ref, 
                  bc_left_dir_values, bc_right_dir_values); 

  // get values of transformed Legendre polynomials in 'e_ref_left'
  double leg_pol_values_left[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m<p_left + 1; m++) { // loop over transf. Leg. polynomials
    for(int j=0; j<pts_num_left; j++) {  
      leg_pol_values_left[m][j] = legendre_left(m, inverse_map(e_ref_left->x1, 
					   e_ref_left->x2, phys_x_left[j]));
    }
  }

  // calculate the projection coefficients for every 
  // transformed Legendre polynomial and every solution 
  // component. Since the basis is orthonormal, these 
  // are just integrals of the fine mesh solution with 
  // the transformed Legendre polynomials
  double proj_coeffs_left[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m<p_left + 1; m++) { // loop over transf. Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs_left[c][m] = 0;
      for(int j=0; j<pts_num_left; j++) { // loop over integration points
        proj_coeffs_left[c][m] += 
          phys_u_ref_left[c][j] * leg_pol_values_left[m][j] * phys_weights_left[j];
      }
    }
  }

  // evaluate the projection in 'e_ref_left' for every solution component
  // and every integration point
  double phys_u_left[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_left; j++) { // loop over integration points
      phys_u_left[c][j] = 0;
      for (int m=0; m<p_left+1; m++) { // loop over transf. Leg. polynomials
        phys_u_left[c][j] += 
          leg_pol_values_left[m][j] + proj_coeffs_left[c][m];
      }
    }
  }

  // calculate the error squared in L2 norm for every solution  
  // component in 'e_ref_left'
  double err_squared_left[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_left[c] = 0;
    for (int j=0; j<pts_num_left; j++) { // loop over integration points
      double diff = phys_u_ref_left[c][j] - phys_u_left[c][j];
      err_squared_left[c] += diff * diff * phys_weights_left[j]; 
    }
  }

  // Second on 'e_ref_right': 
  // Calculate L2 projection of the reference solution on 
  // transformed Legendre polynomials of degree 'p_right'

  // create Gauss quadrature on 'e_ref_right'
  int order_right = 2*max(e_ref_right->p, p_right);
  int pts_num_right;
  double phys_x_right[MAX_PTS_NUM];          // quad points
  double phys_weights_right[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e_ref_right->x1, e_ref_right->x2, 
                                 order_right, phys_x_right, phys_weights_right, &pts_num_right); 

  // get fine mesh solution values and derivatives on 'e_ref_right'
  double phys_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_right->get_solution(phys_x_right, pts_num_right, phys_u_ref_right, phys_dudx_ref_right, 
                  y_prev_ref, bc_left_dir_values, bc_right_dir_values); 

  // get values of transformed Legendre polynomials in 'e_ref_right'
  double leg_pol_values_right[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m<p_right + 1; m++) { // loop over transf. Leg. polynomials
    for(int j=0; j<pts_num_right; j++) {
      leg_pol_values_right[m][j] = legendre_right(m, inverse_map(e_ref_right->x1, 
					   e_ref_right->x2, phys_x_right[j]));
    }
  }

  // calculate the projection coefficients for every 
  // transformed Legendre polynomial and every solution 
  // component. Since the basis is orthonormal, these 
  // are just integrals of the fine mesh solution with 
  // the transformed Legendre polynomials
  double proj_coeffs_right[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m<p_right + 1; m++) { // loop over transf. Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs_right[c][m] = 0;
      for(int j=0; j<pts_num_right; j++) { // loop over integration points
        proj_coeffs_right[c][m] += 
          phys_u_ref_right[c][j] * leg_pol_values_right[m][j] * phys_weights_right[j];
      }
    }
  }

  // evaluate the projection in 'e_ref_right' for every solution component
  // and every integration point
  double phys_u_right[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_right; j++) { // loop over integration points
      phys_u_right[c][j] = 0;
      for (int m=0; m<p_right+1; m++) { // loop over transf. Leg. polynomials
        phys_u_right[c][j] += 
          leg_pol_values_right[m][j] + proj_coeffs_right[c][m];
      }
    }
  }

  // calculate the error squared in L2 norm for every solution  
  // component in 'e_ref_right'
  double err_squared_right[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_right[c] = 0;
    for (int j=0; j<pts_num_right; j++) { // loop over integration points
      double diff = phys_u_ref_right[c][j] - phys_u_right[c][j];
      err_squared_right[c] += diff * diff * phys_weights_right[j]; 
    }
  }

  // summing errors on 'e_ref_left' and 'e_ref_right'
  double err_total = 0;
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_total += (err_squared_left[c] + err_squared_right[c]);
  }
  err_total = sqrt(err_total);

  // penalizing the error by the number of DOF induced by this 
  // refinement candidate
  // NOTE: this may need some experimantation
  int dof_orig = e->p + 1;
  int dof_new = p_left + p_right + 1; 
  int dof_added = dof_new - dof_orig; 
  double error_scaled = -log(err_total) / dof_added; 

  return error_scaled; 
}

// Assumes that reference solution is defined on one single element 'e_ref' = 'e'. 
// The reference solution is projected onto the space of (discontinuous) 
// polynomials of degree 'p_left' on the left half of 'e' and degree 
// 'p_right' on the right half of 'e' 
double check_refin_coarse_hp_fine_p(Element *e, Element *e_ref,
                                    double *y_prev_ref, int p_left, int p_right,
                                    double bc_left_dir_values[MAX_EQN_NUM],
		                    double bc_right_dir_values[MAX_EQN_NUM])
{
  int n_eq = e->dof_size;

  // First in the left half of 'e': 
  // Calculate L2 projection of the reference solution on 
  // transformed Legendre polynomials of degree 'p_left'

  // create Gauss quadrature on the left half
  int order_left = 2*max(e_ref->p, p_left);
  int pts_num_left;
  double phys_x_left[MAX_PTS_NUM];          // quad points
  double phys_weights_left[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e->x1, (e->x1 + e->x2)/2., 
                                 order_left, phys_x_left, phys_weights_left, &pts_num_left); 

  // get fine mesh solution values and derivatives on the left half
  double phys_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref->get_solution(phys_x_left, pts_num_left, phys_u_ref_left, phys_dudx_ref_left, y_prev_ref, 
                  bc_left_dir_values, bc_right_dir_values); 

  // get values of transformed Legendre polynomials in the left half
  double leg_pol_values_left[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m<p_left + 1; m++) { // loop over transf. Leg. polynomials
    for(int j=0; j<pts_num_left; j++) {  
      leg_pol_values_left[m][j] = legendre_left(m, inverse_map(e->x1, (e->x1 + e->x2)/2,  
					        phys_x_left[j]));
    }
  }

  // calculate the projection coefficients for every 
  // transformed Legendre polynomial and every solution 
  // component. Since the basis is orthonormal, these 
  // are just integrals of the fine mesh solution with 
  // the transformed Legendre polynomials
  double proj_coeffs_left[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m<p_left + 1; m++) { // loop over transf. Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs_left[c][m] = 0;
      for(int j=0; j<pts_num_left; j++) { // loop over integration points
        proj_coeffs_left[c][m] += 
          phys_u_ref_left[c][j] * leg_pol_values_left[m][j] * phys_weights_left[j];
      }
    }
  }

  // evaluate the projection on the left half for every solution component
  // and every integration point
  double phys_u_left[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_left; j++) { // loop over integration points
      phys_u_left[c][j] = 0;
      for (int m=0; m<p_left+1; m++) { // loop over transf. Leg. polynomials
        phys_u_left[c][j] += 
          leg_pol_values_left[m][j] + proj_coeffs_left[c][m];
      }
    }
  }

  // calculate the error squared in L2 norm for every solution  
  // component in the left half
  double err_squared_left[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_left[c] = 0;
    for (int j=0; j<pts_num_left; j++) { // loop over integration points
      double diff = phys_u_ref_left[c][j] - phys_u_left[c][j];
      err_squared_left[c] += diff * diff * phys_weights_left[j]; 
    }
  }

  // Second on the right half: 
  // Calculate L2 projection of the reference solution on 
  // transformed Legendre polynomials of degree 'p_right'

  // create Gauss quadrature on the right half
  int order_right = 2*max(e_ref->p, p_right);
  int pts_num_right;
  double phys_x_right[MAX_PTS_NUM];          // quad points
  double phys_weights_right[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature((e->x1 + e->x2)/2., e->x2, 
                                 order_right, phys_x_right, phys_weights_right, &pts_num_right); 

  // get fine mesh solution values and derivatives on the right half
  double phys_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref->get_solution(phys_x_right, pts_num_right, phys_u_ref_right, phys_dudx_ref_right, 
                  y_prev_ref, bc_left_dir_values, bc_right_dir_values); 

  // get values of transformed Legendre polynomials on the right half
  double leg_pol_values_right[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m<p_right + 1; m++) { // loop over transf. Leg. polynomials
    for(int j=0; j<pts_num_right; j++) {
      leg_pol_values_right[m][j] = legendre_right(m, inverse_map((e->x1 + e->x2)/2, e->x2,  
					   phys_x_right[j]));
    }
  }

  // calculate the projection coefficients for every 
  // transformed Legendre polynomial and every solution 
  // component. Since the basis is orthonormal, these 
  // are just integrals of the fine mesh solution with 
  // the transformed Legendre polynomials
  double proj_coeffs_right[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m<p_right + 1; m++) { // loop over transf. Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs_right[c][m] = 0;
      for(int j=0; j<pts_num_right; j++) { // loop over integration points
        proj_coeffs_right[c][m] += 
          phys_u_ref_right[c][j] * leg_pol_values_right[m][j] * phys_weights_right[j];
      }
    }
  }

  // evaluate the projection on the right half for every solution component
  // and every integration point
  double phys_u_right[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_right; j++) { // loop over integration points
      phys_u_right[c][j] = 0;
      for (int m=0; m<p_right+1; m++) { // loop over transf. Leg. polynomials
        phys_u_right[c][j] += 
          leg_pol_values_right[m][j] + proj_coeffs_right[c][m];
      }
    }
  }

  // calculate the error squared in L2 norm for every solution  
  // component on the right half
  double err_squared_right[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_right[c] = 0;
    for (int j=0; j<pts_num_right; j++) { // loop over integration points
      double diff = phys_u_ref_right[c][j] - phys_u_right[c][j];
      err_squared_right[c] += diff * diff * phys_weights_right[j]; 
    }
  }

  // summing errors on the two halves
  double err_total = 0;
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_total += (err_squared_left[c] + err_squared_right[c]);
  }
  err_total = sqrt(err_total);

  // penalizing the error by the number of DOF induced by this 
  // refinement candidate
  // NOTE: this may need some experimantation
  int dof_orig = e->p + 1;
  int dof_new = p_left + p_right + 1; 
  int dof_added = dof_new - dof_orig; 
  double error_scaled = -log(err_total) / dof_added; 

  return error_scaled;
}

// Assumes that reference solution is defined on two half-elements 'e_ref_left'
// and 'e_ref_right'. The reference solution is projected onto the space of 
// polynomials of degree 'p' on 'e'
double check_refin_coarse_p_fine_hp(Element *e, Element *e_ref_left, Element *e_ref_right, 
                                    double *y_prev_ref, int p,
                                    double bc_left_dir_values[MAX_EQN_NUM],
		                    double bc_right_dir_values[MAX_EQN_NUM])
{
  int n_eq = e->dof_size;

  // First in 'e_ref_left': 
  // Calculate first part of projection coefficients

  // create Gauss quadrature on 'e_ref_left'
  int order_left = 2*max(e_ref_left->p, p);
  int pts_num_left;
  double phys_x_left[MAX_PTS_NUM];          // quad points
  double phys_weights_left[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e_ref_left->x1, e_ref_left->x2, 
                                 order_left, phys_x_left, phys_weights_left, &pts_num_left); 

  // get fine mesh solution values and derivatives on 'e_ref_left'
  double phys_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_left->get_solution(phys_x_left, pts_num_left, phys_u_ref_left, phys_dudx_ref_left, 
                           y_prev_ref, 
                           bc_left_dir_values, bc_right_dir_values); 

  // get values of (original) Legendre polynomials in 'e_ref_left'
  double leg_pol_values_left[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m < p + 1; m++) { // loop over transf. Leg. polynomials
    for(int j=0; j<pts_num_left; j++) {  
      leg_pol_values_left[m][j] = legendre(m, inverse_map(e_ref_left->x1, 
					   e_ref_left->x2, phys_x_left[j]));
    }
  }

  // calculate first part of the projection coefficients 
  double proj_coeffs_left[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m<p + 1; m++) { // loop over Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs_left[c][m] = 0;
      for(int j=0; j<pts_num_left; j++) { // loop over integration points
        proj_coeffs_left[c][m] += 
          phys_u_ref_left[c][j] * leg_pol_values_left[m][j] * phys_weights_left[j];
      }
    }
  }

  // Second in 'e_ref_right': 
  // Calculate second part of projection coefficients

  // create Gauss quadrature on 'e_ref_right'
  int order_right = 2*max(e_ref_right->p, p);
  int pts_num_right;
  double phys_x_right[MAX_PTS_NUM];          // quad points
  double phys_weights_right[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e_ref_right->x1, e_ref_right->x2, 
                                 order_right, phys_x_right, phys_weights_right, &pts_num_right); 

  // get fine mesh solution values and derivatives on 'e_ref_right'
  double phys_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_right->get_solution(phys_x_right, pts_num_right, phys_u_ref_right, phys_dudx_ref_right, 
                  y_prev_ref, bc_left_dir_values, bc_right_dir_values); 

  // get values of (original) Legendre polynomials in 'e_ref_right'
  double leg_pol_values_right[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
    for(int j=0; j<pts_num_right; j++) {
      leg_pol_values_right[m][j] = legendre(m, inverse_map(e_ref_right->x1, 
					    e_ref_right->x2, phys_x_right[j]));
    }
  }

  // calculate the second part of the projection coefficients
  double proj_coeffs_right[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m < p + 1; m++) { // loop over transf. Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs_right[c][m] = 0;
      for(int j=0; j<pts_num_right; j++) { // loop over integration points
        proj_coeffs_right[c][m] += 
          phys_u_ref_right[c][j] * leg_pol_values_right[m][j] * phys_weights_right[j];
      }
    }
  }

  // add the two parts of projection coefficients
  double proj_coeffs[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m < p + 1; m++) { // loop over transf. Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs[c][m] = proj_coeffs_left[c][m] + proj_coeffs_right[c][m];
    }
  }

  // evaluate the projection in 'e_ref_left' for every solution component
  // and every integration point
  double phys_u_left[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_left; j++) { // loop over integration points left
      phys_u_left[c][j] = 0;
      for (int m=0; m < p+1; m++) { // loop over Leg. polynomials
        phys_u_left[c][j] += 
          leg_pol_values_left[m][j] + proj_coeffs[c][m];
      }
    }
  }

  // evaluate the projection in 'e_ref_right' for every solution component
  // and every integration point
  double phys_u_right[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_right; j++) { // loop over integration points right
      phys_u_right[c][j] = 0;
      for (int m=0; m < p+1; m++) { // loop over Leg. polynomials
        phys_u_right[c][j] += 
          leg_pol_values_right[m][j] + proj_coeffs[c][m];
      }
    }
  }

  // calculate the error squared in L2 norm for every solution  
  // component in 'e_ref_left'
  double err_squared_left[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_left[c] = 0;
    for (int j=0; j<pts_num_left; j++) { // loop over integration points left
      double diff = phys_u_ref_left[c][j] - phys_u_left[c][j];
      err_squared_left[c] += diff * diff * phys_weights_left[j]; 
    }
  }

  // calculate the error squared in L2 norm for every solution  
  // component in 'e_ref_right'
  double err_squared_right[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_right[c] = 0;
    for (int j=0; j<pts_num_right; j++) { // loop over integration points left
      double diff = phys_u_ref_right[c][j] - phys_u_right[c][j];
      err_squared_right[c] += diff * diff * phys_weights_right[j]; 
    }
  }

  // summing errors over components
  double err_total = 0;
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_total += (err_squared_left[c] + err_squared_right[c]);
  }
  err_total = sqrt(err_total);

  // penalizing the error by the number of DOF induced by this 
  // refinement candidate
  // NOTE: this may need some experimantation
  int dof_orig = e->p + 1;
  int dof_new = p + 1; 
  int dof_added = dof_new - dof_orig; 
  double error_scaled = -log(err_total) / dof_added; 

  return error_scaled; 
}

// Assumes that reference solution is defined on one single element 
// 'e_ref' (reference refinement did not split the element in space). 
// The reference solution is projected onto the space of 
// polynomials of degree 'p' on 'e'
double check_ref_coarse_p_fine_p(Element *e, Element *e_ref,
                                 double *y_prev_ref, int p,
                                 double bc_left_dir_values[MAX_EQN_NUM],
		                 double bc_right_dir_values[MAX_EQN_NUM])
{
  int n_eq = e->dof_size;

  // Calculate L2 projection of the reference solution on 
  // (original) Legendre polynomials of degree 'p'

  // create Gauss quadrature on 'e'
  int order = 2*max(e->p, p);
  int pts_num;
  double phys_x[MAX_PTS_NUM];          // quad points
  double phys_weights[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e->x1, e->x2, 
                                 order, phys_x, phys_weights, &pts_num); 

  // get fine mesh solution values and derivatives on 'e_ref'
  double phys_u_ref[MAX_EQN_NUM][MAX_PTS_NUM], phys_dudx_ref[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref->get_solution(phys_x, pts_num, phys_u_ref, phys_dudx_ref, y_prev_ref, 
                      bc_left_dir_values, bc_right_dir_values); 

  // get values of (original) Legendre polynomials in 'e'
  double leg_pol_values[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
    for(int j=0; j<pts_num; j++) {  
      leg_pol_values[m][j] = legendre(m, inverse_map(e->x1, 
					   e->x2, phys_x[j]));
    }
  }

  // calculate the projection coefficients 
  double proj_coeffs[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs[c][m] = 0;
      for(int j=0; j < pts_num; j++) { // loop over integration points
        proj_coeffs[c][m] +=
          phys_u_ref[c][j] * leg_pol_values[m][j] * phys_weights[j];
      }
    }
  }

  // evaluate the projection in 'e' for every solution component
  // and every integration point
  double phys_u[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num; j++) { // loop over integration points
      phys_u[c][j] = 0;
      for (int m=0; m < p + 1; m++) { // loop over Leg. polynomials
        phys_u[c][j] += 
          leg_pol_values[m][j] + proj_coeffs[c][m];
      }
    }
  }

  // calculate the error squared in L2 norm for every solution  
  // component in 'e'
  double err_squared[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared[c] = 0;
    for (int j=0; j < pts_num; j++) { // loop over integration points
      double diff = phys_u_ref[c][j] - phys_u[c][j];
      err_squared[c] += diff * diff * phys_weights[j]; 
    }
  }

  // summing errors over components
  double err_total = 0;
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_total += err_squared[c];
  }
  err_total = sqrt(err_total);


  // penalizing the error by the number of DOF induced by this 
  // refinement candidate
  // NOTE: this may need some experimantation
  int dof_orig = e->p + 1;
  int dof_new = p + 1; 
  int dof_added = dof_new - dof_orig; 
  double error_scaled = -log(err_total) / dof_added; 

  return error_scaled; 
}

// Refine all elements in the id_array list whose id_array >= 0
void refine_elements(Mesh *mesh, Mesh *mesh_ref, double *y_prev, double *y_prev_ref, 
                     int *id_array, double *err_squared_array) 
{
  int adapt_list[MAX_ELEM_NUM];
  int num_to_adapt = 0;

  // Create list of ids of elements to be refined, in increasing order
  for (int i=0; i<mesh->get_n_active_elem(); i++) {
    if (id_array[i] >= 0) {
      adapt_list[num_to_adapt] = i;
      num_to_adapt++;
    }
  }
 
  // Debug: Printing list of elements to be refined
  //printf("refine_elements(): Elements to be refined:\n");
  //for (int i=0; i<num_to_adapt; i++) printf("Elem[%d]\n", adapt_list[i]);

  Iterator *I = new Iterator(mesh);
  Iterator *I_ref = new Iterator(mesh_ref);

  // simultaneous traversal of 'mesh' and 'mesh_ref'
  Element *e;
  int counter = 0;
  while (counter != num_to_adapt) {
    e = I->next_active_element();
    Element *e_ref = I_ref->next_active_element();
    if (e->id == adapt_list[counter]) {
      counter++;
      if (e->level == e_ref->level) { // element 'e' was not refined in space
                                      // for reference solution
        //calc_best_hp_refinement_p(e, e_ref, y_prev, y_prev_ref, 
        //                          mesh->bc_left_dir_values,
	//  		          mesh->bc_right_dir_values);
      }
      else { // element 'e' was refined in space for reference solution
        Element* e_ref_left = e_ref;
        Element* e_ref_right = I_ref->next_active_element();
        //calc_best_hp_refinement_h(e, e_ref_left, e_ref_right, y_prev, y_prev_ref, 
        //                          mesh->bc_left_dir_values,
	//		          mesh->bc_right_dir_values);
      }
      // perform the refinement
      int p_only = 1;
      int p_new = 2;
      int p_new_left = 1, p_new_right = 1;
      if (p_only) e->p += 1;
      else e->refine(p_new_left, p_new_right);
    }
  }
}


