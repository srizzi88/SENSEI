/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridToUnstructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridToUnstructuredGrid
 * @brief   Convert hyper tree grid to
 * unstructured grid.
 *
 JB Primal mesh
 * Make explicit all leaves of a hyper tree grid by converting them to cells
 * of an unstructured grid.
 * Produces segments in 1D, rectangles in 2D, right hexahedra in 3D.
 * NB: The output will contain superimposed inter-element boundaries and pending
 * nodes as a result of T-junctions.
 *
 * @sa
 * svtkHyperTreeGrid svtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2012
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien, 2014
 * This class was rewritten by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien, 2018
 * This class was corriged (used orientation) by Jacques-Bernard Lekien, 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
*/

#ifndef svtkHyperTreeGridToUnstructuredGrid_h
#define svtkHyperTreeGridToUnstructuredGrid_h

#include "svtkFiltersHyperTreeModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

class svtkBitArray;
class svtkCellArray;
class svtkHyperTreeGrid;
class svtkPoints;
class svtkUnstructuredGrid;
class svtkHyperTreeGridNonOrientedGeometryCursor;

class SVTKFILTERSHYPERTREE_EXPORT svtkHyperTreeGridToUnstructuredGrid
  : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridToUnstructuredGrid* New();
  svtkTypeMacro(svtkHyperTreeGridToUnstructuredGrid, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

protected:
  svtkHyperTreeGridToUnstructuredGrid();
  ~svtkHyperTreeGridToUnstructuredGrid() override;

  /**
   * For this algorithm the output is a svtkUnstructuredGrid instance
   */
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Main routine to convert the grid of tree into an unstructured grid
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(svtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Helper method to generate a 2D or 3D cell
   */
  void AddCell(svtkIdType, double*, double*);

  /**
   * Storage for points of output unstructured mesh
   */
  svtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  svtkCellArray* Cells;

  /**
   * Storage of underlying tree
   */
  unsigned int Dimension;
  unsigned int Orientation;
  const unsigned int* Axes;

private:
  svtkHyperTreeGridToUnstructuredGrid(const svtkHyperTreeGridToUnstructuredGrid&) = delete;
  void operator=(const svtkHyperTreeGridToUnstructuredGrid&) = delete;
};

#endif /* svtkHyperTreeGridToUnstructuredGrid_h */
