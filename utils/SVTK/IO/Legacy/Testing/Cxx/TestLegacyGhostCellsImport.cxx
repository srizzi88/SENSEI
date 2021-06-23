/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLegacyGhostCellsImport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test converting from a svtkGhostLevels to svtkGhostType
// see http://www.kitware.com/blog/home/post/856
// Ghost and Blanking (Visibility) Changes

#include "svtkActor.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkGeometryFilter.h"
#include "svtkNew.h"
#include "svtkPoints.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTesting.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridReader.h"

int TestLegacyGhostCellsImport(int argc, char* argv[])
{
  svtkNew<svtkTesting> testing;
  testing->AddArguments(argc, argv);

  std::string filename = testing->GetDataRoot();
  filename += "/Data/ghost_cells.svtk";

  svtkNew<svtkUnstructuredGridReader> reader;
  reader->SetFileName(filename.c_str());

  // this filter removes the ghost cells
  svtkNew<svtkGeometryFilter> surfaces;
  surfaces->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surfaces->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);

  svtkNew<svtkRenderWindow> renwin;
  renwin->AddRenderer(renderer);
  renwin->SetSize(300, 300);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin);
  iren->Initialize();

  renwin->Render();

  int retVal = svtkRegressionTestImage(renwin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
