// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Distributed under the terms of the BSD license (see the LICENSE
// file for the exact terms).
// Email: hermes1d@googlegroups.com, home page: http://hpfem.org/

#include "transforms.h"
#include "common.h"

// This is great help to debug automatic adaptivity. Generated are 
// Gnuplot files for all refinement candidates, showing both the 
// reference solution and the projection. Thus one can visually 
// examine the candidates and see whether the adaptivity algorithm
// selects the correct one. 
int PLOT_CANDIDATE_PROJECTIONS = 1;

// This is another help for debugging hp-adaptivity; All candidates
//that are tried are printed along with their performance criterion
int PRINT_CANDIDATES = 1;

// returns values of normalized Legendre polynomials on (a, b)
double legendre(int i, double a, double b, double x) {  // x \in (a, b)
  double norm_const = sqrt(2/(b-a));
  return norm_const*legendre_fn_tab_1d[i](inverse_map(a, b, x));
}

// returns derivatives of normalized Legendre polynomials on (a, b)
double legendre_der(int i, double a, double b, double x) {  // x \in (a, b)
  double norm_const = sqrt(2/(b-a));
  return norm_const*legendre_der_tab_1d[i](inverse_map(a, b, x));
}

double calc_elem_norm_squared(int norm, Element *e, double *y_prev, 
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
      if (norm == 1) {
        double der;
        der = phys_der[c][i];
        norm_squared[c] += (val*val + der*der) * phys_weights[i];
      }
      else norm_squared[c] += val*val * phys_weights[i];
    }
  }

  double elem_norm_squared = 0;
  for (int c=0; c<n_eq; c++)  
    elem_norm_squared += norm_squared[c];
  return elem_norm_squared;
}

double calc_elem_est_error_squared_p(int norm, Element *e, Element *e_ref, 
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
      double diff_val = phys_u_ref[c][i] - phys_u[c][i];
      if (norm == 1) {
        double diff_der = phys_dudx_ref[c][i] - phys_dudx[c][i];
        norm_squared[c] += (diff_val*diff_val + diff_der*diff_der)*phys_weights[i];
      }
      else norm_squared[c] += diff_val*diff_val*phys_weights[i];
    }
  }

  double err_squared = 0;
  for (int c=0; c<n_eq; c++)  
    err_squared += norm_squared[c];
  return err_squared;
}

double calc_elem_est_error_squared_hp(int norm, Element *e, 
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
                                 order_left, phys_x_left, 
                                 phys_weights_left, &pts_num_left); 

  // get coarse mesh solution values and derivatives on 'e_ref_left'
  double phys_u_left[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_left[MAX_EQN_NUM][MAX_PTS_NUM];
  e->get_solution(phys_x_left, pts_num_left, phys_u_left, phys_dudx_left, y_prev, 
                  bc_left_dir_values, bc_right_dir_values); 

  // get fine mesh solution values and derivatives on 'e_ref_left'
  double phys_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_left->get_solution(phys_x_left, pts_num_left, 
                           phys_u_ref_left, phys_dudx_ref_left, y_prev_ref, 
                           bc_left_dir_values, bc_right_dir_values); 

  // integrate over 'e_ref_left'
  double norm_squared_left[MAX_EQN_NUM];
  int n_eq = e->dof_size;
  for (int c=0; c<n_eq; c++) {
    norm_squared_left[c] = 0;
    for (int i=0; i<pts_num_left; i++) {
      double diff_val = phys_u_ref_left[c][i] - phys_u_left[c][i];
      if (norm == 1) {
        double diff_der = phys_dudx_ref_left[c][i] - phys_dudx_left[c][i];
        norm_squared_left[c] += (diff_val*diff_val + diff_der*diff_der)
                                 * phys_weights_left[i];
      }
      else norm_squared_left[c] += diff_val*diff_val*phys_weights_left[i];
    }
  }

  // create Gauss quadrature on 'e_ref_right'
  int order_right = 2*e_ref_right->p;
  int pts_num_right;
  double phys_x_right[MAX_PTS_NUM];          // quad points
  double phys_weights_right[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e_ref_right->x1, e_ref_right->x2, 
                                 order_right, phys_x_right, 
                                 phys_weights_right, &pts_num_right); 

  // get coarse mesh solution values and derivatives on 'e_ref_right'
  double phys_u_right[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_right[MAX_EQN_NUM][MAX_PTS_NUM];
  e->get_solution(phys_x_right, pts_num_right, phys_u_right, 
                  phys_dudx_right, y_prev, 
                  bc_left_dir_values, bc_right_dir_values); 

  // get fine mesh solution values and derivatives on 'e_ref_right'
  double phys_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_right->get_solution(phys_x_right, pts_num_right, 
                            phys_u_ref_right, phys_dudx_ref_right, 
                            y_prev_ref, bc_left_dir_values, 
                            bc_right_dir_values); 

  // integrate over 'e_ref_right'
  double norm_squared_right[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) {
    norm_squared_right[c] = 0;
    for (int i=0; i<pts_num_right; i++) {
      double diff_val = phys_u_ref_right[c][i] - phys_u_right[c][i];
      if (norm == 1) {
        double diff_der = phys_dudx_ref_right[c][i] - phys_dudx_right[c][i];
        norm_squared_right[c] += (diff_val*diff_val + diff_der*diff_der)
                                  * phys_weights_right[i];
      }
      else norm_squared_right[c] += diff_val*diff_val*phys_weights_right[i];
    }
  }

  double err_squared = 0;
  for (int c=0; c<n_eq; c++) {
    err_squared += norm_squared_left[c] + norm_squared_right[c];
  }
  
  return err_squared;
}

double calc_elem_est_errors_squared(int norm, Mesh* mesh, Mesh* mesh_ref, 
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
      err_squared = calc_elem_est_error_squared_p(norm, e, e_ref, y_prev, y_prev_ref, 
                                         mesh->bc_left_dir_values,
			                 mesh->bc_right_dir_values);
    }
    else { // element 'e' was refined in space for reference solution
      Element* e_ref_left = e_ref;
      Element* e_ref_right = I_ref->next_active_element();
      err_squared = calc_elem_est_error_squared_hp(norm, e, e_ref_left, e_ref_right, 
                                          y_prev, y_prev_ref, 
                                          mesh->bc_left_dir_values,
			                  mesh->bc_right_dir_values);
    }
    //printf("  Elem (%g, %g): err_est = %g\n", e->x1, e->x2, sqrt(err_squared));
    err_squared_array[e->id] = err_squared;
    err_total_squared += err_squared;
  }
  return sqrt(err_total_squared);
}

