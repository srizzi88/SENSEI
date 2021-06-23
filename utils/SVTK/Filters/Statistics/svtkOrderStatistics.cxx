/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOrderStatistics.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
#include "svtkToolkits.h"

#include "svtkOrderStatistics.h"
#include "svtkStatisticsAlgorithmPrivate.h"

#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>

svtkStandardNewMacro(svtkOrderStatistics);

// ----------------------------------------------------------------------
svtkOrderStatistics::svtkOrderStatistics()
{
  this->QuantileDefinition = svtkOrderStatistics::InverseCDFAveragedSteps;
  this->NumberOfIntervals = 4;       // By default, calculate 5-points statistics
  this->Quantize = false;            // By default, do not force quantization
  this->MaximumHistogramSize = 1000; // A large value by default
  // Number of primary tables is variable
  this->NumberOfPrimaryTables = -1;

  this->AssessNames->SetNumberOfValues(1);
  this->AssessNames->SetValue(0, "Quantile");
}

// ----------------------------------------------------------------------
svtkOrderStatistics::~svtkOrderStatistics() = default;

// ----------------------------------------------------------------------
void svtkOrderStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfIntervals: " << this->NumberOfIntervals << endl;
  os << indent << "QuantileDefinition: " << this->QuantileDefinition << endl;
  os << indent << "Quantize: " << this->Quantize << endl;
  os << indent << "MaximumHistogramSize: " << this->MaximumHistogramSize << endl;
}

// ----------------------------------------------------------------------
void svtkOrderStatistics::SetQuantileDefinition(int qd)
{
  switch (qd)
  {
    case svtkOrderStatistics::InverseCDF:
      break;
    case svtkOrderStatistics::InverseCDFAveragedSteps:
      break;
    default:
      svtkWarningMacro("Incorrect type of quantile definition: " << qd << ". Ignoring it.");
      return;
  }

  this->QuantileDefinition = static_cast<svtkOrderStatistics::QuantileDefinitionType>(qd);
  this->Modified();
}

// ----------------------------------------------------------------------
bool svtkOrderStatistics::SetParameter(
  const char* parameter, int svtkNotUsed(index), svtkVariant value)
{
  if (!strcmp(parameter, "NumberOfIntervals"))
  {
    this->SetNumberOfIntervals(value.ToInt());

    return true;
  }

  if (!strcmp(parameter, "QuantileDefinition"))
  {
    this->SetQuantileDefinition(value.ToInt());

    return true;
  }

  return false;
}

