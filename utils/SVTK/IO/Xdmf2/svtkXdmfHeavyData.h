/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmfHeavyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// SVTK-HeaderTest-Exclude: svtkXdmfHeavyData.h

#ifndef svtkXdmfHeavyData_h
#define svtkXdmfHeavyData_h
#ifndef __SVTK_WRAP__
#ifndef SVTK_WRAPPING_CXX

#include "svtk_xdmf2.h"
#include SVTKXDMF2_HEADER(XdmfDataItem.h)
#include SVTKXDMF2_HEADER(XdmfGrid.h) //won't compile without it
#include "svtkIOXdmf2Module.h"        // For export macro

class svtkAlgorithm;
class svtkDataArray;
class svtkDataObject;
class svtkDataSet;
class svtkImageData;
class svtkMultiBlockDataSet;
class svtkPoints;
class svtkRectilinearGrid;
class svtkStructuredGrid;
class svtkUnstructuredGrid;
class svtkXdmfDomain;

// svtkXdmfHeavyData helps in reading heavy data from Xdmf and putting that into
// svtkDataObject subclasses.
class SVTKIOXDMF2_EXPORT svtkXdmfHeavyData
{
  svtkXdmfDomain* Domain;
  xdmf2::XdmfDataItem DataItem;
  svtkAlgorithm* Reader;

public:
  // These must be set before using this class.
  int Piece;
  int NumberOfPieces;
  int GhostLevels;
  int Extents[6]; // < these are original extents without the stride taken in
                  //   consideration
  int Stride[3];
  XdmfFloat64 Time;

public:
  svtkXdmfHeavyData(svtkXdmfDomain* domain, svtkAlgorithm* reader);
  ~svtkXdmfHeavyData();

  // Description:
  svtkDataObject* ReadData(xdmf2::XdmfGrid* xmfGrid, int blockId = -1);

  // Description:
  svtkDataObject* ReadData();

  // Description:
  // Returns the SVTKCellType for the given xdmf topology. Returns SVTK_EMPTY_CELL
  // on error and SVTK_NUMBER_OF_CELL_TYPES for XDMF_MIXED.
  static int GetSVTKCellType(XdmfInt32 topologyType);

  // Description:
  // Returns the number of points per cell. -1 for error. 0 when no fixed number
  // of points possible.
  static int GetNumberOfPointsPerCell(int svtk_cell_type);

private:
  // Description:
  // Read a temporal collection.
  svtkDataObject* ReadTemporalCollection(xdmf2::XdmfGrid* xmfTemporalCollection, int blockId);

  // Description:
  // Read a spatial-collection or a tree.
  svtkDataObject* ReadComposite(xdmf2::XdmfGrid* xmfColOrTree);

  // Description:
  // Read a non-composite grid. Note here uniform has nothing to do with
  // svtkUniformGrid but to what Xdmf's GridType="Uniform".
  svtkDataObject* ReadUniformData(xdmf2::XdmfGrid* xmfGrid, int blockId);

  // Description:
  // Reads the topology and geometry for an unstructured grid. Does not read any
  // data attributes or geometry.
  svtkDataObject* ReadUnstructuredGrid(xdmf2::XdmfGrid* xmfGrid);

  // Description:
  // Read the image data. Simply initializes the extents and origin and spacing
  // for the image, doesn't really read any attributes including the active
  // point attributes.
  svtkImageData* RequestImageData(xdmf2::XdmfGrid* xmfGrid, bool use_uniform_grid);

  // Description:
  // Reads the geometry and topology for a svtkStructuredGrid.
  svtkStructuredGrid* RequestStructuredGrid(xdmf2::XdmfGrid* xmfGrid);

  // Description:
  // Reads the geometry and topology for a svtkRectilinearGrid.
  svtkRectilinearGrid* RequestRectilinearGrid(xdmf2::XdmfGrid* xmfGrid);

  // Description:
  // Reads geometry for svtkUnstructuredGrid or svtkStructuredGrid i.e. of
  // svtkPointSet subclasses. The extents only make sense when reading
  // svtkStructuredGrid. If non-null, then the only the points for the sub-grid
  // are read.
  svtkPoints* ReadPoints(
    xdmf2::XdmfGeometry* xmfGeometry, int* update_extents = nullptr, int* whole_extents = nullptr);

  // Description:
  // Read attributes.
  bool ReadAttributes(svtkDataSet* dataSet, xdmf2::XdmfGrid* xmfGrid, int* update_extents = 0);

  // Description:
  // Reads an attribute.
  // If update_extents are non-null, then we are reading structured attributes
  // and we read only the sub-set specified by update_extents.
  svtkDataArray* ReadAttribute(
    xdmf2::XdmfAttribute* xmfAttribute, int data_dimensionality, int* update_extents = 0);

  // Description:
  // Read sets that mark ghost cells/nodes and then create attribute arrays for
  // marking the cells as such.
  bool ReadGhostSets(svtkDataSet* ds, xdmf2::XdmfGrid* xmfGrid, int* update_extents = 0);

  svtkMultiBlockDataSet* ReadSets(
    svtkDataSet* dataSet, xdmf2::XdmfGrid* xmfGrid, int* update_extents = 0);

  // Description:
  // Used when reading node-sets.
  // Creates a new dataset with points with given ids extracted from the input
  // dataset.
  svtkDataSet* ExtractPoints(xdmf2::XdmfSet* xmfSet, svtkDataSet* dataSet);

  // Description:
  // Used when reading cell-sets.
  // Creates a new dataset with cells with the given ids extracted from the
  // input dataset.
  svtkDataSet* ExtractCells(xdmf2::XdmfSet* xmfSet, svtkDataSet* dataSet);

  // Description:
  // Used when reading face-sets.
  // Creates a new dataset with faces selected by the set, extracting them from
  // the input dataset.
  svtkDataSet* ExtractFaces(xdmf2::XdmfSet* xmfSet, svtkDataSet* dataSet);

  // Description:
  // Used when reading edge-sets.
  // Creates a new dataset with edges selected by the set, extracting them from
  // the input dataset.
  svtkDataSet* ExtractEdges(xdmf2::XdmfSet* xmfSet, svtkDataSet* dataSet);
};

#endif
#endif
#endif
