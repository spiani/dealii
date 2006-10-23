//---------------------------------------------------------------------------
//    $Id$
//    Version: $Name$
//
//    Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//---------------------------------------------------------------------------

#include <base/path_search.h>

#include <grid/grid_in.h>
#include <grid/tria.h>
#include <grid/grid_reordering.h>
#include <grid/grid_tools.h>

#include <map>
#include <algorithm>
#include <fstream>

#ifdef HAVE_LIBNETCDF
#include <netcdfcpp.h>
#endif

DEAL_II_NAMESPACE_OPEN


template <int dim>
GridIn<dim>::GridIn () :
		tria(0), default_format(ucd)
{}


template <int dim>
void GridIn<dim>::attach_triangulation (Triangulation<dim> &t)
{
  tria = &t;
}


template <int dim>
void GridIn<dim>::read_ucd (std::istream &in)
{
  Assert (tria != 0, ExcNoTriangulationSelected());
  AssertThrow (in, ExcIO());
  
				   // skip comments at start of file
  skip_comment_lines (in, '#');


  unsigned int n_vertices;
  unsigned int n_cells;
  int dummy;

  in >> n_vertices
     >> n_cells
     >> dummy         // number of data vectors
     >> dummy         // cell data
     >> dummy;        // model data

				   // set up array of vertices
  std::vector<Point<dim> >     vertices (n_vertices);
				   // set up mapping between numbering
				   // in ucd-file (key) and in the
				   // vertices vector
  std::map<int,int> vertex_indices;
  
  for (unsigned int vertex=0; vertex<n_vertices; ++vertex) 
    {
      int vertex_number;
      double x[3];

				       // read vertex
      in >> vertex_number
	 >> x[0] >> x[1] >> x[2];

				       // store vertex
      for (unsigned int d=0; d<dim; ++d)
	vertices[vertex](d) = x[d];
				       // store mapping; note that
				       // vertices_indices[i] is automatically
				       // created upon first usage
      vertex_indices[vertex_number] = vertex;
    };

				   // set up array of cells
  std::vector<CellData<dim> > cells;
  SubCellData                 subcelldata;

  for (unsigned int cell=0; cell<n_cells; ++cell) 
    {
				       // note that since in the input
				       // file we found the number of
				       // cells at the top, there
				       // should still be input here,
				       // so check this:
      AssertThrow (in, ExcIO());
      
      std::string cell_type;
      int material_id;
      
      in >> dummy          // cell number
	 >> material_id;
      in >> cell_type;

      if (((cell_type == "line") && (dim == 1)) ||
	  ((cell_type == "quad") && (dim == 2)) ||
	  ((cell_type == "hex" ) && (dim == 3)))
					 // found a cell
	{
					   // allocate and read indices
	  cells.push_back (CellData<dim>());
	  for (unsigned int i=0; i<GeometryInfo<dim>::vertices_per_cell; ++i)
	    in >> cells.back().vertices[i];
	  cells.back().material_id = material_id;

					   // transform from ucd to
					   // consecutive numbering
	  for (unsigned int i=0; i<GeometryInfo<dim>::vertices_per_cell; ++i)
	    if (vertex_indices.find (cells.back().vertices[i]) != vertex_indices.end())
					       // vertex with this index exists
	      cells.back().vertices[i] = vertex_indices[cells.back().vertices[i]];
	    else 
	      {
						 // no such vertex index
		AssertThrow (false, ExcInvalidVertexIndex(cell, cells.back().vertices[i]));
		cells.back().vertices[i] = deal_II_numbers::invalid_unsigned_int;
	      };
	}
      else
	if ((cell_type == "line") && ((dim == 2) || (dim == 3)))
					   // boundary info
	  {
	    subcelldata.boundary_lines.push_back (CellData<1>());
	    in >> subcelldata.boundary_lines.back().vertices[0]
	       >> subcelldata.boundary_lines.back().vertices[1];
	    subcelldata.boundary_lines.back().material_id = material_id;

					     // transform from ucd to
					     // consecutive numbering
	    for (unsigned int i=0; i<2; ++i)
	      if (vertex_indices.find (subcelldata.boundary_lines.back().vertices[i]) !=
		  vertex_indices.end())
						 // vertex with this index exists
		subcelldata.boundary_lines.back().vertices[i]
		  = vertex_indices[subcelldata.boundary_lines.back().vertices[i]];
	      else 
		{
						   // no such vertex index
		  AssertThrow (false,
			       ExcInvalidVertexIndex(cell,
						     subcelldata.boundary_lines.back().vertices[i]));
		  subcelldata.boundary_lines.back().vertices[i]
		    = deal_II_numbers::invalid_unsigned_int;
		};
	  }
	else
	  if ((cell_type == "quad") && (dim == 3))
					     // boundary info
	    {
 	      subcelldata.boundary_quads.push_back (CellData<2>());
 	      in >> subcelldata.boundary_quads.back().vertices[0]
 	         >> subcelldata.boundary_quads.back().vertices[1]
 		 >> subcelldata.boundary_quads.back().vertices[2]
 		 >> subcelldata.boundary_quads.back().vertices[3];
	      
 	      subcelldata.boundary_quads.back().material_id = material_id;
	      
					       // transform from ucd to
					       // consecutive numbering
 	      for (unsigned int i=0; i<4; ++i)
 	        if (vertex_indices.find (subcelldata.boundary_quads.back().vertices[i]) !=
 		    vertex_indices.end())
 		                                   // vertex with this index exists
		  subcelldata.boundary_quads.back().vertices[i]
		    = vertex_indices[subcelldata.boundary_quads.back().vertices[i]];
 	        else
 	          {
						     // no such vertex index
 	            Assert (false,
 		            ExcInvalidVertexIndex(cell,
 			                          subcelldata.boundary_quads.back().vertices[i]));
 		    subcelldata.boundary_quads.back().vertices[i] =
		      deal_II_numbers::invalid_unsigned_int;
 		  };
	      
	    }
	  else
					     // cannot read this
	    AssertThrow (false, ExcUnknownIdentifier(cell_type));
    };

  
				   // check that no forbidden arrays are used
  Assert (subcelldata.check_consistency(dim), ExcInternalError());

  AssertThrow (in, ExcIO());

				   // do some clean-up on vertices...
  GridTools::delete_unused_vertices (vertices, cells, subcelldata);
				   // ... and cells
  GridReordering<dim>::invert_all_cells_of_negative_grid (vertices, cells);
  GridReordering<dim>::reorder_cells (cells);
  tria->create_triangulation_compatibility (vertices, cells, subcelldata);
}



