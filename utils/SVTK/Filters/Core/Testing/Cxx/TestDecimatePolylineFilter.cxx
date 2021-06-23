/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDecimatePolylineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "svtkRegressionTestImage.h"
#include <svtkActor.h>
#include <svtkCellArray.h>
#include <svtkDecimatePolylineFilter.h>
#include <svtkMath.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>

int TestDecimatePolylineFilter(int argc, char* argv[])
{
  const unsigned int numberOfPointsInCircle = 100;

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->SetDataType(SVTK_FLOAT);

  // We will create two polylines: one complete circle, and one circular arc
  // subtending 3/4 of a circle.
  svtkIdType* lineIds = new svtkIdType[(numberOfPointsInCircle * 7) / 4 + 1];

  svtkIdType lineIdCounter = 0;

  // First circle:
  for (unsigned int i = 0; i < numberOfPointsInCircle; ++i)
  {
    const double angle =
      2.0 * svtkMath::Pi() * static_cast<double>(i) / static_cast<double>(numberOfPointsInCircle);
    points->InsertPoint(static_cast<svtkIdType>(i), std::cos(angle), std::sin(angle), 0.0);
    lineIds[i] = lineIdCounter++;
  }
  lineIds[numberOfPointsInCircle] = 0;

  // Second circular arc:
  for (unsigned int i = 0; i < (numberOfPointsInCircle * 3) / 4; ++i)
  {
    const double angle = 3.0 / 2.0 * svtkMath::Pi() * static_cast<double>(i) /
      static_cast<double>((numberOfPointsInCircle * 3) / 4);
    points->InsertPoint(
      static_cast<svtkIdType>(i + numberOfPointsInCircle), std::cos(angle), std::sin(angle), 1.0);
    lineIds[numberOfPointsInCircle + 1 + i] = lineIdCounter++;
  }

  // Construct associated cell array, containing both polylines.
  svtkSmartPointer<svtkCellArray> lines = svtkSmartPointer<svtkCellArray>::New();
  // 1st:
  lines->InsertNextCell(numberOfPointsInCircle + 1, lineIds);
  // 2nd:
  lines->InsertNextCell((numberOfPointsInCircle * 3) / 4, &lineIds[numberOfPointsInCircle + 1]);
  delete[] lineIds;

  svtkSmartPointer<svtkPolyData> circles = svtkSmartPointer<svtkPolyData>::New();
  circles->SetPoints(points);
  circles->SetLines(lines);

  svtkSmartPointer<svtkPolyDataMapper> circleMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  circleMapper->SetInputData(circles);

  svtkSmartPointer<svtkActor> circleActor = svtkSmartPointer<svtkActor>::New();
  circleActor->SetMapper(circleMapper);

  svtkSmartPointer<svtkDecimatePolylineFilter> decimatePolylineFilter =
    svtkSmartPointer<svtkDecimatePolylineFilter>::New();
  decimatePolylineFilter->SetOutputPointsPrecision(svtkAlgorithm::DEFAULT_PRECISION);
  decimatePolylineFilter->SetInputData(circles);
  decimatePolylineFilter->SetTargetReduction(0.9);
  decimatePolylineFilter->Update();

  if (decimatePolylineFilter->GetOutput()->GetPoints()->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  decimatePolylineFilter->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);
  decimatePolylineFilter->Update();

  if (decimatePolylineFilter->GetOutput()->GetPoints()->GetDataType() != SVTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  decimatePolylineFilter->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);
  decimatePolylineFilter->Update();

  if (decimatePolylineFilter->GetOutput()->GetPoints()->GetDataType() != SVTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  svtkSmartPointer<svtkPolyDataMapper> decimatedMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  decimatedMapper->SetInputConnection(decimatePolylineFilter->GetOutputPort());

  svtkSmartPointer<svtkActor> decimatedActor = svtkSmartPointer<svtkActor>::New();
  decimatedActor->SetMapper(decimatedMapper);
  decimatedActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->AddActor(circleActor);
  renderer->AddActor(decimatedActor);

  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(300, 300);

  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->Render();

  int retVal = svtkRegressionTestImageThreshold(renderWindow, 0.3);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
