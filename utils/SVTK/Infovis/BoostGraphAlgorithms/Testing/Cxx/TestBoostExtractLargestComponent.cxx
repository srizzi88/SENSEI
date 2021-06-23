#include <svtkDataSetAttributes.h>
#include <svtkDoubleArray.h>
#include <svtkMutableUndirectedGraph.h>
#include <svtkPoints.h>
#include <svtkSmartPointer.h>

#include "svtkBoostExtractLargestComponent.h"

namespace
{
int TestNormal(svtkMutableUndirectedGraph* g);
int TestInverse(svtkMutableUndirectedGraph* g);
}

int TestBoostExtractLargestComponent(int, char*[])
{
  // Create a graph
  svtkSmartPointer<svtkMutableUndirectedGraph> g = svtkSmartPointer<svtkMutableUndirectedGraph>::New();

  // Add vertices to the graph
  svtkIdType v1 = g->AddVertex();
  svtkIdType v2 = g->AddVertex();
  svtkIdType v3 = g->AddVertex();
  svtkIdType v4 = g->AddVertex();
  svtkIdType v5 = g->AddVertex();
  svtkIdType v6 = g->AddVertex();
  svtkIdType v7 = g->AddVertex();

  // Create one connected component
  g->AddEdge(v1, v2);
  g->AddEdge(v1, v3);

  // Create some disconnected components
  g->AddEdge(v4, v5);
  g->AddEdge(v6, v7);

  std::vector<int> results;

  results.push_back(TestNormal(g));
  results.push_back(TestInverse(g));

  for (unsigned int i = 0; i < results.size(); i++)
  {
    if (results[i] == EXIT_SUCCESS)
    {
      std::cout << "Test " << i << " passed." << std::endl;
    }
    else
    {
      std::cout << "Test " << i << " failed!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

namespace
{

int TestNormal(svtkMutableUndirectedGraph* g)
{
  // Test normal operation (extract largest connected component)
  svtkSmartPointer<svtkBoostExtractLargestComponent> filter =
    svtkSmartPointer<svtkBoostExtractLargestComponent>::New();
  filter->SetInputData(g);
  filter->Update();

  if (filter->GetOutput()->GetNumberOfVertices() != 3)
  {
    std::cout << "Size of largest connected component: "
              << filter->GetOutput()->GetNumberOfVertices() << " (Should have been 3)."
              << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int TestInverse(svtkMutableUndirectedGraph* g)
{
  // Test inverse operation (extract everything but largest connected component)
  svtkSmartPointer<svtkBoostExtractLargestComponent> filter =
    svtkSmartPointer<svtkBoostExtractLargestComponent>::New();
  filter->SetInputData(g);
  filter->SetInvertSelection(true);
  filter->Update();

  if (filter->GetOutput()->GetNumberOfVertices() != 4)
  {
    std::cout << "Size of remainder: " << filter->GetOutput()->GetNumberOfVertices()
              << " (Should have been 4)." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

} // End anonymous namespace
