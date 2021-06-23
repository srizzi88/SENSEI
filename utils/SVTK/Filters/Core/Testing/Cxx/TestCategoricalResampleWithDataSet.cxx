/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoricalResampleWithDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkResampleWithDataSet.h"
#include "svtkSphereSource.h"

#include <cmath>

int TestCategoricalResampleWithDataSet(int, char*[])
{
  svtkNew<svtkImageData> imageData;
  imageData->SetExtent(-5, 5, -5, 5, -5, 5);
  imageData->AllocateScalars(SVTK_DOUBLE, 1);

  int* ext = imageData->GetExtent();

  double radius = 3.;
  double inValue = 10.;
  double outValue = -10.;

  for (int z = ext[0]; z < ext[1]; z++)
  {
    for (int y = ext[2]; y < ext[3]; y++)
    {
      for (int x = ext[4]; x < ext[5]; x++)
      {
        double* p = static_cast<double*>(imageData->GetScalarPointer(x, y, z));
        if (x * x + y * y + z * z < radius * radius)
        {
          p[0] = inValue;
        }
        else
        {
          p[0] = outValue;
        }
      }
    }
  }

  svtkNew<svtkSphereSource> sphere;
  sphere->SetRadius(radius);

  svtkNew<svtkResampleWithDataSet> probeFilter;
  probeFilter->SetInputConnection(sphere->GetOutputPort());
  probeFilter->SetSourceData(imageData);
  probeFilter->SetCategoricalData(true);
  probeFilter->Update();

  svtkDataSet* outputData = svtkDataSet::SafeDownCast(probeFilter->GetOutput());

  svtkDoubleArray* values = svtkDoubleArray::SafeDownCast(outputData->GetPointData()->GetScalars());

  static const double epsilon = 1.e-8;

  for (svtkIdType i = 0; i < values->GetNumberOfValues(); i++)
  {
    if (std::fabs(values->GetValue(i) - inValue) > epsilon &&
      std::fabs(values->GetValue(i) - outValue) > epsilon)
    {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
