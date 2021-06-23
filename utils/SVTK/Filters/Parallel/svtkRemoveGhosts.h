/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRemoveGhosts.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkRemoveGhosts
 * @brief   Remove ghost points, cells and arrays
 *
 *
 * Removes ghost points, cells and associated data arrays. Works on
 * svtkPolyDatas and svtkUnstructuredGrids.
 */

#ifndef svtkRemoveGhosts_h
#define svtkRemoveGhosts_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class svtkPolyData;
class svtkUnsignedCharArray;

class SVTKFILTERSPARALLEL_EXPORT svtkRemoveGhosts : public svtkPassInputTypeAlgorithm
{
public:
  svtkTypeMacro(svtkRemoveGhosts, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkRemoveGhosts* New();

protected:
  svtkRemoveGhosts();
  ~svtkRemoveGhosts() override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkRemoveGhosts(const svtkRemoveGhosts&) = delete;
  void operator=(const svtkRemoveGhosts&) = delete;
};

#endif //_svtkRemoveGhosts_h
