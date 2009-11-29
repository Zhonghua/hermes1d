#include "hermes1d.h"

// ********************************************************************
// This example solves the mathematical pendulum equation 
// y'' + k**2 * sin(y) = 0 in an interval (A, B), equipped with the 
// initial conditions y(A) = Init_angle, y'(0) = Init_vel. The 
// system is decomposed into two first order ODE and solved via 
// the Newton's method starting from zero initial condition.
// Note that the method diverges for longer time intervals, 
// depending on the interval length, number of elements, and 
// the initial polynomial degree.
//
// Derivation:
// m*l*u'' = -m*g*sin(u)
// so:
// u'' + k^2 * sin(u) = 0
// with k^2 = g/l
// so we have to solve a system of two nonlinear second-order equations
// v' + k^2 sin u = 0
// u' - v = 0
// in an interval (0, 2*pi) equipped with Dirichlet bdy conditions
// u(0) = 0, v(0) = k
// The approximate (linearized) solution is u(x) = sin(k*x), v(x) = k*cos(k*x)

// General input:
static int N_eq = 2;
int N_elem = 4;            // number of elements
double A = 0, B = 10;      // domain end points
int P_init = 1;            // initial polynomal degree
double k = 0.5;

// Stopping criteria for Newton
double TOL_NEWTON_COARSE = 1e-5;        // Coarse mesh
double TOL_NEWTON_REF = 1e-3;           // Fine mesh

// Adaptivity
const int ADAPT_TYPE = 0;               // 0... hp-adaptivity
                                        // 1... h-adaptivity
                                        // 2... p-adaptivity
const double THRESHOLD = 0.7;           // Refined will be all elements whose error
                                        // is greater than THRESHOLD*max_elem_error
const double TOL_ERR_REL = 1e-3;        // Tolerance for the relative error between 
                                        // the coarse mesh and fine solutions
const int NORM = 1;                     // To measure errors:
                                        // 1... H1 norm
                                        // 0... L2 norm

// Boundary conditions
double Init_angle = M_PI/2.;      // initial angle
double Init_vel = 0;              // initial velocity

// Exact solution not available for this example
const int EXACT_SOL_PROVIDED = 0;
double exact_sol(double x, double u[MAX_EQN_NUM], double dudx[MAX_EQN_NUM]) 
{
  u[0] = 0;
  dudx[0] = 0;
}

// ********************************************************************

void plotting(Mesh *mesh, Mesh *mesh_ref, double *y_prev, double *y_prev_ref) 
{
  // Plot the coarse mesh solution
  Linearizer l(mesh);
  const char *out_filename = "solution.gp";
  l.plot_solution(out_filename, y_prev);

  // Plot the fine solution
  Linearizer lxx(mesh_ref);
  const char *out_filename2 = "solution_ref.gp";
  lxx.plot_solution(out_filename2, y_prev_ref);

  // Plot the coarse and fine mesh
  const char *mesh_filename = "mesh.gp";
  mesh->plot(mesh_filename);
  const char *mesh_ref_filename = "mesh_ref.gp";
  mesh_ref->plot(mesh_ref_filename);

  // Plot the error estimate (difference between 
  // coarse and fine mesh solutions)
  const char *err_est_filename = "error_est.gp";
  mesh->plot_error_est(NORM, err_est_filename, mesh_ref, y_prev, y_prev_ref);

  // Plot error wrt. exact solution (if available)
  if (EXACT_SOL_PROVIDED) {   
    const char *err_exact_filename = "error_exact.gp";
    mesh->plot_error_exact(NORM, err_exact_filename, y_prev, exact_sol);
  }
}

// ********************************************************************

// Jacobi matrix block 0, 0 (equation 0, solution component 0)
// Note: u_prev[c][i] contains the values of solution component c 
// in integration point x[i]. similarly for du_prevdx.
double jacobian_0_0(int num, double *x, double *weights, 
                double *u, double *dudx, double *v, double *dvdx, 
                double u_prev[MAX_EQN_NUM][MAX_PTS_NUM], 
                double du_prevdx[MAX_EQN_NUM][MAX_PTS_NUM], 
                void *user_data)
{
  double val = 0;
  for(int i = 0; i<num; i++) {
    val += dudx[i]*v[i]*weights[i];
  }
  return val;
};

