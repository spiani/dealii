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
#ifndef __deal2__mg_transfer_h
#define __deal2__mg_transfer_h

#include <base/config.h>

#include <lac/block_vector.h>
#ifdef DEAL_PREFER_MATRIX_EZ
#  include <lac/sparse_matrix_ez.h>
#  include <lac/block_sparse_matrix_ez.h>
#else
#  include <lac/sparsity_pattern.h>
#  include <lac/block_sparsity_pattern.h>
#endif
#include <lac/vector_memory.h>

#include <multigrid/mg_base.h>
#include <multigrid/mg_level_object.h>



#include <dofs/dof_handler.h>

#include <boost/shared_ptr.hpp>


DEAL_II_NAMESPACE_OPEN


template <int dim> class MGDoFHandler;

/*
 * MGTransferBase is defined in mg_base.h
 */

/*!@addtogroup mg */
/*@{*/

/**
 * Implementation of the MGTransferBase interface for which the transfer
 * operations are prebuilt upon construction of the object of this class as
 * matrices. This is the fast way, since it only needs to build the operation
 * once by looping over all cells and storing the result in a matrix for
 * each level, but requires additional memory.
 *
 * See MGTransferBase to find out which of the transfer classes
 * is best for your needs.
 *
 * @author Wolfgang Bangerth, Guido Kanschat, 1999-2004
 */
template <class VECTOR>
class MGTransferPrebuilt : public MGTransferBase<VECTOR> 
{
  public:
				     /**
				      * Destructor.
				      */
    virtual ~MGTransferPrebuilt ();
    
				     /**
				      * Actually build the prolongation
				      * matrices for each level.
				      */
    template <int dim>
    void build_matrices (const MGDoFHandler<dim> &mg_dof);

    virtual void prolongate (const unsigned int    to_level,
			     VECTOR       &dst,
			     const VECTOR &src) const;

    virtual void restrict_and_add (const unsigned int    from_level,
				   VECTOR       &dst,
				   const VECTOR &src) const;

    				     /**
				      * Transfer from a vector on the
				      * global grid to vectors defined
				      * on each of the levels
				      * separately, i.a. an @p MGVector.
				      */
    template <int dim, class InVector>
    void
    copy_to_mg (const MGDoFHandler<dim>& mg_dof,
		MGLevelObject<VECTOR>& dst,
		const InVector& src) const;

				     /**
				      * Transfer from multi-level vector to
				      * normal vector.
				      *
				      * Copies data from active
				      * portions of an MGVector into
				      * the respective positions of a
				      * <tt>Vector<number></tt>. In order to
				      * keep the result consistent,
				      * constrained degrees of freedom
				      * are set to zero.
				      */
    template <int dim, class OutVector>
    void
    copy_from_mg (const MGDoFHandler<dim>& mg_dof,
		  OutVector& dst,
		  const MGLevelObject<VECTOR> &src) const;

				     /**
				      * Add a multi-level vector to a
				      * normal vector.
				      *
				      * Works as the previous
				      * function, but probably not for
				      * continuous elements.
				      */
    template <int dim, class OutVector>
    void
    copy_from_mg_add (const MGDoFHandler<dim>& mg_dof,
		      OutVector& dst,
		      const MGLevelObject<VECTOR>& src) const;

				     /**
				      * Finite element does not
				      * provide prolongation matrices.
				      */
    DeclException0(ExcNoProlongation);
    
				     /**
				      * Call @p build_matrices
				      * function first.
				      */
    DeclException0(ExcMatricesNotBuilt);

    				     /**
				      * Memory used by this object.
				      */
    unsigned int memory_consumption () const;
    

  private:

				   /**
				    * Sizes of the multi-level vectors.
				    */
    std::vector<unsigned int> sizes;

				     /**
				      * Sparsity patterns for transfer
				      * matrices.
				      */
    std::vector<boost::shared_ptr<SparsityPattern> >   prolongation_sparsities;

				     /**
				      * The actual prolongation matrix.
				      * column indices belong to the
				      * dof indices of the mother cell,
				      * i.e. the coarse level.
				      * while row indices belong to the
				      * child cell, i.e. the fine level.
				      */
    std::vector<boost::shared_ptr<SparseMatrix<double> > > prolongation_matrices;    
};


/**
 * Implementation of matrix generation for MGTransferBlock and
 * MGTransferSelect.
 *
 * @author Guido Kanschat, 2001-2003
 */
class MGTransferBlockBase
{
  public:
    				     /**
				      * Memory used by this object.
				      */
    unsigned int memory_consumption () const;
    

