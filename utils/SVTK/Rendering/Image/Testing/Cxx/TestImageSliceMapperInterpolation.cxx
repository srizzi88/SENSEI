/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageSliceMapperInterpolation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the Nearest, Linear, and Cubic interpolation.
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkLookupTable.h"
#include "svtkPNGReader.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestImageSliceMapperInterpolation(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkInteractorStyle* style = svtkInteractorStyleImage::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetSize(400, 400);
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  svtkPNGReader* reader = svtkPNGReader::New();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
  delete[] fname;

  for (int i = 0; i < 4; i++)
  {
    svtkRenderer* renderer = svtkRenderer::New();
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    svtkImageSliceMapper* imageMapper = svtkImageSliceMapper::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());

    svtkImageSlice* image = svtkImageSlice::New();
    image->SetMapper(imageMapper);
    imageMapper->Delete();
    renderer->AddViewProp(image);

    if (i < 3)
    {
      image->GetProperty()->SetColorWindow(1000);
      image->GetProperty()->SetColorLevel(1500);
      image->GetProperty()->SetInterpolationType(i);
    }
    else
    {
      svtkLookupTable* table = svtkLookupTable::New();
      table->Build();
      table->SetRange(1000, 2000);
      image->GetProperty()->SetLookupTable(table);
      image->GetProperty()->UseLookupTableScalarRangeOn();
      table->Delete();
    }

    image->Delete();
    camera->ParallelProjectionOn();
    renderer->ResetCamera();
    camera->SetParallelScale(10.0);
  }

  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  reader->Delete();

  return !retVal;
}
