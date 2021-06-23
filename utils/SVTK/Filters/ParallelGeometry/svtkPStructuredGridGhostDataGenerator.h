/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkPStructuredGridGhostDataGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkPStructuredGridGhostDataGenerator
 *  structured grids.
 *
 *
 *  A concrete implementation of svtkPDataSEtGhostGenerator for generating ghost
 *  data on a partitioned and distributed domain of structured grids.
 *
 * @warning
 *  <ol>
 *   <li>
 *    The input multi-block dataset must:
 *    <ul>
 *      <li> Have the whole-extent set </li>
 *      <li> Each block must be an instance of svtkStructuredGrid </li>
 *      <li> Each block must have its corresponding global extent set in the
 *           meta-data using the PIECE_EXTENT() key </li>
 *      <li> All blocks must have the same fields loaded </li>
 *      <li> The multi-block structure is consistent on all processes </li>
 *    </ul>
 *   </li>
 *   <li>
 *    The code currently does not handle the following cases:
 *    <ul>
 *      <li>Periodic boundaries</li>
 *      <li>Growing ghost layers beyond the extents of the neighboring grid</li>
 *    </ul>
 *   </li>
 *  </ol>
 *
 * @sa
 * svtkDataSetGhostGenerator, svtkStructuredGridGhostDataGenerator,
 * svtkPDataSetGhostGenerator, svtkPUniformGridGhostDataGenerator
 */

#ifndef svtkPStructuredGridGhostDataGenerator_h
#define svtkPStructuredGridGhostDataGenerator_h

#include "svtkFiltersParallelGeometryModule.h" // For export macro
#include "svtkPDataSetGhostGenerator.h"

class svtkMultiBlockDataSet;
class svtkIndent;
class svtkPStructuredGridConnectivity;

class SVTKFILTERSPARALLELGEOMETRY_EXPORT svtkPStructuredGridGhostDataGenerator
  : public svtkPDataSetGhostGenerator
{
public:
  static svtkPStructuredGridGhostDataGenerator* New();
  svtkTypeMacro(svtkPStructuredGridGhostDataGenerator, svtkPDataSetGhostGenerator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkPStructuredGridGhostDataGenerator();
  ~svtkPStructuredGridGhostDataGenerator() override;

  /**
   * Registers the grid associated with this instance of multi-block.
   */
  void RegisterGrids(svtkMultiBlockDataSet* in);

  /**
   * Creates the output.
   */
  void CreateGhostedDataSet(svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out);

  /**
   * Generates ghost layers.
   */
  void GenerateGhostLayers(svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out) override;

  svtkPStructuredGridConnectivity* GridConnectivity;

private:
  svtkPStructuredGridGhostDataGenerator(const svtkPStructuredGridGhostDataGenerator&) = delete;
  void operator=(const svtkPStructuredGridGhostDataGenerator&) = delete;
};

#endif /* svtkPStructuredGridGhostDataGenerator_h */
