
#include "svtkDataSetAttributes.h"
#include "svtkIntArray.h"
#include "svtkSmartPointer.h"
#include "svtkStreamGraph.h"
#include "svtkTable.h"
#include "svtkTableToGraph.h"

int TestStreamGraph(int, char*[])
{
  svtkSmartPointer<svtkIntArray> src = svtkSmartPointer<svtkIntArray>::New();
  src->SetName("source");
  src->SetNumberOfTuples(1);
  svtkSmartPointer<svtkIntArray> tgt = svtkSmartPointer<svtkIntArray>::New();
  tgt->SetName("target");
  tgt->SetNumberOfTuples(1);
  svtkSmartPointer<svtkIntArray> time = svtkSmartPointer<svtkIntArray>::New();
  time->SetName("time");
  time->SetNumberOfTuples(1);
  svtkSmartPointer<svtkTable> table = svtkSmartPointer<svtkTable>::New();
  table->AddColumn(src);
  table->AddColumn(tgt);
  table->AddColumn(time);
  svtkSmartPointer<svtkTableToGraph> t2g = svtkSmartPointer<svtkTableToGraph>::New();
  t2g->SetInputData(table);
  t2g->AddLinkVertex("source");
  t2g->AddLinkVertex("target");
  t2g->AddLinkEdge("source", "target");
  t2g->SetDirected(true);
  svtkSmartPointer<svtkStreamGraph> stream = svtkSmartPointer<svtkStreamGraph>::New();
  stream->SetInputConnection(t2g->GetOutputPort());
  stream->UseEdgeWindowOn();
  stream->SetEdgeWindow(5);
  stream->SetEdgeWindowArrayName("time");
  for (int i = 0; i < 10; ++i)
  {
    src->SetValue(0, i);
    tgt->SetValue(0, i + 1);
    time->SetValue(0, i);
    t2g->Modified();
    stream->Update();
    stream->GetOutput()->Dump();
    svtkSmartPointer<svtkTable> edgeTable = svtkSmartPointer<svtkTable>::New();
    edgeTable->SetRowData(stream->GetOutput()->GetEdgeData());
    edgeTable->Dump();
  }

  svtkGraph* output = stream->GetOutput();
  if (output->GetNumberOfVertices() != 11 || output->GetNumberOfEdges() != 6)
  {
    cerr << "ERROR: Incorrect number of vertices/edges." << endl;
    return 1;
  }
  svtkDataArray* outputTime = output->GetEdgeData()->GetArray("time");
  double timeRange[2];
  outputTime->GetRange(timeRange);
  if (timeRange[0] != 4 || timeRange[1] != 9)
  {
    cerr << "ERROR: Incorrect time range." << endl;
    return 1;
  }

  return 0;
}
