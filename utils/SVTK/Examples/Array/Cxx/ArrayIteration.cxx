#include <svtkArrayPrint.h>
#include <svtkDenseArray.h>
#include <svtkSparseArray.h>

int main(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create a dense matrix:
  svtkDenseArray<double>* matrix = svtkDenseArray<double>::New();
  matrix->Resize(10, 10);
  matrix->Fill(0.0);

  // Increment every value in a sparse-or-dense array
  // with any number of dimensions:
  for (svtkArray::SizeT n = 0; n != matrix->GetNonNullSize(); ++n)
  {
    matrix->SetValueN(n, matrix->GetValueN(n) + 1);
  }

  // Compute the sum of every column in a sparse-or-dense matrix:
  svtkDenseArray<double>* sum = svtkDenseArray<double>::New();
  sum->Resize(matrix->GetExtents()[1]);
  sum->Fill(0.0);

  svtkArrayCoordinates coordinates;
  for (svtkArray::SizeT n = 0; n != matrix->GetNonNullSize(); ++n)
  {
    matrix->GetCoordinatesN(n, coordinates);
    sum->SetValue(coordinates[1], sum->GetValue(coordinates[1]) + matrix->GetValueN(n));
  }

  cout << "matrix:\n";
  svtkPrintMatrixFormat(cout, matrix);
  cout << "\n";

  cout << "sum:\n";
  svtkPrintVectorFormat(cout, sum);
  cout << "\n";

  sum->Delete();
  matrix->Delete();

  return 0;
}
