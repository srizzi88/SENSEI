/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTexturedSphereSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkMinimalStandardRandomSequence.h>
#include <svtkSmartPointer.h>
#include <svtkTexturedSphereSource.h>

int TestTexturedSphereSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkTexturedSphereSource> texturedSphereSource =
    svtkSmartPointer<svtkTexturedSphereSource>::New();
  texturedSphereSource->SetThetaResolution(8);
  texturedSphereSource->SetPhiResolution(8);
  texturedSphereSource->SetTheta(0.0);
  texturedSphereSource->SetPhi(0.0);

  texturedSphereSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  randomSequence->Next();
  double radius = randomSequence->GetValue();
  texturedSphereSource->SetRadius(radius);

  texturedSphereSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = texturedSphereSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  texturedSphereSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  randomSequence->Next();
  radius = randomSequence->GetValue();
  texturedSphereSource->SetRadius(radius);

  texturedSphereSource->Update();

  polyData = texturedSphereSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