// Jacobi matrix block 0, 1 (equation 0, solution component 1)
double jacobian_0_1(int num, double *x, double *weights, 
                double *u, double *dudx, double *v, double *dvdx, 
                double u_prev[MAX_EQN_NUM][MAX_PTS_NUM], 
                double du_prevdx[MAX_EQN_NUM][MAX_PTS_NUM], 
                void *user_data)
{
  double val = 0;
  for(int i = 0; i<num; i++) {
    val -= u[i]*v[i]*weights[i];
  }
  return val;
};

// Jacobi matrix block 1, 0 (equation 1, solution component 0)
double jacobian_1_0(int num, double *x, double *weights, 
                double *u, double *dudx, double *v, double *dvdx, 
                double u_prev[MAX_EQN_NUM][MAX_PTS_NUM], 
                double du_prevdx[MAX_EQN_NUM][MAX_PTS_NUM], 
                void *user_data)
{
  double val = 0;
  for(int i = 0; i<num; i++) {
    val += k*k * cos(u_prev[0][i])*u[i]*v[i]*weights[i];
  }
  return val;
};

// Jacobi matrix block 1, 1 (equation 1, solution component 1)
double jacobian_1_1(int num, double *x, double *weights, 
                double *u, double *dudx, double *v, double *dvdx, 
                double u_prev[MAX_EQN_NUM][MAX_PTS_NUM], 
                double du_prevdx[MAX_EQN_NUM][MAX_PTS_NUM], 
                void *user_data)
{
  double val = 0;
  for(int i = 0; i<num; i++) {
    val += dudx[i]*v[i]*weights[i];
  }
  return val;
};

// Residual part 0 (equation 0)
double residual_0(int num, double *x, double *weights, 
                double u_prev[MAX_EQN_NUM][MAX_PTS_NUM], 
                double du_prevdx[MAX_EQN_NUM][MAX_PTS_NUM], 
                double *v, double *dvdx, void *user_data)
{
  double val = 0;
  for(int i = 0; i<num; i++) {
    val += (du_prevdx[0][i]*v[i] - u_prev[1][i]*v[i])*weights[i];
  }
  return val;
};

// Residual part 1 (equation 1) 
double residual_1(int num, double *x, double *weights, 
                double u_prev[MAX_EQN_NUM][MAX_PTS_NUM], 
                double du_prevdx[MAX_EQN_NUM][MAX_PTS_NUM], 
                double *v, double *dvdx, void *user_data)
{
  double val = 0;
  for(int i = 0; i<num; i++) {
    val += (k*k*sin(u_prev[0][i])*v[i] + du_prevdx[1][i]*v[i])*weights[i];
  }
  return val;
};

