/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MultiView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example...
//

#include "svtkAnnotationLink.h"
#include "svtkCommand.h"
#include "svtkDataRepresentation.h"
#include "svtkDataSetAttributes.h"
#include "svtkGraphLayoutView.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkRandomGraphSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkViewTheme.h"

#include <vector>

class ViewUpdater : public svtkCommand
{
public:
  static ViewUpdater* New() { return new ViewUpdater; }

  void AddView(svtkView* view)
  {
    this->Views.push_back(view);
    view->AddObserver(svtkCommand::SelectionChangedEvent, this);
  }

  void Execute(svtkObject*, unsigned long, void*) override
  {
    for (unsigned int i = 0; i < this->Views.size(); i++)
    {
      this->Views[i]->Update();
    }
  }

private:
  ViewUpdater() = default;
  ~ViewUpdater() override = default;
  std::vector<svtkView*> Views;
};

int main(int, char*[])
{
  svtkMutableDirectedGraph* graph = svtkMutableDirectedGraph::New();
  svtkIdType a = graph->AddVertex();
  svtkIdType b = graph->AddChild(a);
  svtkIdType c = graph->AddChild(a);
  svtkIdType d = graph->AddChild(b);
  svtkIdType e = graph->AddChild(c);
  svtkIdType f = graph->AddChild(c);

  svtkStringArray* labels = svtkStringArray::New();
  labels->SetName("Label");
  labels->InsertValue(a, "a");
  labels->InsertValue(b, "b");
  labels->InsertValue(c, "c");
  labels->InsertValue(d, "d");
  labels->InsertValue(e, "e");
  labels->InsertValue(f, "f");
  graph->GetVertexData()->AddArray(labels);

  svtkTree* tree = svtkTree::New();
  bool validTree = tree->CheckedShallowCopy(graph);
  if (!validTree)
  {
    std::cout << "ERROR: Invalid tree" << std::endl;
    graph->Delete();
    labels->Delete();
    tree->Delete();
    return EXIT_FAILURE;
  }
  svtkGraphLayoutView* view = svtkGraphLayoutView::New();
  svtkDataRepresentation* rep = view->SetRepresentationFromInput(tree);
  svtkViewTheme* theme = svtkViewTheme::CreateMellowTheme();
  view->ApplyViewTheme(theme);
  view->SetVertexColorArrayName("VertexDegree");
  view->SetColorVertices(true);
  view->SetVertexLabelArrayName("Label");
  view->SetVertexLabelVisibility(true);

  svtkGraphLayoutView* view2 = svtkGraphLayoutView::New();
  svtkDataRepresentation* rep2 = view2->SetRepresentationFromInput(tree);
  view2->SetVertexLabelArrayName("Label");
  view2->SetVertexLabelVisibility(true);

  svtkAnnotationLink* link = svtkAnnotationLink::New();
  rep->SetAnnotationLink(link);
  rep2->SetAnnotationLink(link);

  ViewUpdater* update = ViewUpdater::New();
  update->AddView(view);
  update->AddView(view2);

  view->ResetCamera();
  view2->ResetCamera();
  view->Render();
  view2->Render();
  view->GetInteractor()->Start();

  graph->Delete();
  labels->Delete();
  tree->Delete();
  view->Delete();
  theme->Delete();
  view2->Delete();
  link->Delete();
  update->Delete();

  return EXIT_SUCCESS;
}
