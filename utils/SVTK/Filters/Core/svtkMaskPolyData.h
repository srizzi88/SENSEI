/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMaskPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMaskPolyData
 * @brief   sample subset of input polygonal data cells
 *
 * svtkMaskPolyData is a filter that sub-samples the cells of input polygonal
 * data. The user specifies every nth item, with an initial offset to begin
 * sampling.
 *
 * @sa
 * svtkMaskPoints
 */

#ifndef svtkMaskPolyData_h
#define svtkMaskPolyData_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkMaskPolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkMaskPolyData* New();
  svtkTypeMacro(svtkMaskPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Turn on every nth entity (cell).
   */
  svtkSetClampMacro(OnRatio, int, 1, SVTK_INT_MAX);
  svtkGetMacro(OnRatio, int);
  //@}

  //@{
  /**
   * Start with this entity (cell).
   */
  svtkSetClampMacro(Offset, svtkIdType, 0, SVTK_ID_MAX);
  svtkGetMacro(Offset, svtkIdType);
  //@}

protected:
  svtkMaskPolyData();
  ~svtkMaskPolyData() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int OnRatio;      // every OnRatio entity is on; all others are off.
  svtkIdType Offset; // offset (or starting point id)

private:
  svtkMaskPolyData(const svtkMaskPolyData&) = delete;
  void operator=(const svtkMaskPolyData&) = delete;
};

#endif