  protected:  
				     /**
				      * Actually build the prolongation
				      * matrices for each level.
				      *
				      * This function is only called
				      * by derived classes. These can
				      * also set the member variables
				      * #selected and #mg_selected to
				      * restrict the transfer matrices
				      * to certain components.
				      * Furthermore, they use
				      * #target_component and
				      * #mg_target_component for
				      * re-ordering and grouping of
				      * components.
				      */
    template <int dim>
    void build_matrices (const DoFHandler<dim>& dof,
			 const MGDoFHandler<dim>& mg_dof);

				   /**
				    * Flag of selected components.
				    *
				    * The transfer operators only act
				    * on the components having a
				    * <tt>true</tt> entry here. If
				    * renumbering by
				    * #target_component is used,
				    * this refers to the
				    * <b>renumbered</b> components.
				    */
    std::vector<bool> selected;

				   /**
				    * Flag of selected components.
				    *
				    * The transfer operators only act
				    * on the components having a
				    * <tt>true</tt> entry here. If
				    * renumbering by
				    * #mg_target_component is used,
				    * this refers to the
				    * <b>renumbered</b> components.
				    */
    std::vector<bool> mg_selected;

				     /**
				      * Target component of the
				      * fine-level vector if
				      * renumbering is required.
				      */
    std::vector<unsigned int> target_component;
    
				     /**
				      * Target component if
				      * renumbering of level vectors
				      * is required.
				      */
    std::vector<unsigned int> mg_target_component;
    
				   /**
				    * Sizes of the multi-level vectors.
				    */
    mutable std::vector<std::vector<unsigned int> > sizes;
  
				   /**
				    * Start index of each component.
				    */
    std::vector<unsigned int> component_start;
  
				   /**
				    * Start index of each component on
				    * all levels.
				    */
    std::vector<std::vector<unsigned int> > mg_component_start;
    
				     /**
				      * Call build_matrices()
				      * function first.
				      */
    DeclException0(ExcMatricesNotBuilt);
    
  private:
    std::vector<boost::shared_ptr<BlockSparsityPattern> >   prolongation_sparsities;
    
  protected:
    
				     /**
				      * The actual prolongation matrix.
				      * column indices belong to the
				      * dof indices of the mother cell,
				      * i.e. the coarse level.
				      * while row indices belong to the
				      * child cell, i.e. the fine level.
				      */
    std::vector<boost::shared_ptr<BlockSparseMatrix<double> > > prolongation_matrices;
    
				     /**
				      * Unused now, but intended to
				      * hold the mapping for the
				      * <tt>copy_to/from_mg</tt>-functions.
				      */
    std::vector<std::map<unsigned int, unsigned int> >
    copy_to_and_from_indices;
};

//TODO:[GK] Update this class

/**
 * Implementation of the MGTransferBase interface for block
 * matrices and block vectors.
 *
 * Warning! Due to additional requirements on MGTransferSelect, the
 * implementation in the base class has changed. Therefore, this class
 * is left in an untested state. If you use it and you encounter
 * problems, please contact Guido Kanschat.
 *
 * In addition to the functionality of
 * MGTransferPrebuilt, the operation may be restricted to
 * certain blocks of the vector.
 *
 * If the restricted mode is chosen, block vectors used in the
 * transfer routines may only have as many components as there are
 * @p trues in the selected-field.
 *
 * See MGTransferBase to find out which of the transfer classes
 * is best for your needs.
 *
 * @author Guido Kanschat, 2001, 2002
 */
