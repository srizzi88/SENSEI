#include <svtkArrayPrint.h>
#include <svtkDenseArray.h>
#include <svtkSparseArray.h>

int main(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  ////////////////////////////////////////////////////////
  // Creating N-Way Arrays

  // Creating a dense array of 10 integers:
  svtkDenseArray<svtkIdType>* array = svtkDenseArray<svtkIdType>::New();
  array->Resize(10);

  // Creating a dense 20 x 30 matrix:
  svtkDenseArray<double>* matrix = svtkDenseArray<double>::New();
  matrix->Resize(20, 30);

  // Creating a sparse 10 x 20 x 30 x 40 tensor:
  svtkArrayExtents extents;
  extents.SetDimensions(4);
  extents[0] = svtkArrayRange(0, 10);
  extents[1] = svtkArrayRange(0, 20);
  extents[2] = svtkArrayRange(0, 30);
  extents[3] = svtkArrayRange(0, 40);
  svtkSparseArray<svtkIdType>* tensor = svtkSparseArray<svtkIdType>::New();
  tensor->Resize(extents);

  ////////////////////////////////////////////////////////
  // Initializing N-Way Arrays

  // Filling a dense array with ones:
  array->Fill(1);

  // Filling a dense matrix with zeros:
  matrix->Fill(0.0);

  // There's nothing to do for a sparse array - it's already empty.

  ////////////////////////////////////////////////////////
  // Assigning N-Way Array Values

  // Assign array value [5]:
  array->SetValue(5, 42);

  // Assign matrix value [4, 3]:
  matrix->SetValue(4, 3, 1970);

  // Assign tensor value [3, 7, 1, 2]:
  svtkArrayCoordinates coordinates;
  coordinates.SetDimensions(4);
  coordinates[0] = 3;
  coordinates[1] = 7;
  coordinates[2] = 1;
  coordinates[3] = 2;
  tensor->SetValue(coordinates, 38);

  ////////////////////////////////////////////////////////
  // Accessing N-Way Array Values

  // Access array value [5]:
  cout << "array[5]: " << array->GetValue(5) << "\n\n";

  // Access matrix value [4, 3]:
  cout << "matrix[4, 3]: " << matrix->GetValue(4, 3) << "\n\n";

  // Access tensor value [3, 7, 1, 2]:
  cout << "tensor[3, 7, 1, 2]: " << tensor->GetValue(coordinates) << "\n\n";

  ////////////////////////////////////////////////////////
  // Printing N-Way Arrays

  cout << "array:\n";
  svtkPrintVectorFormat(cout, array);
  cout << "\n";

  cout << "matrix:\n";
  svtkPrintMatrixFormat(cout, matrix);
  cout << "\n";

  cout << "tensor:\n";
  svtkPrintCoordinateFormat(cout, tensor);
  cout << "\n";

  // Cleanup array instances ...
  tensor->Delete();
  matrix->Delete();
  array->Delete();

  return 0;
}
