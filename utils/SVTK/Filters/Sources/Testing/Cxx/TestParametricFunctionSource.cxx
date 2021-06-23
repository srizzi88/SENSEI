/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParametricFunctionSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkMinimalStandardRandomSequence.h>
#include <svtkParametricEllipsoid.h>
#include <svtkParametricFunctionSource.h>
#include <svtkSmartPointer.h>

int TestParametricFunctionSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkParametricFunctionSource> parametricFunctionSource =
    svtkSmartPointer<svtkParametricFunctionSource>::New();
  parametricFunctionSource->SetUResolution(64);
  parametricFunctionSource->SetVResolution(64);
  parametricFunctionSource->SetWResolution(64);
  parametricFunctionSource->SetScalarModeToNone();
  parametricFunctionSource->GenerateTextureCoordinatesOff();

  parametricFunctionSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  svtkSmartPointer<svtkParametricEllipsoid> parametricEllipsoid =
    svtkSmartPointer<svtkParametricEllipsoid>::New();

  randomSequence->Next();
  double xRadius = randomSequence->GetValue();
  parametricEllipsoid->SetXRadius(xRadius);

  randomSequence->Next();
  double yRadius = randomSequence->GetValue();
  parametricEllipsoid->SetYRadius(yRadius);

  randomSequence->Next();
  double zRadius = randomSequence->GetValue();
  parametricEllipsoid->SetZRadius(zRadius);

  parametricFunctionSource->SetParametricFunction(parametricEllipsoid);

  parametricFunctionSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = parametricFunctionSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  parametricFunctionSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  randomSequence->Next();
  xRadius = randomSequence->GetValue();
  parametricEllipsoid->SetXRadius(xRadius);

  randomSequence->Next();
  yRadius = randomSequence->GetValue();
  parametricEllipsoid->SetYRadius(yRadius);

  randomSequence->Next();
  zRadius = randomSequence->GetValue();
  parametricEllipsoid->SetZRadius(zRadius);

  parametricFunctionSource->SetParametricFunction(parametricEllipsoid);

  parametricFunctionSource->Update();

  polyData = parametricFunctionSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
