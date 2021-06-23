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

int TestSegY2DReader(int argc, char* argv[])
{
  // Basic visualisation.
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Read file name.
  char* fname[5];
  fname[0] = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineA.sgy");
  fname[1] = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineB.sgy");
  fname[2] = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineC.sgy");
  fname[3] = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineD.sgy");
  fname[4] = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineE.sgy");

  svtkNew<svtkColorTransferFunction> lut;
  lut->AddRGBPoint(-6.4, 0.23, 0.30, 0.75);
  lut->AddRGBPoint(0.0, 0.86, 0.86, 0.86);
  lut->AddRGBPoint(6.6, 0.70, 0.02, 0.15);

  svtkNew<svtkSegYReader> reader[5];
  svtkNew<svtkDataSetMapper> mapper[5];
  svtkNew<svtkActor> actor[5];

  for (int i = 0; i < 5; ++i)
  {
    reader[i]->SetFileName(fname[i]);
    reader[i]->Update();
    delete[] fname[i];

    mapper[i]->SetInputConnection(reader[i]->GetOutputPort());
    mapper[i]->SetLookupTable(lut);
    mapper[i]->SetColorModeToMapScalars();

    actor[i]->SetMapper(mapper[i]);

    ren->AddActor(actor[i]);
    ren->ResetCamera();
  }

  ren->GetActiveCamera()->Azimuth(50);
  ren->GetActiveCamera()->Roll(50);
  ren->GetActiveCamera()->Zoom(1.2);

  // interact with data
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
