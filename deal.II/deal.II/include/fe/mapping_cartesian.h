//---------------------------------------------------------------------------
//    $Id$
//    Version: $Name$
//
//    Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//---------------------------------------------------------------------------
#ifndef __deal2__mapping_cartesian_h
#define __deal2__mapping_cartesian_h


#include <base/config.h>
#include <base/table.h>
#include <cmath>
#include <fe/mapping.h>

DEAL_II_NAMESPACE_OPEN

/*!@addtogroup mapping */
/*@{*/

/**
 * Mapping of an axis-parallel cell.
 *
 * This class maps the unit cell to a grid cell with surfaces parallel
 * to the coordinate lines/planes. The mapping is therefore a scaling
 * along the coordinate directions. It is specifically developed for
 * cartesian meshes. Apply this mapping to a general mesh to get
 * strange results.
 *
 * @author Guido Kanschat, 2001; Ralf Hartmann, 2005
 */
template <int dim>
class MappingCartesian : public Mapping<dim>
{
  public:
    virtual
    typename Mapping<dim>::InternalDataBase *
    get_data (const UpdateFlags,
	      const Quadrature<dim>& quadrature) const;

    virtual
    typename Mapping<dim>::InternalDataBase *
    get_face_data (const UpdateFlags flags,
		   const Quadrature<dim-1>& quadrature) const;

    virtual
    typename Mapping<dim>::InternalDataBase *
    get_subface_data (const UpdateFlags flags,
		      const Quadrature<dim-1>& quadrature) const;

    virtual void
    fill_fe_values (const typename Triangulation<dim>::cell_iterator &cell,
		    const Quadrature<dim>& quadrature,
		    typename Mapping<dim>::InternalDataBase &mapping_data,
		    std::vector<Point<dim> >        &quadrature_points,
		    std::vector<double>             &JxW_values) const ;

    virtual void
    fill_fe_face_values (const typename Triangulation<dim>::cell_iterator &cell,
			 const unsigned int face_no,
			 const Quadrature<dim-1>& quadrature,
			 typename Mapping<dim>::InternalDataBase &mapping_data,
			 std::vector<Point<dim> >        &quadrature_points,
			 std::vector<double>             &JxW_values,
			 std::vector<Tensor<1,dim> >        &boundary_form,
			 std::vector<Point<dim> >        &normal_vectors,
			 std::vector<double>             &cell_JxW_values) const ;
    virtual void
    fill_fe_subface_values (const typename Triangulation<dim>::cell_iterator &cell,
			    const unsigned int face_no,
			    const unsigned int sub_no,
			    const Quadrature<dim-1>& quadrature,
			    typename Mapping<dim>::InternalDataBase &mapping_data,
			    std::vector<Point<dim> >        &quadrature_points,
			    std::vector<double>             &JxW_values,
			    std::vector<Tensor<1,dim> >        &boundary_form,
			    std::vector<Point<dim> >        &normal_vectors,
			    std::vector<double>             &cell_JxW_values) const ;

				     /**
				      * Implementation of the
				      * interface in Mapping.
				      */
    virtual void
    transform_covariant (const VectorSlice<const std::vector<Tensor<1,dim> > > input,
                         const unsigned int                 offset,
			 VectorSlice<std::vector<Tensor<1,dim> > > output,
			 const typename Mapping<dim>::InternalDataBase &internal) const;

				     /**
				      * Implementation of the
				      * interface in Mapping.
				      */
    virtual void
    transform_covariant (const VectorSlice<const std::vector<Tensor<2,dim> > > input,
                         const unsigned int                 offset,
			 VectorSlice<std::vector<Tensor<2,dim> > > output,
			 const typename Mapping<dim>::InternalDataBase &internal) const;
    
				     /**
				      * Implementation of the
				      * interface in Mapping.
				      */
    virtual void
    transform_contravariant (const VectorSlice<const std::vector<Tensor<1,dim> > > input,
                             const unsigned int                 offset,
			     VectorSlice<std::vector<Tensor<1,dim> > > output,
			     const typename Mapping<dim>::InternalDataBase &internal) const;
    
