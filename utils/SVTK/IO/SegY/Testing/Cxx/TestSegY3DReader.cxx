/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSegY2DReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkSegYReader
// .SECTION Description
//

#include "svtkSegYReader.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkDataSetMapper.h"
#include "svtkIdList.h"
#include "svtkNew.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStructuredGrid.h"
#include "svtkTestUtilities.h"

int TestSegY3DReader(int argc, char* argv[])
{
  // Basic visualisation.
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Read file name.
  char* fname;
  fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/waha8.sgy");

  svtkNew<svtkColorTransferFunction> lut;
  lut->AddRGBPoint(-127, 0.23, 0.30, 0.75);
  lut->AddRGBPoint(0.0, 0.86, 0.86, 0.86);
  lut->AddRGBPoint(126, 0.70, 0.02, 0.15);

  svtkNew<svtkSegYReader> reader;
  svtkNew<svtkDataSetMapper> mapper;
  svtkNew<svtkActor> actor;

  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetLookupTable(lut);
  mapper->SetColorModeToMapScalars();

  actor->SetMapper(mapper);

  ren->AddActor(actor);
  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(180);

  // interact with data
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
