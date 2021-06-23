/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPExtractGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPExtractGrid
 * @brief   Extract VOI and/or sub-sample a distributed
 *  structured dataset.
 *
 *
 *  svtkPExtractGrid inherits from svtkExtractGrid and provides additional
 *  functionality when dealing with a distributed dataset. Specifically, when
 *  sub-sampling a dataset, a gap may be introduced between partitions. This
 *  filter handles such cases correctly by growing the grid to the right to
 *  close the gap.
 *
 * @sa
 *  svtkExtractGrid
 */

#ifndef svtkPExtractGrid_h
#define svtkPExtractGrid_h

#include "svtkExtractGrid.h"
#include "svtkFiltersParallelMPIModule.h" // For export macro

// Forward declarations
class svtkMPIController;

class SVTKFILTERSPARALLELMPI_EXPORT svtkPExtractGrid : public svtkExtractGrid
{
public:
  static svtkPExtractGrid* New();
  svtkTypeMacro(svtkPExtractGrid, svtkExtractGrid);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkPExtractGrid();
  ~svtkPExtractGrid() override;

  // Standard SVTK Pipeline methods
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestInformation(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestUpdateExtent(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMPIController* Controller;

private:
  svtkPExtractGrid(const svtkPExtractGrid&) = delete;
  void operator=(const svtkPExtractGrid&) = delete;
};

#endif
