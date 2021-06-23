/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLGhostCellsImport.cxx

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
#include "svtkXMLUnstructuredGridReader.h"
#include "svtkXMLUnstructuredGridWriter.h"

// ghost_cells.vtu was created using this function
svtkSmartPointer<svtkUnstructuredGrid> CreateThreeTetra()
{
  svtkNew<svtkPoints> points;
  points->InsertPoint(0, 0, 0, 0);
  points->InsertPoint(1, 1, 0, 0);
  points->InsertPoint(2, 0.5, 1, 0);
  points->InsertPoint(3, 0.5, 0.5, 1);
  points->InsertPoint(4, 0.5, -1, 0);
  points->InsertPoint(5, 0.5, -0.5, 1);

  svtkIdType v[3][4] = { { 0, 1, 2, 3 }, { 0, 4, 1, 5 }, { 5, 3, 1, 0 } };

  svtkSmartPointer<svtkUnstructuredGrid> grid = svtkSmartPointer<svtkUnstructuredGrid>::New();
  grid->InsertNextCell(SVTK_TETRA, 4, v[0]);
  grid->InsertNextCell(SVTK_TETRA, 4, v[1]);
  grid->InsertNextCell(SVTK_TETRA, 4, v[2]);
  grid->SetPoints(points);

  svtkNew<svtkUnsignedCharArray> ghosts;
  ghosts->InsertNextValue(0);
  ghosts->InsertNextValue(1);
  ghosts->InsertNextValue(2);
  ghosts->SetName("svtkGhostLevels");
  grid->GetCellData()->AddArray(ghosts);

  return grid;
}

void WriteThreeTetra()
{
  svtkSmartPointer<svtkUnstructuredGrid> grid = CreateThreeTetra();

  svtkNew<svtkXMLUnstructuredGridWriter> writer;
  writer->SetInputData(grid);
  writer->SetFileName("ghost_cells.vtu");
  writer->Write();
}

int TestXMLGhostCellsImport(int argc, char* argv[])
{
  svtkNew<svtkTesting> testing;
  testing->AddArguments(argc, argv);

  // this was used to generate ghost_cells.vtu
  // WriteThreeTetra();

  std::string filename = testing->GetDataRoot();
  filename += "/Data/ghost_cells.vtu";

  svtkNew<svtkXMLUnstructuredGridReader> reader;
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
