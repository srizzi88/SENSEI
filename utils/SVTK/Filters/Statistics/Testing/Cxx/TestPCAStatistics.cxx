/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
// for implementing this test.
// Test added for Robust PCA by Tristan Coulange, Kitware SAS 2013

#include "svtkDoubleArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkOrderStatistics.h"
#include "svtkPCAStatistics.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTestUtilities.h"

#include "svtksys/SystemTools.hxx"

// Perform a fuzzy compare of floats/doubles
template <class A>
bool fuzzyCompare(A a, A b)
{
  //  return fabs(a - b) < std::numeric_limits<A>::epsilon();
  return fabs(a - b) < .0001;
}

int TestPCA(int argc, char* argv[]);
int TestPCARobust(int argc, char* argv[]);
int TestPCAPart(int argc, char* argv[], bool RobustPCA);
int TestPCARobust2();
int TestEigen();

//=============================================================================
int TestPCAStatistics(int argc, char* argv[])
{
  int result = EXIT_SUCCESS;

  result |= TestPCA(argc, argv);
  result |= TestPCARobust(argc, argv);
  result |= TestPCARobust2();
  result |= TestEigen();

  if (result == EXIT_FAILURE)
  {
    cout << "FAILURE" << endl;
  }
  else
  {
    cout << "SUCCESS" << endl;
  }

  return result;
}

//=============================================================================
int TestPCA(int argc, char* argv[])
{
  return TestPCAPart(argc, argv, false);
}

//=============================================================================
int TestPCARobust(int argc, char* argv[])
{
  return TestPCAPart(argc, argv, true);
}

