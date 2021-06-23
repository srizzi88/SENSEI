/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Arrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrate the use of SVTK data arrays as attribute
// data as well as field data. It creates geometry (svtkPolyData) as
// well as attribute data explicitly.

// First include the required header files for the svtk classes we are using.
#include <svtkActor.h>
#include <svtkCellArray.h>
#include <svtkDoubleArray.h>
#include <svtkFloatArray.h>
#include <svtkIntArray.h>
#include <svtkNamedColors.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

#include <array>

int main()
{
  svtkNew<svtkNamedColors> colors;

  // Create a float array which represents the points.
  svtkNew<svtkDoubleArray> pcoords;

  // Note that by default, an array has 1 component.
  // We have to change it to 3 for points.
  pcoords->SetNumberOfComponents(3);
  // We ask pcoords to allocate room for at least 4 tuples
  // and set the number of tuples to 4.
  pcoords->SetNumberOfTuples(4);
  // Assign each tuple. There are 5 specialized versions of SetTuple:
  // SetTuple1 SetTuple2 SetTuple3 SetTuple4 SetTuple9
  // These take 1, 2, 3, 4 and 9 components respectively.
  std::array<std::array<double, 3>, 4> pts = { { { { 0.0, 0.0, 0.0 } }, { { 0.0, 1.0, 0.0 } },
    { { 1.0, 0.0, 0.0 } }, { { 1.0, 1.0, 0.0 } } } };
  for (auto i = 0ul; i < pts.size(); ++i)
  {
    pcoords->SetTuple(i, pts[i].data());
  }

  // Create svtkPoints and assign pcoords as the internal data array.
  svtkNew<svtkPoints> points;
  points->SetData(pcoords);

  // Create the cells. In this case, a triangle strip with 2 triangles
  // (which can be represented by 4 points).
  svtkNew<svtkCellArray> strips;
  strips->InsertNextCell(4);
  strips->InsertCellPoint(0);
  strips->InsertCellPoint(1);
  strips->InsertCellPoint(2);
  strips->InsertCellPoint(3);

  // Create an integer array with 4 tuples. Note that when using
  // InsertNextValue (or InsertNextTuple1 which is equivalent in
  // this situation), the array will expand automatically.
  svtkNew<svtkIntArray> temperature;
  temperature->SetName("Temperature");
  temperature->InsertNextValue(10);
  temperature->InsertNextValue(20);
  temperature->InsertNextValue(30);
  temperature->InsertNextValue(40);

  // Create a double array.
  svtkNew<svtkDoubleArray> vorticity;
  vorticity->SetName("Vorticity");
  vorticity->InsertNextValue(2.7);
  vorticity->InsertNextValue(4.1);
  vorticity->InsertNextValue(5.3);
  vorticity->InsertNextValue(3.4);

  // Create the dataset. In this case, we create a svtkPolyData
  svtkNew<svtkPolyData> polydata;
  // Assign points and cells
  polydata->SetPoints(points);
  polydata->SetStrips(strips);
  // Assign scalars
  polydata->GetPointData()->SetScalars(temperature);
  // Add the vorticity array. In this example, this field
  // is not used.
  polydata->GetPointData()->AddArray(vorticity);

  // Create the mapper and set the appropriate scalar range
  // (default is (0,1)
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);
  mapper->SetScalarRange(0, 40);

  // Create an actor.
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  // Create the rendering objects.
  svtkNew<svtkRenderer> ren;
  ren->AddActor(actor);
  ren->SetBackground(colors->GetColor3d("DarkSlateGray").GetData());

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetSize(600, 600);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