double calc_approx_sol_norm(int norm, Mesh* mesh, double* y_prev)
{
  double norm_squared = 0;
  Iterator *I = new Iterator(mesh);

  // traverse 'mesh' and calculate squared solution L2 or H1 norm 
  // in every element 
  Element *e;
  while ((e = I->next_active_element()) != NULL) {
    norm_squared += calc_elem_norm_squared(norm, e, y_prev,
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
double check_cand_coarse_hp_fine_hp(int norm, Element *e, Element *e_ref_left, 
                                    Element *e_ref_right, 
                                    double *y_prev_ref, int p_left, int p_right, 
                                    double bc_left_dir_values[MAX_EQN_NUM],
				    double bc_right_dir_values[MAX_EQN_NUM],
                                    double &err, int &dof)
{
  int n_eq = e->dof_size;

  // First in 'e_ref_left': 
  // Calculate L2 or H1 projection of the reference solution on 
  // transformed Legendre polynomials of degree 'p_left'

  // create Gauss quadrature on 'e_ref_left'
  int order_left = 2*max(e_ref_left->p, p_left);
  int pts_num_left;
  double phys_x_left[MAX_PTS_NUM];          // quad points
  double phys_weights_left[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e_ref_left->x1, e_ref_left->x2, 
                                 order_left, phys_x_left, phys_weights_left, 
                                 &pts_num_left); 

  // get fine mesh solution values and derivatives on 'e_ref_left'
  double phys_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_left->get_solution(phys_x_left, pts_num_left, phys_u_ref_left, 
                           phys_dudx_ref_left, y_prev_ref, 
                           bc_left_dir_values, bc_right_dir_values); 

  // get values of transformed Legendre polynomials in 'e_ref_left'
  double leg_pol_val_left[MAX_P+1][MAX_PTS_NUM];
  double leg_pol_der_left[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m<p_left + 1; m++) { // loop over transf. Leg. polynomials
    for(int j=0; j<pts_num_left; j++) {  
      leg_pol_val_left[m][j] = legendre(m, e_ref_left->x1, e_ref_left->x2, 
                                  phys_x_left[j]);
      if (norm == 1) leg_pol_der_left[m][j] = legendre_der(m, 
                                  e_ref_left->x1, e_ref_left->x2, 
                                  phys_x_left[j]);
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
          phys_u_ref_left[c][j] * leg_pol_val_left[m][j] * phys_weights_left[j];
      }
      //printf("proj_coeffs_left[%d][%d] = %g\n", c, m, proj_coeffs_left[c][m]);
    }
  }

  // evaluate the projection in 'e_ref_left' for every solution component
  // and every integration point
  double phys_u_left[MAX_EQN_NUM][MAX_PTS_NUM];
  double phys_dudx_left[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_left; j++) { // loop over integration points
      phys_u_left[c][j] = 0;
      for (int m=0; m<p_left+1; m++) { // loop over transf. Leg. polynomials
        phys_u_left[c][j] += 
          leg_pol_val_left[m][j] * proj_coeffs_left[c][m];
        if (norm == 1) {
          phys_dudx_left[c][j] += 
            leg_pol_der_left[m][j] * proj_coeffs_left[c][m];
        }
      }
    }
  }

  // calculate the error squared in L2 or H1 norm for every solution  
  // component in 'e_ref_left'
  double err_squared_left[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_left[c] = 0;
    for (int j=0; j<pts_num_left; j++) { // loop over integration points
      double diff_val = phys_u_ref_left[c][j] - phys_u_left[c][j];
      if (norm == 1) {
        double diff_der = phys_dudx_ref_left[c][j] - phys_dudx_left[c][j];
        err_squared_left[c] += (diff_val*diff_val +diff_der*diff_der) 
                                * phys_weights_left[j]; 
      }
      else err_squared_left[c] += diff_val * diff_val * phys_weights_left[j]; 
    }
  }

  // Second on 'e_ref_right': 
  // Calculate L2 or H1 projection of the reference solution on 
  // transformed Legendre polynomials of degree 'p_right'

  // create Gauss quadrature on 'e_ref_right'
  int order_right = 2*max(e_ref_right->p, p_right);
  int pts_num_right;
  double phys_x_right[MAX_PTS_NUM];          // quad points
  double phys_weights_right[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e_ref_right->x1, e_ref_right->x2, 
                                 order_right, phys_x_right, 
                                 phys_weights_right, &pts_num_right); 

  // get fine mesh solution values and derivatives on 'e_ref_right'
  double phys_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_right->get_solution(phys_x_right, pts_num_right, 
                            phys_u_ref_right, phys_dudx_ref_right, 
                            y_prev_ref, bc_left_dir_values, bc_right_dir_values); 

  // get values of transformed Legendre polynomials in 'e_ref_right'
  double leg_pol_val_right[MAX_P+1][MAX_PTS_NUM];
  double leg_pol_der_right[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m<p_right + 1; m++) { // loop over transf. Leg. polynomials
    for(int j=0; j<pts_num_right; j++) {
      leg_pol_val_right[m][j] = legendre(m, e_ref_right->x1, 
                            e_ref_right->x2, phys_x_right[j]);
      if (norm == 1) leg_pol_der_right[m][j] = legendre_der(m, e_ref_right->x1, 
                            e_ref_right->x2, phys_x_right[j]);
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
          phys_u_ref_right[c][j] * leg_pol_val_right[m][j] * phys_weights_right[j];
      }
      //printf("proj_coeffs_right[%d][%d] = %g\n", c, m, proj_coeffs_right[c][m]);
    }
  }

  // evaluate the projection in 'e_ref_right' for every solution component
  // and every integration point
  double phys_u_right[MAX_EQN_NUM][MAX_PTS_NUM];
  double phys_dudx_right[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_right; j++) { // loop over integration points
      phys_u_right[c][j] = 0;
      for (int m=0; m<p_right+1; m++) { // loop over transf. Leg. polynomials
        phys_u_right[c][j] += 
          leg_pol_val_right[m][j] * proj_coeffs_right[c][m];
        if (norm == 1) phys_dudx_right[c][j] += 
          leg_pol_der_right[m][j] * proj_coeffs_right[c][m];
      }
    }
  }

  // calculate the error squared in L2 or H1 norm for every solution  
  // component in 'e_ref_right'
  double err_squared_right[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_right[c] = 0;
    for (int j=0; j<pts_num_right; j++) { // loop over integration points
      double diff_val = phys_u_ref_right[c][j] - phys_u_right[c][j];
      if (norm == 1) {
        double diff_der = phys_dudx_ref_right[c][j] - phys_dudx_right[c][j];
        err_squared_right[c] += (diff_val*diff_val + diff_der*diff_der) 
                                 * phys_weights_right[j]; 
      }
      else err_squared_right[c] += diff_val * diff_val * phys_weights_right[j]; 
    }
  }

  /*
  printf("err_squared_left = %f\n", err_squared_left[0]);
  printf("err_squared_right = %f\n", err_squared_right[0]);
  */
  // summing errors on 'e_ref_left' and 'e_ref_right'
  double err_total = 0;
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_total += (err_squared_left[c] + err_squared_right[c]);
  }
  //if (err_total < 1e-10) {
  //    printf("candidate (1 %d %d)\n", p_left, p_right);
  //    warning("in check_cand_coarse_hp_fine_hp: bad refinement candidate (err_total=0)");
  //    return -1e6;
  //}
  err = sqrt(err_total);
  int dof_orig = e->p + 1;
  int dof_new = p_left + p_right + 1; 
  dof = dof_new - dof_orig; 

  // **************************************************************************
  // Debug - visualizing the reference solution and projection on the candidate
  // It might be a good idea to move this into a separate function later
  if (PLOT_CANDIDATE_PROJECTIONS) {
    static int visit = 0;
    visit++;
    int plot_pts_num = 51;
    // values of reference solution at plotting points left
    double plot_x_left[MAX_PTS_NUM];
    double h_left = (e_ref_left->x2 - e_ref_left->x1)/(plot_pts_num-1);
    for (int i=0; i < plot_pts_num; i++) {
      plot_x_left[i] = e_ref_left->x1 + i*h_left;
    }
    double plot_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], 
           plot_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
    e_ref_left->get_solution(plot_x_left, plot_pts_num, 
                             plot_u_ref_left, plot_dudx_ref_left, 
                             y_prev_ref, bc_left_dir_values, 
                             bc_right_dir_values); 
    // values of reference solution at plotting points right
    double plot_x_right[MAX_PTS_NUM];
    double h_right = (e_ref_right->x2 - e_ref_right->x1)/(plot_pts_num-1);
    for (int i=0; i < plot_pts_num; i++) {
      plot_x_right[i] = e_ref_right->x1 + i*h_right;
    }
    double plot_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], 
           plot_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
    e_ref_right->get_solution(plot_x_right, plot_pts_num, 
                              plot_u_ref_right, plot_dudx_ref_right, 
                              y_prev_ref, bc_left_dir_values, bc_right_dir_values); 
    // plotting the reference solution in the left and right halves
    char filename_refsol[255];
    sprintf(filename_refsol, "refsol_%g_%g_cand_%d_%d_fine_%d_%d_visit_%d.gp", 
	    e->x1, e->x2, p_left, p_right, e_ref_left->p, e_ref_right->p, visit);
    FILE *f_refsol = fopen(filename_refsol, "wb");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points left
      fprintf(f_refsol, "%g %g\n", plot_x_left[j], plot_u_ref_left[0][j]);
    }
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points right
      fprintf(f_refsol, "%g %g\n", plot_x_right[j], plot_u_ref_right[0][j]);
    }
    printf("Refsol (%g, %g) written to file %s\n", e->x1, e->x2, filename_refsol);
    fclose(f_refsol);
    // values of Legendre polynomials at plotting points left
    double plot_leg_pol_val_left[MAX_P+1][MAX_PTS_NUM];
    for(int m=0; m < p_left + 1; m++) { // loop over Leg. polynomials
      for(int j=0; j<plot_pts_num; j++) {  
        plot_leg_pol_val_left[m][j] = legendre(m, e_ref_left->x1, 
                            e_ref_left->x2, plot_x_left[j]);
      }
    }
    // values of projection at plotting points left
    double plot_u_left[MAX_EQN_NUM][MAX_PTS_NUM];
    for (int c=0; c<n_eq; c++) { // loop over solution components
      for (int j=0; j<plot_pts_num; j++) { // loop over plotting points left
        plot_u_left[c][j] = 0;
        for (int m=0; m < p_left + 1; m++) { // loop over Leg. polynomials
          plot_u_left[c][j] += 
            plot_leg_pol_val_left[m][j] * proj_coeffs_left[c][m];
        }
      }
    }
    // values of Legendre polynomials at plotting points right
    double plot_leg_pol_val_right[MAX_P+1][MAX_PTS_NUM];
    for(int m=0; m < p_right + 1; m++) { // loop over Leg. polynomials
      for(int j=0; j<plot_pts_num; j++) {  
        plot_leg_pol_val_right[m][j] = legendre(m, e_ref_right->x1, 
                            e_ref_right->x2, plot_x_right[j]);
      }
    }
    // values of projection at plotting points right
    double plot_u_right[MAX_EQN_NUM][MAX_PTS_NUM];
    for (int c=0; c<n_eq; c++) { // loop over solution components
      for (int j=0; j<plot_pts_num; j++) { // loop over plotting points right
        plot_u_right[c][j] = 0;
        for (int m=0; m < p_right + 1; m++) { // loop over Leg. polynomials
          plot_u_right[c][j] += 
            plot_leg_pol_val_right[m][j] * proj_coeffs_right[c][m];
        }
      }
    }
    // plotting the projection on the left and right halves
    char filename_cand[255];
    sprintf(filename_cand, "cand_%g_%g_cand_%d_%d_fine_%d_%d_visit_%d.gp", 
	    e->x1, e->x2, p_left, p_right, e_ref_left->p, e_ref_right->p, visit);
    FILE *f_cand = fopen(filename_cand, "wb");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points left
      fprintf(f_cand, "%g %g\n", plot_x_left[j], plot_u_left[0][j]);
    }
    fprintf(f_cand, "\n");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points right
      fprintf(f_cand, "%g %g\n", plot_x_right[j], plot_u_right[0][j]);
    }
    printf("Cand (%g, %g) written to file %s\n", e->x1, e->x2, filename_cand);
    fclose(f_cand);
  }
  // **************************************************************************
}

