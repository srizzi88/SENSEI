/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkImageData.h"
#include "svtkImageItem.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPNGReader.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestContextImage(int argc, char* argv[])
{
  char* logo = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/svtk.png");

  // Set up a 2D context view, context test object and add it to the scene
  svtkNew<svtkContextView> view;
  view->GetRenderWindow()->SetSize(320, 181);
  svtkNew<svtkImageItem> item;
  view->GetScene()->AddItem(item);

  svtkNew<svtkPNGReader> reader;
  reader->SetFileName(logo);
  reader->Update();
  item->SetImage(svtkImageData::SafeDownCast(reader->GetOutput()));
  item->SetPosition(25, 30);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  delete[] logo;
  return EXIT_SUCCESS;
}
