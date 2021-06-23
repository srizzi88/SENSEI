/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkUniformGridGhostDataGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkUniformGridGhostDataGenerator
 *
 *
 *  A concrete implementation of svtkDataSetGhostGenerator for generating ghost
 *  data on partitioned uniform grids on a single process. For a distributed
 *  data-set see svtkPUniformGridGhostDataGenerator.
 *
 * @warning
 *  <ol>
 *   <li>
 *    The input multi-block dataset must:
 *    <ul>
 *      <li> Have the whole-extent set </li>
 *      <li> Each block must be an instance of svtkUniformGrid </li>
 *      <li> Each block must have its corresponding global extent set in the
 *           meta-data using the PIECE_EXTENT() key </li>
 *      <li> The spacing of each block is the same </li>
 *      <li> All blocks must have the same fields loaded </li>
 *    </ul>
 *   </li>
 *   <li>
 *    The code currently does not handle the following cases:
 *    <ul>
 *      <li>Ghost cells along Periodic boundaries</li>
 *      <li>Growing ghost layers beyond the extents of the neighboring grid</li>
 *    </ul>
 *   </li>
 *  </ol>
 *
 * @sa
 *  svtkDataSetGhostGenerator, svtkPUniformGhostDataGenerator
 */

#ifndef svtkUniformGridGhostDataGenerator_h
#define svtkUniformGridGhostDataGenerator_h

#include "svtkDataSetGhostGenerator.h"
#include "svtkFiltersGeometryModule.h" // For export macro

// Forward declarations
class svtkMultiBlockDataSet;
class svtkIndent;
class svtkStructuredGridConnectivity;

class SVTKFILTERSGEOMETRY_EXPORT svtkUniformGridGhostDataGenerator : public svtkDataSetGhostGenerator
{
public:
  static svtkUniformGridGhostDataGenerator* New();
  svtkTypeMacro(svtkUniformGridGhostDataGenerator, svtkDataSetGhostGenerator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkUniformGridGhostDataGenerator();
  ~svtkUniformGridGhostDataGenerator() override;

  /**
   * Computes the global origin
   */
  void ComputeOrigin(svtkMultiBlockDataSet* in);

  /**
   * Computes the global spacing vector
   */
  void ComputeGlobalSpacingVector(svtkMultiBlockDataSet* in);

  /**
   * Registers the grid associated with this instance of multi-block.
   */
  void RegisterGrids(svtkMultiBlockDataSet* in);

  /**
   * Creates the output
   */
  void CreateGhostedDataSet(svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out);

  /**
   * Generates ghost layers.
   */
  void GenerateGhostLayers(svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out) override;

  double GlobalSpacing[3];
  double GlobalOrigin[3];
  svtkStructuredGridConnectivity* GridConnectivity;

private:
  svtkUniformGridGhostDataGenerator(const svtkUniformGridGhostDataGenerator&) = delete;
  void operator=(const svtkUniformGridGhostDataGenerator&) = delete;
};

#endif /* svtkUniformGridGhostDataGenerator_h */
