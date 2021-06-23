#include "svtkKMeansStatistics.h"
#include "svtkKMeansAssessFunctor.h"
#include "svtkKMeansDistanceFunctor.h"
#include "svtkStringArray.h"

#include "svtkDataObject.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkStatisticsAlgorithmPrivate.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

#include <map>
#include <sstream>
#include <vector>

svtkStandardNewMacro(svtkKMeansStatistics);
svtkCxxSetObjectMacro(svtkKMeansStatistics, DistanceFunctor, svtkKMeansDistanceFunctor);

// ----------------------------------------------------------------------
svtkKMeansStatistics::svtkKMeansStatistics()
{
  this->AssessNames->SetNumberOfValues(2);
  this->AssessNames->SetValue(0, "Distance");
  this->AssessNames->SetValue(1, "ClosestId");
  this->DefaultNumberOfClusters = 3;
  this->Tolerance = 0.01;
  this->KValuesArrayName = nullptr;
  this->SetKValuesArrayName("K");
  this->MaxNumIterations = 50;
  this->DistanceFunctor = svtkKMeansDistanceFunctor::New();
}

// ----------------------------------------------------------------------
svtkKMeansStatistics::~svtkKMeansStatistics()
{
  this->SetKValuesArrayName(nullptr);
  this->SetDistanceFunctor(nullptr);
}

// ----------------------------------------------------------------------
void svtkKMeansStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DefaultNumberofClusters: " << this->DefaultNumberOfClusters << endl;
  os << indent << "KValuesArrayName: \""
     << (this->KValuesArrayName ? this->KValuesArrayName : "nullptr") << "\"\n";
  os << indent << "MaxNumIterations: " << this->MaxNumIterations << endl;
  os << indent << "Tolerance: " << this->Tolerance << endl;
  os << indent << "DistanceFunctor: " << this->DistanceFunctor << endl;
}

// ----------------------------------------------------------------------
int svtkKMeansStatistics::InitializeDataAndClusterCenters(svtkTable* inParameters, svtkTable* inData,
  svtkTable* dataElements, svtkIdTypeArray* numberOfClusters, svtkTable* curClusterElements,
  svtkTable* newClusterElements, svtkIdTypeArray* startRunID, svtkIdTypeArray* endRunID)
{
  std::set<std::set<svtkStdString> >::const_iterator reqIt;
  if (this->Internals->Requests.size() > 1)
  {
    static int num = 0;
    num++;
    if (num < 10)
    {
      svtkWarningMacro("Only the first request will be processed -- the rest will be ignored.");
    }
  }

  if (this->Internals->Requests.empty())
  {
    svtkErrorMacro("No requests were made.");
    return 0;
  }
  reqIt = this->Internals->Requests.begin();

  svtkIdType numToAllocate;
  svtkIdType numRuns = 0;

  int initialClusterCentersProvided = 0;

  // process parameter input table
  if (inParameters && inParameters->GetNumberOfRows() > 0 && inParameters->GetNumberOfColumns() > 1)
  {
    svtkIdTypeArray* counts = svtkArrayDownCast<svtkIdTypeArray>(inParameters->GetColumn(0));
    if (!counts)
    {
      svtkWarningMacro("The first column of the input parameter table should be of svtkIdType."
        << endl
        << "The input table provided will be ignored and a single run will be performed using the "
           "first "
        << this->DefaultNumberOfClusters << " observations as the initial cluster centers.");
    }
    else
    {
      initialClusterCentersProvided = 1;
      numToAllocate = inParameters->GetNumberOfRows();
      numberOfClusters->SetNumberOfValues(numToAllocate);
      numberOfClusters->SetName(inParameters->GetColumn(0)->GetName());

      for (svtkIdType i = 0; i < numToAllocate; ++i)
      {
        numberOfClusters->SetValue(i, counts->GetValue(i));
      }
      svtkIdType curRow = 0;
      while (curRow < inParameters->GetNumberOfRows())
      {
        numRuns++;
        startRunID->InsertNextValue(curRow);
        curRow += inParameters->GetValue(curRow, 0).ToInt();
        endRunID->InsertNextValue(curRow);
      }
      svtkTable* condensedTable = svtkTable::New();
      std::set<svtkStdString>::const_iterator colItr;
      for (colItr = reqIt->begin(); colItr != reqIt->end(); ++colItr)
      {
        svtkAbstractArray* pArr = inParameters->GetColumnByName(colItr->c_str());
        svtkAbstractArray* dArr = inData->GetColumnByName(colItr->c_str());
        if (pArr && dArr)
        {
          condensedTable->AddColumn(pArr);
          dataElements->AddColumn(dArr);
        }
        else
        {
          svtkWarningMacro("Skipping requested column \"" << colItr->c_str() << "\".");
        }
      }
      newClusterElements->DeepCopy(condensedTable);
      curClusterElements->DeepCopy(condensedTable);
      condensedTable->Delete();
    }
  }
  if (!initialClusterCentersProvided)
  {
    // otherwise create an initial set of cluster coords
    numRuns = 1;
    numToAllocate = this->DefaultNumberOfClusters < inData->GetNumberOfRows()
      ? this->DefaultNumberOfClusters
      : inData->GetNumberOfRows();
    startRunID->InsertNextValue(0);
    endRunID->InsertNextValue(numToAllocate);
    numberOfClusters->SetName(this->KValuesArrayName);

    for (svtkIdType j = 0; j < inData->GetNumberOfColumns(); j++)
    {
      if (reqIt->find(inData->GetColumnName(j)) != reqIt->end())
      {
        svtkAbstractArray* curCoords = this->DistanceFunctor->CreateCoordinateArray();
        svtkAbstractArray* newCoords = this->DistanceFunctor->CreateCoordinateArray();
        curCoords->SetName(inData->GetColumnName(j));
        newCoords->SetName(inData->GetColumnName(j));
        curClusterElements->AddColumn(curCoords);
        newClusterElements->AddColumn(newCoords);
        curCoords->Delete();
        newCoords->Delete();
        dataElements->AddColumn(inData->GetColumnByName(inData->GetColumnName(j)));
      }
    }
    CreateInitialClusterCenters(
      numToAllocate, numberOfClusters, inData, curClusterElements, newClusterElements);
  }

  if (curClusterElements->GetNumberOfColumns() == 0)
  {
    return 0;
  }
  return numRuns;
}

