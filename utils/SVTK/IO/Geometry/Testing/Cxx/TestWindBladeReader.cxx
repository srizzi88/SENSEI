/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProStarReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkWindBladeReader
// .SECTION Description
// Tests the svtkWindBladeReader.

#include "svtkWindBladeReader.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkGeometryFilter.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"

void AddColor(svtkDataSet* grid)
{
  svtkFloatArray* color = svtkFloatArray::New();
  color->SetNumberOfTuples(grid->GetNumberOfPoints());
  for (svtkIdType i = 0; i < grid->GetNumberOfPoints(); i++)
  {
    color->SetValue(i, 1.);
  }
  color->SetName("Density");
  grid->GetPointData()->AddArray(color);
  grid->GetPointData()->SetScalars(color);
  color->Delete();
}

int TestWindBladeReader(int argc, char* argv[])
{
  // Read file name.
  char* fname =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/WindBladeReader/test1_topo.wind");

  // Create the reader.
  svtkSmartPointer<svtkWindBladeReader> reader = svtkSmartPointer<svtkWindBladeReader>::New();
  reader->SetFilename(fname);
  delete[] fname;

  // Convert to PolyData.
  svtkGeometryFilter* fieldGeometryFilter = svtkGeometryFilter::New();
  fieldGeometryFilter->SetInputConnection(reader->GetOutputPort());
  svtkGeometryFilter* bladeGeometryFilter = svtkGeometryFilter::New();
  bladeGeometryFilter->SetInputConnection(reader->GetOutputPort(1));
  svtkGeometryFilter* groundGeometryFilter = svtkGeometryFilter::New();
  groundGeometryFilter->SetInputConnection(reader->GetOutputPort(2));

  fieldGeometryFilter->UpdateInformation();
  svtkExecutive* executive = fieldGeometryFilter->GetExecutive();
  svtkInformationVector* inputVector = executive->GetInputInformation(0);
  double timeReq = 10;
  inputVector->GetInformationObject(0)->Set(
    svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);

  bladeGeometryFilter->UpdateInformation();
  executive = bladeGeometryFilter->GetExecutive();
  inputVector = executive->GetInputInformation(0);
  inputVector->GetInformationObject(0)->Set(
    svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);

  reader->Update();
  bladeGeometryFilter->Update();
  groundGeometryFilter->Update();
  AddColor(bladeGeometryFilter->GetOutput());
  AddColor(groundGeometryFilter->GetOutput());

  // Create a mapper.
  svtkPolyDataMapper* fieldMapper = svtkPolyDataMapper::New();
  fieldMapper->SetInputConnection(fieldGeometryFilter->GetOutputPort());
  fieldMapper->ScalarVisibilityOn();
  fieldMapper->SetColorModeToMapScalars();
  fieldMapper->SetScalarRange(.964, 1.0065);
  fieldMapper->SetScalarModeToUsePointFieldData();
  fieldMapper->SelectColorArray("Density");

  svtkPolyDataMapper* bladeMapper = svtkPolyDataMapper::New();
  bladeMapper->SetInputConnection(bladeGeometryFilter->GetOutputPort());
  bladeMapper->ScalarVisibilityOn();
  svtkPolyDataMapper* groundMapper = svtkPolyDataMapper::New();
  groundMapper->SetInputConnection(groundGeometryFilter->GetOutputPort());
  groundMapper->ScalarVisibilityOn();

  // Create the actor.
  svtkActor* fieldActor = svtkActor::New();
  fieldActor->SetMapper(fieldMapper);
  svtkActor* bladeActor = svtkActor::New();
  bladeActor->SetMapper(bladeMapper);
  double position[3];
  bladeActor->GetPosition(position);
  bladeActor->RotateZ(90);
  bladeActor->SetPosition(position[0] + 100, position[1] + 100, position[2] - 150);
  svtkActor* groundActor = svtkActor::New();
  groundActor->SetMapper(groundMapper);

  // Basic visualisation.
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  svtkRenderer* ren = svtkRenderer::New();
  renWin->AddRenderer(ren);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  svtkCamera* camera = ren->GetActiveCamera();
  double bounds[6];
  reader->GetFieldOutput()->GetBounds(bounds);
  bounds[2] -= 150;
  ren->ResetCamera(bounds);
  camera->Elevation(-90);
  camera->SetViewUp(0, 0, 1);
  camera->Zoom(1.2);

  ren->AddActor(fieldActor);
  ren->AddActor(bladeActor);
  ren->AddActor(groundActor);
  ren->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);

  // interact with data
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  fieldActor->Delete();
  bladeActor->Delete();
  groundActor->Delete();
  fieldMapper->Delete();
  bladeMapper->Delete();
  groundMapper->Delete();
  fieldGeometryFilter->Delete();
  bladeGeometryFilter->Delete();
  groundGeometryFilter->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  return !retVal;
}
