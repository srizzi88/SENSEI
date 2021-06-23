/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRectilinearGridOutlineFilter
 * @brief   create wireframe outline for a rectilinear grid.
 *
 * svtkRectilinearGridOutlineFilter works in parallel.  There is no reason.
 * to use this filter if you are not breaking the processing into pieces.
 * With one piece you can simply use svtkOutlineFilter.  This filter
 * ignores internal edges when the extent is not the whole extent.
 */

#ifndef svtkRectilinearGridOutlineFilter_h
#define svtkRectilinearGridOutlineFilter_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSPARALLEL_EXPORT svtkRectilinearGridOutlineFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkRectilinearGridOutlineFilter* New();
  svtkTypeMacro(svtkRectilinearGridOutlineFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkRectilinearGridOutlineFilter() {}
  ~svtkRectilinearGridOutlineFilter() override {}
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkRectilinearGridOutlineFilter(const svtkRectilinearGridOutlineFilter&) = delete;
  void operator=(const svtkRectilinearGridOutlineFilter&) = delete;
};

#endif