template <int dim>
void GridIn<dim>::read_dbmesh (std::istream &in)
{
  Assert (tria != 0, ExcNoTriangulationSelected());
  Assert (dim==2, ExcNotImplemented());

  AssertThrow (in, ExcIO());
  
				   // skip comments at start of file
  skip_comment_lines (in, '#');


				   // first read in identifier string
  std::string line;
  getline (in, line);

  AssertThrow (line=="MeshVersionFormatted 0",
	       ExcInvalidDBMESHInput(line));

  skip_empty_lines (in);

				   // next read dimension
  getline (in, line);
  AssertThrow (line=="Dimension", ExcInvalidDBMESHInput(line));
  unsigned int dimension;
  in >> dimension;
  AssertThrow (dimension == dim, ExcDBMESHWrongDimension(dimension));
  skip_empty_lines (in);

				   // now there are a lot of fields of
				   // which we don't know the exact
				   // meaning and which are far from
				   // being properly documented in the
				   // manual. we skip everything until
				   // we find a comment line with the
				   // string "# END". at some point in
				   // the future, someone may have the
				   // knowledge to parse and interpret
				   // the other fields in between as
				   // well...
  while (getline(in,line), line.find("# END")==std::string::npos);
  skip_empty_lines (in);


				   // now read vertices
  getline (in, line);
  AssertThrow (line=="Vertices", ExcInvalidDBMESHInput(line));
  
  unsigned int n_vertices;
  double dummy;
  
  in >> n_vertices;
  std::vector<Point<dim> >     vertices (n_vertices);
  for (unsigned int vertex=0; vertex<n_vertices; ++vertex)
    {
				       // read vertex coordinates
      for (unsigned int d=0; d<dim; ++d)
	in >> vertices[vertex][d];
				       // read Ref phi_i, whatever that may be
      in >> dummy;
    };
  AssertThrow (in, ExcInvalidDBMeshFormat());

  skip_empty_lines(in);

				   // read edges. we ignore them at
				   // present, so just read them and
				   // discard the input
  getline (in, line);
  AssertThrow (line=="Edges", ExcInvalidDBMESHInput(line));
  
  unsigned int n_edges;
  in >> n_edges;
  for (unsigned int edge=0; edge<n_edges; ++edge)
    {
				       // read vertex indices
      in >> dummy >> dummy;
				       // read Ref phi_i, whatever that may be
      in >> dummy;
    };
  AssertThrow (in, ExcInvalidDBMeshFormat());

  skip_empty_lines(in);



				   // read cracked edges (whatever
				   // that may be). we ignore them at
				   // present, so just read them and
				   // discard the input
  getline (in, line);
  AssertThrow (line=="CrackedEdges", ExcInvalidDBMESHInput(line));
  
  in >> n_edges;
  for (unsigned int edge=0; edge<n_edges; ++edge)
    {
				       // read vertex indices
      in >> dummy >> dummy;
				       // read Ref phi_i, whatever that may be
      in >> dummy;
    };
  AssertThrow (in, ExcInvalidDBMeshFormat());

  skip_empty_lines(in);


				   // now read cells.
				   // set up array of cells
  getline (in, line);
  AssertThrow (line=="Quadrilaterals", ExcInvalidDBMESHInput(line));

  std::vector<CellData<dim> > cells;
  SubCellData            subcelldata;
  unsigned int n_cells;
  in >> n_cells;
  for (unsigned int cell=0; cell<n_cells; ++cell) 
    {
				       // read in vertex numbers. they
				       // are 1-based, so subtract one
      cells.push_back (CellData<dim>());
      for (unsigned int i=0; i<GeometryInfo<dim>::vertices_per_cell; ++i)
	{
	  in >> cells.back().vertices[i];
	  
	  AssertThrow ((cells.back().vertices[i] >= 1)
		       &&
		       (static_cast<unsigned int>(cells.back().vertices[i]) <= vertices.size()),
		       ExcInvalidVertexIndex(cell, cells.back().vertices[i]));
		  
	  --cells.back().vertices[i];
	};

				       // read and discard Ref phi_i
      in >> dummy;
    };
  AssertThrow (in, ExcInvalidDBMeshFormat());

  skip_empty_lines(in);
  

				   // then there are again a whole lot
				   // of fields of which I have no
				   // clue what they mean. skip them
				   // all and leave the interpretation
				   // to other implementors...
  while (getline(in,line), ((line.find("End")==std::string::npos) && (in)));
				   // ok, so we are not at the end of
				   // the file, that's it, mostly

  
				   // check that no forbidden arrays are used
  Assert (subcelldata.check_consistency(dim), ExcInternalError());

  AssertThrow (in, ExcIO());

				   // do some clean-up on vertices...
  GridTools::delete_unused_vertices (vertices, cells, subcelldata);
				   // ...and cells
  GridReordering<dim>::invert_all_cells_of_negative_grid (vertices, cells);
  GridReordering<dim>::reorder_cells (cells);
  tria->create_triangulation_compatibility (vertices, cells, subcelldata);
}



template <int dim>
void GridIn<dim>::read_xda (std::istream &)
{
  Assert (false, ExcNotImplemented());
}



// 2D XDA meshes
#if deal_II_dimension == 2

template <>
void GridIn<2>::read_xda (std::istream &in)
{
  Assert (tria != 0, ExcNoTriangulationSelected());
  AssertThrow (in, ExcIO());

  std::string line;
				   // skip comments at start of file
  getline (in, line);


  unsigned int n_vertices;
  unsigned int n_cells;

				   // read cells, throw away rest of line
  in >> n_cells;
  getline (in, line);

  in >> n_vertices;
  getline (in, line);

				   // ignore following 8 lines
  for (unsigned int i=0; i<8; ++i)
    getline (in, line);

				   // set up array of cells
  std::vector<CellData<2> > cells (n_cells);
  SubCellData subcelldata;

  for (unsigned int cell=0; cell<n_cells; ++cell) 
    {
				       // note that since in the input
				       // file we found the number of
				       // cells at the top, there
				       // should still be input here,
				       // so check this:
      AssertThrow (in, ExcIO());
      Assert (GeometryInfo<2>::vertices_per_cell == 4,
	      ExcInternalError());
      
      for (unsigned int i=0; i<4; ++i)
	in >> cells[cell].vertices[i];
    };


   
				   // set up array of vertices
  std::vector<Point<2> > vertices (n_vertices);
  for (unsigned int vertex=0; vertex<n_vertices; ++vertex) 
    {
      double x[3];

				       // read vertex
      in >> x[0] >> x[1] >> x[2];

				       // store vertex
      for (unsigned int d=0; d<2; ++d)
	vertices[vertex](d) = x[d];
    };
  AssertThrow (in, ExcIO());

				   // do some clean-up on vertices...
  GridTools::delete_unused_vertices (vertices, cells, subcelldata);
				   // ... and cells
  GridReordering<2>::invert_all_cells_of_negative_grid (vertices, cells);
  GridReordering<2>::reorder_cells (cells);
  tria->create_triangulation_compatibility (vertices, cells, subcelldata);
}

