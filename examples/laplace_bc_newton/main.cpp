#include "hermes1d.h"
#include "solver_umfpack.h"

// ********************************************************************

// This example solves the Poisson equation -u'' - f = 0 in
// an interval (A, B), equipped with a Dirichlet boundary
// condition on the left and a Newton BC "u + \alpha dudn = \beta" 
// on the right. 

// General input:
static int N_eq = 1;
int N_elem = 3;                        // number of elements
double A = 0, B = 2*M_PI;              // domain end points
int P_init = 3;                        // initial polynomal degree

// Boundary conditions
double Val_dir_left = 0;
double Val_newton_alpha = 1;   // must be nonzero
double Val_newton_beta = 1;

// Matrix solver
const int MATRIX_SOLVER = 1;            // 0... default (LU decomposition)
                                        // 1... UMFPACK
                                        // 2... CG (no preconditioning)
                                        // Only relevant for iterative matrix solvers:
const double MATRIX_SOLVER_TOL = 1e-7;  // Tolerance for residual in L2 norm
const int MATRIX_SOLVER_MAXITER = 150;  // Max. number of iterations

// Tolerance for the Newton's method
double TOL_NEWTON = 1e-5;

// Function f(x)
double f(double x) {
  return sin(x);
  //return 1;
}

// Weak forms for Jacobi matrix and residual
#include "forms.cpp"

/******************************************************************************/
int main() {
  // Create coarse mesh, set Dirichlet BC, enumerate 
  // basis functions
  Mesh *mesh = new Mesh(A, B, N_elem, P_init, N_eq);
  mesh->set_bc_left_dirichlet(0, Val_dir_left);
  printf("N_dof = %d\n", mesh->assign_dofs());

  // Register weak forms
  DiscreteProblem *dp = new DiscreteProblem();
  dp->add_matrix_form(0, 0, jacobian_vol);
  dp->add_vector_form(0, residual_vol);
  dp->add_matrix_form_surf(0, 0, jacobian_surf_right, BOUNDARY_RIGHT);
  dp->add_vector_form_surf(0, residual_surf_right, BOUNDARY_RIGHT);

  // Newton's loop
  int success = newton(dp, mesh, 
                       MATRIX_SOLVER, MATRIX_SOLVER_TOL, MATRIX_SOLVER_MAXITER,
                       TOL_NEWTON);
  if (!success) error("Newton's method did not converge."); 

  // Plot the solution
  Linearizer l(mesh);
  l.plot_solution("solution.gp");

  printf("Done.\n");
  return 1;
}
