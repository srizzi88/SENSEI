/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExplicitStructuredGridToUnstructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExplicitStructuredGridToUnstructuredGrid
 * @brief   Filter which converts an explicit structured grid into an unstructured grid.
 */

#ifndef svtkExplicitStructuredGridToUnstructuredGrid_h
#define svtkExplicitStructuredGridToUnstructuredGrid_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkExplicitStructuredGridToUnstructuredGrid
  : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkExplicitStructuredGridToUnstructuredGrid* New();
  svtkTypeMacro(svtkExplicitStructuredGridToUnstructuredGrid, svtkUnstructuredGridAlgorithm);

protected:
  svtkExplicitStructuredGridToUnstructuredGrid() = default;
  ~svtkExplicitStructuredGridToUnstructuredGrid() override = default;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkExplicitStructuredGridToUnstructuredGrid(
    const svtkExplicitStructuredGridToUnstructuredGrid&) = delete;
  void operator=(const svtkExplicitStructuredGridToUnstructuredGrid&) = delete;
};

#endif