#endif // #if deal_II_dimension == 2



// 3-D XDA meshes
#if deal_II_dimension == 3

template <>
void GridIn<3>::read_xda (std::istream &in)
{
  Assert (tria != 0, ExcNoTriangulationSelected());
  AssertThrow (in, ExcIO());

  static const unsigned int xda_to_dealII_map[] = {0,1,5,4,3,2,6,7};
  
  std::string line;
				   // skip comments at start of file
  getline (in, line);


  unsigned int n_vertices;
  unsigned int n_cells;

				   // read cells, throw away rest of line
  in >> n_cells;
  getline (in, line);

  in >> n_vertices;
  getline (in, line);

				   // ignore following 8 lines
  for (unsigned int i=0; i<8; ++i)
    getline (in, line);

				   // set up array of cells
  std::vector<CellData<3> > cells (n_cells);
  SubCellData subcelldata;

  for (unsigned int cell=0; cell<n_cells; ++cell) 
    {
				       // note that since in the input
				       // file we found the number of
				       // cells at the top, there
				       // should still be input here,
				       // so check this:
      AssertThrow (in, ExcIO());
      Assert(GeometryInfo<3>::vertices_per_cell == 8,
	     ExcInternalError());
      
      unsigned int xda_ordered_nodes[8];
      
      for (unsigned int i=0; i<8; ++i)
	in >> xda_ordered_nodes[i];

      for (unsigned int i=0; i<8; i++)
	cells[cell].vertices[i] = xda_ordered_nodes[xda_to_dealII_map[i]];
    };


  
				   // set up array of vertices
  std::vector<Point<3> > vertices (n_vertices);
  for (unsigned int vertex=0; vertex<n_vertices; ++vertex) 
    {
      double x[3];

				       // read vertex
      in >> x[0] >> x[1] >> x[2];

				       // store vertex
      for (unsigned int d=0; d<3; ++d)
	vertices[vertex](d) = x[d];
    };
  AssertThrow (in, ExcIO());

				   // do some clean-up on vertices...
  GridTools::delete_unused_vertices (vertices, cells, subcelldata);
				   // ... and cells
  GridReordering<3>::invert_all_cells_of_negative_grid (vertices, cells);
  GridReordering<3>::reorder_cells (cells);
  tria->create_triangulation_compatibility (vertices, cells, subcelldata);
}

#endif // #if deal_II_dimension == 3