// ----------------------------------------------------------------------
void svtkKMeansStatistics::CreateInitialClusterCenters(svtkIdType numToAllocate,
  svtkIdTypeArray* numberOfClusters, svtkTable* inData, svtkTable* curClusterElements,
  svtkTable* newClusterElements)
{
  std::set<std::set<svtkStdString> >::const_iterator reqIt;
  if (this->Internals->Requests.size() > 1)
  {
    static int num = 0;
    ++num;
    if (num < 10)
    {
      svtkWarningMacro("Only the first request will be processed -- the rest will be ignored.");
    }
  }

  if (this->Internals->Requests.empty())
  {
    svtkErrorMacro("No requests were made.");
    return;
  }
  reqIt = this->Internals->Requests.begin();

  for (svtkIdType i = 0; i < numToAllocate; ++i)
  {
    numberOfClusters->InsertNextValue(numToAllocate);
    svtkVariantArray* curRow = svtkVariantArray::New();
    svtkVariantArray* newRow = svtkVariantArray::New();
    for (int j = 0; j < inData->GetNumberOfColumns(); j++)
    {
      if (reqIt->find(inData->GetColumnName(j)) != reqIt->end())
      {
        curRow->InsertNextValue(inData->GetValue(i, j));
        newRow->InsertNextValue(inData->GetValue(i, j));
      }
    }
    curClusterElements->InsertNextRow(curRow);
    newClusterElements->InsertNextRow(newRow);
    curRow->Delete();
    newRow->Delete();
  }
}

// ----------------------------------------------------------------------
svtkIdType svtkKMeansStatistics::GetTotalNumberOfObservations(svtkIdType numObservations)
{
  return numObservations;
}

// ----------------------------------------------------------------------
void svtkKMeansStatistics::UpdateClusterCenters(svtkTable* newClusterElements,
  svtkTable* curClusterElements, svtkIdTypeArray* svtkNotUsed(numMembershipChanges),
  svtkIdTypeArray* numDataElementsInCluster, svtkDoubleArray* svtkNotUsed(error),
  svtkIdTypeArray* startRunID, svtkIdTypeArray* endRunID, svtkIntArray* computeRun)
{
  for (svtkIdType runID = 0; runID < startRunID->GetNumberOfTuples(); runID++)
  {
    if (computeRun->GetValue(runID))
    {
      for (svtkIdType i = startRunID->GetValue(runID); i < endRunID->GetValue(runID); ++i)
      {
        if (numDataElementsInCluster->GetValue(i) == 0)
        {
          svtkWarningMacro("cluster center " << i - startRunID->GetValue(runID) << " in run "
                                            << runID << " is degenerate. Attempting to perturb");
          this->DistanceFunctor->PerturbElement(newClusterElements, curClusterElements, i,
            startRunID->GetValue(runID), endRunID->GetValue(runID), 0.8);
        }
      }
    }
  }
}