				     /**
				      * Implementation of the
				      * interface in Mapping.
				      */
    virtual void
    transform_contravariant (const VectorSlice<const std::vector<Tensor<2,dim> > > input,
                             const unsigned int                 offset,
			     VectorSlice<std::vector<Tensor<2,dim> > > output,
			     const typename Mapping<dim>::InternalDataBase &internal) const;

    virtual Point<dim>
    transform_unit_to_real_cell (
      const typename Triangulation<dim>::cell_iterator &cell,
      const Point<dim>                                 &p) const;

				     /**
				      * Transforms the point @p p on
				      * the real cell to the point
				      * @p p_unit on the unit cell
				      * @p cell and returns @p p_unit.
				      *
				      * Uses Newton iteration and the
				      * @p transform_unit_to_real_cell
				      * function.
				      */
    virtual Point<dim>
    transform_real_to_unit_cell (
      const typename Triangulation<dim>::cell_iterator &cell,
      const Point<dim>                                 &p) const;
    
    virtual UpdateFlags update_once (const UpdateFlags) const;    
    virtual UpdateFlags update_each (const UpdateFlags) const;
    

                                     /**
                                      * Return a pointer to a copy of the
                                      * present object. The caller of this
                                      * copy then assumes ownership of it.
                                      */
    virtual
    Mapping<dim> * clone () const;

    				     /**
				      * Exception
				      */
    DeclException0 (ExcInvalidData);
				     /**
				      * Exception
				      */
    DeclException0 (ExcAccessToUninitializedField);

  protected:
				     /** 
				      * Storage for internal data of
				      * the scaling.
				      */
    class InternalData : public Mapping<dim>::InternalDataBase
    {
      public:
					 /**
					  * Constructor.
					  */
	InternalData (const Quadrature<dim> &quadrature);

					 /**
					  * Return an estimate (in
					  * bytes) or the memory
					  * consumption of this
					  * object.
					  */
	virtual unsigned int memory_consumption () const;

					 /**
					  * Length of the cell in
					  * different coordinate
					  * directions, <i>h<sub>x</sub></i>,
					  * <i>h<sub>y</sub></i>, <i>h<sub>z</sub></i>.
					  */
	Tensor<1,dim> length;

					 /**
					  * Vector of all quadrature
					  * points. Especially, all
					  * points on all faces.
					  */
	std::vector<Point<dim> > quadrature_points;
    };
    
				     /**
				      * Do the computation for the
				      * <tt>fill_*</tt> functions.
				      */
    void compute_fill (const typename Triangulation<dim>::cell_iterator &cell,
		       const unsigned int face_no,
		       const unsigned int sub_no,
		       InternalData& data,
		       std::vector<Point<dim> > &quadrature_points,
		       std::vector<Point<dim> >& normal_vectors) const;

  private:
				     /**
				      * Value to indicate that a given
				      * face or subface number is
				      * invalid.
				      */
    static const unsigned int invalid_face_number = deal_II_numbers::invalid_unsigned_int;    
};

/*@}*/

/* -------------- declaration of explicit specializations ------------- */

#ifndef DOXYGEN

template <> void MappingCartesian<1>::fill_fe_face_values (
  const Triangulation<1>::cell_iterator &,
  const unsigned,
  const Quadrature<0>&,
  Mapping<1>::InternalDataBase&,
  std::vector<Point<1> >&,
  std::vector<double>&,
  std::vector<Tensor<1,1> >&,
  std::vector<Point<1> >&,
  std::vector<double>&) const;

template <> void MappingCartesian<1>::fill_fe_subface_values (
  const Triangulation<1>::cell_iterator &,
  const unsigned,
  const unsigned,
  const Quadrature<0>&,
  Mapping<1>::InternalDataBase&,
  std::vector<Point<1> >&,
  std::vector<double>&,
  std::vector<Tensor<1,1> >&,
  std::vector<Point<1> >&,
  std::vector<double>&) const;

#endif // DOXYGEN  

DEAL_II_NAMESPACE_CLOSE

#endif
