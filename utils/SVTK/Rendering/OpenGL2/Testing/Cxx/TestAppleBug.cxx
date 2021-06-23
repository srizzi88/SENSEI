/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestColorByCellDataStringArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include <svtkActor.h>
#include <svtkCellData.h>
#include <svtkDiscretizableColorTransferFunction.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSphereSource.h>
#include <svtkStdString.h>
#include <svtkStringArray.h>

#include "svtkOpenGLPolyDataMapper.h"

int TestAppleBug(int argc, char* argv[])
{
  svtkNew<svtkSphereSource> sphere;
  sphere->Update();

  svtkNew<svtkPolyData> polydata;
  polydata->ShallowCopy(sphere->GetOutput());

  // Set up string array associated with cells
  svtkNew<svtkStringArray> sArray;
  sArray->SetName("color");
  sArray->SetNumberOfComponents(1);
  sArray->SetNumberOfTuples(polydata->GetNumberOfCells());

  svtkVariant colors[5];
  colors[0] = "red";
  colors[1] = "blue";
  colors[2] = "green";
  colors[3] = "yellow";
  colors[4] = "cyan";

  // Round-robin assignment of color strings
  for (int i = 0; i < polydata->GetNumberOfCells(); ++i)
  {
    sArray->SetValue(i, colors[i % 5].ToString());
  }

  svtkCellData* cd = polydata->GetCellData();
  cd->AddArray(sArray);

  // Set up transfer function
  svtkNew<svtkDiscretizableColorTransferFunction> tfer;
  tfer->IndexedLookupOn();
  tfer->SetNumberOfIndexedColors(5);
  tfer->SetIndexedColor(0, 1.0, 0.0, 0.0);
  tfer->SetIndexedColor(1, 0.0, 0.0, 1.0);
  tfer->SetIndexedColor(2, 0.0, 1.0, 0.0);
  tfer->SetIndexedColor(3, 1.0, 1.0, 0.0);
  tfer->SetIndexedColor(4, 0.0, 1.0, 1.0);

  svtkStdString red("red");
  tfer->SetAnnotation(red, red);
  svtkStdString blue("blue");
  tfer->SetAnnotation(blue, blue);
  svtkStdString green("green");
  tfer->SetAnnotation(green, green);
  svtkStdString yellow("yellow");
  tfer->SetAnnotation(yellow, yellow);
  svtkStdString cyan("cyan");
  tfer->SetAnnotation(cyan, cyan);

  svtkNew<svtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputDataObject(polydata);
  mapper->SetLookupTable(tfer);
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("color");

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);

  cerr << renderWindow->ReportCapabilities();

  // if it thinks it has the bug, try it
  // without the code
  if (mapper->GetHaveAppleBug())
  {
    mapper->ForceHaveAppleBugOff();
    renderWindow->Render();
    int offRetVal = svtkRegressionTestImage(renderWindow);
    if (offRetVal == svtkRegressionTester::PASSED)
    {
      cerr << "FIX!!!! This system is using the AppleBug (rdar://20747550) code but does not need "
              "it\n\n";
      return !svtkRegressionTester::FAILED;
    }
  }
  else
  {
    // if the test failed see if the apple bug would fix it
    if (retVal == svtkRegressionTester::FAILED)
    {
      mapper->ForceHaveAppleBugOn();
      renderWindow->Render();
      retVal = svtkRegressionTestImage(renderWindow);
      if (retVal == svtkRegressionTester::PASSED)
      {
        cerr
          << "FIX!!! This system needs the AppleBug (rdar://20747550) code but doesn't have it\n\n";
        return !svtkRegressionTester::FAILED;
      }
    }
  }

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
