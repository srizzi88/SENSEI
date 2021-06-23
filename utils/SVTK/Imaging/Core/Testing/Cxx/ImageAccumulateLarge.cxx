/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageAccumulateLarge.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkImageAccumulate.h"
#include "svtkImageData.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"

int ImageAccumulateLarge(int argc, char* argv[])
{
  svtkIdType dim;
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " dimension" << std::endl;
    return EXIT_FAILURE;
  }
  // For routine testing (nightly, local) we keep these dimensions small
  // To test bin overflow, change the 32, to 2048
  dim = atoi(argv[1]);

  // Allocate an image
  svtkSmartPointer<svtkImageData> image = svtkSmartPointer<svtkImageData>::New();
  image->SetDimensions(dim, dim, dim);
  image->AllocateScalars(SVTK_UNSIGNED_CHAR, 1);

  // Initialize the image with zeroes and ones
  svtkIdType oneBinExpected = 10;
  svtkIdType zeroBinExpected = dim * dim * dim - oneBinExpected;

  memset(static_cast<void*>(image->GetScalarPointer(oneBinExpected, 0, 0)), 0, zeroBinExpected);
  memset(static_cast<void*>(image->GetScalarPointer(0, 0, 0)), 1, oneBinExpected);

  svtkSmartPointer<svtkImageAccumulate> filter = svtkSmartPointer<svtkImageAccumulate>::New();
  filter->SetInputData(image);
  filter->SetComponentExtent(0, 1, 0, 0, 0, 0);
  filter->SetComponentOrigin(0, 0, 0);
  filter->SetComponentSpacing(1, 1, 1);
  filter->Update();
  svtkIdType zeroBinResult = static_cast<svtkIdType>(
    *(static_cast<svtkIdType*>(filter->GetOutput()->GetScalarPointer(0, 0, 0))));
  svtkIdType oneBinResult = static_cast<svtkIdType>(
    *(static_cast<svtkIdType*>(filter->GetOutput()->GetScalarPointer(1, 0, 0))));
  int status = EXIT_SUCCESS;
  if (zeroBinResult != zeroBinExpected)
  {
    std::cout << "Expected the 0 bin count to be " << zeroBinExpected << " but got "
              << zeroBinResult << std::endl;
    status = EXIT_FAILURE;
  }
  if (oneBinResult != oneBinExpected)
  {
    std::cout << "Expected the 1 bin count to be " << oneBinExpected << " but got " << oneBinResult
              << std::endl;
    status = EXIT_FAILURE;
  }

  return status;
}