template <int dim>
void GridIn<dim>::read_msh (std::istream &in)
{
  Assert (tria != 0, ExcNoTriangulationSelected());
  AssertThrow (in, ExcIO());

  unsigned int n_vertices;
  unsigned int n_cells;
  unsigned int dummy;
  std::string line;
  
  getline(in, line);

  AssertThrow (line=="$NOD",
	       ExcInvalidGMSHInput(line));

  in >> n_vertices;

  std::vector<Point<dim> >     vertices (n_vertices);
				   // set up mapping between numbering
				   // in msh-file (nod) and in the
				   // vertices vector
  std::map<int,int> vertex_indices;
  
  for (unsigned int vertex=0; vertex<n_vertices; ++vertex) 
    {
      int vertex_number;
      double x[3];

				       // read vertex
      in >> vertex_number
	 >> x[0] >> x[1] >> x[2];
     
      for (unsigned int d=0; d<dim; ++d)
	vertices[vertex](d) = x[d];
				       // store mapping;
      vertex_indices[vertex_number] = vertex;
    };
  
				   // This is needed to flush the last
				   // new line
  getline (in, line);
  
				   // Now read in next bit
  getline (in, line);
  AssertThrow (line=="$ENDNOD",
	       ExcInvalidGMSHInput(line));

  
  getline (in, line);
  AssertThrow (line=="$ELM",
	       ExcInvalidGMSHInput(line));

  in >> n_cells;
				   // set up array of cells
  std::vector<CellData<dim> > cells;
  SubCellData                 subcelldata;

  for (unsigned int cell=0; cell<n_cells; ++cell) 
    {
				       // note that since in the input
				       // file we found the number of
				       // cells at the top, there
				       // should still be input here,
				       // so check this:
      AssertThrow (in, ExcIO());
      
/*  
    $ENDNOD
    $ELM
    NUMBER-OF-ELEMENTS
    ELM-NUMBER ELM-TYPE REG-PHYS REG-ELEM NUMBER-OF-NODES NODE-NUMBER-LIST
    ...
    $ENDELM
*/
      
      unsigned int cell_type;
      unsigned int material_id;
      unsigned int nod_num;
      
      in >> dummy          // ELM-NUMBER
	 >> cell_type	   // ELM-TYPE
	 >> material_id	   // REG-PHYS
	 >> dummy	   // reg_elm
	 >> nod_num;
      
/*       `ELM-TYPE'
	 defines the geometrical type of the N-th element:
	 `1'
	 Line (2 nodes, 1 edge).
                                                                                                
	 `3'
	 Quadrangle (4 nodes, 4 edges).
                                                                                                
	 `5'
	 Hexahedron (8 nodes, 12 edges, 6 faces).
                                                                                    
	 `15'
	 Point (1 node).
*/
      
      if (((cell_type == 1) && (dim == 1)) ||
	  ((cell_type == 3) && (dim == 2)) ||
	  ((cell_type == 5) && (dim == 3)))
					 // found a cell
	{
					   // allocate and read indices
	  cells.push_back (CellData<dim>());
	  for (unsigned int i=0; i<GeometryInfo<dim>::vertices_per_cell; ++i)
	    in >> cells.back().vertices[i];
	  cells.back().material_id = material_id;

					   // transform from ucd to
					   // consecutive numbering
	  for (unsigned int i=0; i<GeometryInfo<dim>::vertices_per_cell; ++i)
	    if (vertex_indices.find (cells.back().vertices[i]) != vertex_indices.end())
					       // vertex with this index exists
	      cells.back().vertices[i] = vertex_indices[cells.back().vertices[i]];
	    else 
	      {
						 // no such vertex index
		AssertThrow (false, ExcInvalidVertexIndex(cell, cells.back().vertices[i]));
		cells.back().vertices[i] = deal_II_numbers::invalid_unsigned_int;
	      };
	}
      else
	if ((cell_type == 1) && ((dim == 2) || (dim == 3)))
					   // boundary info
	  {
	    subcelldata.boundary_lines.push_back (CellData<1>());
	    in >> subcelldata.boundary_lines.back().vertices[0]
	       >> subcelldata.boundary_lines.back().vertices[1];
	    subcelldata.boundary_lines.back().material_id = material_id;

					     // transform from ucd to
					     // consecutive numbering
	    for (unsigned int i=0; i<2; ++i)
	      if (vertex_indices.find (subcelldata.boundary_lines.back().vertices[i]) !=
		  vertex_indices.end())
						 // vertex with this index exists
		subcelldata.boundary_lines.back().vertices[i]
		  = vertex_indices[subcelldata.boundary_lines.back().vertices[i]];
	      else 
		{
						   // no such vertex index
		  AssertThrow (false,
			       ExcInvalidVertexIndex(cell,
						     subcelldata.boundary_lines.back().vertices[i]));
		  subcelldata.boundary_lines.back().vertices[i] =
		    deal_II_numbers::invalid_unsigned_int;
		};
	  }
	else
	  if ((cell_type == 3) && (dim == 3))
					     // boundary info
	    {
 	      subcelldata.boundary_quads.push_back (CellData<2>());
 	      in >> subcelldata.boundary_quads.back().vertices[0]
 	         >> subcelldata.boundary_quads.back().vertices[1]
 		 >> subcelldata.boundary_quads.back().vertices[2]
 		 >> subcelldata.boundary_quads.back().vertices[3];
	      
 	      subcelldata.boundary_quads.back().material_id = material_id;
	      
					       // transform from gmsh to
					       // consecutive numbering
 	      for (unsigned int i=0; i<4; ++i)
 	        if (vertex_indices.find (subcelldata.boundary_quads.back().vertices[i]) !=
 		    vertex_indices.end())
 		                                   // vertex with this index exists
		  subcelldata.boundary_quads.back().vertices[i]
		    = vertex_indices[subcelldata.boundary_quads.back().vertices[i]];
 	        else
 	          {
						     // no such vertex index
 	            Assert (false,
 		            ExcInvalidVertexIndex(cell,
 			                          subcelldata.boundary_quads.back().vertices[i]));
 		    subcelldata.boundary_quads.back().vertices[i] =
		      deal_II_numbers::invalid_unsigned_int;
 		  };
	      
	    }
	  else
					     // cannot read this
	    AssertThrow (false, ExcGmshUnsupportedGeometry(cell_type));
    };

  
				   // check that no forbidden arrays are used
  Assert (subcelldata.check_consistency(dim), ExcInternalError());

  AssertThrow (in, ExcIO());

				   // do some clean-up on vertices...
  GridTools::delete_unused_vertices (vertices, cells, subcelldata);
				   // ... and cells
  GridReordering<dim>::invert_all_cells_of_negative_grid (vertices, cells);
  GridReordering<dim>::reorder_cells (cells);
  tria->create_triangulation_compatibility (vertices, cells, subcelldata);
}


#if deal_II_dimension == 1

template <>
void GridIn<1>::read_netcdf (const std::string &)
{
  AssertThrow(false, ExcImpossibleInDim(1));
}

#endif



#if deal_II_dimension == 2

