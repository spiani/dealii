//---------------------------------------------------------------------------
//    $Id$
//    Version: $Name$
//
//    Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//---------------------------------------------------------------------------


#include <base/tensor.h>
#include <grid/tria_boundary.h>
#include <grid/tria.h>
#include <grid/tria_iterator.h>
#include <grid/tria_accessor.h>
#include <cmath>

DEAL_II_NAMESPACE_OPEN



/* -------------------------- Boundary --------------------- */


template <int dim>
Boundary<dim>::~Boundary ()
{}



template <int dim>
Point<dim>
Boundary<dim>::get_new_point_on_quad (const typename Triangulation<dim>::quad_iterator &) const 
{
  Assert (false, ExcPureFunctionCalled());
  return Point<dim>();
}



template <int dim>
void
Boundary<dim>::
get_intermediate_points_on_line (const typename Triangulation<dim>::line_iterator &,
				 std::vector<Point<dim> > &) const
{
  Assert (false, ExcPureFunctionCalled());
}



template <int dim>
void
Boundary<dim>::
get_intermediate_points_on_quad (const typename Triangulation<dim>::quad_iterator &,
				 std::vector<Point<dim> > &) const
{
  Assert (false, ExcPureFunctionCalled());
}



template <int dim>
void
Boundary<dim>::
get_normals_at_vertices (const typename Triangulation<dim>::face_iterator &,
			 FaceVertexNormals                                &) const
{
  Assert (false, ExcPureFunctionCalled());
}



/* -------------------------- StraightBoundary --------------------- */


template <int dim>
StraightBoundary<dim>::StraightBoundary ()
{}



template <int dim>
Point<dim>
StraightBoundary<dim>::get_new_point_on_line (const typename Triangulation<dim>::line_iterator &line) const 
{
  return (line->vertex(0) + line->vertex(1)) / 2;
}


#if deal_II_dimension < 3

template <int dim>
Point<dim>
StraightBoundary<dim>::get_new_point_on_quad (const typename Triangulation<dim>::quad_iterator &) const 
{
  Assert (false, ExcImpossibleInDim(dim));
  return Point<dim>();
}


#else


template <>
Point<3>
StraightBoundary<3>::
get_new_point_on_quad (const Triangulation<3>::quad_iterator &quad) const 
{
                                   // generate a new point in the middle of
                                   // the face based on the points on the
                                   // edges and the vertices.
                                   //
                                   // there is a pathological situation when
                                   // this face is on a straight boundary, but
                                   // one of its edges and the face behind it
                                   // are not; if that face is refined first,
                                   // the new point in the middle of that edge
                                   // may not be at the same position as
                                   // quad->line(.)->center() would have been,
                                   // but would have been moved to the
                                   // non-straight boundary. We cater to that
                                   // situation by using existing edge
                                   // midpoints if available, or center() if
                                   // not
                                   //
                                   // note that this situation can not happen
                                   // during mesh refinement, as there the
                                   // edges are refined first and only then
                                   // the face. thus, the check whether a line
                                   // has children does not lead to the
                                   // situation where the new face midpoints
                                   // have different positions depending on
                                   // which of the two cells is refined first.
                                   //
                                   // the situation where the edges aren't
                                   // refined happens when the a higher order
                                   // MappingQ requests the midpoint of a
                                   // face, though, and it is for these cases
                                   // that we need to have the check available
  return (quad->vertex(0) + quad->vertex(1) +
	  quad->vertex(2) + quad->vertex(3) +
	  (quad->line(0)->has_children() ?
           quad->line(0)->child(0)->vertex(1) :
           quad->line(0)->center()) +
	  (quad->line(1)->has_children() ?
           quad->line(1)->child(0)->vertex(1) :
           quad->line(1)->center()) +
	  (quad->line(2)->has_children() ?
           quad->line(2)->child(0)->vertex(1) :
           quad->line(2)->center()) +
	  (quad->line(3)->has_children() ?
           quad->line(3)->child(0)->vertex(1) :
           quad->line(3)->center())               ) / 8;
}

#endif


#if deal_II_dimension == 1

template <>
void
StraightBoundary<1>::
get_intermediate_points_on_line (const Triangulation<1>::line_iterator &,
				 std::vector<Point<1> > &) const
{
  Assert(false, ExcImpossibleInDim(1));
}