// Assumes that reference solution is defined on one single element 'e_ref' = 'e'. 
// The reference solution is projected onto the space of (discontinuous) 
// polynomials of degree 'p_left' on the left half of 'e' and degree 
// 'p_right' on the right half of 'e' 
double check_cand_coarse_hp_fine_p(int norm, Element *e, Element *e_ref,
                                   double *y_prev_ref, int p_left, int p_right,
                                   double bc_left_dir_values[MAX_EQN_NUM],
				   double bc_right_dir_values[MAX_EQN_NUM],
                                   double &err, int &dof)
{
  int n_eq = e->dof_size;

  // First in the left half of 'e': 
  // Calculate L2 or H1 projection of the reference solution on 
  // transformed Legendre polynomials of degree 'p_left'

  // create Gauss quadrature on the left half
  int order_left = 2*max(e_ref->p, p_left);
  int pts_num_left;
  double phys_x_left[MAX_PTS_NUM];          // quad points
  double phys_weights_left[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e->x1, (e->x1 + e->x2)/2., 
                                 order_left, phys_x_left, phys_weights_left, &pts_num_left); 

  // get fine mesh solution values and derivatives on the left half
  double phys_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref->get_solution(phys_x_left, pts_num_left, phys_u_ref_left, 
                      phys_dudx_ref_left, y_prev_ref, 
                      bc_left_dir_values, bc_right_dir_values); 

  // get values of transformed Legendre polynomials in the left half
  double leg_pol_val_left[MAX_P+1][MAX_PTS_NUM];
  double leg_pol_der_left[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m<p_left + 1; m++) { // loop over transf. Leg. polynomials
    for(int j=0; j<pts_num_left; j++) {  
      leg_pol_val_left[m][j] = legendre(m, e->x1, (e->x1 + e->x2)/2,  
					      phys_x_left[j]);
      if (norm == 1) leg_pol_der_left[m][j] = legendre_der(m, e->x1, (e->x1 + e->x2)/2,  
					      phys_x_left[j]);
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
          phys_u_ref_left[c][j] * leg_pol_val_left[m][j] * phys_weights_left[j];
      }
    }
  }

  // evaluate the projection on the left half for every solution component
  // and every integration point
  double phys_u_left[MAX_EQN_NUM][MAX_PTS_NUM];
  double phys_dudx_left[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_left; j++) { // loop over integration points
      phys_u_left[c][j] = 0;
      for (int m=0; m<p_left+1; m++) { // loop over transf. Leg. polynomials
        phys_u_left[c][j] += 
          leg_pol_val_left[m][j] * proj_coeffs_left[c][m];
        if (norm == 1) phys_dudx_left[c][j] += 
          leg_pol_der_left[m][j] * proj_coeffs_left[c][m];
      }
    }
  }

  // calculate the error squared in L2 or H1 norm for every solution  
  // component in the left half
  double err_squared_left[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_left[c] = 0;
    for (int j=0; j<pts_num_left; j++) { // loop over integration points
      double diff_val = phys_u_ref_left[c][j] - phys_u_left[c][j];
      if (norm == 1) {
        double diff_der = phys_dudx_ref_left[c][j] - phys_dudx_left[c][j];
        err_squared_left[c] += (diff_val*diff_val + diff_der*diff_der) 
                                * phys_weights_left[j]; 
      }
      else err_squared_left[c] += diff_val * diff_val * phys_weights_left[j]; 
    }
  }

  // Second on the right half: 
  // Calculate L2 or H1 projection of the reference solution on 
  // transformed Legendre polynomials of degree 'p_right'

  // create Gauss quadrature on the right half
  int order_right = 2*max(e_ref->p, p_right);
  int pts_num_right;
  double phys_x_right[MAX_PTS_NUM];          // quad points
  double phys_weights_right[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature((e->x1 + e->x2)/2., e->x2, 
                                 order_right, phys_x_right, 
                                 phys_weights_right, &pts_num_right); 

  // get fine mesh solution values and derivatives on the right half
  double phys_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref->get_solution(phys_x_right, pts_num_right, phys_u_ref_right, phys_dudx_ref_right, 
                  y_prev_ref, bc_left_dir_values, bc_right_dir_values); 

  // get values of transformed Legendre polynomials on the right half
  double leg_pol_val_right[MAX_P+1][MAX_PTS_NUM];
  double leg_pol_der_right[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m < p_right + 1; m++) { // loop over transf. Leg. polynomials
    for(int j=0; j < pts_num_right; j++) {
      leg_pol_val_right[m][j] = legendre(m, (e->x1 + e->x2)/2, e->x2,  
					     phys_x_right[j]);
      if (norm == 1) leg_pol_val_right[m][j] = legendre_der(m, 
                                             (e->x1 + e->x2)/2, e->x2,  
					     phys_x_right[j]);
    }
  }

  // calculate the projection coefficients for every 
  // transformed Legendre polynomial and every solution 
  // component. Since the basis is orthonormal, these 
  // are just integrals of the fine mesh solution with 
  // the transformed Legendre polynomials
  double proj_coeffs_right[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m < p_right + 1; m++) { // loop over transf. Leg. polynomials
    for(int c=0; c < n_eq; c++) { // loop over solution components
      proj_coeffs_right[c][m] = 0;
      for(int j=0; j < pts_num_right; j++) { // loop over integration points
        proj_coeffs_right[c][m] += 
          phys_u_ref_right[c][j] * leg_pol_val_right[m][j] * phys_weights_right[j];
      }
    }
  }

  // evaluate the projection on the right half for every solution component
  // and every integration point
  double phys_u_right[MAX_EQN_NUM][MAX_PTS_NUM];
  double phys_dudx_right[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c < n_eq; c++) { // loop over solution components
    for (int j=0; j < pts_num_right; j++) { // loop over integration points
      phys_u_right[c][j] = 0;
      for (int m=0; m < p_right+1; m++) { // loop over transf. Leg. polynomials
        phys_u_right[c][j] += 
          leg_pol_val_right[m][j] * proj_coeffs_right[c][m];
        if (norm == 1) phys_dudx_right[c][j] += 
          leg_pol_der_right[m][j] * proj_coeffs_right[c][m];
      }
    }
  }

  // calculate the error squared in L2 or H1 norm for every solution  
  // component on the right half
  double err_squared_right[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_right[c] = 0;
    for (int j=0; j<pts_num_right; j++) { // loop over integration points
      double diff_val = phys_u_ref_right[c][j] - phys_u_right[c][j];
      if (norm == 1) {
        double diff_der = phys_dudx_ref_right[c][j] - phys_dudx_right[c][j];
        err_squared_right[c] += (diff_val*diff_val + diff_der*diff_der) 
                                 * phys_weights_right[j]; 
      }
      else err_squared_right[c] += diff_val * diff_val * phys_weights_right[j]; 
    }
  }

  // summing errors on the two halves
  double err_total = 0;
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_total += (err_squared_left[c] + err_squared_right[c]);
  }
  //if (err_total < 1e-10) {
  //    printf("candidate (1 %d %d)\n", p_left, p_right);
  //    warning("in check_cand_coarse_hp_fine_p: bad refinement candidate (err_total=0)");
  //    return -1e6;
  //}
  err = sqrt(err_total);
  int dof_orig = e->p + 1;
  int dof_new = p_left + p_right + 1; 
  dof = dof_new - dof_orig; 

  // **************************************************************************
  // Debug - visualizing the reference solution and projection on the candidate
  // It might be a good idea to move this into a separate function later
  if (PLOT_CANDIDATE_PROJECTIONS) {
    static int visit = 0;
    visit++;
    int plot_pts_num = 51;
    // values of reference solution at plotting points left
    double plot_x_left[MAX_PTS_NUM];
    double h_left = ((e->x2 - e->x1)/2)/(plot_pts_num-1);
    for (int i=0; i < plot_pts_num; i++) {
      plot_x_left[i] = e->x1 + i*h_left;
    }
    double plot_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], 
           plot_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
    e_ref->get_solution(plot_x_left, plot_pts_num, plot_u_ref_left, plot_dudx_ref_left, 
                             y_prev_ref, bc_left_dir_values, bc_right_dir_values); 
    // values of reference solution at plotting points right
    double plot_x_right[MAX_PTS_NUM];
    double h_right = ((e->x2 - e->x1)/2)/(plot_pts_num-1);
    for (int i=0; i < plot_pts_num; i++) {
      plot_x_right[i] = (e->x1 + e->x2)/2 + i*h_right;
    }
    double plot_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], 
           plot_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
    e_ref->get_solution(plot_x_right, plot_pts_num, 
                              plot_u_ref_right, plot_dudx_ref_right, 
                              y_prev_ref, bc_left_dir_values, bc_right_dir_values); 
    // plotting the reference solution in the left and right halves
    char filename_refsol[255];
    sprintf(filename_refsol, "refsol_%g_%g_cand_%d_%d_fine_%d_visit_%d.gp", 
	    e->x1, e->x2, p_left, p_right, e_ref->p, visit);
    FILE *f_refsol = fopen(filename_refsol, "wb");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points left
      fprintf(f_refsol, "%g %g\n", plot_x_left[j], plot_u_ref_left[0][j]);
    }
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points right
      fprintf(f_refsol, "%g %g\n", plot_x_right[j], plot_u_ref_right[0][j]);
    }
    printf("Refsol (%g, %g) written to file %s\n", e->x1, e->x2, filename_refsol);
    fclose(f_refsol);
    // values of Legendre polynomials at plotting points left
    double plot_leg_pol_val_left[MAX_P+1][MAX_PTS_NUM];
    for(int m=0; m < p_left + 1; m++) { // loop over Leg. polynomials
      for(int j=0; j<plot_pts_num; j++) {
        plot_leg_pol_val_left[m][j] = legendre(m, e->x1, (e->x1 + e->x2)/2,  
					      plot_x_left[j]);
      }
    }
    // values of projection at plotting points left
    double plot_u_left[MAX_EQN_NUM][MAX_PTS_NUM];
    for (int c=0; c<n_eq; c++) { // loop over solution components
      for (int j=0; j<plot_pts_num; j++) { // loop over plotting points left
        plot_u_left[c][j] = 0;
        for (int m=0; m < p_left + 1; m++) { // loop over Leg. polynomials
          plot_u_left[c][j] += 
            plot_leg_pol_val_left[m][j] * proj_coeffs_left[c][m];
        }
      }
    }
    // values of Legendre polynomials at plotting points right
    double plot_leg_pol_val_right[MAX_P+1][MAX_PTS_NUM];
    for(int m=0; m < p_right + 1; m++) { // loop over Leg. polynomials
      for(int j=0; j<plot_pts_num; j++) {  
        plot_leg_pol_val_left[m][j] = legendre(m, (e->x1 + e->x2)/2, e->x2, 
					      plot_x_right[j]);
      }
    }
    // values of projection at plotting points right
    double plot_u_right[MAX_EQN_NUM][MAX_PTS_NUM];
    for (int c=0; c<n_eq; c++) { // loop over solution components
      for (int j=0; j<plot_pts_num; j++) { // loop over plotting points right
        plot_u_right[c][j] = 0;
        for (int m=0; m < p_right + 1; m++) { // loop over Leg. polynomials
          plot_u_right[c][j] += 
            plot_leg_pol_val_right[m][j] * proj_coeffs_right[c][m];
        }
      }
    }
    // plotting the projection on the left and right halves
    char filename_cand[255];
    sprintf(filename_cand, "cand_%g_%g_cand_%d_%d_fine_%d_visit_%d.gp", 
	    e->x1, e->x2, p_left, p_right, e_ref->p, visit);
    FILE *f_cand = fopen(filename_cand, "wb");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points left
      fprintf(f_cand, "%g %g\n", plot_x_left[j], plot_u_left[0][j]);
    }
    fprintf(f_cand, "\n");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points right
      fprintf(f_cand, "%g %g\n", plot_x_right[j], plot_u_right[0][j]);
    }
    printf("Cand (%g, %g) written to file %s\n", e->x1, e->x2, filename_cand);
    fclose(f_cand);
  }
  // **************************************************************************
}