/******************************************************************************/
int main() {
  // Create coarse mesh, set Dirichlet BC, enumerate basis functions
  Mesh *mesh = new Mesh(A, B, N_elem, P_init, N_eq);
  mesh->set_bc_left_dirichlet(0, Init_angle);
  mesh->set_bc_left_dirichlet(1, Init_vel);
  int N_dof = mesh->assign_dofs();
  printf("N_dof = %d\n", N_dof);

  // Create discrete problem
  DiscreteProblem *dp = new DiscreteProblem();
  dp->add_matrix_form(0, 0, jacobian_0_0);
  dp->add_matrix_form(0, 1, jacobian_0_1);
  dp->add_matrix_form(1, 0, jacobian_1_0);
  dp->add_matrix_form(1, 1, jacobian_1_1);
  dp->add_vector_form(0, residual_0);
  dp->add_vector_form(1, residual_1);

  // Allocate vectors res and y_prev
  double *res = new double[N_dof];
  double *y_prev = new double[N_dof];
  if (res == NULL || y_prev == NULL)
    error("res or y_prev could not be allocated in main().");

  // Set y_prev zero
  for(int i=0; i<N_dof; i++) y_prev[i] = 0; 

  // Obtain initial coarse mesh solution via Newton's method
  int newton_iterations = 1;
  CooMatrix *mat = NULL;
  while (1) {
    // Reset the matrix:
    if (mat != NULL) delete mat;
    mat = new CooMatrix();

    // Construct residual vector
    dp->assemble_matrix_and_vector(mesh, mat, res, y_prev); 

    // Calculate norm of residual vector
    double res_norm = 0;
    for(int i=0; i<N_dof; i++) res_norm += res[i]*res[i];
    res_norm = sqrt(res_norm);

    // If residual norm less than TOL_NEWTON_COARSE, quit
    // latest solution is in y_prev
    printf("Residual norm (coarse): %.15f\n", res_norm);
    if(res_norm < TOL_NEWTON_COARSE) break;

    // Change sign of vector res
    for(int i=0; i<N_dof; i++) res[i]*= -1;

    // Solve the matrix system
    solve_linear_system_umfpack((CooMatrix*)mat, res);

    // Update y_prev by new solution which is in res
    for(int i=0; i<N_dof; i++) y_prev[i] += res[i];

    newton_iterations++;
  }
  printf("Finished initial coarse mesh Newton loop (%d iter).\n", newton_iterations);

  // Create initial fine mesh
  // Perform refinements in the fine mesh
  // Refines 'num_to_ref' elements starting with element 'start_elem_id'
  // For now, refine entire mesh uniformly in 'h' and 'p'
  Mesh *mesh_ref = mesh->replicate();
  int start_elem_id = 0; 
  int num_to_ref = mesh->get_n_active_elem();
  mesh_ref->reference_refinement(start_elem_id, num_to_ref);
  int N_dof_ref = mesh_ref->assign_dofs();
  printf("Fine mesh created (%d DOF).\n", N_dof_ref);

  // Allocate vectors y_prev_ref and res_ref
  double *y_prev_ref = new double[N_dof_ref];
  double *res_ref = new double[N_dof_ref];
  if (y_prev_ref == NULL || res_ref == NULL) 
    error("y_prev_ref or res_ref could not be allocated in main().");

  // Transfer coarse mesh solution to the fine mesh
  transfer_solution(mesh, mesh_ref, y_prev, y_prev_ref);
  printf("Coarse mesh solution copied to fine mesh.\n");

  // Convergence graph wrt. the number of degrees of freedom
  GnuplotGraph graph;
  graph.set_log_y();
  graph.set_captions("Convergence History", "Degrees of Freedom", "Error [%]");
  //graph.add_row("exact error", "k", "-", "o");
  graph.add_row("error estimate", "k", "--");

  // Main adaptivity loop
  int adapt_iterations = 1;
  while(1) {
    printf("============ Adaptivity step %d ============\n", adapt_iterations); 

    // Reallocate vector res_ref on fine mesh
    if (res_ref != NULL) delete res_ref;
    res_ref = new double[N_dof_ref];

    // Obtain fine mesh solution via Newton's method
    // Initial condition is the coarse mesh solution (in the first 
    // adaptivity step) and then the last fine mesh solution.
    int newton_iterations_ref = 1;
    CooMatrix *mat_ref = NULL;
    while(1) {
      // Reset the matrix:
      if (mat_ref != NULL) delete mat_ref;
      mat_ref = new CooMatrix();

      // Construct residual vector
      dp->assemble_matrix_and_vector(mesh_ref, mat_ref, res_ref, y_prev_ref); 

      // Calculate norm of residual vector
      double res_ref_norm = 0;
      for(int i=0; i<N_dof_ref; i++) res_ref_norm += res_ref[i]*res_ref[i];
      res_ref_norm = sqrt(res_ref_norm);
      printf("Residual norm (fine mesh): %.15f\n", res_ref_norm);

      // If residual norm less than TOL_NEWTON_REF, break
      // NOTE: at least one update of the fine mesh solution is 
      // enforced by the additional condition "&& newton_iterations_ref >= 2". 
      // Otherwise it can (and will) happen that already the initial residual 
      // of the fine mesh solution is too small. Then the next adaptivity 
      // step uses the previous fine mesn solution, which may cause inoptimal 
      // refinements to take place. 
      if(res_ref_norm < TOL_NEWTON_REF && newton_iterations_ref >= 2) break;

      // Change sign of vector res_ref
      for(int i=0; i<N_dof_ref; i++) res_ref[i]*= -1;

      // Solve the matrix system. 
      solve_linear_system_umfpack((CooMatrix*)mat_ref, res_ref);

      // Update y_prev_ref by the increment 'res_ref'
      for(int i=0; i<N_dof_ref; i++) y_prev_ref[i] += res_ref[i];

      newton_iterations_ref++;
    }
    printf("Finished fine mesh Newton loop (%d iter).\n", 
           newton_iterations_ref);

    // Starting with second adaptivity step, obtain new coarse 
    // mesh solution via Newton's method. Initial condition is 
    // the last coarse mesh solution.
    if (adapt_iterations > 1) {
      if (res != NULL) delete res;
      res = new double[N_dof];
      if (res == NULL) error("mat or res could not be allocated.");

      // Obtain coarse mesh solution via Newton's method
      int newton_iterations = 1;
      while (1) {
        // Reset the matrix:
        if (mat != NULL) delete mat;
        mat = new CooMatrix();

        // Construct residual vector
        dp->assemble_matrix_and_vector(mesh, mat, res, y_prev); 

        // Calculate norm of residual vector
        double res_norm = 0;
        for(int i=0; i<N_dof; i++) res_norm += res[i]*res[i];
        res_norm = sqrt(res_norm);

        // If residual norm less than TOL_NEWTON_COARSE, quit
        // latest solution is in y_prev
        printf("Residual norm (coarse mesh): %.15f\n", res_norm);
        if(res_norm < TOL_NEWTON_COARSE && newton_iterations >= 2) break;

        // Change sign of vector res
        for(int i=0; i<N_dof; i++) res[i]*= -1;

        // Solve the matrix system
        solve_linear_system_umfpack((CooMatrix*)mat, res);

        // Update y_prev by new solution which is in res
        for(int i=0; i<N_dof; i++) y_prev[i] += res[i];

        newton_iterations++;
      }
      printf("Finished coarse mesh Newton loop (%d iter).\n", newton_iterations);
    }

    // In the next step, estimate element errors (squared) based on 
    // the difference between the fine mesh and coarse mesh solutions. 
    double err_est_squared_array[MAX_ELEM_NUM]; 
    double err_est_total = calc_elem_est_errors_squared(NORM, mesh, mesh_ref, y_prev, 
                                 y_prev_ref, err_est_squared_array);

    // Calculate the norm of the fine mesh solution
    double ref_sol_norm = calc_approx_sol_norm(NORM, mesh_ref, y_prev_ref);

    // Calculate an estimate of the global relative error
    double err_est_rel = err_est_total/ref_sol_norm;
    printf("Relative error (est) = %g %%\n", 100.*err_est_rel);

    // If exact solution available, also calculate exact error
    if (EXACT_SOL_PROVIDED) {
      // Calculate element errors wrt. exact solution (squared)
      int order = 20; // heuristic parameter
      double err_exact_total = calc_exact_sol_error(NORM, mesh, y_prev, exact_sol, order);
     
      // Calculate the norm of the exact solution
      // (using a fine subdivision and high-order quadrature)
      int subdivision = 500; // heuristic parameter
      double exact_sol_norm = calc_exact_sol_norm(NORM, exact_sol, N_eq, A, B,
                                                  subdivision, order);
      // Calculate an estimate of the global relative error
      double err_exact_rel = err_exact_total/exact_sol_norm;
      printf("Relative error (exact) = %g %%\n", 100.*err_exact_rel);
      //graph.add_values(0, N_dof, 100 * err_exact_rel);
    }

    // add entry to DOF convergence graph
    graph.add_values(0, N_dof, 100 * err_est_rel);

    // Decide whether the relative error is sufficiently small
    if(err_est_rel*100 < TOL_ERR_REL) break;

    // debug
    //if (adapt_iterations == 3) break;

    // Refine coarse mesh elements whose id_array >= 0, and 
    // adjust the fine mesh accordingly.  
    // Returns updated coarse and fine meshes, with the last 
    // coarse and fine mesh solutions on them, respectively. 
    // The coefficient vectors and numbers of degrees of freedom 
    // on both meshes are also updated. 
    adapt(NORM, ADAPT_TYPE, THRESHOLD, err_est_squared_array,
          mesh, mesh_ref, y_prev, y_prev_ref, N_dof, N_dof_ref);

    adapt_iterations++;
  }

  // Plot meshes, results, and errors
  plotting(mesh, mesh_ref, y_prev, y_prev_ref);

  // Save convergence graph
  graph.save("conv_dof.gp");

  printf("Done.\n");
  return 1;

}