// ----------------------------------------------------------------------
bool svtkKMeansStatistics::SetParameter(
  const char* parameter, int svtkNotUsed(index), svtkVariant value)
{
  if (!parameter)
    return false;

  svtkStdString pname = parameter;
  if (pname == "DefaultNumberOfClusters" || pname == "k" || pname == "K")
  {
    bool valid;
    int k = value.ToInt(&valid);
    if (valid && k > 0)
    {
      this->SetDefaultNumberOfClusters(k);
      return true;
    }
  }
  else if (pname == "Tolerance")
  {
    double tol = value.ToDouble();
    this->SetTolerance(tol);
    return true;
  }
  else if (pname == "MaxNumIterations")
  {
    bool valid;
    int maxit = value.ToInt(&valid);
    if (valid && maxit >= 0)
    {
      this->SetMaxNumIterations(maxit);
      return true;
    }
  }

  return false;
}

// ----------------------------------------------------------------------
void svtkKMeansStatistics::Learn(
  svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta)
{
  if (!outMeta)
  {
    return;
  }

  if (!inData)
  {
    return;
  }

  if (!this->DistanceFunctor)
  {
    svtkErrorMacro("Distance functor is nullptr");
    return;
  }

  // Data initialization
  svtkIdTypeArray* numberOfClusters = svtkIdTypeArray::New();
  svtkTable* curClusterElements = svtkTable::New();
  svtkTable* newClusterElements = svtkTable::New();
  svtkIdTypeArray* startRunID = svtkIdTypeArray::New();
  svtkIdTypeArray* endRunID = svtkIdTypeArray::New();
  svtkTable* dataElements = svtkTable::New();
  int numRuns = InitializeDataAndClusterCenters(inParameters, inData, dataElements,
    numberOfClusters, curClusterElements, newClusterElements, startRunID, endRunID);
  if (numRuns == 0)
  {
    numberOfClusters->Delete();
    curClusterElements->Delete();
    newClusterElements->Delete();
    startRunID->Delete();
    endRunID->Delete();
    dataElements->Delete();
    return;
  }

  svtkIdType numObservations = inData->GetNumberOfRows();
  svtkIdType totalNumberOfObservations = this->GetTotalNumberOfObservations(numObservations);
  svtkIdType numToAllocate = curClusterElements->GetNumberOfRows();
  svtkIdTypeArray* numIterations = svtkIdTypeArray::New();
  svtkIdTypeArray* numDataElementsInCluster = svtkIdTypeArray::New();
  svtkDoubleArray* error = svtkDoubleArray::New();
  svtkIdTypeArray* clusterMemberID = svtkIdTypeArray::New();
  svtkIdTypeArray* numMembershipChanges = svtkIdTypeArray::New();
  svtkIntArray* computeRun = svtkIntArray::New();
  svtkIdTypeArray* clusterRunIDs = svtkIdTypeArray::New();

  numDataElementsInCluster->SetNumberOfValues(numToAllocate);
  numDataElementsInCluster->SetName("Cardinality");
  clusterRunIDs->SetNumberOfValues(numToAllocate);
  clusterRunIDs->SetName("Run ID");
  error->SetNumberOfValues(numToAllocate);
  error->SetName("Error");
  numIterations->SetNumberOfValues(numToAllocate);
  numIterations->SetName("Iterations");
  numMembershipChanges->SetNumberOfValues(numRuns);
  computeRun->SetNumberOfValues(numRuns);
  clusterMemberID->SetNumberOfValues(numObservations * numRuns);
  clusterMemberID->SetName("cluster member id");

  for (int i = 0; i < numRuns; ++i)
  {
    for (svtkIdType j = startRunID->GetValue(i); j < endRunID->GetValue(i); j++)
    {
      clusterRunIDs->SetValue(j, i);
    }
  }

  numIterations->FillComponent(0, 0);
  computeRun->FillComponent(0, 1);
  int allConverged, numIter = 0;
  clusterMemberID->FillComponent(0, -1);

  // Iterate until new cluster centers have converged OR we have reached a max number of iterations
  do
  {
    // Initialize coordinates, cluster sizes and errors
    numMembershipChanges->FillComponent(0, 0);
    for (int runID = 0; runID < numRuns; runID++)
    {
      if (computeRun->GetValue(runID))
      {
        for (svtkIdType j = startRunID->GetValue(runID); j < endRunID->GetValue(runID); j++)
        {
          curClusterElements->SetRow(j, newClusterElements->GetRow(j));
          newClusterElements->SetRow(
            j, this->DistanceFunctor->GetEmptyTuple(newClusterElements->GetNumberOfColumns()));
          numDataElementsInCluster->SetValue(j, 0);
          error->SetValue(j, 0.0);
        }
      }
    }

    // Find minimum distance between each observation and each cluster center,
    // then assign the observation to the nearest cluster.
    svtkIdType localMemberID, offsetLocalMemberID;
    double minDistance, curDistance;
    for (svtkIdType observation = 0; observation < dataElements->GetNumberOfRows(); observation++)
    {
      for (int runID = 0; runID < numRuns; runID++)
      {
        if (computeRun->GetValue(runID))
        {
          svtkIdType runStartIdx = startRunID->GetValue(runID);
          svtkIdType runEndIdx = endRunID->GetValue(runID);
          if (runStartIdx >= runEndIdx)
          {
            continue;
          }
          svtkIdType j = runStartIdx;
          localMemberID = 0;
          offsetLocalMemberID = runStartIdx;
          (*this->DistanceFunctor)(
            minDistance, curClusterElements->GetRow(j), dataElements->GetRow(observation));
          curDistance = minDistance;
          ++j;
          for (/* no init */; j < runEndIdx; j++)
          {
            (*this->DistanceFunctor)(
              curDistance, curClusterElements->GetRow(j), dataElements->GetRow(observation));
            if (curDistance < minDistance)
            {
              minDistance = curDistance;
              localMemberID = j - runStartIdx;
              offsetLocalMemberID = j;
            }
          }
          // We've located the nearest cluster center. Has it changed since the last iteration?
          if (clusterMemberID->GetValue(observation * numRuns + runID) != localMemberID)
          {
            numMembershipChanges->SetValue(runID, numMembershipChanges->GetValue(runID) + 1);
            clusterMemberID->SetValue(observation * numRuns + runID, localMemberID);
          }
          // Give the distance functor a chance to modify any derived quantities used to
          // change the cluster centers between iterations, now that we know which cluster
          // center the observation is assigned to.
          svtkIdType newCardinality = numDataElementsInCluster->GetValue(offsetLocalMemberID) + 1;
          numDataElementsInCluster->SetValue(offsetLocalMemberID, newCardinality);
          this->DistanceFunctor->PairwiseUpdate(newClusterElements, offsetLocalMemberID,
            dataElements->GetRow(observation), 1, newCardinality);
          // Update the error for this cluster center to account for this observation.
          error->SetValue(offsetLocalMemberID, error->GetValue(offsetLocalMemberID) + minDistance);
        }
      }
    }
    // update cluster centers
    this->UpdateClusterCenters(newClusterElements, curClusterElements, numMembershipChanges,
      numDataElementsInCluster, error, startRunID, endRunID, computeRun);

    // check for convergence
    numIter++;
    allConverged = 0;

    for (int j = 0; j < numRuns; j++)
    {
      if (computeRun->GetValue(j))
      {
        double percentChanged = static_cast<double>(numMembershipChanges->GetValue(j)) /
          static_cast<double>(totalNumberOfObservations);
        if (percentChanged < this->Tolerance || numIter == this->MaxNumIterations)
        {
          allConverged++;
          computeRun->SetValue(j, 0);
          for (int k = startRunID->GetValue(j); k < endRunID->GetValue(j); k++)
          {
            numIterations->SetValue(k, numIter);
          }
        }
      }
      else
      {
        allConverged++;
      }
    }
  } while (allConverged < numRuns && numIter < this->MaxNumIterations);

  // add columns to output table
  svtkTable* outputTable = svtkTable::New();
  outputTable->AddColumn(clusterRunIDs);
  outputTable->AddColumn(numberOfClusters);
  outputTable->AddColumn(numIterations);
  outputTable->AddColumn(error);
  outputTable->AddColumn(numDataElementsInCluster);
  for (svtkIdType i = 0; i < newClusterElements->GetNumberOfColumns(); ++i)
  {
    outputTable->AddColumn(newClusterElements->GetColumn(i));
  }

  outMeta->SetNumberOfBlocks(1);
  outMeta->SetBlock(0, outputTable);
  outMeta->GetMetaData(static_cast<unsigned>(0))
    ->Set(svtkCompositeDataSet::NAME(), "Updated Cluster Centers");

  clusterRunIDs->Delete();
  numberOfClusters->Delete();
  numDataElementsInCluster->Delete();
  numIterations->Delete();
  error->Delete();
  curClusterElements->Delete();
  newClusterElements->Delete();
  dataElements->Delete();
  clusterMemberID->Delete();
  outputTable->Delete();
  startRunID->Delete();
  endRunID->Delete();
  computeRun->Delete();
  numMembershipChanges->Delete();
}