// Assumes that reference solution is defined on two half-elements 'e_ref_left'
// and 'e_ref_right'. The reference solution is projected onto the space of 
// polynomials of degree 'p' on 'e'
double check_cand_coarse_p_fine_hp(int norm, Element *e, Element *e_ref_left, 
                                   Element *e_ref_right, 
                                   double *y_prev_ref, int p,
                                   double bc_left_dir_values[MAX_EQN_NUM],
				   double bc_right_dir_values[MAX_EQN_NUM],
                                   double &err, int &dof)
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
  double phys_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_left->get_solution(phys_x_left, pts_num_left, phys_u_ref_left, phys_dudx_ref_left, 
                           y_prev_ref, 
                           bc_left_dir_values, bc_right_dir_values); 

  // get values of (original) Legendre polynomials in 'e_ref_left'
  double leg_pol_val_left[MAX_P+1][MAX_PTS_NUM];
  double leg_pol_der_left[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
    for(int j=0; j<pts_num_left; j++) {  
      leg_pol_val_left[m][j] = legendre(m, e->x1, 
					   e->x2, phys_x_left[j]);
      if (norm == 1) leg_pol_der_left[m][j] = legendre_der(m, e->x1, 
					      e->x2, phys_x_left[j]);
    }
  }

  // calculate first part of the projection coefficients 
  double proj_coeffs_left[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m<p + 1; m++) { // loop over Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs_left[c][m] = 0;
      for(int j=0; j<pts_num_left; j++) { // loop over integration points
        proj_coeffs_left[c][m] += 
          phys_u_ref_left[c][j] * leg_pol_val_left[m][j] * phys_weights_left[j];
      }
      //printf("proj_coeffs_left[%d][%d] = %g\n", c, m, proj_coeffs_left[c][m]);
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
                                 order_right, phys_x_right, 
                                 phys_weights_right, &pts_num_right); 

  // get fine mesh solution values and derivatives on 'e_ref_right'
  double phys_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
  e_ref_right->get_solution(phys_x_right, pts_num_right, 
                            phys_u_ref_right, phys_dudx_ref_right, 
                            y_prev_ref, bc_left_dir_values, bc_right_dir_values); 

  // get values of (original) Legendre polynomials in 'e_ref_right'
  double leg_pol_val_right[MAX_P+1][MAX_PTS_NUM];
  double leg_pol_der_right[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
    for(int j=0; j < pts_num_right; j++) {
      leg_pol_val_right[m][j] = legendre(m, e->x1, 
					    e->x2, phys_x_right[j]);
      if (norm == 1) leg_pol_der_right[m][j] = legendre_der(m, e->x1, 
					       e->x2, phys_x_right[j]);
    }
  }

  // calculate the second part of the projection coefficients
  double proj_coeffs_right[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs_right[c][m] = 0;
      for(int j=0; j<pts_num_right; j++) { // loop over integration points
        proj_coeffs_right[c][m] += 
          phys_u_ref_right[c][j] * leg_pol_val_right[m][j] * phys_weights_right[j];
      }
      //printf("proj_coeffs_right[%d][%d] = %g\n", c, m, proj_coeffs_right[c][m]);
    }
  }

  // debug
  //printf("p_fine_hp: e = (%g, %g)\n", e->x1, e->x2);

  // add the two parts of projection coefficients
  double proj_coeffs[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
    for(int c=0; c<n_eq; c++) { // loop over solution components
      proj_coeffs[c][m] = proj_coeffs_left[c][m] + proj_coeffs_right[c][m];
      //printf("  proj_coeffs[%d][%d] = %g\n", c, m, proj_coeffs[c][m]);
    }
  }

  // evaluate the projection in 'e_ref_left' for every solution component
  // and every integration point
  double phys_u_left[MAX_EQN_NUM][MAX_PTS_NUM];
  double phys_dudx_left[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_left; j++) { // loop over integration points left
      phys_u_left[c][j] = 0;
      for (int m=0; m < p+1; m++) { // loop over Leg. polynomials
        phys_u_left[c][j] += 
          leg_pol_val_left[m][j] * proj_coeffs[c][m];
        if (norm == 1) phys_dudx_left[c][j] += 
          leg_pol_der_left[m][j] * proj_coeffs[c][m];
      }
    }
  }

  // evaluate the projection in 'e_ref_right' for every solution component
  // and every integration point
  double phys_u_right[MAX_EQN_NUM][MAX_PTS_NUM];
  double phys_dudx_right[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    for (int j=0; j<pts_num_right; j++) { // loop over integration points right
      phys_u_right[c][j] = 0;
      for (int m=0; m < p+1; m++) { // loop over Leg. polynomials
        phys_u_right[c][j] += 
          leg_pol_val_right[m][j] * proj_coeffs[c][m];
        if (norm == 1) phys_dudx_right[c][j] += 
          leg_pol_der_right[m][j] * proj_coeffs[c][m];
      }
    }
  }

  // calculate the error squared in L2 or H1 norm for every solution  
  // component in 'e_ref_left'
  double err_squared_left[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_left[c] = 0;
    for (int j=0; j<pts_num_left; j++) { // loop over integration points left
      double diff_val = phys_u_ref_left[c][j] - phys_u_left[c][j];
      if (norm == 1) {
        double diff_der = phys_dudx_ref_left[c][j] - phys_dudx_left[c][j];
        err_squared_left[c] += (diff_val*diff_val + diff_der*diff_der)
                                * phys_weights_left[j]; 
      }
      else err_squared_left[c] += diff_val * diff_val * phys_weights_left[j]; 
    }
  }

  // calculate the error squared in L2 or H1 norm for every solution  
  // component in 'e_ref_right'
  double err_squared_right[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared_right[c] = 0;
    for (int j=0; j<pts_num_right; j++) { // loop over integration points left
      double diff_val = phys_u_ref_right[c][j] - phys_u_right[c][j];
      if (norm == 1) {
        double diff_der = phys_dudx_ref_right[c][j] - phys_dudx_right[c][j];
        err_squared_right[c] += (diff_val*diff_val + diff_der*diff_der)
                                 * phys_weights_right[j]; 
      }
      else err_squared_right[c] += diff_val * diff_val * phys_weights_right[j]; 
    }
  }

  // summing errors over components
  double err_total = 0;
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_total += (err_squared_left[c] + err_squared_right[c]);
  }
  //if (err_total < 1e-10) {
  //    printf("candidate (0 %d -1)\n", p);
  //    warning("in check_cand_coarse_p_fine_hp: bad refinement candidate (err_total=0)");
  //    return -1e6;
  //}
  err = sqrt(err_total);
  int dof_orig = e->p + 1;
  int dof_new = p + 1; 
  dof = dof_new - dof_orig; 

  // **************************************************************************
  // Debug - visualizing the reference solution and projection on the candidate
  // It might be a good idea to move this into a separate function later
  if (PLOT_CANDIDATE_PROJECTIONS) {
    static int visit = 0;
    visit++;
    int plot_pts_num = 51;
    // values of reference solution at plotting points left
    double plot_x_left[MAX_PTS_NUM];
    double h_left = (e_ref_left->x2 - e_ref_left->x1)/(plot_pts_num-1);
    for (int i=0; i < plot_pts_num; i++) {
      plot_x_left[i] = e_ref_left->x1 + i*h_left;
    }
    double plot_u_ref_left[MAX_EQN_NUM][MAX_PTS_NUM], 
           plot_dudx_ref_left[MAX_EQN_NUM][MAX_PTS_NUM];
    e_ref_left->get_solution(plot_x_left, plot_pts_num, plot_u_ref_left, plot_dudx_ref_left, 
                             y_prev_ref, bc_left_dir_values, bc_right_dir_values); 
    // values of reference solution at plotting points right
    double plot_x_right[MAX_PTS_NUM];
    double h_right = (e_ref_right->x2 - e_ref_right->x1)/(plot_pts_num-1);
    for (int i=0; i < plot_pts_num; i++) {
      plot_x_right[i] = e_ref_right->x1 + i*h_right;
    }
    double plot_u_ref_right[MAX_EQN_NUM][MAX_PTS_NUM], 
           plot_dudx_ref_right[MAX_EQN_NUM][MAX_PTS_NUM];
    e_ref_right->get_solution(plot_x_right, plot_pts_num, 
                              plot_u_ref_right, plot_dudx_ref_right, 
                              y_prev_ref, bc_left_dir_values, bc_right_dir_values); 
    // plotting the reference solution in the left and right halves
    char filename_refsol[255];
    sprintf(filename_refsol, "refsol_%g_%g_cand_%d_fine_%d_%d_visit_%d.gp", 
	    e->x1, e->x2, p, e_ref_left->p, e_ref_right->p, visit);
    FILE *f_refsol = fopen(filename_refsol, "wb");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points left
      fprintf(f_refsol, "%g %g\n", plot_x_left[j], plot_u_ref_left[0][j]);
    }
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points right
      fprintf(f_refsol, "%g %g\n", plot_x_right[j], plot_u_ref_right[0][j]);
    }
    printf("Refsol (%g, %g) written to file %s\n", e->x1, e->x2, filename_refsol);
    fclose(f_refsol);
    // values of Legendre polynomials at plotting points left
    double plot_leg_pol_val_left[MAX_P+1][MAX_PTS_NUM];
    for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
      for(int j=0; j < plot_pts_num; j++) {  
        plot_leg_pol_val_left[m][j] = legendre(m, e->x1, 
	  				   e->x2, plot_x_left[j]);
      }
    }
    // values of projection at plotting points left
    double plot_u_left[MAX_EQN_NUM][MAX_PTS_NUM];
    for (int c=0; c < n_eq; c++) { // loop over solution components
      for (int j=0; j < plot_pts_num; j++) { // loop over plotting points left
        plot_u_left[c][j] = 0;
        for (int m=0; m < p+1; m++) { // loop over Leg. polynomials
          plot_u_left[c][j] += 
            plot_leg_pol_val_left[m][j] * proj_coeffs[c][m];
        }
      }
    }
    // values of Legendre polynomials at plotting points right
    double plot_leg_pol_val_right[MAX_P+1][MAX_PTS_NUM];
    for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
      for(int j=0; j < plot_pts_num; j++) {  
        plot_leg_pol_val_right[m][j] = legendre(m, e->x1, 
  					   e->x2, plot_x_right[j]);
      }
    }
    // values of projection at plotting points right
    double plot_u_right[MAX_EQN_NUM][MAX_PTS_NUM];
    for (int c=0; c < n_eq; c++) { // loop over solution components
      for (int j=0; j < plot_pts_num; j++) { // loop over plotting points left
        plot_u_right[c][j] = 0;
        for (int m=0; m < p+1; m++) { // loop over Leg. polynomials
          plot_u_right[c][j] += 
            plot_leg_pol_val_right[m][j] * proj_coeffs[c][m];
        }
      }
    }
    // plotting the projection on the left and right halves
    char filename_cand[255];
    sprintf(filename_cand, "cand_%g_%g_cand_%d_fine_%d_%d_visit_%d.gp", 
	    e->x1, e->x2, p, e_ref_left->p, e_ref_right->p, visit);
    FILE *f_cand = fopen(filename_cand, "wb");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points left
      fprintf(f_cand, "%g %g\n", plot_x_left[j], plot_u_left[0][j]);
    }
    fprintf(f_cand, "\n");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points right
      fprintf(f_cand, "%g %g\n", plot_x_right[j], plot_u_right[0][j]);
    }
    printf("Cand (%g, %g) written to file %s\n", e->x1, e->x2, filename_cand);
    fclose(f_cand);
  }
  // **************************************************************************
}

