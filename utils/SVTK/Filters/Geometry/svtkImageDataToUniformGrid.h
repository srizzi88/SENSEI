/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataToUniformGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDataToUniformGrid
 * @brief   convert svtkImageData to svtkUniformGrid
 *
 * Convert a svtkImageData to svtkUniformGrid and set blanking based on
 * specified by named arrays. By default, values of 0 in the named
 * array will result in the point or cell being blanked. Set Reverse
 * to 1 to indicate that values of 0 will result in the point or
 * cell to not be blanked.
 */

#ifndef svtkImageDataToUniformGrid_h
#define svtkImageDataToUniformGrid_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersGeometryModule.h" // For export macro

class svtkDataArray;
class svtkFieldData;
class svtkImageData;
class svtkUniformGrid;

class SVTKFILTERSGEOMETRY_EXPORT svtkImageDataToUniformGrid : public svtkDataObjectAlgorithm
{
public:
  static svtkImageDataToUniformGrid* New();
  svtkTypeMacro(svtkImageDataToUniformGrid, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * By default, values of 0 (i.e. Reverse = 0) in the array will
   * result in that point or cell to be blanked. Set Reverse to
   * 1 to make points or cells to not be blanked for array values
   * of 0.
   */
  svtkSetClampMacro(Reverse, svtkTypeBool, 0, 1);
  svtkGetMacro(Reverse, svtkTypeBool);
  svtkBooleanMacro(Reverse, svtkTypeBool);
  //@}

protected:
  svtkImageDataToUniformGrid();
  ~svtkImageDataToUniformGrid() override;

  int RequestData(
    svtkInformation* req, svtkInformationVector** inV, svtkInformationVector* outV) override;
  int RequestDataObject(
    svtkInformation* req, svtkInformationVector** inV, svtkInformationVector* outV) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  virtual int Process(
    svtkImageData* input, int association, const char* arrayName, svtkUniformGrid* output);

private:
  svtkImageDataToUniformGrid(const svtkImageDataToUniformGrid&) = delete;
  void operator=(const svtkImageDataToUniformGrid&) = delete;

  svtkTypeBool Reverse;
};

#endif