// ----------------------------------------------------------------------
void svtkOrderStatistics::Learn(
  svtkTable* inData, svtkTable* svtkNotUsed(inParameters), svtkMultiBlockDataSet* outMeta)
{
  if (!inData)
  {
    return;
  }

  if (!outMeta)
  {
    return;
  }

  // Loop over requests
  svtkIdType nRow = inData->GetNumberOfRows();
  for (std::set<std::set<svtkStdString> >::iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    // Each request contains only one column of interest (if there are others, they are ignored)
    std::set<svtkStdString>::const_iterator it = rit->begin();
    svtkStdString col = *it;
    if (!inData->GetColumnByName(col))
    {
      svtkWarningMacro("InData table does not have a column " << col.c_str() << ". Ignoring it.");
      continue;
    }

    // Get hold of data for this variable
    svtkAbstractArray* vals = inData->GetColumnByName(col);

    // Create histogram table for this variable
    svtkTable* histogramTab = svtkTable::New();

    // Row to be used to insert into histogram table
    svtkVariantArray* row = svtkVariantArray::New();
    row->SetNumberOfValues(2);

    // Switch depending on data type
    if (vals->IsA("svtkDataArray"))
    {
      svtkDoubleArray* doubleCol = svtkDoubleArray::New();
      doubleCol->SetName("Value");
      histogramTab->AddColumn(doubleCol);
      doubleCol->Delete();
    }
    else if (vals->IsA("svtkStringArray"))
    {
      svtkStringArray* stringCol = svtkStringArray::New();
      stringCol->SetName("Value");
      histogramTab->AddColumn(stringCol);
      stringCol->Delete();
    }
    else if (vals->IsA("svtkVariantArray"))
    {
      svtkVariantArray* variantCol = svtkVariantArray::New();
      variantCol->SetName("Value");
      histogramTab->AddColumn(variantCol);
      variantCol->Delete();
    }
    else
    {
      svtkWarningMacro("Unsupported data type for column " << col.c_str() << ". Ignoring it.");

      continue;
    }

    svtkIdTypeArray* idTypeCol = svtkIdTypeArray::New();
    idTypeCol->SetName("Cardinality");
    histogramTab->AddColumn(idTypeCol);
    idTypeCol->Delete();

    // Switch depending on data type
    if (vals->IsA("svtkDataArray"))
    {
      // Downcast column to data array for efficient data access
      svtkDataArray* dvals = svtkArrayDownCast<svtkDataArray>(vals);

      // Calculate histogram
      std::map<double, svtkIdType> histogram;
      for (svtkIdType r = 0; r < nRow; ++r)
      {
        ++histogram[dvals->GetTuple1(r)];
      }

      // If maximum size was requested, make sure it is satisfied
      if (this->Quantize)
      {
        // Retrieve achieved histogram size
        svtkIdType Nq = static_cast<svtkIdType>(histogram.size());

        // If histogram is too big, quantization will have to occur
        while (Nq > this->MaximumHistogramSize)
        {
          // Retrieve extremal values
          double mini = histogram.begin()->first;
          double maxi = histogram.rbegin()->first;

          // Create bucket width based on target histogram size
          // FIXME: .5 is arbitrary at this point
          double width = (maxi - mini) / std::round(Nq / 2.);

          // Now re-calculate histogram by quantizing values
          histogram.clear();
          double reading;
          double quantum;
          for (svtkIdType r = 0; r < nRow; ++r)
          {
            reading = dvals->GetTuple1(r);
            quantum = mini + std::round((reading - mini) / width) * width;
            ++histogram[quantum];
          }

          // Update histogram size for conditional clause
          Nq = static_cast<svtkIdType>(histogram.size());
        }
      }

      // Store histogram
      for (std::map<double, svtkIdType>::iterator mit = histogram.begin(); mit != histogram.end();
           ++mit)
      {
        row->SetValue(0, mit->first);
        row->SetValue(1, mit->second);
        histogramTab->InsertNextRow(row);
      }
    } // if ( vals->IsA("svtkDataArray") )
    else if (vals->IsA("svtkStringArray"))
    {
      // Downcast column to string array for efficient data access
      svtkStringArray* svals = svtkArrayDownCast<svtkStringArray>(vals);

      // Calculate histogram
      std::map<svtkStdString, svtkIdType> histogram;
      for (svtkIdType r = 0; r < nRow; ++r)
      {
        ++histogram[svals->GetValue(r)];
      }

      // Store histogram
      for (std::map<svtkStdString, svtkIdType>::iterator mit = histogram.begin();
           mit != histogram.end(); ++mit)
      {
        row->SetValue(0, mit->first);
        row->SetValue(1, mit->second);
        histogramTab->InsertNextRow(row);
      }
    } // else if ( vals->IsA("svtkStringArray") )
    else if (vals->IsA("svtkVariantArray"))
    {
      // Downcast column to variant array for efficient data access
      svtkVariantArray* vvals = svtkArrayDownCast<svtkVariantArray>(vals);

      // Calculate histogram
      std::map<svtkVariant, svtkIdType> histogram;
      for (svtkIdType r = 0; r < nRow; ++r)
      {
        ++histogram[vvals->GetVariantValue(r)];
      }

      // Store histogram
      for (std::map<svtkVariant, svtkIdType>::iterator mit = histogram.begin();
           mit != histogram.end(); ++mit)
      {
        row->SetValue(0, mit->first);
        row->SetValue(1, mit->second);
        histogramTab->InsertNextRow(row);
      }
    } // else if ( vals->IsA("svtkVariantArray") )
    else
    {
      svtkWarningMacro("Unsupported data type for column " << col.c_str() << ". Ignoring it.");

      continue;
    } // else

    // Resize output meta so histogram table can be appended
    unsigned int nBlocks = outMeta->GetNumberOfBlocks();
    outMeta->SetNumberOfBlocks(nBlocks + 1);
    outMeta->GetMetaData(nBlocks)->Set(svtkCompositeDataSet::NAME(), col);
    outMeta->SetBlock(nBlocks, histogramTab);

    // Clean up
    histogramTab->Delete();
    row->Delete();
  } // rit
}

