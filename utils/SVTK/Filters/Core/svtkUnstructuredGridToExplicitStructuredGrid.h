/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridToExplicitStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUnstructuredGridToExplicitStructuredGrid
 * @brief   Filter which converts an unstructured grid data into an explicit structured grid.
 *          The input grid must have a structured coordinates int cell array.
 *          Moreover, its cell must be listed in the i-j-k order (k varying more ofter)
 */

#ifndef svtkUnstructuredGridToExplicitStructuredGrid_h
#define svtkUnstructuredGridToExplicitStructuredGrid_h

#include "svtkExplicitStructuredGridAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class SVTKFILTERSCORE_EXPORT svtkUnstructuredGridToExplicitStructuredGrid
  : public svtkExplicitStructuredGridAlgorithm
{
public:
  static svtkUnstructuredGridToExplicitStructuredGrid* New();
  svtkTypeMacro(svtkUnstructuredGridToExplicitStructuredGrid, svtkExplicitStructuredGridAlgorithm);

  //@{
  /**
   * Get/Set the whole extents for the grid to produce. The size of the grid
   * must match the number of cells in the input.
   */
  svtkSetVector6Macro(WholeExtent, int);
  svtkGetVector6Macro(WholeExtent, int);
  //@}

protected:
  svtkUnstructuredGridToExplicitStructuredGrid();
  ~svtkUnstructuredGridToExplicitStructuredGrid() override = default;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int WholeExtent[6];

private:
  svtkUnstructuredGridToExplicitStructuredGrid(
    const svtkUnstructuredGridToExplicitStructuredGrid&) = delete;
  void operator=(const svtkUnstructuredGridToExplicitStructuredGrid&) = delete;
};

#endif
