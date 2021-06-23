/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkNamedColors.h>

#include "svtkRegressionTestImage.h"
#include <svtkActor.h>
#include <svtkAlgorithm.h>
#include <svtkBandedPolyDataContourFilter.h>
#include <svtkConeSource.h>
#include <svtkElevationFilter.h>
#include <svtkLookupTable.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>

// Create a cone, contour it using the banded contour filter and
// color it with the primary additive and subtractive colors.
int TestNamedColorsIntegration(int argc, char* argv[])
{
  svtkSmartPointer<svtkNamedColors> namedColors = svtkSmartPointer<svtkNamedColors>::New();
  // namedColors->PrintSelf(std::cout,svtkIndent(2));

  // Create a cone
  svtkSmartPointer<svtkConeSource> coneSource = svtkSmartPointer<svtkConeSource>::New();
  coneSource->SetCenter(0.0, 0.0, 0.0);
  coneSource->SetRadius(5.0);
  coneSource->SetHeight(10);
  coneSource->SetDirection(0, 1, 0);
  coneSource->Update();

  double bounds[6];
  coneSource->GetOutput()->GetBounds(bounds);

  svtkSmartPointer<svtkElevationFilter> elevation = svtkSmartPointer<svtkElevationFilter>::New();
  elevation->SetInputConnection(coneSource->GetOutputPort());
  elevation->SetLowPoint(0, bounds[2], 0);
  elevation->SetHighPoint(0, bounds[3], 0);

  svtkSmartPointer<svtkBandedPolyDataContourFilter> bcf =
    svtkSmartPointer<svtkBandedPolyDataContourFilter>::New();
  bcf->SetInputConnection(elevation->GetOutputPort());
  bcf->SetScalarModeToValue();
  bcf->GenerateContourEdgesOn();
  bcf->GenerateValues(7, elevation->GetScalarRange());

  // Build a simple lookup table of
  // primary additive and subtractive colors.
  svtkSmartPointer<svtkLookupTable> lut = svtkSmartPointer<svtkLookupTable>::New();
  lut->SetNumberOfTableValues(7);
  double rgba[4];
  // Test setting and getting colors here.
  namedColors->GetColor("Red", rgba);
  namedColors->SetColor("My Red", rgba);
  namedColors->GetColor("My Red", rgba);
  lut->SetTableValue(0, rgba);
  namedColors->GetColor("DarkGreen", rgba);
  lut->SetTableValue(1, rgba);
  // Alternatively we can use tuple methods here:
  lut->SetTableValue(2, namedColors->GetColor4d("Blue").GetData());
  lut->SetTableValue(3, namedColors->GetColor4d("Cyan").GetData());
  lut->SetTableValue(4, namedColors->GetColor4d("Magenta").GetData());
  lut->SetTableValue(5, namedColors->GetColor4d("Yellow").GetData());
  lut->SetTableValue(6, namedColors->GetColor4d("White").GetData());
  lut->SetTableRange(elevation->GetScalarRange());
  lut->Build();

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(bcf->GetOutputPort());
  mapper->SetScalarRange(elevation->GetScalarRange());
  mapper->SetLookupTable(lut);
  mapper->SetScalarModeToUseCellData();

  svtkSmartPointer<svtkPolyDataMapper> contourLineMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  contourLineMapper->SetInputData(bcf->GetContourEdgesOutput());
  contourLineMapper->SetScalarRange(elevation->GetScalarRange());
  contourLineMapper->SetResolveCoincidentTopologyToPolygonOffset();

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkActor> contourLineActor = svtkSmartPointer<svtkActor>::New();
  contourLineActor->SetMapper(contourLineMapper);
  contourLineActor->GetProperty()->SetColor(namedColors->GetColor3d("Black").GetData());

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->AddActor(contourLineActor);
  renderer->SetBackground(namedColors->GetColor3d("SteelBlue").GetData());
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