// ----------------------------------------------------------------------
void svtkKMeansStatistics::Derive(svtkMultiBlockDataSet* outMeta)
{
  svtkTable* outTable;
  svtkIdTypeArray* clusterRunIDs;
  svtkIdTypeArray* numIterations;
  svtkIdTypeArray* numberOfClusters;
  svtkDoubleArray* error;

  if (!outMeta || !(outTable = svtkTable::SafeDownCast(outMeta->GetBlock(0))) ||
    !(clusterRunIDs = svtkArrayDownCast<svtkIdTypeArray>(outTable->GetColumn(0))) ||
    !(numberOfClusters = svtkArrayDownCast<svtkIdTypeArray>(outTable->GetColumn(1))) ||
    !(numIterations = svtkArrayDownCast<svtkIdTypeArray>(outTable->GetColumn(2))) ||
    !(error = svtkArrayDownCast<svtkDoubleArray>(outTable->GetColumn(3))))
  {
    return;
  }

  // Create an output table
  // outMeta and which is presumed to exist upon entry to Derive).

  outMeta->SetNumberOfBlocks(2);

  svtkIdTypeArray* totalClusterRunIDs = svtkIdTypeArray::New();
  svtkIdTypeArray* totalNumberOfClusters = svtkIdTypeArray::New();
  svtkIdTypeArray* totalNumIterations = svtkIdTypeArray::New();
  svtkIdTypeArray* globalRank = svtkIdTypeArray::New();
  svtkIdTypeArray* localRank = svtkIdTypeArray::New();
  svtkDoubleArray* totalError = svtkDoubleArray::New();

  totalClusterRunIDs->SetName(clusterRunIDs->GetName());
  totalNumberOfClusters->SetName(numberOfClusters->GetName());
  totalNumIterations->SetName(numIterations->GetName());
  totalError->SetName("Total Error");
  globalRank->SetName("Global Rank");
  localRank->SetName("Local Rank");

  std::multimap<double, svtkIdType> globalErrorMap;
  std::map<svtkIdType, std::multimap<double, svtkIdType> > localErrorMap;

  svtkIdType curRow = 0;
  while (curRow < outTable->GetNumberOfRows())
  {
    totalClusterRunIDs->InsertNextValue(clusterRunIDs->GetValue(curRow));
    totalNumIterations->InsertNextValue(numIterations->GetValue(curRow));
    totalNumberOfClusters->InsertNextValue(numberOfClusters->GetValue(curRow));
    double totalErr = 0.0;
    for (svtkIdType i = curRow; i < curRow + numberOfClusters->GetValue(curRow); ++i)
    {
      totalErr += error->GetValue(i);
    }
    totalError->InsertNextValue(totalErr);
    globalErrorMap.insert(
      std::multimap<double, svtkIdType>::value_type(totalErr, clusterRunIDs->GetValue(curRow)));
    localErrorMap[numberOfClusters->GetValue(curRow)].insert(
      std::multimap<double, svtkIdType>::value_type(totalErr, clusterRunIDs->GetValue(curRow)));
    curRow += numberOfClusters->GetValue(curRow);
  }

  globalRank->SetNumberOfValues(totalClusterRunIDs->GetNumberOfTuples());
  localRank->SetNumberOfValues(totalClusterRunIDs->GetNumberOfTuples());
  int rankID = 1;

  for (std::multimap<double, svtkIdType>::iterator itr = globalErrorMap.begin();
       itr != globalErrorMap.end(); ++itr)
  {
    globalRank->SetValue(itr->second, rankID++);
  }
  for (std::map<svtkIdType, std::multimap<double, svtkIdType> >::iterator itr = localErrorMap.begin();
       itr != localErrorMap.end(); ++itr)
  {
    rankID = 1;
    for (std::multimap<double, svtkIdType>::iterator rItr = itr->second.begin();
         rItr != itr->second.end(); ++rItr)
    {
      localRank->SetValue(rItr->second, rankID++);
    }
  }

  svtkTable* ranked = svtkTable::New();
  outMeta->SetBlock(1, ranked);
  outMeta->GetMetaData(static_cast<unsigned>(1))
    ->Set(svtkCompositeDataSet::NAME(), "Ranked Cluster Centers");
  ranked->Delete(); // outMeta now owns ranked
  ranked->AddColumn(totalClusterRunIDs);
  ranked->AddColumn(totalNumberOfClusters);
  ranked->AddColumn(totalNumIterations);
  ranked->AddColumn(totalError);
  ranked->AddColumn(localRank);
  ranked->AddColumn(globalRank);

  totalError->Delete();
  localRank->Delete();
  globalRank->Delete();
  totalClusterRunIDs->Delete();
  totalNumberOfClusters->Delete();
  totalNumIterations->Delete();
}

