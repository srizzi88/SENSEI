/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkPUniformGridGhostDataGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkPUniformGridGhostDataGenerator
 *  uniform grids.
 *
 *
 *  A concrete implementation of svtkPDataSetGhostGenerator for generating ghost
 *  data on a partitioned and distributed domain of uniform grids.
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
 * svtkDataSetGhostGenerator,svtkUniformGhostDataGenerator,
 * svtkPDataSetGhostGenerator
 */

#ifndef svtkPUniformGridGhostDataGenerator_h
#define svtkPUniformGridGhostDataGenerator_h

#include "svtkFiltersParallelGeometryModule.h" // For export macro
#include "svtkPDataSetGhostGenerator.h"

class svtkMultiBlockDataSet;
class svtkIndent;
class svtkPStructuredGridConnectivity;

class SVTKFILTERSPARALLELGEOMETRY_EXPORT svtkPUniformGridGhostDataGenerator
  : public svtkPDataSetGhostGenerator
{
public:
  static svtkPUniformGridGhostDataGenerator* New();
  svtkTypeMacro(svtkPUniformGridGhostDataGenerator, svtkPDataSetGhostGenerator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkPUniformGridGhostDataGenerator();
  ~svtkPUniformGridGhostDataGenerator() override;

  /**
   * Registers grids associated with this object instance on this process.
   */
  void RegisterGrids(svtkMultiBlockDataSet* in);

  /**
   * A collective operation that computes the global origin of the domain.
   */
  void ComputeOrigin(svtkMultiBlockDataSet* in);

  /**
   * A collective operations that computes the global spacing.
   */
  void ComputeGlobalSpacing(svtkMultiBlockDataSet* in);

  /**
   * Create ghosted data-set.
   */
  void CreateGhostedDataSet(svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out);

  /**
   * Generates ghost-layers
   */
  virtual void GenerateGhostLayers(svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out) override;

  double GlobalSpacing[3];
  double GlobalOrigin[3];
  svtkPStructuredGridConnectivity* GridConnectivity;

private:
  svtkPUniformGridGhostDataGenerator(const svtkPUniformGridGhostDataGenerator&) = delete;
  void operator=(const svtkPUniformGridGhostDataGenerator&) = delete;
};

#endif /* svtkPUniformGridGhostDataGenerator_h */