template <>
void GridIn<2>::read_netcdf (const std::string &filename)
{
#ifndef HAVE_LIBNETCDF
				   // do something with unused
				   // filename
  filename.c_str();
  AssertThrow(false, ExcNeedsNetCDF());
#else
  const unsigned int dim=2;
  const bool output=false;
  Assert (tria != 0, ExcNoTriangulationSelected());
				   // this function assumes the TAU
				   // grid format.
				   //
				   // This format stores 2d grids as
				   // 3d grids. In particular, a 2d
				   // grid of n_cells quadrilaterals
				   // in the y=0 plane is duplicated
				   // to y=1 to build n_cells
				   // hexaeders.  The surface
				   // quadrilaterals of this 3d grid
				   // are marked with boundary
				   // marker. In the following we read
				   // in all data required, find the
				   // boundary marker associated with
				   // the plane y=0, and extract the
				   // corresponding 2d data to build a
				   // Triangulation<2>.

				   // In the following, we assume that
				   // the 2d grid lies in the x-z
				   // plane (y=0). I.e. we choose:
				   // point[coord]=0, with coord=1
  const unsigned int coord=1;
				   // Also x-y-z (0-1-2) point
				   // coordinates will be transformed
				   // to x-y (x2d-y2d) coordinates.
				   // With coord=1 as above, we have
				   // x-z (0-2) -> (x2d-y2d)
  const unsigned int x2d=0;
  const unsigned int y2d=2;
				   // For the case, the 2d grid lies
				   // in x-y or y-z plane instead, the
				   // following code must be extended
				   // to find the right value for
				   // coord, and setting x2d and y2d
				   // accordingly.
  
				   // First, open the file
  NcFile nc (filename.c_str());
  AssertThrow(nc.is_valid(), ExcIO());

				   // then read n_cells
  NcDim *elements_dim=nc.get_dim("no_of_elements");
  AssertThrow(elements_dim->is_valid(), ExcIO());
  const unsigned int n_cells=elements_dim->size();

				   // then we read
				   //   int marker(no_of_markers)
  NcDim *marker_dim=nc.get_dim("no_of_markers");
  AssertThrow(marker_dim->is_valid(), ExcIO());
  const unsigned int n_markers=marker_dim->size();

  NcVar *marker_var=nc.get_var("marker");
  AssertThrow(marker_var->is_valid(), ExcIO());
  AssertThrow(marker_var->num_dims()==1, ExcIO());
  AssertThrow(static_cast<unsigned int>(
    marker_var->get_dim(0)->size())==n_markers, ExcIO());

  std::vector<int> marker(n_markers);
				   // use &* to convert
				   // vector<int>::iterator to int *
  marker_var->get(&*marker.begin(), n_markers);
  
  if (output)
    {
      std::cout << "n_cell=" << n_cells << std::endl;
      std::cout << "marker: ";
      for (unsigned int i=0; i<n_markers; ++i)
	std::cout << marker[i] << " ";
      std::cout << std::endl;
    }

				   // next we read
				   // int boundarymarker_of_surfaces(
				   //   no_of_surfaceelements)
  NcDim *bquads_dim=nc.get_dim("no_of_surfacequadrilaterals");
  AssertThrow(bquads_dim->is_valid(), ExcIO());
  const unsigned int n_bquads=bquads_dim->size();

  NcVar *bmarker_var=nc.get_var("boundarymarker_of_surfaces");
  AssertThrow(bmarker_var->is_valid(), ExcIO());
  AssertThrow(bmarker_var->num_dims()==1, ExcIO());
  AssertThrow(static_cast<unsigned int>(
    bmarker_var->get_dim(0)->size())==n_bquads, ExcIO());

  std::vector<int> bmarker(n_bquads);
  bmarker_var->get(&*bmarker.begin(), n_bquads);

				       // for each marker count the
				       // number of boundary quads
				       // which carry this marker
  std::map<int, unsigned int> n_bquads_per_bmarker;
  for (unsigned int i=0; i<n_markers; ++i)
    {
				       // the markers should all be
				       // different
      AssertThrow(n_bquads_per_bmarker.find(marker[i])==
		  n_bquads_per_bmarker.end(), ExcIO());
      
      n_bquads_per_bmarker[marker[i]]=
	count(bmarker.begin(), bmarker.end(), marker[i]);
    }
				   // Note: the n_bquads_per_bmarker
				   // map could be used to find the
				   // right coord by finding the
				   // marker0 such that
				   // a/ n_bquads_per_bmarker[marker0]==n_cells
				   // b/ point[coord]==0,
				   // Condition a/ would hold for at
				   // least two markers, marker0 and
				   // marker1, whereas b/ would hold
				   // for marker0 only. For marker1 we
				   // then had point[coord]=constant
				   // with e.g. constant=1 or -1
  if (output)
    {
      std::cout << "n_bquads_per_bmarker: " << std::endl;
      std::map<int, unsigned int>::const_iterator
	iter=n_bquads_per_bmarker.begin();
      for (; iter!=n_bquads_per_bmarker.end(); ++iter)
	std::cout << "  n_bquads_per_bmarker[" << iter->first
		  << "]=" << iter->second << std::endl;
    }

				   // next we read
				   // int points_of_surfacequadrilaterals(
				   //   no_of_surfacequadrilaterals,
				   //   points_per_surfacequadrilateral)
  NcDim *quad_vertices_dim=nc.get_dim("points_per_surfacequadrilateral");
  AssertThrow(quad_vertices_dim->is_valid(), ExcIO());
  const unsigned int vertices_per_quad=quad_vertices_dim->size();
  AssertThrow(vertices_per_quad==GeometryInfo<dim>::vertices_per_cell, ExcIO());
  
  NcVar *vertex_indices_var=nc.get_var("points_of_surfacequadrilaterals");
  AssertThrow(vertex_indices_var->is_valid(), ExcIO());
  AssertThrow(vertex_indices_var->num_dims()==2, ExcIO());
  AssertThrow(static_cast<unsigned int>(
    vertex_indices_var->get_dim(0)->size())==n_bquads, ExcIO());
  AssertThrow(static_cast<unsigned int>(
    vertex_indices_var->get_dim(1)->size())==vertices_per_quad, ExcIO());

  std::vector<int> vertex_indices(n_bquads*vertices_per_quad);
  vertex_indices_var->get(&*vertex_indices.begin(), n_bquads, vertices_per_quad);

  for (unsigned int i=0; i<vertex_indices.size(); ++i)
    AssertThrow(vertex_indices[i]>=0, ExcInternalError());

  if (output)
    {
      std::cout << "vertex_indices:" << std::endl;
      for (unsigned int i=0, v=0; i<n_bquads; ++i)
	{
	  for (unsigned int j=0; j<vertices_per_quad; ++j)
	    std::cout << vertex_indices[v++] << " ";
	  std::cout << std::endl;
	}
    }

				   // next we read
				   //   double points_xc(no_of_points)
				   //   double points_yc(no_of_points)
				   //   double points_zc(no_of_points)
  NcDim *vertices_dim=nc.get_dim("no_of_points");
  AssertThrow(vertices_dim->is_valid(), ExcIO());
  const unsigned int n_vertices=vertices_dim->size();
  if (output)
    std::cout << "n_vertices=" << n_vertices << std::endl;

  NcVar *points_xc=nc.get_var("points_xc");
  NcVar *points_yc=nc.get_var("points_yc");
  NcVar *points_zc=nc.get_var("points_zc");
  AssertThrow(points_xc->is_valid(), ExcIO());
  AssertThrow(points_yc->is_valid(), ExcIO());
  AssertThrow(points_zc->is_valid(), ExcIO());
  AssertThrow(points_xc->num_dims()==1, ExcIO());
  AssertThrow(points_yc->num_dims()==1, ExcIO());
  AssertThrow(points_zc->num_dims()==1, ExcIO());
  AssertThrow(points_yc->get_dim(0)->size()==
	      static_cast<int>(n_vertices), ExcIO());
  AssertThrow(points_zc->get_dim(0)->size()==
	      static_cast<int>(n_vertices), ExcIO());
  AssertThrow(points_xc->get_dim(0)->size()==
	      static_cast<int>(n_vertices), ExcIO());
  std::vector<std::vector<double> > point_values(
    3, std::vector<double> (n_vertices));
  points_xc->get(&*point_values[0].begin(), n_vertices);
  points_yc->get(&*point_values[1].begin(), n_vertices);
  points_zc->get(&*point_values[2].begin(), n_vertices);

				   // and fill the vertices
  std::vector<Point<dim> > vertices (n_vertices);
  for (unsigned int i=0; i<n_vertices; ++i)
    {
      vertices[i](0)=point_values[x2d][i];
      vertices[i](1)=point_values[y2d][i];
    }

				   // Run over all boundary quads
				   // until we find a quad in the
				   // point[coord]=0 plane
  unsigned int quad0=0;
  for (;quad0<n_bquads; ++quad0)
    {
      bool in_plane=true;
      for (unsigned int i=0; i<vertices_per_quad; ++i)
	if (point_values[coord][vertex_indices[quad0*vertices_per_quad+i]]!=0)
	  {
	    in_plane=false;
	    break;
	  }
      
      if (in_plane)
	break;
    }
  const int bmarker0=bmarker[quad0];
  if (output)
    std::cout << "bmarker0=" << bmarker0 << std::endl;
  AssertThrow(n_bquads_per_bmarker[bmarker0]==n_cells, ExcIO());

				   // fill cells with all quad
				   // associated with bmarker0
  std::vector<CellData<dim> > cells(n_cells);
  for (unsigned int quad=0, cell=0; quad<n_bquads; ++quad)
    if (bmarker[quad]==bmarker0)
      {
	for (unsigned int i=0; i<vertices_per_quad; ++i)
	  {
	    Assert(point_values[coord][vertex_indices[
	      quad*vertices_per_quad+i]]==0, ExcNotImplemented());
	    cells[cell].vertices[i]=vertex_indices[quad*vertices_per_quad+i];
	  }
	++cell;
      }

  SubCellData subcelldata;
  GridTools::delete_unused_vertices(vertices, cells, subcelldata);
  GridReordering<dim>::reorder_cells (cells);
  tria->create_triangulation_compatibility (vertices, cells, subcelldata);  
#endif
}