// ----------------------------------------------------------------------
void svtkKMeansStatistics::Assess(svtkTable* inData, svtkMultiBlockDataSet* inMeta, svtkTable* outData)
{
  if (!inData)
  {
    return;
  }

  if (!inMeta)
  {
    return;
  }

  // Add a column to the output data related to the each input datum wrt the model in the request.
  // Column names of the metadata and input data are assumed to match.
  // The output columns will be named "this->AssessNames->GetValue(0)(A,B,C)" where
  // "A", "B", and "C" are the column names specified in the per-request metadata tables.
  AssessFunctor* dfunc = nullptr;
  // only one request allowed in when learning, so there will only be one
  svtkTable* reqModel = svtkTable::SafeDownCast(inMeta->GetBlock(0));
  if (!reqModel)
  {
    // silently skip invalid entries. Note we leave assessValues column in output data even when
    // it's empty.
    return;
  }

  this->SelectAssessFunctor(inData, reqModel, nullptr, dfunc);
  if (!dfunc)
  {
    svtkWarningMacro("Assessment could not be accommodated. Skipping.");
    return;
  }

  svtkKMeansAssessFunctor* kmfunc = static_cast<svtkKMeansAssessFunctor*>(dfunc);

  svtkIdType nv = this->AssessNames->GetNumberOfValues();
  int numRuns = kmfunc->GetNumberOfRuns();
  std::vector<svtkStdString> names(nv * numRuns);
  svtkIdType nRow = inData->GetNumberOfRows();
  for (int i = 0; i < numRuns; ++i)
  {
    for (svtkIdType v = 0; v < nv; ++v)
    {
      std::ostringstream assessColName;
      assessColName << this->AssessNames->GetValue(v) << "(" << i << ")";

      svtkAbstractArray* assessValues;
      if (v)
      { // The "closest id" column for each request will always be integer-valued
        assessValues = svtkIntArray::New();
      }
      else
      { // We'll assume for now that the "distance" column for each request will be a real number.
        assessValues = svtkDoubleArray::New();
      }
      names[i * nv + v] =
        assessColName.str()
          .c_str(); // Storing names to be able to use SetValueByName which is faster than SetValue
      assessValues->SetName(names[i * nv + v]);
      assessValues->SetNumberOfTuples(nRow);
      outData->AddColumn(assessValues);
      assessValues->Delete();
    }
  }

  // Assess each entry of the column
  svtkDoubleArray* assessResult = svtkDoubleArray::New();
  for (svtkIdType r = 0; r < nRow; ++r)
  {
    (*dfunc)(assessResult, r);
    for (svtkIdType j = 0; j < nv * numRuns; ++j)
    {
      outData->SetValueByName(r, names[j], assessResult->GetValue(j));
    }
  }
  assessResult->Delete();

  delete dfunc;
}

