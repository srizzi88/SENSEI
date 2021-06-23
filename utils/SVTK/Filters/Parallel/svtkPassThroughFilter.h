/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPassThroughFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPassThroughFilter
 * @brief   Filter which shallow copies it's input to it's output
 *
 * This filter shallow copies it's input to it's output. It is normally
 * used by PVSources with multiple outputs as the SVTK filter in the
 * dummy connection objects at each output.
 */

#ifndef svtkPassThroughFilter_h
#define svtkPassThroughFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersParallelModule.h" // For export macro

class svtkFieldData;

class SVTKFILTERSPARALLEL_EXPORT svtkPassThroughFilter : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkPassThroughFilter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create a new svtkPassThroughFilter.
   */
  static svtkPassThroughFilter* New();

protected:
  svtkPassThroughFilter() {}
  ~svtkPassThroughFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPassThroughFilter(const svtkPassThroughFilter&) = delete;
  void operator=(const svtkPassThroughFilter&) = delete;
};

#endif