#endif

#if deal_II_dimension == 3

template <>
void GridIn<3>::read_netcdf (const std::string &filename)
{
#ifndef HAVE_LIBNETCDF
                                   // do something with the function argument
                                   // to make sure it at least looks used,
                                   // even if it is not
  (void)filename;
  AssertThrow(false, ExcNeedsNetCDF());
#else
  const unsigned int dim=3;
  const bool output=false;
  Assert (tria != 0, ExcNoTriangulationSelected());
				   // this function assumes the TAU
				   // grid format.
  
				   // First, open the file
  NcFile nc (filename.c_str());
  AssertThrow(nc.is_valid(), ExcIO());

				   // then read n_cells
  NcDim *elements_dim=nc.get_dim("no_of_elements");
  AssertThrow(elements_dim->is_valid(), ExcIO());
  const unsigned int n_cells=elements_dim->size();  
  if (output)
    std::cout << "n_cell=" << n_cells << std::endl;
				   // and n_hexes
  NcDim *hexes_dim=nc.get_dim("no_of_hexaeders");
  AssertThrow(hexes_dim->is_valid(), ExcIO());
  const unsigned int n_hexes=hexes_dim->size();
  AssertThrow(n_hexes==n_cells,
	      ExcMessage("deal.II can handle purely hexaedral grids, only."));
  
				   // next we read
				   // int points_of_hexaeders(
				   //   no_of_hexaeders,
				   //   points_per_hexaeder)
  NcDim *hex_vertices_dim=nc.get_dim("points_per_hexaeder");
  AssertThrow(hex_vertices_dim->is_valid(), ExcIO());
  const unsigned int vertices_per_hex=hex_vertices_dim->size();
  AssertThrow(vertices_per_hex==GeometryInfo<dim>::vertices_per_cell, ExcIO());
  
  NcVar *vertex_indices_var=nc.get_var("points_of_hexaeders");
  AssertThrow(vertex_indices_var->is_valid(), ExcIO());
  AssertThrow(vertex_indices_var->num_dims()==2, ExcIO());
  AssertThrow(static_cast<unsigned int>(
    vertex_indices_var->get_dim(0)->size())==n_cells, ExcIO());
  AssertThrow(static_cast<unsigned int>(
    vertex_indices_var->get_dim(1)->size())==vertices_per_hex, ExcIO());

  std::vector<int> vertex_indices(n_cells*vertices_per_hex);
  				   // use &* to convert
				   // vector<int>::iterator to int *
  vertex_indices_var->get(&*vertex_indices.begin(), n_cells, vertices_per_hex);

  for (unsigned int i=0; i<vertex_indices.size(); ++i)
    AssertThrow(vertex_indices[i]>=0, ExcInternalError());

  if (output)
    {
      std::cout << "vertex_indices:" << std::endl;
      for (unsigned int cell=0, v=0; cell<n_cells; ++cell)
	{
	  for (unsigned int i=0; i<vertices_per_hex; ++i)
	    std::cout << vertex_indices[v++] << " ";
	  std::cout << std::endl;
	}
    }

				   // next we read
				   //   double points_xc(no_of_points)
				   //   double points_yc(no_of_points)
				   //   double points_zc(no_of_points)
  NcDim *vertices_dim=nc.get_dim("no_of_points");
  AssertThrow(vertices_dim->is_valid(), ExcIO());
  const unsigned int n_vertices=vertices_dim->size();
  if (output)
    std::cout << "n_vertices=" << n_vertices << std::endl;

  NcVar *points_xc=nc.get_var("points_xc");
  NcVar *points_yc=nc.get_var("points_yc");
  NcVar *points_zc=nc.get_var("points_zc");
  AssertThrow(points_xc->is_valid(), ExcIO());
  AssertThrow(points_yc->is_valid(), ExcIO());
  AssertThrow(points_zc->is_valid(), ExcIO());
  AssertThrow(points_xc->num_dims()==1, ExcIO());
  AssertThrow(points_yc->num_dims()==1, ExcIO());
  AssertThrow(points_zc->num_dims()==1, ExcIO());
  AssertThrow(points_yc->get_dim(0)->size()==
	      static_cast<int>(n_vertices), ExcIO());
  AssertThrow(points_zc->get_dim(0)->size()==
	      static_cast<int>(n_vertices), ExcIO());
  AssertThrow(points_xc->get_dim(0)->size()==
	      static_cast<int>(n_vertices), ExcIO());
  std::vector<std::vector<double> > point_values(
    3, std::vector<double> (n_vertices));
				   // we switch y and z
  const bool switch_y_z=true;
  points_xc->get(&*point_values[0].begin(), n_vertices);
  if (switch_y_z)
    {
      points_yc->get(&*point_values[2].begin(), n_vertices);
      points_zc->get(&*point_values[1].begin(), n_vertices);
    }
  else
    {
      points_yc->get(&*point_values[1].begin(), n_vertices);
      points_zc->get(&*point_values[2].begin(), n_vertices);
    }

				   // and fill the vertices
  std::vector<Point<dim> > vertices (n_vertices);
  for (unsigned int i=0; i<n_vertices; ++i)
    {
      vertices[i](0)=point_values[0][i];
      vertices[i](1)=point_values[1][i];
      vertices[i](2)=point_values[2][i];
    }

				   // and cells
  std::vector<CellData<dim> > cells(n_cells);
  for (unsigned int cell=0; cell<n_cells; ++cell)
    for (unsigned int i=0; i<vertices_per_hex; ++i)
      cells[cell].vertices[i]=vertex_indices[cell*vertices_per_hex+i];

				   // for setting up the SubCellData
				   // we read the vertex indices of
				   // the boundary quadrilaterals and
				   // their boundary markers
  
				   // first we read
				   // int points_of_surfacequadrilaterals(
				   //   no_of_surfacequadrilaterals,
				   //   points_per_surfacequadrilateral)
  NcDim *quad_vertices_dim=nc.get_dim("points_per_surfacequadrilateral");
  AssertThrow(quad_vertices_dim->is_valid(), ExcIO());
  const unsigned int vertices_per_quad=quad_vertices_dim->size();
  AssertThrow(vertices_per_quad==GeometryInfo<dim>::vertices_per_face, ExcIO());
  
  NcVar *bvertex_indices_var=nc.get_var("points_of_surfacequadrilaterals");
  AssertThrow(bvertex_indices_var->is_valid(), ExcIO());
  AssertThrow(bvertex_indices_var->num_dims()==2, ExcIO());
  const unsigned int n_bquads=bvertex_indices_var->get_dim(0)->size();
  AssertThrow(static_cast<unsigned int>(
		bvertex_indices_var->get_dim(1)->size())==
	      GeometryInfo<dim>::vertices_per_face, ExcIO());

  std::vector<int> bvertex_indices(n_bquads*vertices_per_quad);
  bvertex_indices_var->get(&*bvertex_indices.begin(), n_bquads, vertices_per_quad);

  if (output)
    {
      std::cout << "bquads: ";
      for (unsigned int i=0; i<n_bquads; ++i)
	{
	  for (unsigned int v=0; v<GeometryInfo<dim>::vertices_per_face; ++v)
	    std::cout << bvertex_indices[
	      i*GeometryInfo<dim>::vertices_per_face+v] << " ";
	  std::cout << std::endl;
	}
    }

				   // next we read
				   // int boundarymarker_of_surfaces(
				   //   no_of_surfaceelements)
  NcDim *bquads_dim=nc.get_dim("no_of_surfacequadrilaterals");
  AssertThrow(bquads_dim->is_valid(), ExcIO());
  AssertThrow(static_cast<unsigned int>(
		bquads_dim->size())==n_bquads, ExcIO());

  NcVar *bmarker_var=nc.get_var("boundarymarker_of_surfaces");
  AssertThrow(bmarker_var->is_valid(), ExcIO());
  AssertThrow(bmarker_var->num_dims()==1, ExcIO());
  AssertThrow(static_cast<unsigned int>(
    bmarker_var->get_dim(0)->size())==n_bquads, ExcIO());

  std::vector<int> bmarker(n_bquads);
  bmarker_var->get(&*bmarker.begin(), n_bquads);
				   // we only handle boundary
				   // indicators that fit into an
				   // unsigned char. Also, we don't
				   // take 255 as it denotes an
				   // internal face
  for (unsigned int i=0; i<bmarker.size(); ++i)
    Assert(0<=bmarker[i] && bmarker[i]<=254, ExcIO());

				   // finally we setup the boundary
				   // information
  SubCellData subcelldata;
  subcelldata.boundary_quads.resize(n_bquads);
  for (unsigned int i=0; i<n_bquads; ++i)
    {
      for (unsigned int v=0; v<GeometryInfo<dim>::vertices_per_face; ++v)
	subcelldata.boundary_quads[i].vertices[v]=bvertex_indices[
	  i*GeometryInfo<dim>::vertices_per_face+v];
      subcelldata.boundary_quads[i].material_id=bmarker[i];
    }

  GridTools::delete_unused_vertices(vertices, cells, subcelldata);
  if (switch_y_z)
    GridReordering<dim>::invert_all_cells_of_negative_grid (vertices, cells);
  GridReordering<dim>::reorder_cells (cells);
  tria->create_triangulation_compatibility (vertices, cells, subcelldata);  
#endif
}

