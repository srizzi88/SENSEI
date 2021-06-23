/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestButtonWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test svtkProp3DButtonRepresentation

#include "svtkButtonWidget.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkProp3DButtonRepresentation.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

// ----------------------------------------------------------------------------
bool TestUnMapped();

// ----------------------------------------------------------------------------
int TestProp3DButtonRepresentation(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkObjectFactory::SetAllEnableFlags(false, "svtkRenderWindowInteractor", "svtkTestingInteractor");
  bool res = true;
  res = TestUnMapped() && res;
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}

// ----------------------------------------------------------------------------
bool TestUnMapped()
{
  svtkNew<svtkRenderWindow> renderWindow;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  svtkNew<svtkProp3DButtonRepresentation> representation1;
  svtkNew<svtkButtonWidget> buttonWidget1;
  buttonWidget1->SetInteractor(iren);
  buttonWidget1->SetRepresentation(representation1);
  buttonWidget1->SetEnabled(1);

  svtkNew<svtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkProp3DButtonRepresentation> representation2;
  svtkNew<svtkButtonWidget> buttonWidget2;
  buttonWidget2->SetInteractor(iren);
  buttonWidget2->SetRepresentation(representation2);
  buttonWidget2->SetEnabled(1);

  iren->Initialize();

  svtkNew<svtkProp3DButtonRepresentation> representation3;
  svtkNew<svtkButtonWidget> buttonWidget3;
  buttonWidget3->SetInteractor(iren);
  buttonWidget3->SetRepresentation(representation3);
  buttonWidget3->SetEnabled(1);

  iren->Start();

  svtkNew<svtkProp3DButtonRepresentation> representation4;
  svtkNew<svtkButtonWidget> buttonWidget4;
  buttonWidget4->SetInteractor(iren);
  buttonWidget4->SetRepresentation(representation4);
  buttonWidget4->SetEnabled(1);

  return true;
}
