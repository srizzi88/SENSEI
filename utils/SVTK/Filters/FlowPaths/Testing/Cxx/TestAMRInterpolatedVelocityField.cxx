/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestsvtkAMRInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkUniformGrid.h"
#include <svtkAMRGaussianPulseSource.h>
#include <svtkAMRInterpolatedVelocityField.h>
#include <svtkCompositeDataPipeline.h>
#include <svtkGradientFilter.h>
#include <svtkMath.h>
#include <svtkNew.h>
#include <svtkOverlappingAMR.h>
#define RETURNONFALSE(b)                                                                           \
  if (!(b))                                                                                        \
  {                                                                                                \
    svtkAlgorithm::SetDefaultExecutivePrototype(nullptr);                                           \
    return EXIT_FAILURE;                                                                           \
  }

int TestAMRInterpolatedVelocityField(int, char*[])
{
  svtkNew<svtkCompositeDataPipeline> cexec;
  svtkAlgorithm::SetDefaultExecutivePrototype(cexec);

  char name[100] = "Gaussian-Pulse";
  svtkNew<svtkAMRGaussianPulseSource> imageSource;
  svtkNew<svtkGradientFilter> gradientFilter;
  gradientFilter->SetInputConnection(imageSource->GetOutputPort());
  gradientFilter->SetInputScalars(svtkDataObject::FIELD_ASSOCIATION_CELLS, name);
  gradientFilter->SetResultArrayName("Gradient");
  gradientFilter->Update();

  svtkOverlappingAMR* amrGrad =
    svtkOverlappingAMR::SafeDownCast(gradientFilter->GetOutputDataObject(0));
  amrGrad->GenerateParentChildInformation();
  for (unsigned int datasetLevel = 0; datasetLevel < amrGrad->GetNumberOfLevels(); datasetLevel++)
  {
    for (unsigned int id = 0; id < amrGrad->GetNumberOfDataSets(datasetLevel); id++)
    {
      svtkUniformGrid* grid = amrGrad->GetDataSet(datasetLevel, id);
      int numBlankedCells(0);
      for (int i = 0; i < grid->GetNumberOfCells(); i++)
      {
        numBlankedCells += grid->IsCellVisible(i) ? 0 : 1;
      }
      cout << numBlankedCells << " ";
    }
  }
  cout << endl;

  svtkNew<svtkAMRInterpolatedVelocityField> func;
  func->SetAMRData(amrGrad);
  func->SelectVectors(svtkDataObject::FIELD_ASSOCIATION_CELLS, "Gradient");

  double Points[4][3] = {
    { -2.1, -0.51, 1 },
    { -1.9, -0.51, 1 },
    { -0.9, -0.51, 1 },
    { -0.1, -0.51, 1 },
  };

  double v[3];
  bool res;
  unsigned int level, id;
  res = func->FunctionValues(Points[0], v) != 0;
  RETURNONFALSE(!res);
  res = func->FunctionValues(Points[1], v) != 0;
  RETURNONFALSE(res);
  func->GetLastDataSetLocation(level, id);
  RETURNONFALSE(level == 1)
  res = func->FunctionValues(Points[2], v) != 0;
  RETURNONFALSE(res);
  func->GetLastDataSetLocation(level, id);
  RETURNONFALSE(level == 0)
  res = func->FunctionValues(Points[3], v) != 0;
  RETURNONFALSE(res);
  func->GetLastDataSetLocation(level, id);
  RETURNONFALSE(level == 1)

  svtkAlgorithm::SetDefaultExecutivePrototype(nullptr);
  return EXIT_SUCCESS;
}