#endif



template <int dim>
void GridIn<dim>::skip_empty_lines (std::istream &in)
{    
  std::string line;
  while (in) 
    {
				       // get line
      getline (in, line);
				       // eat all spaces from the back
      while ((line.length()>0) && (line[line.length()-1]==' '))
	line.erase (line.length()-1, 1);
				       // if still non-null, then this
				       // is a non-empty line. put
				       // back all info and leave
      if (line.length() > 0)
	{
	  in.putback ('\n');
	  for (int i=line.length()-1; i>=0; --i)
	    in.putback (line[i]);
	  return;
	};

				       // else: go on with next line
    };
}



template <int dim>
void GridIn<dim>::skip_comment_lines (std::istream &in,
				      const char    comment_start)
{
  char c;
				   // loop over the following comment
				   // lines
  while ((c=in.get()) == comment_start)
				     // loop over the characters after
				     // the comment starter
    while (in.get() != '\n');
  
				   // put back first character of
				   // first non-comment line
  in.putback (c);
}



template <int dim>
void GridIn<dim>::debug_output_grid (const std::vector<CellData<dim> > &/*cells*/,
				     const std::vector<Point<dim> >    &/*vertices*/,
				     std::ostream                      &/*out*/)
{
  Assert (false, ExcNotImplemented());
}


#if deal_II_dimension == 2

template <>
void
GridIn<2>::debug_output_grid (const std::vector<CellData<2> > &cells,
			      const std::vector<Point<2> >    &vertices,
			      std::ostream                    &out)
{
  double min_x = vertices[cells[0].vertices[0]](0),
	 max_x = vertices[cells[0].vertices[0]](0),
	 min_y = vertices[cells[0].vertices[0]](1),
	 max_y = vertices[cells[0].vertices[0]](1);
  
  for (unsigned int i=0; i<cells.size(); ++i)
    {
      for (unsigned int v=0; v<4; ++v)
	{
	  const Point<2> &p = vertices[cells[i].vertices[v]];
	  
	  if (p(0) < min_x)
	    min_x = p(0);
	  if (p(0) > max_x)
	    max_x = p(0);
	  if (p(1) < min_y)
	    min_y = p(1);
	  if (p(1) > max_y)
	    max_y = p(1);
	};

      out << "# cell " << i << std::endl;
      Point<2> center;
      for (unsigned int f=0; f<4; ++f)
	center += vertices[cells[i].vertices[f]];
      center /= 4;

      out << "set label \"" << i << "\" at "
	  << center(0) << ',' << center(1)
	  << " center"
	  << std::endl;

				       // first two line right direction
      for (unsigned int f=0; f<2; ++f)
	out << "set arrow from "
	    << vertices[cells[i].vertices[f]](0) << ',' 
	    << vertices[cells[i].vertices[f]](1)
	    << " to "
	    << vertices[cells[i].vertices[(f+1)%4]](0) << ',' 
	    << vertices[cells[i].vertices[(f+1)%4]](1)
	    << std::endl;
				       // other two lines reverse direction
      for (unsigned int f=2; f<4; ++f)
	out << "set arrow from "
	    << vertices[cells[i].vertices[(f+1)%4]](0) << ',' 
	    << vertices[cells[i].vertices[(f+1)%4]](1)
	    << " to "
	    << vertices[cells[i].vertices[f]](0) << ',' 
	    << vertices[cells[i].vertices[f]](1)
	    << std::endl;
      out << std::endl;
    };
  

  out << std::endl
      << "set nokey" << std::endl
      << "pl [" << min_x << ':' << max_x << "]["
      << min_y << ':' << max_y <<  "] "
      << min_y << std::endl
      << "pause -1" << std::endl;
}