#else


template <int dim>
void
StraightBoundary<dim>::
get_intermediate_points_on_line (const typename Triangulation<dim>::line_iterator &line,
				 std::vector<Point<dim> > &points) const
{
  const unsigned int n=points.size();
  Assert(n>0, ExcInternalError());
  
  const double dx=1./(n+1);
  double x=dx;

  const Point<dim> vertices[2] = { line->vertex(0),
				   line->vertex(1) };
  
  for (unsigned int i=0; i<n; ++i, x+=dx)
    points[i] = (1-x)*vertices[0] + x*vertices[1];
}

#endif



#if deal_II_dimension < 3

template <int dim>
void
StraightBoundary<dim>::
get_intermediate_points_on_quad (const typename Triangulation<dim>::quad_iterator &,
				 std::vector<Point<dim> > &) const
{
  Assert(false, ExcImpossibleInDim(dim));
}

#else

template <>
void
StraightBoundary<3>::
get_intermediate_points_on_quad (const Triangulation<3>::quad_iterator &quad,
				 std::vector<Point<3> > &points) const
{
  const unsigned int dim = 3;
  
  const unsigned int n=points.size(),
		     m=static_cast<unsigned int>(std::sqrt(static_cast<double>(n)));
				   // is n a square number
  Assert(m*m==n, ExcInternalError());

  const double ds=1./(m+1);
  double y=ds;

  const Point<dim> vertices[4] = { quad->vertex(0),
				   quad->vertex(1),
				   quad->vertex(2),
				   quad->vertex(3) };

  for (unsigned int i=0; i<m; ++i, y+=ds)
    {
      double x=ds;
      for (unsigned int j=0; j<m; ++j, x+=ds)
	points[i*m+j]=((1-x) * vertices[0] +
		       x     * vertices[1]) * (1-y) +
		      ((1-x) * vertices[2] +
		       x     * vertices[3]) * y;
    }
}

#endif



#if deal_II_dimension == 1

template <>
void
StraightBoundary<1>::
get_normals_at_vertices (const Triangulation<1>::face_iterator &,
			 Boundary<1>::FaceVertexNormals &) const
{
  Assert (false, ExcImpossibleInDim(1));
}

#endif


#if deal_II_dimension == 2

template <>
void
StraightBoundary<2>::
get_normals_at_vertices (const Triangulation<2>::face_iterator &face,
			 Boundary<2>::FaceVertexNormals &face_vertex_normals) const
{
  const Tensor<1,2> tangent = face->vertex(1) - face->vertex(0);
  for (unsigned int vertex=0; vertex<GeometryInfo<2>::vertices_per_face; ++vertex)
				     // compute normals from tangent
    face_vertex_normals[vertex] = Point<2>(tangent[1],
                                           -tangent[0]);
}

#endif



#if deal_II_dimension == 3

template <>
void
StraightBoundary<3>::
get_normals_at_vertices (const Triangulation<3>::face_iterator &face,
			 Boundary<3>::FaceVertexNormals &face_vertex_normals) const
{
  const unsigned int vertices_per_face = GeometryInfo<3>::vertices_per_face;

  static const unsigned int neighboring_vertices[4][2]=
  { {1,2},{3,0},{0,3},{2,1}};
  for (unsigned int vertex=0; vertex<vertices_per_face; ++vertex)
    {
				       // first define the two tangent
				       // vectors at the vertex by
				       // using the two lines
				       // radiating away from this
				       // vertex
      const Tensor<1,3> tangents[2]
	= { face->vertex(neighboring_vertices[vertex][0])
	      - face->vertex(vertex),
	    face->vertex(neighboring_vertices[vertex][1])
	      - face->vertex(vertex)      };

				       // then compute the normal by
				       // taking the cross
				       // product. since the normal is
				       // not required to be
				       // normalized, no problem here
      cross_product (face_vertex_normals[vertex],
		     tangents[0], tangents[1]);
    };
}

#endif


// explicit instantiations
template class Boundary<deal_II_dimension>;
template class StraightBoundary<deal_II_dimension>;

DEAL_II_NAMESPACE_CLOSE

