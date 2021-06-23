#include <svtkAdjacencyMatrixToEdgeTable.h>
#include <svtkArrayPrint.h>
#include <svtkDenseArray.h>
#include <svtkDiagonalMatrixSource.h>
#include <svtkGraphLayoutView.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTable.h>
#include <svtkTableToGraph.h>
#include <svtkViewTheme.h>

int main(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkDiagonalMatrixSource> source = svtkSmartPointer<svtkDiagonalMatrixSource>::New();
  source->SetExtents(10);
  source->SetDiagonal(0);
  source->SetSuperDiagonal(1);
  source->SetSubDiagonal(2);
  source->Update();

  cout << "adjacency matrix:\n";
  svtkPrintMatrixFormat(cout, svtkDenseArray<double>::SafeDownCast(source->GetOutput()->GetArray(0)));
  cout << "\n";

  svtkSmartPointer<svtkAdjacencyMatrixToEdgeTable> edges =
    svtkSmartPointer<svtkAdjacencyMatrixToEdgeTable>::New();
  edges->SetInputConnection(source->GetOutputPort());

  svtkSmartPointer<svtkTableToGraph> graph = svtkSmartPointer<svtkTableToGraph>::New();
  graph->SetInputConnection(edges->GetOutputPort());
  graph->AddLinkVertex("rows", "stuff", false);
  graph->AddLinkVertex("columns", "stuff", false);
  graph->AddLinkEdge("rows", "columns");

  svtkSmartPointer<svtkViewTheme> theme;
  theme.TakeReference(svtkViewTheme::CreateMellowTheme());
  theme->SetLineWidth(5);
  theme->SetCellOpacity(0.9);
  theme->SetCellAlphaRange(0.5, 0.5);
  theme->SetPointSize(10);
  theme->SetSelectedCellColor(1, 0, 1);
  theme->SetSelectedPointColor(1, 0, 1);

  svtkSmartPointer<svtkGraphLayoutView> view = svtkSmartPointer<svtkGraphLayoutView>::New();
  view->AddRepresentationFromInputConnection(graph->GetOutputPort());
  view->EdgeLabelVisibilityOn();
  view->SetEdgeLabelArrayName("value");
  view->ApplyViewTheme(theme);
  view->SetVertexLabelFontSize(20);
  view->SetEdgeLabelFontSize(18);
  view->VertexLabelVisibilityOn();

  view->GetRenderWindow()->SetSize(600, 600);
  view->ResetCamera();
  view->GetInteractor()->Start();

  return 0;
}
