/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// .SECTION Thanks
// Thanks to Janine Bennett, Philippe Pebay, and David Thompson from Sandia National Laboratories
// for implementing this test.

#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkKMeansStatistics.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTimerLog.h"

#include <sstream>

//=============================================================================
int main(int, char*[])
{
  int testStatus = 0;

  const int nDim = 4;
  int nVals = 50;

  // Seed random number generator
  svtkMath::RandomSeed(static_cast<int>(svtkTimerLog::GetUniversalTime()));

  // Generate an input table that contains samples of mutually independent random variables over [0,
  // 1]
  svtkTable* inputData = svtkTable::New();
  svtkDoubleArray* doubleArray;

  int numComponents = 1;
  for (int c = 0; c < nDim; ++c)
  {
    std::ostringstream colName;
    colName << "coord " << c;
    doubleArray = svtkDoubleArray::New();
    doubleArray->SetNumberOfComponents(numComponents);
    doubleArray->SetName(colName.str().c_str());
    doubleArray->SetNumberOfTuples(nVals);

    double x;
    for (int r = 0; r < nVals; ++r)
    {
      // x = svtkMath::Gaussian();
      x = svtkMath::Random();
      doubleArray->SetValue(r, x);
    }

    inputData->AddColumn(doubleArray);
    doubleArray->Delete();
  }

  svtkTable* paramData = svtkTable::New();
  svtkIdTypeArray* paramCluster;
  svtkDoubleArray* paramArray;
  const int numRuns = 5;
  const int numClustersInRun[] = { 5, 2, 3, 4, 5 };
  paramCluster = svtkIdTypeArray::New();
  paramCluster->SetName("K");

  for (int curRun = 0; curRun < numRuns; curRun++)
  {
    for (int nInRun = 0; nInRun < numClustersInRun[curRun]; nInRun++)
    {
      paramCluster->InsertNextValue(numClustersInRun[curRun]);
    }
  }
  paramData->AddColumn(paramCluster);
  paramCluster->Delete();

  for (int c = 0; c < 5; ++c)
  {
    std::ostringstream colName;
    colName << "coord " << c;
    paramArray = svtkDoubleArray::New();
    paramArray->SetNumberOfComponents(numComponents);
    paramArray->SetName(colName.str().c_str());

    double x;
    for (int curRun = 0; curRun < numRuns; curRun++)
    {
      for (int nInRun = 0; nInRun < numClustersInRun[curRun]; nInRun++)
      {
        // x = svtkMath::Gaussian();
        x = svtkMath::Random();
        paramArray->InsertNextValue(x);
      }
    }
    paramData->AddColumn(paramArray);
    paramArray->Delete();
  }

  // Set k-means statistics algorithm and its input data port
  svtkKMeansStatistics* haruspex = svtkKMeansStatistics::New();

  // First verify that absence of input does not cause trouble
  cout << "## Verifying that absence of input does not cause trouble... ";
  haruspex->Update();
  cout << "done.\n";

  // Prepare first test with data
  haruspex->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, inputData);
  haruspex->SetColumnStatus(inputData->GetColumnName(0), 1);
  haruspex->SetColumnStatus(inputData->GetColumnName(2), 1);
  haruspex->SetColumnStatus("Testing", 1);
  haruspex->RequestSelectedColumns();
  haruspex->SetDefaultNumberOfClusters(3);

  cout << "## Testing with no input data:"
       << "\n";
  // Test Learn and Derive options
  haruspex->SetLearnOption(true);
  haruspex->SetDeriveOption(true);
  haruspex->SetTestOption(false);
  haruspex->SetAssessOption(false);

  haruspex->Update();
  svtkMultiBlockDataSet* outputMetaDS = svtkMultiBlockDataSet::SafeDownCast(
    haruspex->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));
  for (unsigned int b = 0; b < outputMetaDS->GetNumberOfBlocks(); ++b)
  {
    svtkTable* outputMeta = svtkTable::SafeDownCast(outputMetaDS->GetBlock(b));
    if (b == 0)
    {

      svtkIdType testIntValue = 0;
      for (svtkIdType r = 0; r < outputMeta->GetNumberOfRows(); r++)
      {
        testIntValue += outputMeta->GetValueByName(r, "Cardinality").ToInt();
      }

      cout << "## Computed clusters (cardinality: " << testIntValue << " / run):\n";

      if (testIntValue != nVals)
      {
        svtkGenericWarningMacro(
          "Sum of cluster cardinalities is incorrect: " << testIntValue << " != " << nVals << ".");
        testStatus = 1;
      }
    }
    else
    {
      cout << "## Ranked cluster: "
           << "\n";
    }

    outputMeta->Dump();
    cout << "\n";
  }

  haruspex->SetInputData(svtkStatisticsAlgorithm::LEARN_PARAMETERS, paramData);
  cout << "## Testing with input table:"
       << "\n";

  paramData->Dump();
  cout << "\n";

  // Test Assess option only
  haruspex->SetLearnOption(true);
  haruspex->SetDeriveOption(true);
  haruspex->SetTestOption(false);
  haruspex->SetAssessOption(false);

  haruspex->Update();
  outputMetaDS = svtkMultiBlockDataSet::SafeDownCast(
    haruspex->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));
  for (unsigned int b = 0; b < outputMetaDS->GetNumberOfBlocks(); ++b)
  {
    svtkTable* outputMeta = svtkTable::SafeDownCast(outputMetaDS->GetBlock(b));
    if (b == 0)
    {
      svtkIdType r = 0;
      svtkIdType testIntValue = 0;
      for (int curRun = 0; curRun < numRuns; curRun++)
      {
        testIntValue = 0;
        for (int nInRun = 0; nInRun < numClustersInRun[curRun]; nInRun++)
        {
          testIntValue += outputMeta->GetValueByName(r, "Cardinality").ToInt();
          r++;
        }
      }

      if (r != outputMeta->GetNumberOfRows())
      {
        svtkGenericWarningMacro("Inconsistency in number of rows: "
          << r << " != " << outputMeta->GetNumberOfRows() << ".");
        testStatus = 1;
      }

      cout << "## Computed clusters (cardinality: " << testIntValue << " / run):\n";

      if (testIntValue != nVals)
      {
        svtkGenericWarningMacro(
          "Sum of cluster cardinalities is incorrect: " << testIntValue << " != " << nVals << ".");
        testStatus = 1;
      }
    }
    else
    {
      cout << "## Ranked cluster: "
           << "\n";
    }

    outputMeta->Dump();
    cout << "\n";
  }

  cout << "=================== ASSESS ==================== " << endl;
  svtkMultiBlockDataSet* paramsTables = svtkMultiBlockDataSet::New();
  paramsTables->ShallowCopy(outputMetaDS);

  haruspex->SetInputData(svtkStatisticsAlgorithm::INPUT_MODEL, paramsTables);

  // Test Assess option only (do not recalculate nor rederive a model)
  haruspex->SetLearnOption(false);
  haruspex->SetDeriveOption(false);
  haruspex->SetTestOption(false);
  haruspex->SetAssessOption(true);
  haruspex->Update();
  svtkTable* outputData = haruspex->GetOutput();
  outputData->Dump();
  paramsTables->Delete();
  paramData->Delete();
  inputData->Delete();
  haruspex->Delete();

  return testStatus;
}
