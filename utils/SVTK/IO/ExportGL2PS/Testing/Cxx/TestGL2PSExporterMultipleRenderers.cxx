/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSExporterMultipleRenderers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGL2PSExporter.h"
#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestingInteractor.h"
#include "svtkTextActor.h"
#include "svtkTextMapper.h"

#include <string>

int TestGL2PSExporterMultipleRenderers(int, char*[])
{
  svtkNew<svtkTextActor> text1;
  text1->SetPosition(25, 25);
  text1->SetInput("String1");

  svtkNew<svtkTextActor> text2;
  text2->SetPosition(100, 100);
  text2->SetInput("String2");

  svtkNew<svtkTextMapper> textMap3;
  textMap3->SetInput("String3");
  svtkNew<svtkActor2D> text3;
  text3->SetMapper(textMap3);
  text3->SetPosition(75, 200);

  svtkNew<svtkRenderer> ren1;
  ren1->AddActor(text1);
  ren1->SetBackground(0.2, 0.2, 0.4);
  ren1->SetViewport(.5, 0, 1, 1);

  svtkNew<svtkRenderer> ren2;
  ren2->AddActor(text2);
  ren2->AddActor(text3);
  ren2->SetBackground(0.2, 0.2, 0.4);
  ren2->SetViewport(0, 0, .5, 1);

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);
  renWin->AddRenderer(ren2);
  renWin->SetSize(500, 500);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  svtkNew<svtkGL2PSExporter> exp;
  exp->SetRenderWindow(renWin);
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToSimple();
  exp->DrawBackgroundOn();

  std::string fileprefix =
    svtkTestingInteractor::TempDirectory + std::string("/TestGL2PSExporterMultipleRenderers");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  exp->SetFileFormatToPDF();
  exp->Write();

  renWin->SetMultiSamples(0);
  renWin->GetInteractor()->Initialize();
  renWin->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