// ----------------------------------------------------------------------
void svtkOrderStatistics::Derive(svtkMultiBlockDataSet* inMeta)
{
  if (!inMeta || inMeta->GetNumberOfBlocks() < 1)
  {
    return;
  }

  // Create cardinality table
  svtkTable* cardinalityTab = svtkTable::New();

  svtkStringArray* stringCol = svtkStringArray::New();
  stringCol->SetName("Variable");
  cardinalityTab->AddColumn(stringCol);
  stringCol->Delete();

  svtkIdTypeArray* idTypeCol = svtkIdTypeArray::New();
  idTypeCol->SetName("Cardinality");
  cardinalityTab->AddColumn(idTypeCol);
  idTypeCol->Delete();

  // Create quantile table
  svtkTable* quantileTab = svtkTable::New();

  stringCol = svtkStringArray::New();
  stringCol->SetName("Quantile");
  quantileTab->AddColumn(stringCol);
  stringCol->Delete();

  double dq = 1. / static_cast<double>(this->NumberOfIntervals);
  for (svtkIdType i = 0; i <= this->NumberOfIntervals; ++i)
  {

    // Handle special case of quartiles and median for convenience
    ldiv_t q = ldiv(i << 2, this->NumberOfIntervals);
    if (q.rem)
    {
      // General case
      stringCol->InsertNextValue(svtkStdString(svtkVariant(i * dq).ToString() + "-quantile").c_str());
    }
    else
    {
      // Case where q is a multiple of 4
      switch (q.quot)
      {
        case 0:
          stringCol->InsertNextValue("Minimum");
          break;
        case 1:
          stringCol->InsertNextValue("First Quartile");
          break;
        case 2:
          stringCol->InsertNextValue("Median");
          break;
        case 3:
          stringCol->InsertNextValue("Third Quartile");
          break;
        case 4:
          stringCol->InsertNextValue("Maximum");
          break;
        default:
          stringCol->InsertNextValue(
            svtkStdString(svtkVariant(i * dq).ToString() + "-quantile").c_str());
          break;
      }
    }
  }

  // Prepare row for insertion into cardinality table
  svtkVariantArray* row = svtkVariantArray::New();
  row->SetNumberOfValues(2);

  // Iterate over primary tables
  unsigned int nBlocks = inMeta->GetNumberOfBlocks();
  for (unsigned int b = 0; b < nBlocks; ++b)
  {
    svtkTable* histogramTab = svtkTable::SafeDownCast(inMeta->GetBlock(b));
    if (!histogramTab)
    {
      continue;
    }

    // Downcast columns to typed arrays for efficient data access
    svtkAbstractArray* vals = histogramTab->GetColumnByName("Value");
    svtkIdTypeArray* card =
      svtkArrayDownCast<svtkIdTypeArray>(histogramTab->GetColumnByName("Cardinality"));

    // The CDF will be used for quantiles calculation (effectively as a reverse look-up table)
    svtkIdType nRowHist = histogramTab->GetNumberOfRows();
    std::vector<svtkIdType> cdf(nRowHist);

    // Calculate variable cardinality and CDF
    svtkIdType c;
    svtkIdType n = 0;
    for (svtkIdType r = 0; r < nRowHist; ++r)
    {
      // Update cardinality and CDF
      c = card->GetValue(r);
      n += c;
      cdf[r] = n;
    }

    // Get block variable name
    svtkStdString varName = inMeta->GetMetaData(b)->Get(svtkCompositeDataSet::NAME());

    // Store cardinality
    row->SetValue(0, varName);
    row->SetValue(1, n);
    cardinalityTab->InsertNextRow(row);

    // Find or create column of probability mass function of histogram table
    svtkStdString probaName("P");
    svtkDoubleArray* probaCol;
    svtkAbstractArray* abstrCol = histogramTab->GetColumnByName(probaName);
    if (!abstrCol)
    {
      probaCol = svtkDoubleArray::New();
      probaCol->SetName(probaName);
      probaCol->SetNumberOfTuples(nRowHist);
      histogramTab->AddColumn(probaCol);
      probaCol->Delete();
    }
    else
    {
      probaCol = svtkArrayDownCast<svtkDoubleArray>(abstrCol);
    }

    // Finally calculate and store probabilities
    double inv_n = 1. / n;
    double p;
    for (svtkIdType r = 0; r < nRowHist; ++r)
    {
      c = card->GetValue(r);
      p = inv_n * c;

      probaCol->SetValue(r, p);
    }

    // Storage for quantile indices
    std::vector<std::pair<svtkIdType, svtkIdType> > quantileIndices;
    std::pair<svtkIdType, svtkIdType> qIdxPair;

    // First quantile index is always 0 with no jump (corresponding to the first and the smallest
    // value)
    qIdxPair.first = 0;
    qIdxPair.second = 0;
    quantileIndices.push_back(qIdxPair);

    // Calculate all interior quantiles (i.e. for 0 < k < q)
    svtkIdType rank = 0;
    double dh = n / static_cast<double>(this->NumberOfIntervals);
    for (svtkIdType k = 1; k < this->NumberOfIntervals; ++k)
    {
      // Calculate np value
      double np = k * dh;

      // Calculate first quantile index
      svtkIdType qIdx1;
      if (this->QuantileDefinition == svtkOrderStatistics::InverseCDFAveragedSteps)
      {
        qIdx1 = static_cast<svtkIdType>(std::round(np));
      }
      else
      {
        qIdx1 = static_cast<svtkIdType>(ceil(np));
      }

      // Find rank of the entry where first quantile index is reached using the CDF
      while (qIdx1 > cdf[rank])
      {
        ++rank;

        if (rank >= nRowHist)
        {
          svtkErrorMacro("Inconsistent quantile table: at last rank "
            << rank << " the CDF is  " << cdf[rank - 1] << " < " << qIdx1
            << " the quantile index. Cannot derive model.");
          return;
        }
      }

      // Store rank in histogram of first quantile index
      qIdxPair.first = rank;

      // Decide whether midpoint interpolation will be used for this numeric type input
      if (this->QuantileDefinition == svtkOrderStatistics::InverseCDFAveragedSteps)
      {
        // Calculate second quantile index for mid-point interpolation
        svtkIdType qIdx2 = static_cast<svtkIdType>(floor(np + 1.));

        // If the two quantile indices differ find rank where second is reached
        if (qIdx1 != qIdx2)
        {
          while (qIdx2 > cdf[rank])
          {
            ++rank;

            if (rank >= nRowHist)
            {
              svtkErrorMacro("Inconsistent quantile table: at last rank "
                << rank << " the CDF is  " << cdf[rank - 1] << " < " << qIdx2
                << " the quantile index. Cannot derive model.");
              return;
            }
          }
        }
      } // if ( this->QuantileDefinition == svtkOrderStatistics::InverseCDFAveragedSteps )

      // Store rank in histogram of second quantile index
      qIdxPair.second = rank;

      // Store pair of ranks
      quantileIndices.push_back(qIdxPair);
    }

    // Last quantile index is always cardinality with no jump (corresponding to the last and thus
    // largest value)
    qIdxPair.first = nRowHist - 1;
    qIdxPair.second = nRowHist - 1;
    quantileIndices.push_back(qIdxPair);

    // Finally prepare quantile values column depending on data type
    if (vals->IsA("svtkDataArray"))
    {
      // Downcast column to data array for efficient data access
      svtkDataArray* dvals = svtkArrayDownCast<svtkDataArray>(vals);

      // Create column for quantiles of the same type as the values
      svtkDataArray* quantCol = svtkDataArray::CreateDataArray(dvals->GetDataType());
      quantCol->SetName(varName);
      quantCol->SetNumberOfTuples(this->NumberOfIntervals + 1);
      quantileTab->AddColumn(quantCol);
      quantCol->Delete();

      // Decide whether midpoint interpolation will be used for this numeric type input
      if (this->QuantileDefinition == svtkOrderStatistics::InverseCDFAveragedSteps)
      {
        // Compute and store quantile values
        svtkIdType k = 0;
        for (std::vector<std::pair<svtkIdType, svtkIdType> >::iterator qit = quantileIndices.begin();
             qit != quantileIndices.end(); ++qit, ++k)
        {
          // Retrieve data values from rank into histogram and interpolate
          double Qp = .5 * (dvals->GetTuple1(qit->first) + dvals->GetTuple1(qit->second));

          // Store quantile value
          quantCol->SetTuple1(k, Qp);
        } // qit
      }
      else // if ( this->QuantileDefinition == svtkOrderStatistics::InverseCDFAveragedSteps )
      {
        // Compute and store quantile values
        svtkIdType k = 0;
        for (std::vector<std::pair<svtkIdType, svtkIdType> >::iterator qit = quantileIndices.begin();
             qit != quantileIndices.end(); ++qit, ++k)
        {
          // Retrieve data value from rank into histogram
          double Qp = dvals->GetTuple1(qit->first);

          // Store quantile value
          quantCol->SetTuple1(k, Qp);
        } // qit
      }   // else ( this->QuantileDefinition == svtkOrderStatistics::InverseCDFAveragedSteps )
    }     // if ( vals->IsA("svtkDataArray") )
    else if (vals->IsA("svtkStringArray"))
    {
      // Downcast column to string array for efficient data access
      svtkStringArray* svals = svtkArrayDownCast<svtkStringArray>(vals);

      // Create column for quantiles of the same type as the values
      svtkStringArray* quantCol = svtkStringArray::New();
      quantCol->SetName(varName);
      quantCol->SetNumberOfTuples(this->NumberOfIntervals + 1);
      quantileTab->AddColumn(quantCol);
      quantCol->Delete();

      // Compute and store quantile values
      svtkIdType k = 0;
      for (std::vector<std::pair<svtkIdType, svtkIdType> >::iterator qit = quantileIndices.begin();
           qit != quantileIndices.end(); ++qit, ++k)
      {
        // Retrieve data value from rank into histogram
        svtkStdString Qp = svals->GetValue(qit->first);

        // Store quantile value
        quantCol->SetValue(k, Qp);
      }
    } // else if ( vals->IsA("svtkStringArray") )
    else if (vals->IsA("svtkVariantArray"))
    {
      // Downcast column to variant array for efficient data access
      svtkVariantArray* vvals = svtkArrayDownCast<svtkVariantArray>(vals);

      // Create column for quantiles of the same type as the values
      svtkVariantArray* quantCol = svtkVariantArray::New();
      quantCol->SetName(varName);
      quantCol->SetNumberOfTuples(this->NumberOfIntervals + 1);
      quantileTab->AddColumn(quantCol);
      quantCol->Delete();

      // Compute and store quantile values
      svtkIdType k = 0;
      for (std::vector<std::pair<svtkIdType, svtkIdType> >::iterator qit = quantileIndices.begin();
           qit != quantileIndices.end(); ++qit, ++k)
      {
        // Retrieve data value from rank into histogram
        svtkVariant Qp = vvals->GetValue(qit->first);

        // Store quantile value
        quantCol->SetValue(k, Qp);
      }
    } // else if ( vals->IsA("svtkVariantArray") )
    else
    {
      svtkWarningMacro("Unsupported data type for column "
        << varName.c_str() << ". Cannot calculate quantiles for it.");

      continue;
    } // else

  } // for ( unsigned int b = 0; b < nBlocks; ++ b )

  // Resize output meta so cardinality and quantile tables can be appended
  nBlocks = inMeta->GetNumberOfBlocks();
  inMeta->SetNumberOfBlocks(nBlocks + 2);

  // Append cardinality table at block nBlocks
  inMeta->GetMetaData(nBlocks)->Set(svtkCompositeDataSet::NAME(), "Cardinalities");
  inMeta->SetBlock(nBlocks, cardinalityTab);

  // Increment number of blocks and append quantile table at the end
  ++nBlocks;
  inMeta->GetMetaData(nBlocks)->Set(svtkCompositeDataSet::NAME(), "Quantiles");
  inMeta->SetBlock(nBlocks, quantileTab);

  // Clean up
  row->Delete();
  cardinalityTab->Delete();
  quantileTab->Delete();
}

