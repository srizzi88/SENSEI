/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedPolyDataIds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractSelectedPolyDataIds
 * @brief   extract a list of cells from a polydata
 *
 * svtkExtractSelectedPolyDataIds extracts all cells in svtkSelection from a
 * svtkPolyData.
 * @sa
 * svtkSelection
 */

#ifndef svtkExtractSelectedPolyDataIds_h
#define svtkExtractSelectedPolyDataIds_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkSelection;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractSelectedPolyDataIds : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkExtractSelectedPolyDataIds, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkExtractSelectedPolyDataIds* New();

protected:
  svtkExtractSelectedPolyDataIds();
  ~svtkExtractSelectedPolyDataIds() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkExtractSelectedPolyDataIds(const svtkExtractSelectedPolyDataIds&) = delete;
  void operator=(const svtkExtractSelectedPolyDataIds&) = delete;
};

#endif