template <typename number>
class MGTransferBlock : public MGTransferBase<BlockVector<number> >,
			private MGTransferBlockBase
{
  public:
				     /**
				      * Default constructor.
				      */
    MGTransferBlock();

				     /**
				      * Destructor.
				      */
    virtual ~MGTransferBlock ();
    
				     /**
				      * Initialize additional #factors
				      * and #memory if the restriction
				      * of the components is to be
				      * weighted differently.
				      */
    void initialize (const std::vector<number>& factors,
		     VectorMemory<Vector<number> >& memory);
    
				     /**
				      * Build the prolongation
				      * matrices for each level.
				      * 
				      * This function is a front-end
				      * for the same function in
				      * MGTransferBlockBase.
				      *
				      * If <tt>mg_target_component</tt> is
				      * present, this refers to the
				      * renumbered components.
				      *
				      * @arg target_component: this
				      * argument allows grouping and
				      * renumbering of components in
				      * the fine-level vector (see
				      * DoFRenumbering::component_wise).
				      * 
				      * @arg mg_target_component: this
				      * argument allows grouping and
				      * renumbering of components in
				      * the level vectors (see
				      * DoFRenumbering::component_wise). It
				      * also affects the behavior of
				      * the <tt>selected</tt> argument
				      */
    template <int dim>
    void build_matrices (const DoFHandler<dim> &dof,
			 const MGDoFHandler<dim> &mg_dof,
			 const std::vector<unsigned int>& target_component
			 = std::vector<unsigned int>(),
			 const std::vector<unsigned int>& mg_target_component
			 =std::vector<unsigned int>());

    virtual void prolongate (const unsigned int    to_level,
			     BlockVector<number>       &dst,
			     const BlockVector<number> &src) const;

    virtual void restrict_and_add (const unsigned int    from_level,
				   BlockVector<number>       &dst,
				   const BlockVector<number> &src) const;

    				     /**
				      * Transfer from a vector on the
				      * global grid to a multilevel
				      * vector.
				      *
				      * The action for discontinuous
				      * elements is as follows: on an
				      * active mesh cell, the global
				      * vector entries are simply
				      * copied to the corresponding
				      * entries of the level
				      * vector. Then, these values are
				      * restricted down to the
				      * coarsest level.
				      *
				      * If the arguments
				      * <tt>target_component</tt> and
				      * <tt>mg_target_component</tt>
				      * was used in build_matrices(),
				      * then the blocks are shuffled
				      * around accordingly.
				      */
    template <int dim, class InVector>
    void
    copy_to_mg (const MGDoFHandler<dim>             &mg_dof,
		MGLevelObject<BlockVector<number> > &dst,
		const InVector                      &src) const;

				     /**
				      * Transfer from multi-level vector to
				      * normal vector.
				      *
				      * Copies data from active
				      * portions of a multilevel
				      * vector into the respective
				      * positions of a global vector.
				      *
				      * If the arguments
				      * <tt>target_component</tt> and
				      * <tt>mg_target_component</tt>
				      * was used in build_matrices(),
				      * then the blocks are shuffled
				      * around accordingly.
				      */
    template <int dim, class OutVector>
    void
    copy_from_mg (const MGDoFHandler<dim>                   &mg_dof,
		  OutVector                                 &dst,
		  const MGLevelObject<BlockVector<number> > &src) const;

				     /**
				      * Add a multi-level vector to a
				      * normal vector.
				      *
				      * Works as the previous
				      * function, but probably not for
				      * continuous elements.
				      */
    template <int dim, class OutVector>
    void
    copy_from_mg_add (const MGDoFHandler<dim>                   &mg_dof,
		      OutVector                                 &dst,
		      const MGLevelObject<BlockVector<number> > &src) const;

    MGTransferBlockBase::memory_consumption;
    
  private:
				     /**
				      * Optional multiplication
				      * factors for each
				      * component. Requires
				      * initialization of #memory.
				      */
    std::vector<number> factors;

				     /**
				      * Memory pool required if
				      * additional multiplication
				      * using #factors is desired.
				      */
    SmartPointer<VectorMemory<Vector<number> > > memory;
};


//TODO:[GK] Update documentation for copy_* functions

/**
 * Implementation of the MGTransferBase interface for block
 * matrices and simple vectors. This class uses MGTransferBlockBase
 * selecting a single component or grouping several components into a
 * single block. The transfer operators themselves are implemented for
 * Vector and BlockVector objects.
 *
 * See MGTransferBase to find out which of the transfer classes
 * is best for your needs.
 *
 * @author Guido Kanschat, 2001, 2002, 2003
 */