// ----------------------------------------------------------------------
void svtkOrderStatistics::Test(svtkTable* inData, svtkMultiBlockDataSet* inMeta, svtkTable* outMeta)
{
  if (!inMeta)
  {
    return;
  }

  unsigned nBlocks = inMeta->GetNumberOfBlocks();
  if (nBlocks < 1)
  {
    return;
  }

  svtkTable* quantileTab = svtkTable::SafeDownCast(inMeta->GetBlock(nBlocks - 1));
  if (!quantileTab ||
    inMeta->GetMetaData(nBlocks - 1)->Get(svtkCompositeDataSet::NAME()) != svtkStdString("Quantiles"))
  {
    return;
  }

  if (!outMeta)
  {
    return;
  }

  // Prepare columns for the test:
  // 0: variable name
  // 1: Maximum vertical distance between CDFs
  // 2: Kolmogorov-Smirnov test statistic (the above times the square root of the cardinality)
  // NB: These are not added to the output table yet, for they will be filled individually first
  //     in order that R be invoked only once.
  svtkStringArray* nameCol = svtkStringArray::New();
  nameCol->SetName("Variable");

  svtkDoubleArray* distCol = svtkDoubleArray::New();
  distCol->SetName("Maximum Distance");

  svtkDoubleArray* statCol = svtkDoubleArray::New();
  statCol->SetName("Kolmogorov-Smirnov");

  // Prepare storage for quantiles and model CDFs
  svtkIdType nQuant = quantileTab->GetNumberOfRows();
  std::vector<svtkStdString> quantiles(nQuant);

  // Loop over requests
  svtkIdType nRowData = inData->GetNumberOfRows();
  double inv_nq = 1. / nQuant;
  double inv_card = 1. / nRowData;
  double sqrt_card = sqrt(static_cast<double>(nRowData));
  for (std::set<std::set<svtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    // Each request contains only one column of interest (if there are others, they are ignored)
    std::set<svtkStdString>::const_iterator it = rit->begin();
    svtkStdString varName = *it;
    if (!inData->GetColumnByName(varName))
    {
      svtkWarningMacro(
        "InData table does not have a column " << varName.c_str() << ". Ignoring it.");
      continue;
    }

    // Find the quantile column that corresponds to the variable of the request
    svtkAbstractArray* quantCol = quantileTab->GetColumnByName(varName);
    if (!quantCol)
    {
      svtkWarningMacro(
        "Quantile table table does not have a column " << varName.c_str() << ". Ignoring it.");
      continue;
    }

    // First iterate over all observations to calculate empirical PDF
    typedef std::map<svtkStdString, double> CDF;
    CDF cdfEmpirical;
    for (svtkIdType j = 0; j < nRowData; ++j)
    {
      // Read observation and update PDF
      cdfEmpirical[inData->GetValueByName(j, varName).ToString()] += inv_card;
    }

    // Now integrate to obtain empirical CDF
    double sum = 0.;
    for (CDF::iterator cit = cdfEmpirical.begin(); cit != cdfEmpirical.end(); ++cit)
    {
      sum += cit->second;
      cit->second = sum;
    }

    // Sanity check: verify that empirical CDF = 1
    if (fabs(sum - 1.) > 1.e-6)
    {
      svtkWarningMacro(
        "Incorrect empirical CDF for variable:" << varName.c_str() << ". Ignoring it.");

      continue;
    }

    // Retrieve quantiles to calculate model CDF and insert value into empirical CDF
    for (svtkIdType i = 0; i < nQuant; ++i)
    {
      // Read quantile and update CDF
      quantiles[i] = quantileTab->GetValueByName(i, varName).ToString();

      // Update empirical CDF if new value found (with unknown ECDF)
      std::pair<CDF::iterator, bool> result =
        cdfEmpirical.insert(std::pair<svtkStdString, double>(quantiles[i], -1));
      if (result.second == true)
      {
        CDF::iterator eit = result.first;
        // Check if new value has no predecessor, in which case CDF = 0
        if (eit == cdfEmpirical.begin())
        {
          result.first->second = 0.;
        }
        else
        {
          --eit;
          result.first->second = eit->second;
        }
      }
    }

    // Iterate over all CDF jump values
    int currentQ = 0;
    double mcdf = 0.;
    double Dmn = 0.;
    for (CDF::iterator cit = cdfEmpirical.begin(); cit != cdfEmpirical.end(); ++cit)
    {
      // If observation is smaller than minimum then there is nothing to do
      if (cit->first >= quantiles[0])
      {
        while (currentQ < nQuant && cit->first >= quantiles[currentQ])
        {
          ++currentQ;
        }

        // Calculate model CDF at observation
        mcdf = currentQ * inv_nq;
      }

      // Calculate vertical distance between CDFs and update maximum if needed
      double d = fabs(cit->second - mcdf);
      if (d > Dmn)
      {
        Dmn = d;
      }
    }

    // Insert variable name and calculated Kolmogorov-Smirnov statistic
    // NB: R will be invoked only once at the end for efficiency
    nameCol->InsertNextValue(varName);
    distCol->InsertNextTuple1(Dmn);
    statCol->InsertNextTuple1(sqrt_card * Dmn);
  } // rit

  // Now, add the already prepared columns to the output table
  outMeta->AddColumn(nameCol);
  outMeta->AddColumn(distCol);
  outMeta->AddColumn(statCol);

  // Clean up
  nameCol->Delete();
  distCol->Delete();
  statCol->Delete();
}

