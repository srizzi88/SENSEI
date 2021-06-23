#include "svtkDIMACSGraphReader.h"
#include "svtkGraph.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestDIMACSGraphReader(int argc, char* argv[])
{
  SVTK_CREATE(svtkDIMACSGraphReader, src_pattern);
  SVTK_CREATE(svtkDIMACSGraphReader, src_target);
  SVTK_CREATE(svtkDIMACSGraphReader, src_flow);

  char* file_pattern =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/DimacsGraphs/iso_pattern.gr");
  char* file_target =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/DimacsGraphs/iso_target.gr");
  char* file_flow =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/DimacsGraphs/maxflow.max");

  src_pattern->SetFileName(file_pattern);
  src_target->SetFileName(file_target);
  src_flow->SetFileName(file_flow);

  delete[] file_pattern;
  delete[] file_target;
  delete[] file_flow;

  src_pattern->GetFileName();
  src_target->GetFileName();
  src_flow->GetFileName();

  src_pattern->Update();
  src_target->Update();
  src_flow->Update();

  // Do a quick check on the data, the pattern graph should have
  // 5 edges and 5 vertices
  svtkGraph* G = svtkGraph::SafeDownCast(src_pattern->GetOutput());
  if (G->GetNumberOfVertices() != 5)
  {
    cout << "\tERROR: iso_pattern.gr vertex count wrong. "
         << "Expected 5, Got " << G->GetNumberOfVertices() << endl;
    return 1;
  }
  if (G->GetNumberOfEdges() != 5)
  {
    cout << "\tERROR: iso_pattern.gr edge count wrong. "
         << "Expected 5, Got " << G->GetNumberOfEdges() << endl;
    return 1;
  }

  return 0;
}
