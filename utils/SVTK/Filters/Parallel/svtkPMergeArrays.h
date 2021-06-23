/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPMergeArrays.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPMergeArrays
 * @brief   Multiple inputs with one output, parallel version
 *
 * Like it's super class, this filter tries to combine all arrays from
 * inputs into one output.
 *
 * @sa
 * svtkMergeArrays
 */

#ifndef svtkPMergeArrays_h
#define svtkPMergeArrays_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkMergeArrays.h"

class SVTKFILTERSPARALLEL_EXPORT svtkPMergeArrays : public svtkMergeArrays
{
public:
  svtkTypeMacro(svtkPMergeArrays, svtkMergeArrays);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPMergeArrays* New();

protected:
  svtkPMergeArrays();
  ~svtkPMergeArrays() override {}

  int MergeDataObjectFields(svtkDataObject* input, int idx, svtkDataObject* output) override;

private:
  svtkPMergeArrays(const svtkPMergeArrays&) = delete;
  void operator=(const svtkPMergeArrays&) = delete;
};

#endif
