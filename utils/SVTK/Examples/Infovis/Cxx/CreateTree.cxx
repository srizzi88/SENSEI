/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CreateTree.cxx

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

#include "svtkDataSetAttributes.h"
#include "svtkGraphLayoutView.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkRandomGraphSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkViewTheme.h"

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
    std::cout << "Invalid tree" << std::endl;
    graph->Delete();
    labels->Delete();
    tree->Delete();
    return EXIT_FAILURE;
  }

  svtkGraphLayoutView* view = svtkGraphLayoutView::New();
  view->SetRepresentationFromInput(tree);
  svtkViewTheme* theme = svtkViewTheme::CreateMellowTheme();
  view->ApplyViewTheme(theme);
  theme->Delete();
  view->SetVertexColorArrayName("VertexDegree");
  view->SetColorVertices(true);
  view->SetVertexLabelArrayName("Label");
  view->SetVertexLabelVisibility(true);

  view->ResetCamera();
  view->GetInteractor()->Start();

  graph->Delete();
  labels->Delete();
  tree->Delete();
  view->Delete();

  return EXIT_SUCCESS;
}