#endif


#if deal_II_dimension == 3

template <>
void
GridIn<3>::debug_output_grid (const std::vector<CellData<3> > &cells,
			      const std::vector<Point<3> >    &vertices,
			      std::ostream                    &out)
{
  for (unsigned int cell=0; cell<cells.size(); ++cell)
    {
				       // line 0
      out << vertices[cells[cell].vertices[0]]
	  << std::endl
	  << vertices[cells[cell].vertices[1]]
	  << std::endl << std::endl << std::endl;
				       // line 1
      out << vertices[cells[cell].vertices[1]]
	  << std::endl
	  << vertices[cells[cell].vertices[2]]
	  << std::endl << std::endl << std::endl;
				       // line 2
      out << vertices[cells[cell].vertices[3]]
	  << std::endl
	  << vertices[cells[cell].vertices[2]]
	  << std::endl << std::endl << std::endl;
				       // line 3
      out << vertices[cells[cell].vertices[0]]
	  << std::endl
	  << vertices[cells[cell].vertices[3]]
	  << std::endl << std::endl << std::endl;
				       // line 4
      out << vertices[cells[cell].vertices[4]]
	  << std::endl
	  << vertices[cells[cell].vertices[5]]
	  << std::endl << std::endl << std::endl;
				       // line 5
      out << vertices[cells[cell].vertices[5]]
	  << std::endl
	  << vertices[cells[cell].vertices[6]]
	  << std::endl << std::endl << std::endl;
				       // line 6
      out << vertices[cells[cell].vertices[7]]
	  << std::endl
	  << vertices[cells[cell].vertices[6]]
	  << std::endl << std::endl << std::endl;
				       // line 7
      out << vertices[cells[cell].vertices[4]]
	  << std::endl
	  << vertices[cells[cell].vertices[7]]
	  << std::endl << std::endl << std::endl;
				       // line 8
      out << vertices[cells[cell].vertices[0]]
	  << std::endl
	  << vertices[cells[cell].vertices[4]]
	  << std::endl << std::endl << std::endl;
				       // line 9
      out << vertices[cells[cell].vertices[1]]
	  << std::endl
	  << vertices[cells[cell].vertices[5]]
	  << std::endl << std::endl << std::endl;
				       // line 10
      out << vertices[cells[cell].vertices[2]]
	  << std::endl
	  << vertices[cells[cell].vertices[6]]
	  << std::endl << std::endl << std::endl;
				       // line 11
      out << vertices[cells[cell].vertices[3]]
	  << std::endl
	  << vertices[cells[cell].vertices[7]]
	  << std::endl << std::endl << std::endl;
    };
}

#endif

template <int dim>
void GridIn<dim>::read (const std::string& filename,
			Format format)
{
				   // Search file class for meshes
  PathSearch search("MESH");
  std::string name;
				   // Open the file and remember its name
  if (format == Default)
   name = search.find(filename);
  else
    name = search.find(filename, default_suffix(format));

  std::ifstream in(name.c_str());
  
  if (format == Default)
    {
      const std::string::size_type slashpos = name.find_last_of('/');
      const std::string::size_type dotpos = name.find_last_of('.');
      if (dotpos < name.length()
	  && (dotpos > slashpos || slashpos == std::string::npos))
	{
	  std::string ext = name.substr(dotpos+1);
	  format = parse_format(ext);
	}
    }
  if (format == netcdf)
    read_netcdf(filename);
  else
    read(in, format);
}


template <int dim>
void GridIn<dim>::read (std::istream& in,
			Format format)
{
  if (format == Default)
    format = default_format;
  
  switch (format)
    {
      case dbmesh:
	read_dbmesh (in);
	return;
	
      case msh:
	read_msh (in);
	return;
	
      case ucd:
	read_ucd (in);
	return;
	
      case xda:
	read_xda (in);
	return;

      case netcdf:
	Assert(false, ExcMessage("There is no read_netcdf(istream &) function. "
				 "Use the read(_netcdf)(string &filename) "
				 "functions, instead."));
	return;

      case Default:
	break;
   }
  Assert (false, ExcInternalError());
}



template <int dim>
std::string
GridIn<dim>::default_suffix (const Format format) 
{
  switch (format) 
    {
      case dbmesh:
        return ".dbmesh";
      case msh: 
	return ".msh";
      case ucd: 
	return ".inp";
      case xda:
	return ".xda";
      case netcdf:
	return ".nc";
      default: 
	Assert (false, ExcNotImplemented()); 
	return ".unknown_format";
    }
}



template <int dim>
typename GridIn<dim>::Format
GridIn<dim>::parse_format (const std::string &format_name)
{
  if (format_name == "dbmesh")
    return dbmesh;

  if (format_name == "msh")
    return msh;
  
  if (format_name == "inp")
    return ucd;

  if (format_name == "ucd")
    return ucd;

  if (format_name == "xda")
    return xda;

  if (format_name == "netcdf")
    return netcdf;

  if (format_name == "nc")
    return netcdf;

  AssertThrow (false, ExcInvalidState ());
				   // return something weird
  return Format(Default);
}



template <int dim>
std::string GridIn<dim>::get_format_names () 
{
  return "dbmesh|msh|ucd|xda|netcdf";
}



//explicit instantiations
template class GridIn<deal_II_dimension>;

DEAL_II_NAMESPACE_CLOSE
