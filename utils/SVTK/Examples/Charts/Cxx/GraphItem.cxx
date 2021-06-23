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

#include "svtkCommand.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkGraph.h"
#include "svtkGraphItem.h"
#include "svtkRandomGraphSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkVariant.h"

#include "svtkObjectFactory.h"

// NOTE: @hcwiley commented this out as its not being used, and causes builds
// to fail with examples on but testing off. Why is this included?
//#include "svtkRegressionTestImage.h"

class GraphAnimate : public svtkCommand
{
public:
  static GraphAnimate* New() { return new GraphAnimate(); }
  svtkTypeMacro(GraphAnimate, svtkCommand);
  void Execute(svtkObject*, unsigned long, void*) override
  {
    this->GraphItem->UpdatePositions();
    this->View->Render();
    this->View->GetRenderWindow()->GetInteractor()->CreateOneShotTimer(10);
  }
  svtkGraphItem* GraphItem;
  svtkContextView* View;
};

//----------------------------------------------------------------------------
int main(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkSmartPointer<svtkContextView> view = svtkSmartPointer<svtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 600);

  svtkSmartPointer<svtkRandomGraphSource> source = svtkSmartPointer<svtkRandomGraphSource>::New();
  source->SetNumberOfVertices(100);
  source->SetNumberOfEdges(0);
  source->StartWithTreeOn();
  source->Update();
  svtkSmartPointer<svtkGraphItem> item = svtkSmartPointer<svtkGraphItem>::New();
  item->SetGraph(source->GetOutput());
  view->GetScene()->AddItem(item);

  svtkSmartPointer<GraphAnimate> anim = svtkSmartPointer<GraphAnimate>::New();
  anim->View = view;
  anim->GraphItem = item;
  view->GetRenderWindow()->GetInteractor()->Initialize();
  view->GetRenderWindow()->GetInteractor()->CreateOneShotTimer(10);
  view->GetRenderWindow()->GetInteractor()->AddObserver(svtkCommand::TimerEvent, anim);

  view->GetRenderWindow()->GetInteractor()->Start();
}
