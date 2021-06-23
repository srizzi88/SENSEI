#include "svtkMultiNewickTreeReader.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTree.h"

int TestMultiNewickTreeReader(int argc, char* argv[])
{
  char* file = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/multi_tree.tre");

  cerr << "file: " << file << endl;

  svtkSmartPointer<svtkMultiNewickTreeReader> reader =
    svtkSmartPointer<svtkMultiNewickTreeReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->Update();
  svtkMultiPieceDataSet* forest = reader->GetOutput();

  unsigned int numOfTrees = forest->GetNumberOfPieces();

  int error_count = 0;

  if (numOfTrees != 3)
  {
    ++error_count;
  }

  for (unsigned int i = 0; i < numOfTrees; i++)
  {
    svtkTree* tr = svtkTree::SafeDownCast(forest->GetPieceAsDataObject(i));
    if (!tr)
    {
      ++error_count;
    }
  }

  cerr << error_count << " errors" << endl;
  return error_count;
}
