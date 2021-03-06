// ---------------------------------------------------------------------
//
// Copyright (C) 2012 - 2017 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE.md at
// the top level directory of deal.II.
//
// ---------------------------------------------------------------------


// check that the l2 norm is exactly the same for many runs on random vector
// size with random vector entries. this is to ensure that the result is
// reproducible also when parallel evaluation is done

#include <deal.II/lac/la_vector.h>

#include "../tests.h"



template <typename number>
void
check_norms()
{
  for (unsigned int test = 0; test < 20; ++test)
    {
      const unsigned int            size = Testing::rand() % 100000;
      LinearAlgebra::Vector<number> vec(size);
      for (unsigned int i = 0; i < size; ++i)
        vec(i) = random_value<number>();
      const typename LinearAlgebra::ReadWriteVector<number>::real_type norm =
        vec.l2_norm();
      for (unsigned int i = 0; i < 30; ++i)
        AssertThrow(vec.l2_norm() == norm, ExcInternalError());

      LinearAlgebra::Vector<number> vec2(vec);
      for (unsigned int i = 0; i < 10; ++i)
        AssertThrow(vec2.l2_norm() == norm, ExcInternalError());
    }
}


int
main()
{
  std::ofstream logfile("output");
  deallog << std::fixed;
  deallog << std::setprecision(2);
  deallog.attach(logfile);

  check_norms<float>();
  check_norms<double>();
#ifdef DEAL_II_WITH_COMPLEX_VALUES
  check_norms<std::complex<double>>();
#endif
  deallog << "OK" << std::endl;
}