// ----------------------------------------------------------------------
void svtkKMeansStatistics::SelectAssessFunctor(svtkTable* inData, svtkDataObject* inMetaDO,
  svtkStringArray* svtkNotUsed(rowNames), AssessFunctor*& dfunc)
{
  (void)inData;

  dfunc = nullptr;
  svtkTable* reqModel = svtkTable::SafeDownCast(inMetaDO);
  if (!reqModel)
  {
    return;
  }

  if (!this->DistanceFunctor)
  {
    svtkErrorMacro("Distance functor is nullptr");
    return;
  }

  svtkKMeansAssessFunctor* kmfunc = svtkKMeansAssessFunctor::New();

  if (!kmfunc->Initialize(inData, reqModel, this->DistanceFunctor))
  {
    delete kmfunc;
    return;
  }
  dfunc = kmfunc;
}

// ----------------------------------------------------------------------
svtkKMeansAssessFunctor* svtkKMeansAssessFunctor::New()
{
  return new svtkKMeansAssessFunctor;
}

// ----------------------------------------------------------------------
svtkKMeansAssessFunctor::~svtkKMeansAssessFunctor()
{
  this->ClusterMemberIDs->Delete();
  this->Distances->Delete();
}

// ----------------------------------------------------------------------
bool svtkKMeansAssessFunctor::Initialize(
  svtkTable* inData, svtkTable* inModel, svtkKMeansDistanceFunctor* dfunc)
{
  svtkIdType numObservations = inData->GetNumberOfRows();
  svtkTable* dataElements = svtkTable::New();
  svtkTable* curClusterElements = svtkTable::New();
  svtkIdTypeArray* startRunID = svtkIdTypeArray::New();
  svtkIdTypeArray* endRunID = svtkIdTypeArray::New();

  this->Distances = svtkDoubleArray::New();
  this->ClusterMemberIDs = svtkIdTypeArray::New();
  this->NumRuns = 0;

  // cluster coordinates start in column 5 of the inModel table
  for (svtkIdType i = 5; i < inModel->GetNumberOfColumns(); ++i)
  {
    curClusterElements->AddColumn(inModel->GetColumn(i));
    dataElements->AddColumn(inData->GetColumnByName(inModel->GetColumnName(i)));
  }

  svtkIdType curRow = 0;
  while (curRow < inModel->GetNumberOfRows())
  {
    this->NumRuns++;
    startRunID->InsertNextValue(curRow);
    // number of clusters "K" is stored in column 1 of the inModel table
    curRow += inModel->GetValue(curRow, 1).ToInt();
    endRunID->InsertNextValue(curRow);
  }

  this->Distances->SetNumberOfValues(numObservations * this->NumRuns);
  this->ClusterMemberIDs->SetNumberOfValues(numObservations * this->NumRuns);

  // find minimum distance between each data object and cluster center
  for (svtkIdType observation = 0; observation < numObservations; ++observation)
  {
    for (int runID = 0; runID < this->NumRuns; ++runID)
    {
      svtkIdType runStartIdx = startRunID->GetValue(runID);
      svtkIdType runEndIdx = endRunID->GetValue(runID);
      if (runStartIdx >= runEndIdx)
      {
        continue;
      }
      // Find the closest cluster center to the observation across all centers in the runID-th run.
      svtkIdType j = runStartIdx;
      double minDistance = 0.0;
      double curDistance = 0.0;
      (*dfunc)(minDistance, curClusterElements->GetRow(j), dataElements->GetRow(observation));
      svtkIdType localMemberID = 0;
      for (/* no init */; j < runEndIdx; ++j)
      {
        (*dfunc)(curDistance, curClusterElements->GetRow(j), dataElements->GetRow(observation));
        if (curDistance < minDistance)
        {
          minDistance = curDistance;
          localMemberID = j - runStartIdx;
        }
      }
      this->ClusterMemberIDs->SetValue(observation * this->NumRuns + runID, localMemberID);
      this->Distances->SetValue(observation * this->NumRuns + runID, minDistance);
    }
  }

  dataElements->Delete();
  curClusterElements->Delete();
  startRunID->Delete();
  endRunID->Delete();
  return true;
}

// ----------------------------------------------------------------------
void svtkKMeansAssessFunctor::operator()(svtkDoubleArray* result, svtkIdType row)
{

  result->SetNumberOfValues(2 * this->NumRuns);
  svtkIdType resIndex = 0;
  for (int runID = 0; runID < this->NumRuns; runID++)
  {
    result->SetValue(resIndex++, this->Distances->GetValue(row * this->NumRuns + runID));
    result->SetValue(resIndex++, this->ClusterMemberIDs->GetValue(row * this->NumRuns + runID));
  }
}
