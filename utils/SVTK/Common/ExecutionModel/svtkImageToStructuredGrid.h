/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkImageDataToStructuredGridFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkImageToStructuredGrid
 * a structured grid instance.
 *
 *
 * A concrete instance of svtkStructuredGridAlgorithm which provides
 * functionality for converting instances of svtkImageData to svtkStructuredGrid.
 */

#ifndef svtkImageToStructuredGrid_h
#define svtkImageToStructuredGrid_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkStructuredGridAlgorithm.h"

class svtkStructuredGrid;
class svtkImageData;
class svtkInformation;
class svtkInformationVector;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkImageToStructuredGrid : public svtkStructuredGridAlgorithm
{
public:
  static svtkImageToStructuredGrid* New();
  svtkTypeMacro(svtkImageToStructuredGrid, svtkStructuredGridAlgorithm);
  void PrintSelf(ostream& oss, svtkIndent indent) override;

protected:
  svtkImageToStructuredGrid();
  ~svtkImageToStructuredGrid() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //@{
  /**
   * Helper function to copy point/cell data from image to grid
   */
  void CopyPointData(svtkImageData*, svtkStructuredGrid*);
  void CopyCellData(svtkImageData*, svtkStructuredGrid*);
  //@}

  int FillInputPortInformation(int, svtkInformation* info) override;
  int FillOutputPortInformation(int, svtkInformation* info) override;

private:
  svtkImageToStructuredGrid(const svtkImageToStructuredGrid&) = delete;
  void operator=(const svtkImageToStructuredGrid&) = delete;
};

#endif /* SVTKIMAGEDATATOSTRUCTUREDGRIDFILTER_H_ */
