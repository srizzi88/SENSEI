/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridToDualGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridToDualGrid
 * @brief   Convert hyper tree grid to the dual unstructured grid.
 *
 * This filter is the new home for what was the dataset API within the
 * svtkHyperTreeGrid class.
 */

#ifndef svtkHyperTreeGridToDualGrid_h
#define svtkHyperTreeGridToDualGrid_h

#include "svtkFiltersHyperTreeModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

#include <map> // std::map

class svtkHyperTreeGrid;
class svtkHyperTreeGridNonOrientedMooreSuperCursor;
class svtkIdTypeArray;
class svtkUnstructuredGrid;
class svtkPoints;

class SVTKFILTERSHYPERTREE_EXPORT svtkHyperTreeGridToDualGrid : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridToDualGrid* New();
  svtkTypeMacro(svtkHyperTreeGridToDualGrid, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

protected:
  svtkHyperTreeGridToDualGrid();
  ~svtkHyperTreeGridToDualGrid() override;

  /**
   * For this algorithm the output is a svtkUnstructuredGrid instance
   */
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Main routine to convert the grid of tree into an unstructured grid
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

private:
  svtkHyperTreeGridToDualGrid(const svtkHyperTreeGridToDualGrid&) = delete;
  void operator=(const svtkHyperTreeGridToDualGrid&) = delete;

  void TraverseDualRecursively(
    svtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, svtkHyperTreeGrid* in);
  void GenerateDualCornerFromLeaf1D(
    svtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, svtkHyperTreeGrid* in);
  void GenerateDualCornerFromLeaf2D(
    svtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, svtkHyperTreeGrid* in);
  void GenerateDualCornerFromLeaf3D(
    svtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, svtkHyperTreeGrid* in);

  void TraverseDualRecursively(
    svtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, svtkBitArray* mask, svtkHyperTreeGrid* in);
  void ShiftDualCornerFromMaskedLeaf2D(
    svtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, svtkBitArray* mask, svtkHyperTreeGrid* in);
  void ShiftDualCornerFromMaskedLeaf3D(
    svtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, svtkBitArray* mask, svtkHyperTreeGrid* in);
  void GenerateDualCornerFromLeaf2D(
    svtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, svtkBitArray* mask, svtkHyperTreeGrid* in);
  void GenerateDualCornerFromLeaf3D(
    svtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, svtkBitArray* mask, svtkHyperTreeGrid* in);

  svtkPoints* Points;
  svtkIdTypeArray* Connectivity;
  std::map<svtkIdType, bool> PointShifted;
  std::map<svtkIdType, double> PointShifts[3];
  std::map<svtkIdType, double> ReductionFactors;
};

#endif /* svtkHyperTreeGridToDualGrid_h */