//=============================================================================
int TestPCARobust2()
{
  const int nVals = 7;
  double mingledData[] = {
    0., 1.,  //
    1., 1.,  //
    2., 1.,  //
    3., 1.,  //
    4., 1.,  //
    5., 1.,  //
    10., 10. //
  };

  const char m0Name[] = "M0";
  svtkNew<svtkDoubleArray> dataset1Arr;
  dataset1Arr->SetNumberOfComponents(1);
  dataset1Arr->SetName(m0Name);

  const char m1Name[] = "M1";
  svtkNew<svtkDoubleArray> dataset2Arr;
  dataset2Arr->SetNumberOfComponents(1);
  dataset2Arr->SetName(m1Name);

  for (int i = 0; i < nVals; ++i)
  {
    dataset1Arr->InsertNextValue(mingledData[i * 2]);
    dataset2Arr->InsertNextValue(mingledData[i * 2 + 1]);
  }

  svtkNew<svtkTable> datasetTable;
  datasetTable->AddColumn(dataset1Arr);
  datasetTable->AddColumn(dataset2Arr);

  // Set PCA statistics algorithm and its input data port
  svtkNew<svtkPCAStatistics> pcas;

  // Prepare first test with data
  pcas->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, datasetTable);
  pcas->MedianAbsoluteDeviationOn();

  // -- Select Column Pairs of Interest ( Learn Mode ) --
  pcas->SetColumnStatus(m0Name, 1);
  pcas->SetColumnStatus(m1Name, 1);

  // Test all options but Assess
  pcas->SetLearnOption(true);
  pcas->SetDeriveOption(true);
  pcas->SetTestOption(true);
  pcas->SetAssessOption(true);
  pcas->Update();

  svtkTable* outputData = pcas->GetOutput();

  double res[] = { -3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 7.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 9.0 };

  for (svtkIdType j = 0; j < 2; j++)
  {
    for (svtkIdType i = 0; i < 7; i++)
    {
      if (outputData->GetValue(i, j + 2) != res[j * outputData->GetNumberOfRows() + i])
      {
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

//=============================================================================
int TestPCAPart(int argc, char* argv[], bool robustPCA)
{
  char* normScheme = svtkTestUtilities::GetArgOrEnvOrDefault(
    "-normalize-covariance", argc, argv, "SVTK_NORMALIZE_COVARIANCE", "None");
  int testStatus = 0;

  /* */
  double mingledData[] = {
    46, 45, //
    47, 49, //
    46, 47, //
    46, 46, //
    47, 46, //
    47, 49, //
    49, 49, //
    47, 45, //
    50, 50, //
    46, 46, //
    51, 50, //
    48, 48, //
    52, 54, //
    48, 47, //
    52, 52, //
    49, 49, //
    53, 54, //
    50, 50, //
    53, 54, //
    50, 52, //
    53, 53, //
    50, 51, //
    54, 54, //
    49, 49, //
    52, 52, //
    50, 51, //
    52, 52, //
    49, 47, //
    48, 48, //
    48, 50, //
    46, 48, //
    47, 47  //
  };
  int nVals = 32;

  const char m0Name[] = "M0";
  svtkDoubleArray* dataset1Arr = svtkDoubleArray::New();
  dataset1Arr->SetNumberOfComponents(1);
  dataset1Arr->SetName(m0Name);

  const char m1Name[] = "M1";
  svtkDoubleArray* dataset2Arr = svtkDoubleArray::New();
  dataset2Arr->SetNumberOfComponents(1);
  dataset2Arr->SetName(m1Name);

  const char m2Name[] = "M2";
  svtkDoubleArray* dataset3Arr = svtkDoubleArray::New();
  dataset3Arr->SetNumberOfComponents(1);
  dataset3Arr->SetName(m2Name);

  for (int i = 0; i < nVals; ++i)
  {
    int ti = i << 1;
    dataset1Arr->InsertNextValue(mingledData[ti]);
    dataset2Arr->InsertNextValue(mingledData[ti + 1]);
    dataset3Arr->InsertNextValue(i != 12 ? -1. : -1.001);
  }

  svtkTable* datasetTable = svtkTable::New();
  datasetTable->AddColumn(dataset1Arr);
  dataset1Arr->Delete();
  datasetTable->AddColumn(dataset2Arr);
  dataset2Arr->Delete();
  datasetTable->AddColumn(dataset3Arr);
  dataset3Arr->Delete();

  // Set PCA statistics algorithm and its input data port
  svtkPCAStatistics* pcas = svtkPCAStatistics::New();
  pcas->SetMedianAbsoluteDeviation(robustPCA);

  // First verify that absence of input does not cause trouble
  cout << "## Verifying that absence of input does not cause trouble... ";
  pcas->Update();
  cout << "done.\n";

  // Prepare first test with data
  pcas->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, datasetTable);
  pcas->SetNormalizationSchemeByName(normScheme);
  pcas->SetBasisSchemeByName("FixedBasisEnergy");
  pcas->SetFixedBasisEnergy(1. - 1e-8);

  datasetTable->Delete();

  // -- Select Column Pairs of Interest ( Learn Mode ) --
  pcas->SetColumnStatus(m0Name, 1);
  pcas->SetColumnStatus(m1Name, 1);
  pcas->RequestSelectedColumns();
  pcas->ResetAllColumnStates();
  pcas->SetColumnStatus(m0Name, 1);
  pcas->SetColumnStatus(m1Name, 1);
  pcas->SetColumnStatus(m2Name, 1);
  pcas->SetColumnStatus(m2Name, 0);
  pcas->SetColumnStatus(m2Name, 1);
  pcas->RequestSelectedColumns();
  pcas->RequestSelectedColumns(); // Try a duplicate entry. This should have no effect.
  pcas->SetColumnStatus(m0Name, 0);
  pcas->SetColumnStatus(m2Name, 0);
  pcas->SetColumnStatus("Metric 3",
    1); // An invalid name. This should result in a request for metric 1's self-correlation.
  // pcas->RequestSelectedColumns(); will get called in RequestData()

  // Test all options but Assess
  pcas->SetLearnOption(true);
  pcas->SetDeriveOption(true);
  pcas->SetTestOption(true);
  pcas->SetAssessOption(false);
  pcas->Update();

  svtkMultiBlockDataSet* outputMetaDS = svtkMultiBlockDataSet::SafeDownCast(
    pcas->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));
  svtkTable* outputTest = pcas->GetOutput(svtkStatisticsAlgorithm::OUTPUT_TEST);

  cout << "## Calculated the following statistics for data set:\n";
  for (unsigned int b = 0; b < outputMetaDS->GetNumberOfBlocks(); ++b)
  {
    svtkTable* outputMeta = svtkTable::SafeDownCast(outputMetaDS->GetBlock(b));

    if (b == 0)
    {
      cout << "Primary Statistics\n";
    }
    else
    {
      cout << "Derived Statistics " << (b - 1) << "\n";
    }

    outputMeta->Dump();
  }

  // Check some results of the Test option
  cout << "\n## Calculated the following Jarque-Bera-Srivastava statistics for pseudo-random "
          "variables (n="
       << nVals;

#ifdef USE_GNU_R
  int nNonGaussian = 1;
  int nRejected = 0;
  double alpha = .01;

  cout << ", null hypothesis: binormality, significance level=" << alpha;
#endif // USE_GNU_R

  cout << "):\n";

  // Loop over Test table
  for (svtkIdType r = 0; r < outputTest->GetNumberOfRows(); ++r)
  {
    cout << "   ";
    for (int i = 0; i < outputTest->GetNumberOfColumns(); ++i)
    {
      cout << outputTest->GetColumnName(i) << "=" << outputTest->GetValue(r, i).ToString() << "  ";
    }

#ifdef USE_GNU_R
    // Check if null hypothesis is rejected at specified significance level
    double p = outputTest->GetValueByName(r, "P").ToDouble();
    // Must verify that p value is valid (it is set to -1 if R has failed)
    if (p > -1 && p < alpha)
    {
      cout << "N.H. rejected";

      ++nRejected;
    }
#endif // USE_GNU_R

    cout << "\n";
  }

#ifdef USE_GNU_R
  if (nRejected < nNonGaussian)
  {
    svtkGenericWarningMacro("Rejected only "
      << nRejected << " null hypotheses of binormality whereas " << nNonGaussian
      << " variable pairs are not Gaussian");
    testStatus = 1;
  }
#endif // USE_GNU_R

  // Test Assess option
  svtkMultiBlockDataSet* paramsTables = svtkMultiBlockDataSet::New();
  paramsTables->ShallowCopy(outputMetaDS);

  pcas->SetInputData(svtkStatisticsAlgorithm::INPUT_MODEL, paramsTables);
  paramsTables->Delete();

  // Test Assess only (Do not recalculate nor rederive nor retest a model)
  pcas->SetLearnOption(false);
  pcas->SetDeriveOption(false);
  pcas->SetTestOption(false);
  pcas->SetAssessOption(true);
  pcas->Update();

  cout << "\n## Assessment results:\n";
  svtkTable* outputData = pcas->GetOutput();
  outputData->Dump();

  pcas->Delete();
  delete[] normScheme;

  return testStatus;
}

int TestEigen()
{
  const char m0Name[] = "M0";
  svtkSmartPointer<svtkDoubleArray> dataset1Arr = svtkSmartPointer<svtkDoubleArray>::New();
  dataset1Arr->SetNumberOfComponents(1);
  dataset1Arr->SetName(m0Name);
  dataset1Arr->InsertNextValue(0);
  dataset1Arr->InsertNextValue(1);
  dataset1Arr->InsertNextValue(0);

  const char m1Name[] = "M1";
  svtkSmartPointer<svtkDoubleArray> dataset2Arr = svtkSmartPointer<svtkDoubleArray>::New();
  dataset2Arr->SetNumberOfComponents(1);
  dataset2Arr->SetName(m1Name);
  dataset2Arr->InsertNextValue(0);
  dataset2Arr->InsertNextValue(0);
  dataset2Arr->InsertNextValue(1);

  const char m2Name[] = "M2";
  svtkSmartPointer<svtkDoubleArray> dataset3Arr = svtkSmartPointer<svtkDoubleArray>::New();
  dataset3Arr->SetNumberOfComponents(1);
  dataset3Arr->SetName(m2Name);
  dataset3Arr->InsertNextValue(0);
  dataset3Arr->InsertNextValue(0);
  dataset3Arr->InsertNextValue(0);

  svtkSmartPointer<svtkTable> datasetTable = svtkSmartPointer<svtkTable>::New();
  datasetTable->AddColumn(dataset1Arr);
  datasetTable->AddColumn(dataset2Arr);
  datasetTable->AddColumn(dataset3Arr);

  svtkSmartPointer<svtkPCAStatistics> pcaStatistics = svtkSmartPointer<svtkPCAStatistics>::New();
  pcaStatistics->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, datasetTable);

  pcaStatistics->SetColumnStatus("M0", 1);
  pcaStatistics->SetColumnStatus("M1", 1);
  pcaStatistics->SetColumnStatus("M2", 1);
  pcaStatistics->RequestSelectedColumns();

  pcaStatistics->SetDeriveOption(true);

  pcaStatistics->Update();

  svtkSmartPointer<svtkMultiBlockDataSet> outputMetaDS = svtkMultiBlockDataSet::SafeDownCast(
    pcaStatistics->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));

  svtkSmartPointer<svtkTable> outputMeta = svtkTable::SafeDownCast(outputMetaDS->GetBlock(1));

  outputMeta->Dump();

  ///////// Eigenvalues ////////////
  svtkSmartPointer<svtkDoubleArray> eigenvalues = svtkSmartPointer<svtkDoubleArray>::New();
  pcaStatistics->GetEigenvalues(eigenvalues);
  double eigenvaluesGroundTruth[3] = { .5, .166667, 0 };
  svtkIdType eigenvaluesCount = eigenvalues->GetNumberOfTuples();
  if (eigenvaluesCount > 3)
  {
    return EXIT_FAILURE;
  }
  for (svtkIdType i = 0; i < eigenvaluesCount; i++)
  {
    std::cout << "Eigenvalue " << i << " = " << eigenvalues->GetValue(i) << std::endl;
    if (!fuzzyCompare(eigenvalues->GetValue(i), eigenvaluesGroundTruth[i]))
    {
      std::cerr << "Eigenvalues (GetEigenvalues) are not correct! (" << eigenvalues->GetValue(i)
                << " vs " << eigenvaluesGroundTruth[i] << ")" << std::endl;
      return EXIT_FAILURE;
    }

    if (!fuzzyCompare(pcaStatistics->GetEigenvalue(i), eigenvaluesGroundTruth[i]))
    {
      std::cerr << "Eigenvalues (GetEigenvalue) are not correct! ("
                << pcaStatistics->GetEigenvalue(i) << " vs " << eigenvaluesGroundTruth[i] << ")"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  std::vector<std::vector<double> > eigenvectorsGroundTruth;
  std::vector<double> e0(3);
  e0[0] = -.707107;
  e0[1] = .707107;
  e0[2] = 0;
  std::vector<double> e1(3);
  e1[0] = .707107;
  e1[1] = .707107;
  e1[2] = 0;
  std::vector<double> e2(3);
  e2[0] = 0;
  e2[1] = 0;
  e2[2] = 1;
  eigenvectorsGroundTruth.push_back(e0);
  eigenvectorsGroundTruth.push_back(e1);
  eigenvectorsGroundTruth.push_back(e2);

  svtkSmartPointer<svtkDoubleArray> eigenvectors = svtkSmartPointer<svtkDoubleArray>::New();

  pcaStatistics->GetEigenvectors(eigenvectors);
  for (svtkIdType i = 0; i < eigenvectors->GetNumberOfTuples(); i++)
  {
    std::cout << "Eigenvector " << i << " : ";
    double* evec = new double[eigenvectors->GetNumberOfComponents()];
    eigenvectors->GetTuple(i, evec);
    int iamax = 0;
    double vamax = fabs(eigenvectorsGroundTruth[i][0]);
    for (svtkIdType j = 1; j < eigenvectors->GetNumberOfComponents(); j++)
    {
      double tmp = fabs(eigenvectorsGroundTruth[i][j]);
      if (tmp > vamax)
      {
        iamax = j;
        vamax = tmp;
      }
    }
    double factor = (vamax == eigenvectorsGroundTruth[i][iamax]) ? +1 : -1;
    for (svtkIdType j = 0; j < eigenvectors->GetNumberOfComponents(); j++)
    {
      std::cout << evec[j] << " ";
      svtkSmartPointer<svtkDoubleArray> eigenvectorSingle = svtkSmartPointer<svtkDoubleArray>::New();
      pcaStatistics->GetEigenvector(i, eigenvectorSingle);
      if (!fuzzyCompare(factor * eigenvectorsGroundTruth[i][j], evec[j]) ||
        !fuzzyCompare(factor * eigenvectorsGroundTruth[i][j], eigenvectorSingle->GetValue(j)))
      {
        std::cerr << "Eigenvectors do not match!" << std::endl;
        return EXIT_FAILURE;
      }
    }
    delete[] evec;
    std::cout << std::endl;
  }

  return EXIT_SUCCESS;
}
