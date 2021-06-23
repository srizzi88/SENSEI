/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataSetGradientPrecompute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkDataSetGradient.h>
#include <svtkDataSetGradientPrecompute.h>
#include <svtkSmartPointer.h>

#include <svtkArrowSource.h>
#include <svtkGlyph3D.h>
#include <svtkMaskPoints.h>

#include <svtkCellData.h>
#include <svtkDoubleArray.h>
#include <svtkGenericCell.h>
#include <svtkPointData.h>

#include <svtkUnstructuredGrid.h>
#include <svtkUnstructuredGridReader.h>

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

#include "svtkTestUtilities.h"

// NOTE: This test is identical to TestDataSetGradient except is uses
// svtkDataSetGradientPrecompute to compute the gradients.

int TestDataSetGradientPrecompute(int argc, char* argv[])
{
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/hexa.svtk");

  // Read the data
  svtkSmartPointer<svtkUnstructuredGridReader> reader =
    svtkSmartPointer<svtkUnstructuredGridReader>::New();
  reader->SetFileName(fileName);
  delete[] fileName;

  svtkSmartPointer<svtkDataSetGradientPrecompute> gradientPrecompute =
    svtkSmartPointer<svtkDataSetGradientPrecompute>::New();
  gradientPrecompute->SetInputConnection(reader->GetOutputPort());

  // This class computes the gradient for each cell
  svtkSmartPointer<svtkDataSetGradient> gradient = svtkSmartPointer<svtkDataSetGradient>::New();
  gradient->SetInputConnection(gradientPrecompute->GetOutputPort());
  gradient->SetInputArrayToProcess(0, 0, 0, 0, "scalars");
  gradient->Update();

  // Create a polydata
  //  Points at the parametric center of each cell
  //  PointData contains the gradient
  svtkDoubleArray* gradientAtCenters =
    svtkDoubleArray::SafeDownCast(gradient->GetOutput()->GetCellData()->GetArray("gradient"));

  svtkSmartPointer<svtkDoubleArray> gradients = svtkSmartPointer<svtkDoubleArray>::New();
  gradients->ShallowCopy(gradientAtCenters);

  svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->SetNumberOfPoints(gradient->GetOutput()->GetNumberOfCells());

  svtkSmartPointer<svtkGenericCell> aCell = svtkSmartPointer<svtkGenericCell>::New();
  for (svtkIdType cellId = 0; cellId < gradient->GetOutput()->GetNumberOfCells(); ++cellId)
  {
    gradient->GetOutput()->GetCell(cellId, aCell);
    reader->GetOutput()->GetCell(cellId, aCell);

    double pcenter[3], center[3];
    aCell->GetParametricCenter(pcenter);
    std::vector<double> cweights(aCell->GetNumberOfPoints());
    int pSubId = 0;
    aCell->EvaluateLocation(pSubId, pcenter, center, &(*cweights.begin()));
    points->SetPoint(cellId, center);
  }
  polyData->SetPoints(points);
  polyData->GetPointData()->SetVectors(gradientAtCenters);

  // Select a small percentage of the gradients
  // Use 10% of the points
  int onRatio =
    reader->GetOutput()->GetNumberOfPoints() / (reader->GetOutput()->GetNumberOfPoints() * .1);

  svtkSmartPointer<svtkMaskPoints> maskPoints = svtkSmartPointer<svtkMaskPoints>::New();
  maskPoints->SetInputData(polyData);
  maskPoints->RandomModeOff();
  maskPoints->SetOnRatio(onRatio);

  // Create the Glyphs for the gradient
  svtkSmartPointer<svtkArrowSource> arrowSource = svtkSmartPointer<svtkArrowSource>::New();

  double scaleFactor = .005;
  svtkSmartPointer<svtkGlyph3D> vectorGradientGlyph = svtkSmartPointer<svtkGlyph3D>::New();
  vectorGradientGlyph->SetSourceConnection(arrowSource->GetOutputPort());
  vectorGradientGlyph->SetInputConnection(maskPoints->GetOutputPort());
  vectorGradientGlyph->SetScaleModeToScaleByVector();
  vectorGradientGlyph->SetVectorModeToUseVector();
  vectorGradientGlyph->SetScaleFactor(scaleFactor);

  svtkSmartPointer<svtkPolyDataMapper> vectorGradientMapper =
    svtkSmartPointer<svtkPolyDataMapper>::New();
  vectorGradientMapper->SetInputConnection(vectorGradientGlyph->GetOutputPort());
  vectorGradientMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> vectorGradientActor = svtkSmartPointer<svtkActor>::New();
  vectorGradientActor->SetMapper(vectorGradientMapper);
  vectorGradientActor->GetProperty()->SetColor(1.0000, 0.3882, 0.2784);

  // Create a renderer, render window, and interactor
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->SetBackground(.5, .5, .5);

  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Add the actor to the scene
  renderer->AddActor(vectorGradientActor);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(120);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->GetActiveCamera()->Dolly(1.0);
  renderer->ResetCameraClippingRange();

  // Render and interact
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
