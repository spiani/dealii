// ---------------------------------------------------------------------
//
// Copyright (C) 2014 - 2015 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the deal.II distribution.
//
// ---------------------------------------------------------------------



// Tests CellwiseInverseMassMatrix on vector DG elements, otherwise the same
// as inverse_mass_01.cc

#include "../tests.h"
#include <deal.II/base/function.h>
#include <deal.II/matrix_free/matrix_free.h>
#include <deal.II/matrix_free/fe_evaluation.h>
#include <deal.II/grid/tria.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria_boundary_lib.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/fe/fe_dgq.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/lac/vector.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/precondition.h>

#include <deal.II/matrix_free/operators.h>


std::ofstream logfile("output");



template <int dim, int fe_degree, typename Number, typename VectorType=Vector<Number> >
class MatrixFreeTest
{
public:

  MatrixFreeTest(const MatrixFree<dim,Number> &data_in):
    data (data_in)
  {};

  void
  local_mass_operator (const MatrixFree<dim,Number>               &data,
                       VectorType                                 &dst,
                       const VectorType                           &src,
                       const std::pair<unsigned int,unsigned int> &cell_range) const
  {
    FEEvaluation<dim,fe_degree,fe_degree+1,dim,Number> fe_eval (data);
    const unsigned int n_q_points = fe_eval.n_q_points;

    for (unsigned int cell=cell_range.first; cell<cell_range.second; ++cell)
      {
        fe_eval.reinit (cell);
        fe_eval.read_dof_values (src);
        fe_eval.evaluate (true, false);
        for (unsigned int q=0; q<n_q_points; ++q)
          fe_eval.submit_value (fe_eval.get_value(q),q);
        fe_eval.integrate (true, false);
        fe_eval.distribute_local_to_global (dst);
      }
  }

  void
  local_inverse_mass_operator (const MatrixFree<dim,Number>               &data,
                               VectorType                                 &dst,
                               const VectorType                           &src,
                               const std::pair<unsigned int,unsigned int> &cell_range) const
  {
    FEEvaluation<dim,fe_degree,fe_degree+1,dim,Number> fe_eval (data);
    MatrixFreeOperators::CellwiseInverseMassMatrix<dim,fe_degree,dim,Number> mass_inv(fe_eval);
    const unsigned int n_q_points = fe_eval.n_q_points;
    AlignedVector<VectorizedArray<Number> > inverse_coefficients(n_q_points);

    for (unsigned int cell=cell_range.first; cell<cell_range.second; ++cell)
      {
        fe_eval.reinit (cell);
        mass_inv.fill_inverse_JxW_values(inverse_coefficients);
        fe_eval.read_dof_values (src);
        mass_inv.apply(inverse_coefficients, dim, fe_eval.begin_dof_values(),
                       fe_eval.begin_dof_values());
        fe_eval.distribute_local_to_global (dst);
      }
  }

  void vmult (VectorType   &dst,
              const VectorType &src) const
  {
    dst = 0;
    data.cell_loop (&MatrixFreeTest<dim,fe_degree,Number,VectorType>::local_mass_operator,
                    this, dst, src);
  };

  void apply_inverse (VectorType   &dst,
                      const VectorType &src) const
  {
    dst = 0;
    data.cell_loop (&MatrixFreeTest<dim,fe_degree,Number,VectorType>::local_inverse_mass_operator,
                    this, dst, src);
  };

private:
  const MatrixFree<dim,Number> &data;
};



template <int dim, int fe_degree, typename number>
void do_test (const DoFHandler<dim> &dof)
{

  deallog << "Testing " << dof.get_fe().get_name() << std::endl;

  MatrixFree<dim,number> mf_data;
  {
    const QGauss<1> quad (fe_degree+1);
    typename MatrixFree<dim,number>::AdditionalData data;
    data.tasks_parallel_scheme =
      MatrixFree<dim,number>::AdditionalData::partition_color;
    data.tasks_block_size = 3;
    ConstraintMatrix constraints;

    mf_data.reinit (dof, constraints, quad, data);
  }

  MatrixFreeTest<dim,fe_degree,number> mf (mf_data);
  Vector<number> in (dof.n_dofs()), inverse (dof.n_dofs()), reference(dof.n_dofs());

  for (unsigned int i=0; i<dof.n_dofs(); ++i)
    {
      const double entry = random_value<double>();
      in(i) = entry;
    }

  mf.apply_inverse (inverse, in);

  SolverControl control(1000, 1e-12);
  std::ostringstream stream;
  deallog.attach(stream);
  SolverCG<Vector<number> > solver(control);
  solver.solve (mf, reference, in, PreconditionIdentity());
  deallog.attach(logfile);

  inverse -= reference;
  const double diff_norm = inverse.linfty_norm() / reference.linfty_norm();

  deallog << "Norm of difference: " << diff_norm << std::endl << std::endl;
}



template <int dim, int fe_degree>
void test ()
{
  Triangulation<dim> tria;
  GridGenerator::hyper_cube (tria, -1, 1);
  tria.refine_global(1);
  if (dim < 3 || fe_degree < 2)
    tria.refine_global(1);
  tria.begin(tria.n_levels()-1)->set_refine_flag();
  tria.last()->set_refine_flag();
  tria.execute_coarsening_and_refinement();
  typename Triangulation<dim>::active_cell_iterator
  cell = tria.begin_active (),
  endc = tria.end();
  for (; cell!=endc; ++cell)
    if (cell->center().norm()<1e-8)
      cell->set_refine_flag();
  tria.execute_coarsening_and_refinement();

  const unsigned int degree = fe_degree;
  FESystem<dim> fe (FE_DGQ<dim>(degree), dim);
  DoFHandler<dim> dof (tria);
  dof.distribute_dofs(fe);

  do_test<dim, fe_degree, double> (dof);
}



int main ()
{
  deallog.attach(logfile);

  deallog << std::setprecision (3);

  {
    deallog.push("2d");
    test<2,1>();
    test<2,2>();
    test<2,4>();
    deallog.pop();
    deallog.push("3d");
    test<3,1>();
    test<3,2>();
    deallog.pop();
  }
}