// ----------------------------------------------------------------------
class DataArrayQuantizer : public svtkStatisticsAlgorithm::AssessFunctor
{
public:
  svtkDataArray* Data;
  svtkDataArray* Quantiles;

  DataArrayQuantizer(svtkAbstractArray* vals, svtkAbstractArray* quantiles)
  {
    this->Data = svtkArrayDownCast<svtkDataArray>(vals);
    this->Quantiles = svtkArrayDownCast<svtkDataArray>(quantiles);
  }
  ~DataArrayQuantizer() override = default;
  void operator()(svtkDoubleArray* result, svtkIdType id) override
  {
    result->SetNumberOfValues(1);

    double dval = this->Data->GetTuple1(id);
    if (dval < this->Quantiles->GetTuple1(0))
    {
      // dval is smaller than lower bound
      result->SetValue(0, 0);

      return;
    }

    svtkIdType q = 1;
    svtkIdType n = this->Quantiles->GetNumberOfTuples();
    while (q < n && dval > this->Quantiles->GetTuple1(q))
    {
      ++q;
    }

    result->SetValue(0, q);
  }
};

// ----------------------------------------------------------------------
class StringArrayQuantizer : public svtkStatisticsAlgorithm::AssessFunctor
{
public:
  svtkStringArray* Data;
  svtkStringArray* Quantiles;