// Assumes that reference solution is defined on one single element 
// 'e_ref' (reference refinement did not split the element in space). 
// The reference solution is projected onto the space of 
// polynomials of degree 'p' on 'e'
double check_cand_coarse_p_fine_p(int norm, Element *e, Element *e_ref,
                                  double *y_prev_ref, int p,
                                  double bc_left_dir_values[MAX_EQN_NUM],
				  double bc_right_dir_values[MAX_EQN_NUM],
                                  double &err, int &dof)
{
  int n_eq = e->dof_size;

  // Calculate L2 or H1 projection of the reference solution on 
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
  double leg_pol_val[MAX_P+1][MAX_PTS_NUM];
  double leg_pol_der[MAX_P+1][MAX_PTS_NUM];
  for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
    for(int j=0; j < pts_num; j++) {  
      leg_pol_val[m][j] = legendre(m, e->x1, e->x2, phys_x[j]);
      if (norm == 1) leg_pol_der[m][j] = legendre_der(m, e->x1, e->x2, phys_x[j]);
    }
  }

  // calculate the projection coefficients 
  double proj_coeffs[MAX_EQN_NUM][MAX_P+1];
  for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
    for(int c=0; c < n_eq; c++) { // loop over solution components
      proj_coeffs[c][m] = 0;
      for(int j=0; j < pts_num; j++) { // loop over integration points
        proj_coeffs[c][m] +=
          phys_u_ref[c][j] * leg_pol_val[m][j] * phys_weights[j];
      }
    }
  }

  // evaluate the projection in 'e' for every solution component
  // and every integration point
  double phys_u[MAX_EQN_NUM][MAX_PTS_NUM];
  double phys_dudx[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int c=0; c < n_eq; c++) { // loop over solution components
    for (int j=0; j < pts_num; j++) { // loop over integration points
      phys_u[c][j] = 0;
      for (int m=0; m < p + 1; m++) { // loop over Leg. polynomials
        phys_u[c][j] += 
          leg_pol_val[m][j] * proj_coeffs[c][m];
        if (norm == 1) phys_dudx[c][j] += 
          leg_pol_der[m][j] * proj_coeffs[c][m];
      }
    }
  }

  // calculate the error squared in L2 or H1 norm for every solution  
  // component in 'e'
  double err_squared[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_squared[c] = 0;
    for (int j=0; j < pts_num; j++) { // loop over integration points
      double diff_val = phys_u_ref[c][j] - phys_u[c][j];
      if (norm == 1) {
        double diff_der = phys_dudx_ref[c][j] - phys_dudx[c][j];
        err_squared[c] += (diff_val*diff_val + diff_der*diff_der)
                           * phys_weights[j];
      }
      else err_squared[c] += diff_val * diff_val * phys_weights[j]; 
    }
  }

  // summing errors over components
  double err_total = 0;
  for (int c=0; c<n_eq; c++) { // loop over solution components
    err_total += err_squared[c];
  }
  //if (err_total < 1e-10) {
  //    printf("candidate (0 %d -1)\n", p);
  //    warning("in check_cand_coarse_p_fine_p: bad refinement candidate (err_total=0)");
  //    return -1e6;
  //}
  err = sqrt(err_total);
  int dof_orig = e->p + 1;
  int dof_new = p + 1; 
  dof = dof_new - dof_orig; 

  // **************************************************************************
  // Debug - visualizing the reference solution and projection on the candidate
  // It might be a good idea to move this into a separate function later
  if (PLOT_CANDIDATE_PROJECTIONS) {
    static int visit = 0;
    visit++;
    int plot_pts_num = 51;
    // values of reference solution at plotting points
    double plot_x[MAX_PTS_NUM];
    double h = (e->x2 - e->x1)/(plot_pts_num-1);
    for (int i=0; i < plot_pts_num; i++) {
      plot_x[i] = e->x1 + i*h;
    }
    double plot_u_ref[MAX_EQN_NUM][MAX_PTS_NUM], 
           plot_dudx_ref[MAX_EQN_NUM][MAX_PTS_NUM];
    e_ref->get_solution(plot_x, plot_pts_num, plot_u_ref, plot_dudx_ref, 
                             y_prev_ref, bc_left_dir_values, bc_right_dir_values); 
    // plotting the reference solution
    char filename_refsol[255];
    sprintf(filename_refsol, "refsol_%g_%g_cand_%d_fine_%d_visit_%d.gp", 
	    e->x1, e->x2, p, e_ref->p, visit);
    FILE *f_refsol = fopen(filename_refsol, "wb");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting points
      fprintf(f_refsol, "%g %g\n", plot_x[j], plot_u_ref[0][j]);
    }
    printf("Refsol (%g, %g) written to file %s\n", e->x1, e->x2, filename_refsol);
    fclose(f_refsol);
    // values of Legendre polynomials at plotting points
    double plot_leg_pol_val[MAX_P+1][MAX_PTS_NUM];
    for(int m=0; m < p + 1; m++) { // loop over Leg. polynomials
      for(int j=0; j < plot_pts_num; j++) {  
        plot_leg_pol_val[m][j] = legendre(m, e->x1, 
	  				   e->x2, plot_x[j]);
      }
    }
    // values of projection at plotting points
    double plot_u[MAX_EQN_NUM][MAX_PTS_NUM];
    for (int c=0; c < n_eq; c++) { // loop over solution components
      for (int j=0; j < plot_pts_num; j++) { // loop over plotting points
        plot_u[c][j] = 0;
        for (int m=0; m < p+1; m++) { // loop over Leg. polynomials
          plot_u[c][j] += 
            plot_leg_pol_val[m][j] * proj_coeffs[c][m];
        }
      }
    }
    // plotting the projection
    char filename_cand[255];
    sprintf(filename_cand, "cand_%g_%g_cand_%d_fine_%d_visit_%d.gp", 
	    e->x1, e->x2, p, e_ref->p, visit);
    FILE *f_cand = fopen(filename_cand, "wb");
    for (int j=0; j<plot_pts_num; j++) { // loop over plotting
      fprintf(f_cand, "%g %g\n", plot_x[j], plot_u[0][j]);
    }
    printf("Cand (%g, %g) written to file %s\n", e->x1, e->x2, filename_cand);
    fclose(f_cand);
  }
  // **************************************************************************
}

