/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GraphItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkPiecewiseControlPointsItem.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPiecewiseFunctionItem.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

//----------------------------------------------------------------------------
int main(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkSmartPointer<svtkContextView> view = svtkSmartPointer<svtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 600);
  view->GetRenderWindow()->SetMultiSamples(0);

  svtkSmartPointer<svtkPiecewiseFunction> source = svtkSmartPointer<svtkPiecewiseFunction>::New();
  source->AddPoint(0, 0);
  source->AddPoint(200, 200);
  source->AddPoint(400, 500);
  source->AddPoint(700, 500);
  //   source->Update();
  svtkSmartPointer<svtkPiecewiseControlPointsItem> item =
    svtkSmartPointer<svtkPiecewiseControlPointsItem>::New();
  item->SetPiecewiseFunction(source);
  view->GetScene()->AddItem(item);

  // NOT WORKING...
  // svtkSmartPointer<svtkPiecewiseFunctionItem> item2 =
  // svtkSmartPointer<svtkPiecewiseFunctionItem>::New(); item2->SetPiecewiseFunction(source);
  // view->GetScene()->AddItem(item2);

  view->GetRenderWindow()->GetInteractor()->Initialize();
  view->GetRenderWindow()->GetInteractor()->CreateOneShotTimer(10);

  view->GetRenderWindow()->GetInteractor()->Start();
}
