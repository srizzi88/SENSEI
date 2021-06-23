#include "svtkDelimitedTextReader.h"
#include "svtkGraph.h"
#include "svtkKCoreLayout.h"
#include "svtkSmartPointer.h"
#include "svtkTableToGraph.h"
#include "svtkTestUtilities.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestKCoreLayout(int argc, char* argv[])
{
  SVTK_CREATE(svtkDelimitedTextReader, csv_vert_source);
  SVTK_CREATE(svtkDelimitedTextReader, csv_edge_source);
  SVTK_CREATE(svtkTableToGraph, tbl2graph);
  SVTK_CREATE(svtkKCoreLayout, kcoreLayout);

  char* file_verts =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/kcore_verts.csv");
  char* file_edges =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/kcore_edges.csv");

  csv_vert_source->SetFieldDelimiterCharacters(",");
  csv_vert_source->DetectNumericColumnsOn();
  csv_vert_source->SetHaveHeaders(true);
  csv_vert_source->SetFileName(file_verts);

  csv_edge_source->SetFieldDelimiterCharacters(",");
  csv_edge_source->DetectNumericColumnsOn();
  csv_edge_source->SetHaveHeaders(true);
  csv_edge_source->SetFileName(file_edges);

  tbl2graph->SetDirected(false);
  tbl2graph->AddInputConnection(csv_edge_source->GetOutputPort());
  tbl2graph->SetVertexTableConnection(csv_vert_source->GetOutputPort());
  tbl2graph->AddLinkVertex("source", "vertex id", false);
  tbl2graph->AddLinkVertex("target", "vertex id", false);
  tbl2graph->AddLinkEdge("source", "target");

  kcoreLayout->SetGraphConnection(tbl2graph->GetOutputPort());
  kcoreLayout->SetCartesian(true);
  kcoreLayout->SetEpsilon(0.2);
  kcoreLayout->SetUnitRadius(1.0);

  kcoreLayout->Update();

  // TODO: Add some tests on this graph to check validity

  delete[] file_verts;
  delete[] file_edges;

  return EXIT_SUCCESS;
}
