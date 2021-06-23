/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFrustumSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCamera.h>
#include <svtkFrustumSource.h>
#include <svtkMinimalStandardRandomSequence.h>
#include <svtkPlanes.h>
#include <svtkSmartPointer.h>

int TestFrustumSource(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkMinimalStandardRandomSequence> randomSequence =
    svtkSmartPointer<svtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  svtkSmartPointer<svtkFrustumSource> frustumSource = svtkSmartPointer<svtkFrustumSource>::New();
  frustumSource->ShowLinesOn();

  frustumSource->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);

  randomSequence->Next();
  double linesLength = randomSequence->GetValue();
  frustumSource->SetLinesLength(linesLength);

  svtkSmartPointer<svtkCamera> camera = svtkSmartPointer<svtkCamera>::New();

  double position[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    position[i] = randomSequence->GetValue();
  }
  camera->SetPosition(position);
  double focalPoint[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    focalPoint[i] = randomSequence->GetValue();
  }
  camera->SetFocalPoint(focalPoint);
  double planeCoefficients[24];
  camera->GetFrustumPlanes(1.0, planeCoefficients);

  svtkSmartPointer<svtkPlanes> planes = svtkSmartPointer<svtkPlanes>::New();
  planes->SetFrustumPlanes(planeCoefficients);
  frustumSource->SetPlanes(planes);

  frustumSource->Update();

  svtkSmartPointer<svtkPolyData> polyData = frustumSource->GetOutput();
  svtkSmartPointer<svtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  frustumSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);

  randomSequence->Next();
  linesLength = randomSequence->GetValue();
  frustumSource->SetLinesLength(linesLength);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    position[i] = randomSequence->GetValue();
  }
  camera->SetPosition(position);
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    focalPoint[i] = randomSequence->GetValue();
  }
  camera->SetFocalPoint(focalPoint);
  camera->GetFrustumPlanes(1.0, planeCoefficients);

  planes->SetFrustumPlanes(planeCoefficients);
  frustumSource->SetPlanes(planes);

  frustumSource->Update();

  polyData = frustumSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
