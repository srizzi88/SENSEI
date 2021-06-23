/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPExtractRectilinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPExtractRectilinearGrid
 * @brief   Extract VOI and/or sub-sample a distributed
 *  rectilinear grid dataset.
 *
 *
 *  svtkPExtractRectilinearGrid inherits from svtkExtractVOI & provides additional
 *  functionality when dealing with a distributed dataset. Specifically, when
 *  sub-sampling a dataset, a gap may be introduced between partitions. This
 *  filter handles such cases correctly by growing the grid to the right to
 *  close the gap.
 *
 * @sa
 *  svtkExtractRectilinearGrid
 */

#ifndef svtkPExtractRectilinearGrid_h
#define svtkPExtractRectilinearGrid_h

#include "svtkExtractRectilinearGrid.h"
#include "svtkFiltersParallelMPIModule.h" // For export macro

// Forward Declarations
class svtkInformation;
class svtkInformationVector;
class svtkMPIController;

class SVTKFILTERSPARALLELMPI_EXPORT svtkPExtractRectilinearGrid : public svtkExtractRectilinearGrid
{
public:
  static svtkPExtractRectilinearGrid* New();
  svtkTypeMacro(svtkPExtractRectilinearGrid, svtkExtractRectilinearGrid);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkPExtractRectilinearGrid();
  virtual ~svtkPExtractRectilinearGrid();

  // Standard SVTK Pipeline methods
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestInformation(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestUpdateExtent(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMPIController* Controller;

private:
  svtkPExtractRectilinearGrid(const svtkPExtractRectilinearGrid&) = delete;
  void operator=(const svtkPExtractRectilinearGrid&) = delete;
};

#endif /* SVTKPEXTRACTRECTILINEARGRID_H_ */
