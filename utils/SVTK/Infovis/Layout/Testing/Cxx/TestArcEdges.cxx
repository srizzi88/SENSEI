
#include "svtkActor.h"
#include "svtkArcParallelEdgeStrategy.h"
#include "svtkCircularLayoutStrategy.h"
#include "svtkEdgeLayout.h"
#include "svtkForceDirectedLayoutStrategy.h"
#include "svtkGraphLayout.h"
#include "svtkGraphToPolyData.h"
#include "svtkPassThroughEdgeStrategy.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRandomGraphSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkVertexGlyphFilter.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestArcEdges(int argc, char* argv[])
{
  SVTK_CREATE(svtkRandomGraphSource, source);
  SVTK_CREATE(svtkGraphLayout, layout);
  SVTK_CREATE(svtkCircularLayoutStrategy, strategy);
  SVTK_CREATE(svtkEdgeLayout, edgeLayout);
  SVTK_CREATE(svtkArcParallelEdgeStrategy, edgeStrategy);
  SVTK_CREATE(svtkGraphToPolyData, graphToPoly);
  SVTK_CREATE(svtkPolyDataMapper, edgeMapper);
  SVTK_CREATE(svtkActor, edgeActor);
  SVTK_CREATE(svtkVertexGlyphFilter, vertGlyph);
  SVTK_CREATE(svtkPolyDataMapper, vertMapper);
  SVTK_CREATE(svtkActor, vertActor);
  SVTK_CREATE(svtkRenderer, ren);
  SVTK_CREATE(svtkRenderWindow, win);
  win->SetMultiSamples(0);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);

  source->SetNumberOfVertices(3);
  source->SetNumberOfEdges(50);
  source->AllowSelfLoopsOn();
  source->AllowParallelEdgesOn();
  source->StartWithTreeOff();
  source->DirectedOff();
  layout->SetInputConnection(source->GetOutputPort());
  layout->SetLayoutStrategy(strategy);
  edgeStrategy->SetNumberOfSubdivisions(50);
  edgeLayout->SetInputConnection(layout->GetOutputPort());
  edgeLayout->SetLayoutStrategy(edgeStrategy);

  // Pull the graph out of the pipeline so we can test the
  // edge points API.
  edgeLayout->Update();
  svtkGraph* g = edgeLayout->GetOutput();
  svtkIdType npts = g->GetNumberOfEdgePoints(0);
  double* pts = new double[3 * npts];
  for (svtkIdType i = 0; i < npts; ++i)
  {
    double* pt = g->GetEdgePoint(0, i);
    pts[3 * i + 0] = pt[0];
    pts[3 * i + 1] = pt[1];
    pts[3 * i + 2] = pt[2];
  }
  g->ClearEdgePoints(0);
  for (svtkIdType i = 0; i < npts; ++i)
  {
    double* pt = pts + 3 * i;
    g->AddEdgePoint(0, pt);
    g->SetEdgePoint(0, i, pt);
    g->SetEdgePoint(0, i, pt[0], pt[1], pt[2]);
  }
  delete[] pts;

  graphToPoly->SetInputData(g);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  edgeActor->SetMapper(edgeMapper);
  ren->AddActor(edgeActor);

  vertGlyph->SetInputData(g);
  vertMapper->SetInputConnection(vertGlyph->GetOutputPort());
  vertActor->SetMapper(vertMapper);
  vertActor->GetProperty()->SetPointSize(1);
  ren->AddActor(vertActor);

  win->AddRenderer(ren);
  win->SetInteractor(iren);
  win->Render();

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
