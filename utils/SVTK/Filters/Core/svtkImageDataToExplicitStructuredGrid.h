/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataToExplicitStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDataToExplicitStructuredGrid
 * @brief   Filter which converts a 3D image data into an explicit structured grid.
 */

#ifndef svtkImageDataToExplicitStructuredGrid_h
#define svtkImageDataToExplicitStructuredGrid_h

#include "svtkExplicitStructuredGridAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class SVTKFILTERSCORE_EXPORT svtkImageDataToExplicitStructuredGrid
  : public svtkExplicitStructuredGridAlgorithm
{
public:
  static svtkImageDataToExplicitStructuredGrid* New();
  svtkTypeMacro(svtkImageDataToExplicitStructuredGrid, svtkExplicitStructuredGridAlgorithm);

protected:
  svtkImageDataToExplicitStructuredGrid() = default;
  ~svtkImageDataToExplicitStructuredGrid() override = default;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkImageDataToExplicitStructuredGrid(const svtkImageDataToExplicitStructuredGrid&) = delete;
  void operator=(const svtkImageDataToExplicitStructuredGrid&) = delete;
};

#endif