double calc_exact_sol_norm(int norm, exact_sol_type exact_sol, 
                           int n_eq, 
                           double A, double B, int subdivision, 
                           int order)
{
  double norm_squared = 0;
  double h = (B - A)/subdivision;
  // loop over intervals of subdivision
  for (int i=0; i < subdivision; i++) {
    double a0 = A + i*h;
    double b0 = a0 + h;
    int pts_num;
    double x_phys[MAX_PTS_NUM];
    double w_phys[MAX_PTS_NUM];
    create_phys_element_quadrature(a0, b0, order, x_phys, 
                                   w_phys, &pts_num);
    double val = 0;
    // integration over an interval
    for (int j=0; j < pts_num; j++) {
      double fn_val[MAX_EQN_NUM];
      double fn_der[MAX_EQN_NUM];
      exact_sol(x_phys[j], fn_val, fn_der);
      for (int c=0; c<n_eq; c++) {
	if (norm == 1) {
          val += (fn_val[c]*fn_val[c] + fn_der[c]*fn_der[c]) * w_phys[j];
        }
        else val += fn_val[c] * fn_val[c] * w_phys[j];
      }
    }
    norm_squared += val;
  }

  return sqrt(norm_squared);
}

double calc_elem_exact_error_squared(int norm, exact_sol_type exact_sol,
                                     Element *e, double *y_prev, 
                                     double *bc_left_dir_values,
			             double *bc_right_dir_values,
                                     int order)
{
  // create Gauss quadrature on 'e'
  int pts_num;
  double phys_x[MAX_PTS_NUM];          // quad points
  double phys_weights[MAX_PTS_NUM];    // quad weights
  create_phys_element_quadrature(e->x1, e->x2, order, phys_x, 
                                 phys_weights, &pts_num); 

  // get coarse mesh solution values and derivatives
  double phys_u[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx[MAX_EQN_NUM][MAX_PTS_NUM];
  e->get_solution(phys_x, pts_num, phys_u, phys_dudx, y_prev, 
                  bc_left_dir_values, bc_right_dir_values); 

  // get exact solution values and derivatives
  int n_eq = e->dof_size;
  double phys_u_exact[MAX_EQN_NUM][MAX_PTS_NUM], 
         phys_dudx_exact[MAX_EQN_NUM][MAX_PTS_NUM];
  for (int j=0; j < pts_num; j++) {
    double phys_u_exact_pt[MAX_EQN_NUM], 
           phys_dudx_exact_pt[MAX_EQN_NUM]; 
    exact_sol(phys_x[j], phys_u_exact_pt, phys_dudx_exact_pt); 
    for (int c=0; c < n_eq; c++) {
      phys_u_exact[c][j] = phys_u_exact_pt[c];
      phys_dudx_exact[c][j] = phys_dudx_exact_pt[c];
    }
  }

  // integrate over 'e'
  double norm_squared[MAX_EQN_NUM];
  for (int c=0; c<n_eq; c++) {
    norm_squared[c] = 0;
    for (int i=0; i<pts_num; i++) {
      double diff_val = phys_u_exact[c][i] - phys_u[c][i];
      if (norm == 1) {
        double diff_der = phys_dudx_exact[c][i] - phys_dudx[c][i];
        norm_squared[c] += (diff_val*diff_val + diff_der*diff_der)
                            * phys_weights[i];
      }
      else norm_squared[c] += diff_val * diff_val * phys_weights[i];
    }
  }

  double err_squared = 0;
  for (int c=0; c < n_eq; c++)  
    err_squared += norm_squared[c];

  return err_squared;
}

double calc_exact_sol_error(int norm, Mesh *mesh, double *y_prev, 
                            exact_sol_type exact_sol, 
                            int order) 
{
  double total_err_squared = 0;
  Iterator *I = new Iterator(mesh);
  Element *e;
  while ((e = I->next_active_element()) != NULL) {
      double elem_err_squared = 
        calc_elem_exact_error_squared(norm, exact_sol, e, y_prev, 
                                      mesh->bc_left_dir_values,
			              mesh->bc_right_dir_values,
                                      order);
      total_err_squared += elem_err_squared;
  }
  
  return sqrt(total_err_squared);
}

int select_hp_refinement_ref_p(int norm, int num_cand, int3 *cand_list, 
                               Element *e, Element *e_ref, 
                               double *y_prev_ref, double *bc_left_dir_values,
			       double *bc_right_dir_values) 
{
  int choice = -1;
  double crit_max = -1e10;
  double crit;
  for (int i=0; i<num_cand; i++) {
    if (cand_list[i][0] == 0) { // p-refinement
      int p_new = cand_list[i][1];
      double err;
      int dof;
      check_cand_coarse_p_fine_p(norm, e, e_ref, y_prev_ref, p_new,
                                 bc_left_dir_values,
				 bc_right_dir_values, err, dof);
      // NOTE: this may need some experimantation
      double error_scaled = -log(err) / dof; 

    }
    else {                      // hp-refinement
      double err;
      int dof;
      int p_new_left = cand_list[i][1];
      int p_new_right = cand_list[i][2];
      check_cand_coarse_hp_fine_p(norm, e, e_ref, 
                                  y_prev_ref, p_new_left, p_new_right, 
                                  bc_left_dir_values,
				  bc_right_dir_values, err, dof);
      // NOTE: this may need some experimantation
      double error_scaled = -log(err) / dof; 
    }

    // debug
    if (PRINT_CANDIDATES) {
      printf("  Elem (%g, %g): cand (%d %d %d), crit = %g\n", e->x1, e->x2, 
             cand_list[i][0], cand_list[i][1], cand_list[i][2], crit);
    }

    if (crit > crit_max) {
      crit_max = crit;
      choice = i;
    }
  }

  if (choice == -1) error("Candidate not found in select_hp_refinement_ref_p().");

  // debug
  if (PRINT_CANDIDATES) {
    printf("  Elem (%g, %g): choice = %d\n", e->x1, e->x2, choice);
  }
  return choice;
}

int select_hp_refinement_ref_hp(int norm,  int num_cand, int3 *cand_list, 
                                Element *e, Element *e_ref_left, 
                                Element *e_ref_right, double *y_prev_ref, 
                                double *bc_left_dir_values,
			        double *bc_right_dir_values) 
{
  int choice = -1;
  double crit_max = -1e10;
  double crit;
  for (int i=0; i<num_cand; i++) {
    if (cand_list[i][0] == 0) { // p-refinement
      double err;
      int dof;
      int p_new = cand_list[i][1];
      check_cand_coarse_p_fine_hp(norm, e, e_ref_left, e_ref_right, 
                                  y_prev_ref, p_new,
                                  bc_left_dir_values,
				  bc_right_dir_values, err, dof);
      // NOTE: this may need some experimantation
      double error_scaled = -log(err) / dof; 
    }
    else {                      // hp-refinement
      double err;
      int dof;
      int p_new_left = cand_list[i][1];
      int p_new_right = cand_list[i][2];
      check_cand_coarse_hp_fine_hp(norm, e, e_ref_left, e_ref_right, 
                                   y_prev_ref, p_new_left, p_new_right, 
                                   bc_left_dir_values,
				   bc_right_dir_values, err, dof);
      // NOTE: this may need some experimantation
      double error_scaled = -log(err) / dof; 
    }

    // debug
    if (PRINT_CANDIDATES) {
      printf("  Elem (%g, %g): ref hp, cand (%d %d %d), crit = %g\n", e->x1, e->x2, 
             cand_list[i][0], cand_list[i][1], cand_list[i][2], crit);
    }

    if (crit > crit_max) {
      crit_max = crit;
      choice = i;
    }
  }

  if (choice == -1) error("Candidate not found in select_hp_refinement_ref_hp().");

  // debug
  if (PRINT_CANDIDATES) {
    printf("  Elem (%g, %g): choice = %d\n", e->x1, e->x2, choice);
  }

  return choice;
}


