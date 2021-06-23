// .SECTION Thanks
// This test was implemented by Philippe Pebay, Kitware SAS 2012

#include "svtkAutoCorrelativeStatistics.h"
#include "svtkDataObjectCollection.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTimerLog.h"
#include "svtkVariantArray.h"

//=============================================================================
int TestAutoCorrelativeStatistics(int, char*[])
{
  int testStatus = 0;

  // ************** Test with 2 columns of input data **************

  // Input data
  double mingledData[] = {
    46,
    45,
    47,
    49,
    46,
    47,
    46,
    46,
    47,
    46,
    47,
    49,
    49,
    49,
    47,
    45,
    50,
    50,
    46,
    46,
    51,
    50,
    48,
    48,
    52,
    54,
    48,
    47,
    52,
    52,
    49,
    49,
    53,
    54,
    50,
    50,
    53,
    54,
    50,
    52,
    53,
    53,
    50,
    51,
    54,
    54,
    49,
    49,
    52,
    52,
    50,
    51,
    52,
    52,
    49,
    47,
    48,
    48,
    48,
    50,
    46,
    48,
    47,
    47,
  };

  // Test with entire data set
  int nVals1 = 32;

  svtkDoubleArray* dataset1Arr = svtkDoubleArray::New();
  dataset1Arr->SetNumberOfComponents(1);
  dataset1Arr->SetName("Metric 0");

  svtkDoubleArray* dataset2Arr = svtkDoubleArray::New();
  dataset2Arr->SetNumberOfComponents(1);
  dataset2Arr->SetName("Metric 1");

  for (int i = 0; i < nVals1; ++i)
  {
    int ti = i << 1;
    dataset1Arr->InsertNextValue(mingledData[ti]);
    dataset2Arr->InsertNextValue(mingledData[ti + 1]);
  }

  // Create input data table
  svtkTable* datasetTable1 = svtkTable::New();
  datasetTable1->AddColumn(dataset1Arr);
  dataset1Arr->Delete();
  datasetTable1->AddColumn(dataset2Arr);
  dataset2Arr->Delete();

  // Create input parameter table for the stationary case
  svtkIdTypeArray* timeLags = svtkIdTypeArray::New();
  timeLags->SetName("Time Lags");
  timeLags->SetNumberOfTuples(1);
  timeLags->SetValue(0, 0);
  svtkTable* paramTable = svtkTable::New();
  paramTable->AddColumn(timeLags);
  timeLags->Delete();

  // Columns of interest
  int nMetrics1 = 2;
  svtkStdString columns1[] = { "Metric 1", "Metric 0" };

  // Reference values
  // Means for metrics 0 and 1 respectively
  double meansXs1[] = { 49.21875, 49.5 };

  // Standard deviations for metrics 0 and 1, respectively
  double varsXs1[] = { 5.9828629, 7.548397 };

  // Set autocorrelative statistics algorithm and its input data port
  svtkAutoCorrelativeStatistics* as1 = svtkAutoCorrelativeStatistics::New();

  // First verify that absence of input does not cause trouble
  cout << "\n## Verifying that absence of input does not cause trouble... ";
  as1->Update();
  cout << "done.\n";

  // Prepare first test with data
  as1->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, datasetTable1);
  datasetTable1->Delete();

  // Select columns of interest
  for (int i = 0; i < nMetrics1; ++i)
  {
    as1->AddColumn(columns1[i]);
  }

  // Set spatial cardinality
  as1->SetSliceCardinality(nVals1);

  // Set parameters for autocorrelation of whole data set with respect to itself
  as1->SetInputData(svtkStatisticsAlgorithm::LEARN_PARAMETERS, paramTable);

  // Test Learn and Derive options
  as1->SetLearnOption(true);
  as1->SetDeriveOption(true);
  as1->SetAssessOption(false);
  as1->SetTestOption(false);
  as1->Update();

  // Get output model tables
  svtkMultiBlockDataSet* outputModelAS1 = svtkMultiBlockDataSet::SafeDownCast(
    as1->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));

  cout << "\n## Calculated the following statistics for first data set:\n";
  for (unsigned b = 0; b < outputModelAS1->GetNumberOfBlocks(); ++b)
  {
    svtkStdString varName = outputModelAS1->GetMetaData(b)->Get(svtkCompositeDataSet::NAME());

    svtkTable* modelTab = svtkTable::SafeDownCast(outputModelAS1->GetBlock(b));
    if (varName == "Autocorrelation FFT")
    {
      if (modelTab->GetNumberOfRows())
      {
        cout << "\n   Autocorrelation FFT:\n";
        modelTab->Dump();
        continue;
      }
    }

    cout << "   Variable=" << varName << "\n";

    cout << "   ";
    for (int i = 0; i < modelTab->GetNumberOfColumns(); ++i)
    {
      cout << modelTab->GetColumnName(i) << "=" << modelTab->GetValue(0, i).ToString() << "  ";
    }

    // Verify some of the calculated statistics
    if (fabs(modelTab->GetValueByName(0, "Mean Xs").ToDouble() - meansXs1[b]) > 1.e-6)
    {
      svtkGenericWarningMacro("Incorrect mean for Xs");
      testStatus = 1;
    }

    if (fabs(modelTab->GetValueByName(0, "Variance Xs").ToDouble() - varsXs1[b]) > 1.e-5)
    {
      svtkGenericWarningMacro("Incorrect variance for Xs");
      testStatus = 1;
    }

    if (fabs(modelTab->GetValueByName(0, "Autocorrelation").ToDouble() - 1.) > 1.e-6)
    {
      svtkGenericWarningMacro("Incorrect autocorrelation");
      testStatus = 1;
    }

    cout << "\n";
  }

  // Test with a slight variation of initial data set (to test model aggregation)
  int nVals2 = 32;

  svtkDoubleArray* dataset4Arr = svtkDoubleArray::New();
  dataset4Arr->SetNumberOfComponents(1);
  dataset4Arr->SetName("Metric 0");

  svtkDoubleArray* dataset5Arr = svtkDoubleArray::New();
  dataset5Arr->SetNumberOfComponents(1);
  dataset5Arr->SetName("Metric 1");

  for (int i = 0; i < nVals2; ++i)
  {
    int ti = i << 1;
    dataset4Arr->InsertNextValue(mingledData[ti] + 1.);
    dataset5Arr->InsertNextValue(mingledData[ti + 1]);
  }

  svtkTable* datasetTable2 = svtkTable::New();
  datasetTable2->AddColumn(dataset4Arr);
  dataset4Arr->Delete();
  datasetTable2->AddColumn(dataset5Arr);
  dataset5Arr->Delete();

  // Set auto-correlative statistics algorithm and its input data port
  svtkAutoCorrelativeStatistics* as2 = svtkAutoCorrelativeStatistics::New();
  as2->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, datasetTable2);

  // Select columns of interest
  for (int i = 0; i < nMetrics1; ++i)
  {
    as2->AddColumn(columns1[i]);
  }

  // Set spatial cardinality
  as2->SetSliceCardinality(nVals2);

  // Set parameters for autocorrelation of whole data set with respect to itself
  as2->SetInputData(svtkStatisticsAlgorithm::LEARN_PARAMETERS, paramTable);

  // Update with Learn option only
  as2->SetLearnOption(true);
  as2->SetDeriveOption(false);
  as2->SetTestOption(false);
  as2->SetAssessOption(false);
  as2->Update();

  // Get output meta tables
  svtkMultiBlockDataSet* outputModelAS2 = svtkMultiBlockDataSet::SafeDownCast(
    as2->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));

  cout << "\n## Calculated the following statistics for second data set:\n";
  for (unsigned b = 0; b < outputModelAS2->GetNumberOfBlocks(); ++b)
  {
    svtkStdString varName = outputModelAS2->GetMetaData(b)->Get(svtkCompositeDataSet::NAME());

    svtkTable* modelTab = svtkTable::SafeDownCast(outputModelAS2->GetBlock(b));
    if (varName == "Autocorrelation FFT")
    {
      if (modelTab->GetNumberOfRows())
      {
        cout << "\n   Autocorrelation FFT:\n";
        modelTab->Dump();
        continue;
      }
    }

    cout << "\n   Variable=" << varName << "\n";

    cout << "   ";
    for (int i = 0; i < modelTab->GetNumberOfColumns(); ++i)
    {
      cout << modelTab->GetColumnName(i) << "=" << modelTab->GetValue(0, i).ToString() << "  ";
    }

    cout << "\n";
  }

  // Test model aggregation by adding new data to engine which already has a model
  as1->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, datasetTable2);
  svtkMultiBlockDataSet* model = svtkMultiBlockDataSet::New();
  model->ShallowCopy(outputModelAS1);
  as1->SetInputData(svtkStatisticsAlgorithm::INPUT_MODEL, model);

  // Clean up
  model->Delete();
  datasetTable2->Delete();
  as2->Delete();

  // Update with Learn and Derive options only
  as1->SetLearnOption(true);
  as1->SetDeriveOption(true);
  as1->SetTestOption(false);
  as1->SetAssessOption(false);
  as1->Update();

  // Updated reference values
  // Means and variances for metrics 0 and 1, respectively
  double meansXs0[] = { 49.71875, 49.5 };
  double varsXs0[] = { 6.1418651, 7.548397 * 62. / 63. };

  // Get output meta tables
  outputModelAS1 = svtkMultiBlockDataSet::SafeDownCast(
    as1->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));

  cout << "\n## Calculated the following statistics for aggregated (first + second) data set:\n";
  for (unsigned b = 0; b < outputModelAS1->GetNumberOfBlocks(); ++b)
  {
    svtkStdString varName = outputModelAS1->GetMetaData(b)->Get(svtkCompositeDataSet::NAME());

    svtkTable* modelTab = svtkTable::SafeDownCast(outputModelAS1->GetBlock(b));
    if (varName == "Autocorrelation FFT")
    {
      if (modelTab->GetNumberOfRows())
      {
        cout << "\n   Autocorrelation FFT:\n";
        modelTab->Dump();
        continue;
      }
    }

    cout << "\n   Variable=" << varName << "\n";

    cout << "   ";
    for (int i = 0; i < modelTab->GetNumberOfColumns(); ++i)
    {
      cout << modelTab->GetColumnName(i) << "=" << modelTab->GetValue(0, i).ToString() << "  ";
    }

    // Verify some of the calculated statistics
    if (fabs(modelTab->GetValueByName(0, "Mean Xs").ToDouble() - meansXs0[b]) > 1.e-6)
    {
      svtkGenericWarningMacro("Incorrect mean for Xs");
      testStatus = 1;
    }

    if (fabs(modelTab->GetValueByName(0, "Variance Xs").ToDouble() - varsXs0[b]) > 1.e-5)
    {
      svtkGenericWarningMacro("Incorrect variance for Xs");
      testStatus = 1;
    }

    cout << "\n";
  }

  // Clean up
  as1->Delete();

  // ************** Test with 2 columns of synthetic data **************

  // Space and time parameters
  svtkIdType nSteps = 2;
  svtkIdType cardSlice = 1000;
  svtkIdType cardTotal = nSteps * cardSlice;

  // Expand parameter table to contain all steps
  svtkVariantArray* row = svtkVariantArray::New();
  row->SetNumberOfValues(1);
  for (svtkIdType p = 1; p < nSteps; ++p)
  {
    row->SetValue(0, p);
    paramTable->InsertNextRow(row);
  }
  row->Delete();

  svtkDoubleArray* lineArr = svtkDoubleArray::New();
  lineArr->SetNumberOfComponents(1);
  lineArr->SetName("Line");

  svtkDoubleArray* vArr = svtkDoubleArray::New();
  vArr->SetNumberOfComponents(1);
  vArr->SetName("V");

  svtkDoubleArray* circleArr = svtkDoubleArray::New();
  circleArr->SetNumberOfComponents(1);
  circleArr->SetName("Circle");

  // Fill data columns
  svtkIdType midPoint = cardTotal >> 1;
  double dAlpha = (2.0 * svtkMath::Pi()) / cardSlice;
  for (int i = 0; i < cardTotal; ++i)
  {
    lineArr->InsertNextValue(i);
    if (i < midPoint)
    {
      vArr->InsertNextValue(cardTotal - i);
      circleArr->InsertNextValue(cos(i * dAlpha));
    }
    else
    {
      vArr->InsertNextValue(i);
      circleArr->InsertNextValue(sin(i * dAlpha));
    }
  }

  // Create input data table
  svtkTable* datasetTable3 = svtkTable::New();
  datasetTable3->AddColumn(lineArr);
  lineArr->Delete();
  datasetTable3->AddColumn(vArr);
  vArr->Delete();
  datasetTable3->AddColumn(circleArr);
  circleArr->Delete();

  // Columns of interest
  int nMetrics2 = 3;
  svtkStdString columns2[] = { "Line", "V", "Circle" };

  // Reference values
  double halfNm1 = .5 * (cardSlice - 1);

  // Means of Xs for circle, line, and v-shaped variables respectively
  double meansXt3[] = { 0., 0., halfNm1, halfNm1 + cardSlice, cardTotal - halfNm1,
    cardTotal - halfNm1 - 1. };

  // Autocorrelation values for circle, line, and v-shaped variables respectively
  double autocorr3[] = { 1., 0., 1., 1., 1., -1. };

  // Prepare autocorrelative statistics algorithm and its input data port
  svtkAutoCorrelativeStatistics* as3 = svtkAutoCorrelativeStatistics::New();
  as3->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, datasetTable3);
  datasetTable3->Delete();

  // Select Columns of Interest
  for (int i = 0; i < nMetrics2; ++i)
  {
    as3->AddColumn(columns2[i]);
  }

  // Set spatial cardinality
  as3->SetSliceCardinality(cardSlice);

  // Set autocorrelation parameters for first slice against slice following midpoint
  as3->SetInputData(svtkStatisticsAlgorithm::LEARN_PARAMETERS, paramTable);

  // Test Learn, and Derive options
  as3->SetLearnOption(true);
  as3->SetDeriveOption(true);
  as3->SetAssessOption(false);
  as3->SetTestOption(false);
  as3->Update();

  // Get output data and meta tables
  svtkMultiBlockDataSet* outputModelAS3 = svtkMultiBlockDataSet::SafeDownCast(
    as3->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));

  cout << "\n## Calculated the following statistics for third data set:\n";
  for (unsigned b = 0; b < outputModelAS3->GetNumberOfBlocks(); ++b)
  {
    svtkStdString varName = outputModelAS3->GetMetaData(b)->Get(svtkCompositeDataSet::NAME());

    svtkTable* modelTab = svtkTable::SafeDownCast(outputModelAS3->GetBlock(b));
    if (varName == "Autocorrelation FFT")
    {
      if (modelTab->GetNumberOfRows())
      {
        cout << "\n   Autocorrelation FFT:\n";
        modelTab->Dump();
        continue;
      }
    }

    cout << "\n   Variable=" << varName << "\n";

    for (int r = 0; r < modelTab->GetNumberOfRows(); ++r)
    {
      cout << "   ";
      for (int i = 0; i < modelTab->GetNumberOfColumns(); ++i)
      {
        cout << modelTab->GetColumnName(i) << "=" << modelTab->GetValue(r, i).ToString() << "  ";
      }

      // Verify some of the calculated statistics
      int idx = nSteps * b + r;
      if (fabs(modelTab->GetValueByName(r, "Mean Xt").ToDouble() - meansXt3[idx]) > 1.e-6)
      {
        svtkGenericWarningMacro("Incorrect mean for Xt");
        testStatus = 1;
      }

      if (fabs(modelTab->GetValueByName(r, "Autocorrelation").ToDouble() - autocorr3[idx]) > 1.e-6)
      {
        svtkGenericWarningMacro("Incorrect autocorrelation " << autocorr3[idx]);
        testStatus = 1;
      }

      cout << "\n";
    } // i
  }   // r

  // Clean up
  as3->Delete();
  paramTable->Delete();

  return testStatus;
}