  StringArrayQuantizer(svtkAbstractArray* vals, svtkAbstractArray* quantiles)
  {
    this->Data = svtkArrayDownCast<svtkStringArray>(vals);
    this->Quantiles = svtkArrayDownCast<svtkStringArray>(quantiles);
  }
  ~StringArrayQuantizer() override = default;
  void operator()(svtkDoubleArray* result, svtkIdType id) override
  {
    result->SetNumberOfValues(1);

    svtkStdString sval = this->Data->GetValue(id);
    if (sval < this->Quantiles->GetValue(0))
    {
      // sval is smaller than lower bound
      result->SetValue(0, 0);

      return;
    }

    svtkIdType q = 1;
    svtkIdType n = this->Quantiles->GetNumberOfValues();
    while (q < n && sval > this->Quantiles->GetValue(q))
    {
      ++q;
    }

    result->SetValue(0, q);
  }
};

// ----------------------------------------------------------------------
class VariantArrayQuantizer : public svtkStatisticsAlgorithm::AssessFunctor
{
public:
  svtkVariantArray* Data;
  svtkVariantArray* Quantiles;

  VariantArrayQuantizer(svtkAbstractArray* vals, svtkAbstractArray* quantiles)
  {
    this->Data = svtkArrayDownCast<svtkVariantArray>(vals);
    this->Quantiles = svtkArrayDownCast<svtkVariantArray>(quantiles);
  }
  ~VariantArrayQuantizer() override = default;
  void operator()(svtkDoubleArray* result, svtkIdType id) override
  {
    result->SetNumberOfValues(1);

    svtkVariant vval = this->Data->GetValue(id);
    if (vval < this->Quantiles->GetValue(0))
    {
      // vval is smaller than lower bound
      result->SetValue(0, 0);

      return;
    }

    svtkIdType q = 1;
    svtkIdType n = this->Quantiles->GetNumberOfValues();
    while (q < n && vval > this->Quantiles->GetValue(q))
    {
      ++q;
    }

    result->SetValue(0, q);
  }
};

