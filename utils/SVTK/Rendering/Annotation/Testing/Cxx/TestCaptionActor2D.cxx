/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCaptionActor2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCaptionActor2D.h>
#include <svtkNew.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkTextActor.h>
#include <svtkTextProperty.h>

int TestCaptionActor2D(int, char*[])
{
  // Draw text with diameter measure
  svtkNew<svtkCaptionActor2D> captionActor;
  captionActor->SetAttachmentPoint(0, 0, 0);
  captionActor->SetCaption("(2) 2.27");
  captionActor->BorderOff();
  captionActor->LeaderOff();
  captionActor->SetPadding(0);
  captionActor->GetCaptionTextProperty()->SetJustificationToLeft();
  captionActor->GetCaptionTextProperty()->ShadowOff();
  captionActor->GetCaptionTextProperty()->ItalicOff();
  captionActor->GetCaptionTextProperty()->SetFontFamilyToCourier();
  captionActor->GetCaptionTextProperty()->SetFontSize(24);
  captionActor->GetTextActor()->SetTextScaleModeToNone();

  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0, 0, 0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(captionActor);

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();
  renderWindow->GetInteractor()->Initialize();
  renderWindow->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
