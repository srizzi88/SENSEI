#include "svtkKMeansDistanceFunctor.h"

#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

svtkStandardNewMacro(svtkKMeansDistanceFunctor);

// ----------------------------------------------------------------------
svtkKMeansDistanceFunctor::svtkKMeansDistanceFunctor()
{
  this->EmptyTuple = svtkVariantArray::New();
}

// ----------------------------------------------------------------------
svtkKMeansDistanceFunctor::~svtkKMeansDistanceFunctor()
{
  this->EmptyTuple->Delete();
}

// ----------------------------------------------------------------------
void svtkKMeansDistanceFunctor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EmptyTuple: " << this->EmptyTuple << "\n";
}

// ----------------------------------------------------------------------
svtkVariantArray* svtkKMeansDistanceFunctor::GetEmptyTuple(svtkIdType dimension)
{
  if (this->EmptyTuple->GetNumberOfValues() != dimension)
  {
    this->EmptyTuple->SetNumberOfValues(dimension);
    for (svtkIdType i = 0; i < dimension; ++i)
    {
      this->EmptyTuple->SetValue(i, 0.0);
    }
  }
  return this->EmptyTuple;
}

// ----------------------------------------------------------------------
void svtkKMeansDistanceFunctor::operator()(
  double& distance, svtkVariantArray* clusterCoord, svtkVariantArray* dataCoord)
{
  distance = 0.0;
  if (clusterCoord->GetNumberOfValues() != dataCoord->GetNumberOfValues())
  {
    cout << "The dimensions of the cluster and data do not match." << endl;
    distance = -1;
  }

  for (svtkIdType i = 0; i < clusterCoord->GetNumberOfValues(); i++)
  {
    distance += (clusterCoord->GetValue(i).ToDouble() - dataCoord->GetValue(i).ToDouble()) *
      (clusterCoord->GetValue(i).ToDouble() - dataCoord->GetValue(i).ToDouble());
  }
}

// ----------------------------------------------------------------------
void svtkKMeansDistanceFunctor::PairwiseUpdate(svtkTable* clusterCoords, svtkIdType rowIndex,
  svtkVariantArray* dataCoord, svtkIdType dataCoordCardinality, svtkIdType totalCardinality)
{
  if (clusterCoords->GetNumberOfColumns() != dataCoord->GetNumberOfValues())
  {
    cout << "The dimensions of the cluster and/or data do not match." << endl;
    return;
  }

  if (totalCardinality > 0)
  {
    for (svtkIdType i = 0; i < clusterCoords->GetNumberOfColumns(); ++i)
    {
      double curCoord = clusterCoords->GetValue(rowIndex, i).ToDouble();
      clusterCoords->SetValue(rowIndex, i,
        curCoord +
          static_cast<double>(dataCoordCardinality) *
            (dataCoord->GetValue(i).ToDouble() - curCoord) / static_cast<double>(totalCardinality));
    }
  }
}

// ----------------------------------------------------------------------
void svtkKMeansDistanceFunctor::PerturbElement(svtkTable* newClusterElements,
  svtkTable* curClusterElements, svtkIdType changeID, svtkIdType startRunID, svtkIdType endRunID,
  double alpha)
{
  double numInRange = static_cast<double>(endRunID - startRunID);
  svtkIdType dimension = newClusterElements->GetNumberOfColumns();
  std::vector<double> perturbedValues(dimension);

  for (svtkIdType i = startRunID; i < endRunID; i++)
  {
    for (svtkIdType j = 0; j < dimension; j++)
    {
      if (i == changeID)
      {
        perturbedValues[j] = alpha * curClusterElements->GetValue(i, j).ToDouble();
      }
      else
      {
        if (numInRange > 1.0)
        {
          perturbedValues[j] =
            (1.0 - alpha) / (numInRange - 1.0) * curClusterElements->GetValue(i, j).ToDouble();
        }
        else
        {
          perturbedValues[j] =
            (1.0 - alpha) / (numInRange)*curClusterElements->GetValue(i, j).ToDouble();
        }
      }
    }
  }
}

// ----------------------------------------------------------------------
void* svtkKMeansDistanceFunctor::AllocateElementArray(svtkIdType size)
{
  return new double[size];
}

// ----------------------------------------------------------------------
void svtkKMeansDistanceFunctor::DeallocateElementArray(void* array)
{
  delete[] static_cast<double*>(array);
}

// ----------------------------------------------------------------------
svtkAbstractArray* svtkKMeansDistanceFunctor::CreateCoordinateArray()
{
  return svtkDoubleArray::New();
}

// ----------------------------------------------------------------------
void svtkKMeansDistanceFunctor::PackElements(svtkTable* curTable, void* vElements)
{
  svtkIdType numCols = curTable->GetNumberOfColumns();
  svtkIdType numRows = curTable->GetNumberOfRows();
  double* localElements = static_cast<double*>(vElements);

  for (svtkIdType col = 0; col < numCols; col++)
  {
    svtkDoubleArray* doubleArr = svtkArrayDownCast<svtkDoubleArray>(curTable->GetColumn(col));
    memcpy(&(localElements[col * numRows]), doubleArr->GetPointer(0), numRows * sizeof(double));
  }
}

// ----------------------------------------------------------------------
void svtkKMeansDistanceFunctor::UnPackElements(
  svtkTable* curTable, void* vLocalElements, svtkIdType numRows, svtkIdType numCols)
{
  double* localElements = static_cast<double*>(vLocalElements);
  for (svtkIdType i = 0; i < numRows; i++)
  {
    svtkVariantArray* curRow = svtkVariantArray::New();
    for (int j = 0; j < numCols; j++)
    {
      curRow->InsertNextValue(localElements[j * numRows + i]);
    }
    curTable->InsertNextRow(curRow);
    curRow->Delete();
  }
}

// ----------------------------------------------------------------------
void svtkKMeansDistanceFunctor::UnPackElements(
  svtkTable* curTable, svtkTable* newTable, void* vLocalElements, void* vGlobalElements, int np)
{
  double* globalElements = static_cast<double*>(vGlobalElements);
  double* localElements = static_cast<double*>(vLocalElements);
  svtkIdType numCols = curTable->GetNumberOfColumns();
  svtkIdType numRows = curTable->GetNumberOfRows();
  svtkIdType numElements = numCols * numRows;
  for (svtkIdType col = 0; col < numCols; col++)
  {
    svtkDoubleArray* doubleArr = svtkDoubleArray::New();
    doubleArr->SetName(curTable->GetColumnName(col));
    doubleArr->SetNumberOfComponents(1);
    doubleArr->SetNumberOfTuples(numRows * np);
    for (int j = 0; j < np; j++)
    {
      double* ptr = doubleArr->GetPointer(j * numRows);
      memcpy(ptr, &(globalElements[j * numElements + col * numRows]), numRows * sizeof(double));
    }
    newTable->AddColumn(doubleArr);
    doubleArr->Delete();
  }
  delete[] localElements;
  delete[] globalElements;
}

// ----------------------------------------------------------------------
int svtkKMeansDistanceFunctor::GetDataType()
{
  return SVTK_DOUBLE;
}
