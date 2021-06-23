/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBoostDividedEdgeBundling.h"
#include "svtkDataSetAttributes.h"
#include "svtkFloatArray.h"
#include "svtkGraphItem.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkStringArray.h"
#include "svtkViewTheme.h"
#include "svtkXMLTreeReader.h"

#include "svtkContext2D.h"
#include "svtkContextActor.h"
#include "svtkContextInteractorStyle.h"
#include "svtkContextItem.h"
#include "svtkContextScene.h"
#include "svtkContextTransform.h"
#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkRegressionTestImage.h"

//----------------------------------------------------------------------------
void BuildSampleGraph(svtkMutableDirectedGraph* graph)
{
  svtkNew<svtkPoints> points;

  graph->AddVertex();
  points->InsertNextPoint(20, 40, 0);
  graph->AddVertex();
  points->InsertNextPoint(20, 80, 0);
  graph->AddVertex();
  points->InsertNextPoint(20, 120, 0);
  graph->AddVertex();
  points->InsertNextPoint(20, 160, 0);
  graph->AddVertex();
  points->InsertNextPoint(380, 40, 0);
  graph->AddVertex();
  points->InsertNextPoint(380, 80, 0);
  graph->AddVertex();
  points->InsertNextPoint(380, 120, 0);
  graph->AddVertex();
  points->InsertNextPoint(380, 160, 0);
  graph->SetPoints(points);

  graph->AddEdge(0, 4);
  graph->AddEdge(0, 5);
  graph->AddEdge(1, 4);
  graph->AddEdge(1, 5);
  graph->AddEdge(2, 6);
  graph->AddEdge(2, 7);
  graph->AddEdge(3, 6);
  graph->AddEdge(3, 7);

  graph->AddEdge(4, 0);
  graph->AddEdge(5, 0);
  graph->AddEdge(6, 0);
}

//----------------------------------------------------------------------------
void BuildGraphMLGraph(svtkMutableDirectedGraph* graph, std::string file)
{
  svtkNew<svtkXMLTreeReader> reader;
  reader->SetFileName(file.c_str());
  reader->ReadCharDataOn();
  reader->Update();
  svtkTree* tree = reader->GetOutput();
  svtkStringArray* keyArr =
    svtkArrayDownCast<svtkStringArray>(tree->GetVertexData()->GetAbstractArray("key"));
  svtkStringArray* sourceArr =
    svtkArrayDownCast<svtkStringArray>(tree->GetVertexData()->GetAbstractArray("source"));
  svtkStringArray* targetArr =
    svtkArrayDownCast<svtkStringArray>(tree->GetVertexData()->GetAbstractArray("target"));
  svtkStringArray* contentArr =
    svtkArrayDownCast<svtkStringArray>(tree->GetVertexData()->GetAbstractArray(".chardata"));
  double x = 0.0;
  double y = 0.0;
  svtkIdType source = 0;
  svtkIdType target = 0;
  svtkNew<svtkPoints> points;
  graph->SetPoints(points);
  for (svtkIdType i = 0; i < tree->GetNumberOfVertices(); ++i)
  {
    svtkStdString k = keyArr->GetValue(i);
    if (k == "x")
    {
      x = svtkVariant(contentArr->GetValue(i)).ToDouble();
    }
    if (k == "y")
    {
      y = svtkVariant(contentArr->GetValue(i)).ToDouble();
      graph->AddVertex();
      points->InsertNextPoint(x, y, 0.0);
    }
    svtkStdString s = sourceArr->GetValue(i);
    if (s != "")
    {
      source = svtkVariant(s).ToInt();
    }
    svtkStdString t = targetArr->GetValue(i);
    if (t != "")
    {
      target = svtkVariant(t).ToInt();
      graph->AddEdge(source, target);
    }
  }
}

//----------------------------------------------------------------------------
class svtkBundledGraphItem : public svtkGraphItem
{
public:
  static svtkBundledGraphItem* New();
  svtkTypeMacro(svtkBundledGraphItem, svtkGraphItem);

protected:
  svtkBundledGraphItem() {}
  ~svtkBundledGraphItem() override {}

  virtual svtkColor4ub EdgeColor(svtkIdType line, svtkIdType point) override;
  virtual float EdgeWidth(svtkIdType line, svtkIdType point) override;
};

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkBundledGraphItem);

//----------------------------------------------------------------------------
svtkColor4ub svtkBundledGraphItem::EdgeColor(svtkIdType edgeIdx, svtkIdType pointIdx)
{
  float fraction = static_cast<float>(pointIdx) / (this->NumberOfEdgePoints(edgeIdx) - 1);
  return svtkColor4ub(fraction * 255, 0, 255 - fraction * 255, 255);
}

//----------------------------------------------------------------------------
float svtkBundledGraphItem::EdgeWidth(svtkIdType svtkNotUsed(lineIdx), svtkIdType svtkNotUsed(pointIdx))
{
  return 4.0f;
}

//----------------------------------------------------------------------------
int TestBoostDividedEdgeBundling(int argc, char* argv[])
{
  svtkNew<svtkMutableDirectedGraph> graph;
  svtkNew<svtkBoostDividedEdgeBundling> bundle;

  BuildSampleGraph(graph);
  // BuildGraphMLGraph(graph, "airlines_flipped.graphml");

  bundle->SetInputData(graph);
  bundle->Update();

  svtkDirectedGraph* output = bundle->GetOutput();

  svtkNew<svtkContextActor> actor;

  svtkNew<svtkBundledGraphItem> graphItem;
  graphItem->SetGraph(output);

  svtkNew<svtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(graphItem);
  actor->GetScene()->AddItem(trans);

  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(1.0, 1.0, 1.0);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 200);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);

  svtkNew<svtkContextInteractorStyle> interactorStyle;
  interactorStyle->SetScene(actor->GetScene());

  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetInteractorStyle(interactorStyle);
  interactor->SetRenderWindow(renderWindow);
  renderWindow->SetMultiSamples(0);
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindow->Render();
    interactor->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return !retVal;
}