template <typename number>
class MGTransferSelect : public MGTransferBase<Vector<number> >,
			 private MGTransferBlockBase
{
  public:
				     /**
				      * Destructor.
				      */
    virtual ~MGTransferSelect ();
    
				     /**
				      * Actually build the prolongation
				      * matrices for grouped components.
				      *
				      * This function is a front-end
				      * for the same function in
				      * MGTransferBlockBase.
				      *
				      * @arg selected: Number of the
				      * component of the global vector
				      * to be copied from and to the
				      * multilevel vector. This number
				      * refers to the renumbering by
				      * <tt>target_component</tt>.
				      *
				      * @arg mg_selected: Number
				      * of the component for which the
				      * transfer matrices should be
				      * built.
				      *
				      * If <tt>mg_target_component</tt> is
				      * present, this refers to the
				      * renumbered components.
				      *
				      * @arg target_component: this
				      * argument allows grouping and
				      * renumbering of components in
				      * the fine-level vector (see
				      * DoFRenumbering::component_wise).
				      * 
				      * @arg mg_target_component: this
				      * argument allows grouping and
				      * renumbering of components in
				      * the level vectors (see
				      * DoFRenumbering::component_wise). It
				      * also affects the behavior of
				      * the <tt>selected</tt> argument
				      */
    template <int dim>
    void build_matrices (const DoFHandler<dim> &dof,
			 const MGDoFHandler<dim> &mg_dof,
			 unsigned int selected,
			 unsigned int mg_selected,
			 const std::vector<unsigned int>& target_component
			 = std::vector<unsigned int>(),
			 const std::vector<unsigned int>& mg_target_component
			 = std::vector<unsigned int>());

				     /**
				      * Change selected
				      * component. Handle with care!
				      */
    void select (const unsigned int component,
		 const unsigned int mg_component = deal_II_numbers::invalid_unsigned_int);
    
    virtual void prolongate (const unsigned int    to_level,
			     Vector<number>       &dst,
			     const Vector<number> &src) const;

    virtual void restrict_and_add (const unsigned int    from_level,
				   Vector<number>       &dst,
				   const Vector<number> &src) const;
    
    				     /**
				      * Transfer from a vector on the
				      * global grid to a multilevel vector.
				      */
    template <int dim, typename number2>
    void
    copy_to_mg (const MGDoFHandler<dim>        &mg_dof,
		MGLevelObject<Vector<number> > &dst,
		const Vector<number2>          &src) const;

				     /**
				      * Transfer from multilevel vector to
				      * normal vector.
				      *
				      * Copies data from active
				      * portions of an multilevel
				      * vector into the respective
				      * positions of a Vector.
				      */
    template <int dim, typename number2>
    void
    copy_from_mg (const MGDoFHandler<dim>              &mg_dof,
		  Vector<number2>                      &dst,
		  const MGLevelObject<Vector<number> > &src) const;

				     /**
				      * Add a multi-level vector to a
				      * normal vector.
				      *
				      * Works as the previous
				      * function, but probably not for
				      * continuous elements.
				      */
    template <int dim, typename number2>
    void
    copy_from_mg_add (const MGDoFHandler<dim>              &mg_dof,
		      Vector<number2>                      &dst,
		      const MGLevelObject<Vector<number> > &src) const;

    				     /**
				      * Transfer from a vector on the
				      * global grid to multilevel vectors.
				      */
    template <int dim, typename number2>
    void
    copy_to_mg (const MGDoFHandler<dim>        &mg_dof,
		MGLevelObject<Vector<number> > &dst,
		const BlockVector<number2>     &src) const;

				     /**
				      * Transfer from multilevel vector to
				      * normal vector.
				      *
				      * Copies data from active
				      * portions of a multilevel
				      * vector into the respective
				      * positions of a global
				      * BlockVector.
				      */
    template <int dim, typename number2>
    void
    copy_from_mg (const MGDoFHandler<dim>              &mg_dof,
		  BlockVector<number2>                 &dst,
		  const MGLevelObject<Vector<number> > &src) const;

				     /**
				      * Add a multi-level vector to a
				      * normal vector.
				      *
				      * Works as the previous
				      * function, but probably not for
				      * continuous elements.
				      */
    template <int dim, typename number2>
    void
    copy_from_mg_add (const MGDoFHandler<dim>              &mg_dof,
		      BlockVector<number2>                 &dst,
		      const MGLevelObject<Vector<number> > &src) const;

				     /**
				      * Memory used by this object.
				      */
    unsigned int memory_consumption () const;
    
  private:
				     /**
				      * Implementation of the public
				      * function.
				      */
    template <int dim, class OutVector>
    void
    do_copy_from_mg (const MGDoFHandler<dim>              &mg_dof,
		     OutVector                            &dst,
		     const MGLevelObject<Vector<number> > &src,
		     const unsigned int offset) const;

				     /**
				      * Implementation of the public
				      * function.
				      */
    template <int dim, class OutVector>
    void
    do_copy_from_mg_add (const MGDoFHandler<dim>              &mg_dof,
			 OutVector                            &dst,
			 const MGLevelObject<Vector<number> > &src,
			 const unsigned int offset) const;

				     /**
				      * Actual implementation of
				      * copy_to_mg().
				      */
    template <int dim, class InVector>
    void
    do_copy_to_mg (const MGDoFHandler<dim>        &mg_dof,
		   MGLevelObject<Vector<number> > &dst,
		   const InVector                 &src,
		   const unsigned int              offset) const;
                                     /**
                                      * Selected component of global vector.
                                      */
    unsigned int selected_component;
                                     /**
                                      * Selected component inside multigrid.
                                      */
    unsigned int mg_selected_component;
};

/*@}*/

//---------------------------------------------------------------------------
template <typename number>
inline void
MGTransferSelect<number>::select(const unsigned int component,
				 const unsigned int mg_component)
{
  selected_component = component;
  mg_selected_component = (mg_component == deal_II_numbers::invalid_unsigned_int)
			  ? component
			  : mg_component;
}

DEAL_II_NAMESPACE_CLOSE

#endif