// ----------------------------------------------------------------------
void svtkOrderStatistics::SelectAssessFunctor(
  svtkTable* outData, svtkDataObject* inMetaDO, svtkStringArray* rowNames, AssessFunctor*& dfunc)
{
  dfunc = nullptr;
  svtkMultiBlockDataSet* inMeta = svtkMultiBlockDataSet::SafeDownCast(inMetaDO);
  if (!inMeta)
  {
    return;
  }

  unsigned nBlocks = inMeta->GetNumberOfBlocks();
  if (nBlocks < 1)
  {
    return;
  }

  svtkTable* quantileTab = svtkTable::SafeDownCast(inMeta->GetBlock(nBlocks - 1));
  if (!quantileTab ||
    inMeta->GetMetaData(nBlocks - 1)->Get(svtkCompositeDataSet::NAME()) != svtkStdString("Quantiles"))
  {
    return;
  }

  // Retrieve name of variable of the request
  svtkStdString varName = rowNames->GetValue(0);

  // Grab the data for the requested variable
  svtkAbstractArray* vals = outData->GetColumnByName(varName);
  if (!vals)
  {
    return;
  }

  // Find the quantile column that corresponds to the variable of the request
  svtkAbstractArray* quantiles = quantileTab->GetColumnByName(varName);
  if (!quantiles)
  {
    svtkWarningMacro(
      "Quantile table table does not have a column " << varName.c_str() << ". Ignoring it.");
    return;
  }

  // Select assess functor depending on data and quantile type
  if (vals->IsA("svtkDataArray") && quantiles->IsA("svtkDataArray"))
  {
    dfunc = new DataArrayQuantizer(vals, quantiles);
  }
  else if (vals->IsA("svtkStringArray") && quantiles->IsA("svtkStringArray"))
  {
    dfunc = new StringArrayQuantizer(vals, quantiles);
  }
  else if (vals->IsA("svtkVariantArray") && quantiles->IsA("svtkVariantArray"))
  {
    dfunc = new VariantArrayQuantizer(vals, quantiles);
  }
  else
  {
    svtkWarningMacro("Unsupported (data,quantiles) type for column "
      << varName.c_str() << ": data type is " << vals->GetClassName() << " and quantiles type is "
      << quantiles->GetClassName() << ". Ignoring it.");
  }
}
