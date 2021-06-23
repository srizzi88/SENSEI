/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkStructuredGridGhostDataGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkStructuredGridGhostDataGenerator
 *
 *
 *  A concrete implementation of svtkDataSetGhostGenerator for generating ghost
 *  data on partitioned structured grids on a singled process. For a distributed
 *  data-set see svtkPStructuredGridGhostDataGenerator.
 *
 * @warning
 * <ol>
 *   <li>
 *    The input multi-block dataset must:
 *    <ul>
 *      <li> Have the whole-extent set </li>
 *      <li> Each block must be an instance of svtkStructuredGrid </li>
 *      <li> Each block must have its corresponding global extent set in the
 *           meta-data using the PIECE_EXTENT() key </li>
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
 * </ol>
 *
 * @sa
 * svtkDataSetGhostGenerator, svtkPStructuredGridGhostDataGenerator
 */

#ifndef svtkStructuredGridGhostDataGenerator_h
#define svtkStructuredGridGhostDataGenerator_h

#include "svtkDataSetGhostGenerator.h"
#include "svtkFiltersGeometryModule.h" // For export macro

// Forward declarations
class svtkMultiBlockDataSet;
class svtkIndent;
class svtkStructuredGridConnectivity;

class SVTKFILTERSGEOMETRY_EXPORT svtkStructuredGridGhostDataGenerator
  : public svtkDataSetGhostGenerator
{
public:
  static svtkStructuredGridGhostDataGenerator* New();
  svtkTypeMacro(svtkStructuredGridGhostDataGenerator, svtkDataSetGhostGenerator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkStructuredGridGhostDataGenerator();
  ~svtkStructuredGridGhostDataGenerator() override;

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

  svtkStructuredGridConnectivity* GridConnectivity;

private:
  svtkStructuredGridGhostDataGenerator(const svtkStructuredGridGhostDataGenerator&) = delete;
  void operator=(const svtkStructuredGridGhostDataGenerator&) = delete;
};

#endif /* svtkStructuredGridGhostDataGenerator_h */
