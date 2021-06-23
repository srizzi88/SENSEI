/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlankStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBlankStructuredGrid
 * @brief   translate point attribute data into a blanking field
 *
 *
 * svtkBlankStructuredGrid is a filter that sets the blanking field in a
 * svtkStructuredGrid dataset. The blanking field is set by examining a
 * specified point attribute data array (e.g., scalars) and converting
 * values in the data array to either a "1" (visible) or "0" (blanked) value
 * in the blanking array. The values to be blanked are specified by giving
 * a min/max range. All data values in the data array indicated and laying
 * within the range specified (inclusive on both ends) are translated to
 * a "off" blanking value.
 *
 * @sa
 * svtkStructuredGrid
 */

#ifndef svtkBlankStructuredGrid_h
#define svtkBlankStructuredGrid_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkStructuredGridAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkBlankStructuredGrid : public svtkStructuredGridAlgorithm
{
public:
  static svtkBlankStructuredGrid* New();
  svtkTypeMacro(svtkBlankStructuredGrid, svtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the lower data value in the data array specified which will be
   * converted into a "blank" (or off) value in the blanking array.
   */
  svtkSetMacro(MinBlankingValue, double);
  svtkGetMacro(MinBlankingValue, double);
  //@}

  //@{
  /**
   * Specify the upper data value in the data array specified which will be
   * converted into a "blank" (or off) value in the blanking array.
   */
  svtkSetMacro(MaxBlankingValue, double);
  svtkGetMacro(MaxBlankingValue, double);
  //@}

  //@{
  /**
   * Specify the data array name to use to generate the blanking
   * field. Alternatively, you can specify the array id. (If both are set,
   * the array name takes precedence.)
   */
  svtkSetStringMacro(ArrayName);
  svtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Specify the data array id to use to generate the blanking
   * field. Alternatively, you can specify the array name. (If both are set,
   * the array name takes precedence.)
   */
  svtkSetMacro(ArrayId, int);
  svtkGetMacro(ArrayId, int);
  //@}

  //@{
  /**
   * Specify the component in the data array to use to generate the blanking
   * field.
   */
  svtkSetClampMacro(Component, int, 0, SVTK_INT_MAX);
  svtkGetMacro(Component, int);
  //@}

protected:
  svtkBlankStructuredGrid();
  ~svtkBlankStructuredGrid() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double MinBlankingValue;
  double MaxBlankingValue;
  char* ArrayName;
  int ArrayId;
  int Component;

private:
  svtkBlankStructuredGrid(const svtkBlankStructuredGrid&) = delete;
  void operator=(const svtkBlankStructuredGrid&) = delete;
};

#endif
