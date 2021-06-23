/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example shows how to manually create a structured grid.
// The basic idea is to instantiate svtkStructuredGrid, set its dimensions,
// and then assign points defining the grid coordinate. The number of
// points must equal the number of points implicit in the dimensions
// (i.e., dimX*dimY*dimZ). Also, data attributes (either point or cell)
// can be added to the dataset.
//
//
#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkFloatArray.h>
#include <svtkHedgeHog.h>
#include <svtkMath.h>
#include <svtkNamedColors.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkStructuredGrid.h>

#include <array>

int main()
{
  svtkNew<svtkNamedColors> colors;

  float rMin = 0.5, rMax = 1.0, deltaRad, deltaZ;
  std::array<int, 3> dims = { { 13, 11, 11 } };

  // Create the structured grid.
  svtkNew<svtkStructuredGrid> sgrid;
  sgrid->SetDimensions(dims.data());

  // We also create the points and vectors. The points
  // form a hemi-cylinder of data.
  svtkNew<svtkFloatArray> vectors;
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(dims[0] * dims[1] * dims[2]);
  svtkNew<svtkPoints> points;
  points->Allocate(dims[0] * dims[1] * dims[2]);

  deltaZ = 2.0 / (dims[2] - 1);
  deltaRad = (rMax - rMin) / (dims[1] - 1);
  float x[3], v[3];
  v[2] = 0.0;
  for (auto k = 0; k < dims[2]; k++)
  {
    x[2] = -1.0 + k * deltaZ;
    int kOffset = k * dims[0] * dims[1];
    for (auto j = 0; j < dims[1]; j++)
    {
      float radius = rMin + j * deltaRad;
      int jOffset = j * dims[0];
      for (auto i = 0; i < dims[0]; i++)
      {
        float theta = i * svtkMath::RadiansFromDegrees(15.0);
        x[0] = radius * cos(theta);
        x[1] = radius * sin(theta);
        v[0] = -x[1];
        v[1] = x[0];
        int offset = i + jOffset + kOffset;
        points->InsertPoint(offset, x);
        vectors->InsertTuple(offset, v);
      }
    }
  }
  sgrid->SetPoints(points);
  sgrid->GetPointData()->SetVectors(vectors);

  // We create a simple pipeline to display the data.
  svtkNew<svtkHedgeHog> hedgehog;
  hedgehog->SetInputData(sgrid);
  hedgehog->SetScaleFactor(0.1);

  svtkNew<svtkPolyDataMapper> sgridMapper;
  sgridMapper->SetInputConnection(hedgehog->GetOutputPort());
  svtkNew<svtkActor> sgridActor;
  sgridActor->SetMapper(sgridMapper);
  sgridActor->GetProperty()->SetColor(colors->GetColor3d("Indigo").GetData());

  // Create the usual rendering stuff
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renderer->AddActor(sgridActor);
  renderer->SetBackground(colors->GetColor3d("Cornsilk").GetData());
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(60.0);
  renderer->GetActiveCamera()->Azimuth(30.0);
  renderer->GetActiveCamera()->Zoom(1.0);
  renWin->SetSize(600, 600);

  // interact with data
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
