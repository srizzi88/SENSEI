/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridGhostCellsGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridGhostCellsGenerator
 * @brief   Generated ghost cells (HyperTree's distributed).
 *
 * This filter generates ghost cells for svtkHyperTreeGrid type data. The input svtkHyperTreeGrid
 * should have hyper trees distributed to a single process. This filter produces ghost hyper trees
 * at the interfaces between different processes, only composed of the nodes and leafs at this
 * interface to avoid data waste.
 *
 * This filter should be used in a multi-processes environment, and is only required if wanting to
 * filter a svtkHyperTreeGrid with algorithms using Von Neumann or Moore supercursors afterwards.
 *
 * @par Thanks:
 * This class was written by Jacques-Bernard Lekien, 2019
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridGhostCellsGenerator_h
#define svtkHyperTreeGridGhostCellsGenerator_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

#include <vector> // For svtkHypertreeGridGhostCellsGenerator::ExtractInterface

class svtkBitArray;
class svtkHyperTreeGrid;
class svtkHyperTreeGridNonOrientedCursor;
class svtkPointData;

class SVTKFILTERSPARALLEL_EXPORT svtkHyperTreeGridGhostCellsGenerator
  : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridGhostCellsGenerator* New();
  svtkTypeMacro(svtkHyperTreeGridGhostCellsGenerator, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

protected:
  svtkHyperTreeGridGhostCellsGenerator();
  ~svtkHyperTreeGridGhostCellsGenerator() override;

  struct svtkInternals;

  /**
   * For this algorithm the output is a svtkHyperTreeGrid instance
   */
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Main routine to extract cells based on thresholded value
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  /**
   * Copies the input to the output, filling memory gaps if present.
   */
  void CopyInputTreeToOutput(svtkHyperTreeGridNonOrientedCursor* inCursor,
    svtkHyperTreeGridNonOrientedCursor* outCursor, svtkPointData* inPointData,
    svtkPointData* outPointData, svtkBitArray* inMask, svtkBitArray* outMask);

  /**
   * Reads the input interface with neighbor processes.
   * This method is built in mirror with svtkHyperTreeGridGhostCellsGenerator::CreateGhostTree
   *
   * @param inCursor Cursor on the current tree to read from the input
   * @param isParent A bit array being produced by this filter,
   * telling if the corresponding node is parent or not. A node is
   * a parent if it is not a leaf. The map of the tracking is stored in indices.
   * For example, if the data array of the input is called inArray,
   * isParent->GetValue(m) equals one if inArray->GetTuple1(indices[m]) is not a leaf.
   * @param indices An array produced by this filter mapping the nodes of the interface with their
   * location in the input data array.
   * @param grid Input svtkHyperTreeGrid used to have the neighborhood profile. This neighborhood
   * profile is tested with the mask parameter to know wether to descend or not in the current hyper
   * tree.
   * @param mask Input parameter which should be shaped as svtkHyperTreeGrid::GetChildMask() of the
   * input. This parameter is used to only descend on the interface with the other processes.
   * @param pos This parameter will be equal to the number of nodes in the hyper tree to send to the
   * other processes.
   */
  void ExtractInterface(svtkHyperTreeGridNonOrientedCursor* inCursor, svtkBitArray* isParent,
    std::vector<svtkIdType>& indices, svtkHyperTreeGrid* grid, unsigned int mask, svtkIdType& pos);

  /**
   * Creates a ghost tree in the output. It is built in mirror with
   * svtkHyperTreeGridGhostCellsGenerator::ExtractInterface.
   *
   * @param outCursor Cursor on the output tree that will create the hyper tree.
   * @param isParent Input svtkBitArray produced by a neighbor process to tell if the current node is
   * a leaf or not.
   * @param indices Output array mapping the created nodes to their position in the output data
   * arrays.
   * @param pos Parameter which should be left untouched, it is used to keep track of the number of
   * inserted data.
   */
  svtkIdType CreateGhostTree(svtkHyperTreeGridNonOrientedCursor* outCursor, svtkBitArray* isParent,
    svtkIdType* indices, svtkIdType&& pos = 0);

  svtkInternals* Internals;

private:
  svtkHyperTreeGridGhostCellsGenerator(const svtkHyperTreeGridGhostCellsGenerator&) = delete;
  void operator=(const svtkHyperTreeGridGhostCellsGenerator&) = delete;
};

#endif /* svtkHyperTreeGridGhostCellsGenerator */
