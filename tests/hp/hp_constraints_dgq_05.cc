//----------------------------  hp_constraints_dgq_05.cc  ---------------------------
//    $Id$
//    Version: $Name$ 
//
//    Copyright (C) 2006 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//----------------------------  hp_constraints_dgq_05.cc  ---------------------------


// check that computation of hp constraints works for DGQ elements correctly

char logname[] = "hp_constraints_dgq_05/output";


#include "hp_constraints_common.h"


template <int dim>
void test ()
{
  hp::FECollection<dim> fe;
  for (unsigned int i=0; i<4; ++i)
    fe.push_back (FE_DGQ<dim>(i));
  test_with_2d_deformed_refined_mesh (fe);
}
