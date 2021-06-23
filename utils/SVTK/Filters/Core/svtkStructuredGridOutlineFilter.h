/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredGridOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStructuredGridOutlineFilter
 * @brief   create wireframe outline for structured grid
 *
 * svtkStructuredGridOutlineFilter is a filter that generates a wireframe
 * outline of a structured grid (svtkStructuredGrid). Structured data is
 * topologically a cube, so the outline will have 12 "edges".
 */

#ifndef svtkStructuredGridOutlineFilter_h
#define svtkStructuredGridOutlineFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkStructuredGridOutlineFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkStructuredGridOutlineFilter* New();
  svtkTypeMacro(svtkStructuredGridOutlineFilter, svtkPolyDataAlgorithm);

protected:
  svtkStructuredGridOutlineFilter() {}
  ~svtkStructuredGridOutlineFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkStructuredGridOutlineFilter(const svtkStructuredGridOutlineFilter&) = delete;
  void operator=(const svtkStructuredGridOutlineFilter&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkStructuredGridOutlineFilter.h
