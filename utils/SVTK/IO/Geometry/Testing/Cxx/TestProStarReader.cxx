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
// .NAME Test of svtkProStarReader
// .SECTION Description
//

#include "svtkDebugLeaks.h"
#include "svtkProStarReader.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkGeometryFilter.h"
#include "svtkIdList.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"

#include "svtkPNGWriter.h"
#include "svtkWindowToImageFilter.h"

int TestProStarReader(int argc, char* argv[])
{
  // Read file name.
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/prostar.vrt");

  // Create the reader.
  svtkSmartPointer<svtkProStarReader> reader = svtkSmartPointer<svtkProStarReader>::New();
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;
  svtkUnstructuredGrid* grid = svtkUnstructuredGrid::SafeDownCast(reader->GetOutput());
  if (grid->GetNumberOfPoints() != 44)
  {
    svtkGenericWarningMacro(
      "Input grid has " << grid->GetNumberOfPoints() << " but should have 44.");
    return 1;
  }
  if (grid->GetNumberOfCells() != 10)
  {
    svtkGenericWarningMacro("Input grid has " << grid->GetNumberOfCells() << " but should have 10.");
    return 1;
  }

  // There are render issues with 7 and 9 so we ignore those
  // for the tests.
  svtkUnstructuredGrid* newGrid = svtkUnstructuredGrid::New();
  newGrid->SetPoints(grid->GetPoints());
  newGrid->Allocate(8);
  svtkIdList* cellIds = svtkIdList::New();
  for (svtkIdType i = 0; i < grid->GetNumberOfCells(); i++)
  {
    if (i != 8 && i != 9)
    {
      grid->GetCellPoints(i, cellIds);
      newGrid->InsertNextCell(grid->GetCellType(i), cellIds);
    }
  }
  cellIds->Delete();

  // Convert to PolyData.
  svtkGeometryFilter* geometryFilter = svtkGeometryFilter::New();
  geometryFilter->SetInputData(newGrid);
  newGrid->Delete();

  // Create a mapper.
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(geometryFilter->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);

  // Basic visualisation.
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  svtkRenderer* ren = svtkRenderer::New();
  renWin->AddRenderer(ren);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);

  // interact with data
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  actor->Delete();
  mapper->Delete();
  geometryFilter->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  return !retVal;
}
