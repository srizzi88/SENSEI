/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkContingencyStatistics.cxx

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

#include "svtkContingencyStatistics.h"
#include "svtkStatisticsAlgorithmPrivate.h"

#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLongArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

#include <map>
#include <vector>

#include <sstream>

typedef std::map<svtkStdString, svtkIdType> StringCounts;
typedef std::map<svtkIdType, double> Entropies;

// ----------------------------------------------------------------------
template <typename TypeSpec, typename svtkType>
class BivariateContingenciesAndInformationFunctor : public svtkStatisticsAlgorithm::AssessFunctor
{
  typedef std::vector<TypeSpec> Tuple;

  typedef std::map<Tuple, double> PDF;

public:
  svtkDataArray* DataX;
  svtkDataArray* DataY;
  std::map<Tuple, PDF> PdfX_Y;
  std::map<Tuple, PDF> PdfYcX;
  std::map<Tuple, PDF> PdfXcY;
  std::map<Tuple, PDF> PmiX_Y;

  BivariateContingenciesAndInformationFunctor(svtkAbstractArray* valsX, svtkAbstractArray* valsY,
    const std::map<Tuple, PDF>& pdfX_Y, const std::map<Tuple, PDF>& pdfYcX,
    const std::map<Tuple, PDF>& pdfXcY, const std::map<Tuple, PDF>& pmiX_Y)
    : PdfX_Y(pdfX_Y)
    , PdfYcX(pdfYcX)
    , PdfXcY(pdfXcY)
    , PmiX_Y(pmiX_Y)
  {
    this->DataX = svtkArrayDownCast<svtkDataArray>(valsX);
    this->DataY = svtkArrayDownCast<svtkDataArray>(valsY);
  }
  ~BivariateContingenciesAndInformationFunctor() override = default;
  void operator()(svtkDoubleArray* result, svtkIdType id) override
  {
    Tuple x(this->DataX->GetNumberOfComponents());
    Tuple y(this->DataX->GetNumberOfComponents());

    for (int c = 0; c < this->DataX->GetNumberOfComponents(); c++)
    {
      x[c] = this->DataX->GetComponent(id, c);
    }
    for (int c = 0; c < this->DataY->GetNumberOfComponents(); c++)
    {
      y[c] = this->DataY->GetComponent(id, c);
    }

    result->SetNumberOfValues(4);
    result->SetValue(0, this->PdfX_Y[x][y]);
    result->SetValue(1, this->PdfYcX[x][y]);
    result->SetValue(2, this->PdfXcY[x][y]);
    result->SetValue(3, this->PmiX_Y[x][y]);
  }
};

template <>
class BivariateContingenciesAndInformationFunctor<svtkStdString, svtkStringArray>
  : public svtkStatisticsAlgorithm::AssessFunctor
{
  typedef svtkStdString TypeSpec;

  typedef std::map<TypeSpec, double> PDF;

public:
  svtkAbstractArray* DataX;
  svtkAbstractArray* DataY;
  std::map<TypeSpec, PDF> PdfX_Y;
  std::map<TypeSpec, PDF> PdfYcX;
  std::map<TypeSpec, PDF> PdfXcY;
  std::map<TypeSpec, PDF> PmiX_Y;

  BivariateContingenciesAndInformationFunctor(svtkAbstractArray* valsX, svtkAbstractArray* valsY,
    const std::map<TypeSpec, PDF>& pdfX_Y, const std::map<TypeSpec, PDF>& pdfYcX,
    const std::map<TypeSpec, PDF>& pdfXcY, const std::map<TypeSpec, PDF>& pmiX_Y)
    : PdfX_Y(pdfX_Y)
    , PdfYcX(pdfYcX)
    , PdfXcY(pdfXcY)
    , PmiX_Y(pmiX_Y)
  {
    this->DataX = valsX;
    this->DataY = valsY;
  }
  ~BivariateContingenciesAndInformationFunctor() override = default;
  void operator()(svtkDoubleArray* result, svtkIdType id) override
  {
    TypeSpec x = this->DataX->GetVariantValue(id).ToString();
    TypeSpec y = this->DataY->GetVariantValue(id).ToString();

    result->SetNumberOfValues(4);
    result->SetValue(0, this->PdfX_Y[x][y]);
    result->SetValue(1, this->PdfYcX[x][y]);
    result->SetValue(2, this->PdfXcY[x][y]);
    result->SetValue(3, this->PmiX_Y[x][y]);
  }
};

// Count is separated from the class so that it can be properly specialized
template <typename TypeSpec>
void Count(std::map<std::vector<TypeSpec>, std::map<std::vector<TypeSpec>, svtkIdType> >& table,
  svtkAbstractArray* valsX, svtkAbstractArray* valsY)
{
  svtkDataArray* dataX = svtkArrayDownCast<svtkDataArray>(valsX);
  svtkDataArray* dataY = svtkArrayDownCast<svtkDataArray>(valsY);
  if (dataX == nullptr || dataY == nullptr)
    return;
  svtkIdType nRow = dataX->GetNumberOfTuples();
  for (svtkIdType r = 0; r < nRow; ++r)
  {
    std::vector<TypeSpec> x(dataX->GetNumberOfComponents());
    std::vector<TypeSpec> y(dataX->GetNumberOfComponents());

    for (int c = 0; c < dataX->GetNumberOfComponents(); c++)
    {
      x[c] = dataX->GetComponent(r, c);
    }
    for (int c = 0; c < dataY->GetNumberOfComponents(); c++)
    {
      y[c] = dataY->GetComponent(r, c);
    }

    ++table[x][y];
  }
}

void Count(std::map<svtkStdString, std::map<svtkStdString, svtkIdType> >& table,
  svtkAbstractArray* valsX, svtkAbstractArray* valsY)
{
  svtkIdType nRow = valsX->GetNumberOfTuples();
  for (svtkIdType r = 0; r < nRow; ++r)
  {
    ++table[valsX->GetVariantValue(r).ToString()][valsY->GetVariantValue(r).ToString()];
  }
}

// ----------------------------------------------------------------------
template <typename TypeSpec, typename svtkType>
class ContingencyImpl
{
  typedef std::vector<TypeSpec> Tuple;

  typedef std::map<Tuple, svtkIdType> Counts;
  typedef std::map<Tuple, Counts> Table;

  typedef std::map<Tuple, double> PDF;

public:
  ContingencyImpl() = default;
  ~ContingencyImpl() = default;

  // ----------------------------------------------------------------------
  static void CalculateContingencyRow(
    svtkAbstractArray* valsX, svtkAbstractArray* valsY, svtkTable* contingencyTab, svtkIdType refRow)
  {
    // Calculate contingency table
    Table table;
    Count<TypeSpec>(table, valsX, valsY);

    svtkDataArray* dataX = svtkArrayDownCast<svtkDataArray>(contingencyTab->GetColumn(1));
    svtkDataArray* dataY = svtkArrayDownCast<svtkDataArray>(contingencyTab->GetColumn(2));

    // Store contingency table
    int row = contingencyTab->GetNumberOfRows();
    for (typename Table::iterator mit = table.begin(); mit != table.end(); ++mit)
    {
      for (typename Counts::iterator dit = mit->second.begin(); dit != mit->second.end(); ++dit)
      {
        contingencyTab->InsertNextBlankRow();

        contingencyTab->SetValue(row, 0, refRow);

        for (int c = 0; c < dataX->GetNumberOfComponents(); c++)
        {
          dataX->SetComponent(row, c, mit->first[c]);
        }

        for (int c = 0; c < dataY->GetNumberOfComponents(); c++)
        {
          dataY->SetComponent(row, c, dit->first[c]);
        }

        contingencyTab->SetValue(row, 3, dit->second);
        row++;
      }
    }
  }

  // ----------------------------------------------------------------------
  void ComputeMarginals(svtkIdTypeArray* keys, svtkStringArray* varX, svtkStringArray* varY,
    svtkAbstractArray* valsX, svtkAbstractArray* valsY, svtkIdTypeArray* card,
    svtkTable* contingencyTab)
  {
    svtkType* dataX = svtkType::SafeDownCast(valsX);
    svtkType* dataY = svtkType::SafeDownCast(valsY);

    if (dataX == nullptr || dataY == nullptr)
      return;

    int nRowSumm = varX->GetNumberOfTuples();
    if (nRowSumm != varY->GetNumberOfTuples())
      return;

    // Temporary counters, used to check that all pairs of variables have indeed the same number of
    // observations
    std::map<svtkIdType, svtkIdType> cardinalities;

    // Calculate marginal counts (marginal PDFs are calculated at storage time to avoid redundant
    // summations)
    std::map<svtkStdString, std::pair<svtkStdString, svtkStdString> > marginalToPair;

    marginalCounts.clear();

    svtkIdType nRowCont = contingencyTab->GetNumberOfRows();
    for (int r = 1; r < nRowCont; ++r) // Skip first row which contains data set cardinality
    {
      // Find the pair of variables to which the key corresponds
      svtkIdType key = keys->GetValue(r);

      if (key < 0 || key >= nRowSumm)
      {
        cerr << "Inconsistent input: dictionary does not have a row " << key
             << ". Cannot derive model." << endl;
        return;
      }

      svtkStdString c1 = varX->GetValue(key);
      svtkStdString c2 = varY->GetValue(key);

      if (marginalToPair.find(c1) == marginalToPair.end())
      {
        // c1 has not yet been used as a key... add it with (c1,c2) as the corresponding pair
        marginalToPair[c1].first = c1;
        marginalToPair[c1].second = c2;
      }

      if (marginalToPair.find(c2) == marginalToPair.end())
      {
        // c2 has not yet been used as a key... add it with (c1,c2) as the corresponding pair
        marginalToPair[c2].first = c1;
        marginalToPair[c2].second = c2;
      }

      Tuple x(dataX->GetNumberOfComponents());
      Tuple y(dataY->GetNumberOfComponents());

      for (int c = 0; c < dataX->GetNumberOfComponents(); c++)
      {
        x[c] = dataX->GetComponent(r, c);
      }
      for (int c = 0; c < dataY->GetNumberOfComponents(); c++)
      {
        y[c] = dataY->GetComponent(r, c);
      }

      svtkIdType c = card->GetValue(r);
      cardinalities[key] += c;

      if (marginalToPair[c1].first == c1 && marginalToPair[c1].second == c2)
      {
        marginalCounts[c1][x] += c;
      }

      if (marginalToPair[c2].first == c1 && marginalToPair[c2].second == c2)
      {
        marginalCounts[c2][y] += c;
      }
    }

    // Data set cardinality: unknown yet, pick the cardinality of the first pair and make sure all
    // other pairs have the same cardinality.
    svtkIdType n = cardinalities[0];
    for (std::map<svtkIdType, svtkIdType>::iterator iit = cardinalities.begin();
         iit != cardinalities.end(); ++iit)
    {
      if (iit->second != n)
      {
        cerr << "Inconsistent input: variable pairs do not have equal cardinalities: " << iit->first
             << " != " << n << ". Cannot derive model." << endl;
        return;
      }
    }

    // We have a unique value for the cardinality and can henceforth proceed
    contingencyTab->SetValueByName(0, "Cardinality", n);
  }

  // ----------------------------------------------------------------------
  void ComputePDFs(svtkMultiBlockDataSet* inMeta, svtkTable* contingencyTab)
  {
    // Resize output meta so marginal PDF tables can be appended
    unsigned int nBlocks = inMeta->GetNumberOfBlocks();
    inMeta->SetNumberOfBlocks(nBlocks + static_cast<unsigned int>(marginalCounts.size()));

    // Rows of the marginal PDF tables contain:
    // 0: variable value
    // 1: marginal cardinality
    // 2: marginal probability
    svtkVariantArray* row = svtkVariantArray::New();
    row->SetNumberOfValues(3);
    svtkType* array = svtkType::New();

    // Add marginal PDF tables as new blocks to the meta output starting at block nBlock
    // NB: block nBlock is kept for information entropy
    double n = contingencyTab->GetValueByName(0, "Cardinality").ToDouble();
    double inv_n = 1. / n;

    marginalPDFs.clear();

    for (typename std::map<svtkStdString, Counts>::iterator sit = marginalCounts.begin();
         sit != marginalCounts.end(); ++sit, ++nBlocks)
    {
      svtkTable* marginalTab = svtkTable::New();

      svtkStringArray* stringCol = svtkStringArray::New();
      stringCol->SetName(sit->first.c_str());
      marginalTab->AddColumn(stringCol);
      stringCol->Delete();

      svtkIdTypeArray* idTypeCol = svtkIdTypeArray::New();
      idTypeCol->SetName("Cardinality");
      marginalTab->AddColumn(idTypeCol);
      idTypeCol->Delete();

      svtkDoubleArray* doubleCol = svtkDoubleArray::New();
      doubleCol->SetName("P");
      marginalTab->AddColumn(doubleCol);
      doubleCol->Delete();

      double p;
      for (typename Counts::iterator xit = sit->second.begin(); xit != sit->second.end(); ++xit)
      {
        // Calculate and retain marginal PDF
        p = inv_n * xit->second;
        marginalPDFs[sit->first][xit->first] = p;

        array->SetNumberOfValues(static_cast<svtkIdType>(xit->first.size()));
        for (size_t i = 0; i < xit->first.size(); i++)
        {
          array->SetValue(static_cast<int>(i), xit->first[i]);
        }

        // Insert marginal cardinalities and probabilities
        row->SetValue(0, array);       // variable value
        row->SetValue(1, xit->second); // marginal cardinality
        row->SetValue(2, p);           // marginal probability
        marginalTab->InsertNextRow(row);
      }

      // Add marginal PDF block
      inMeta->GetMetaData(nBlocks)->Set(svtkCompositeDataSet::NAME(), sit->first.c_str());
      inMeta->SetBlock(nBlocks, marginalTab);

      // Clean up
      marginalTab->Delete();
    }

    array->Delete();
    row->Delete();
  }

  // ----------------------------------------------------------------------
  void ComputeDerivedValues(svtkIdTypeArray* keys, svtkStringArray* varX, svtkStringArray* varY,
    svtkAbstractArray* valsX, svtkAbstractArray* valsY, svtkIdTypeArray* card,
    svtkTable* contingencyTab, svtkDoubleArray** derivedCols, int nDerivedVals, Entropies* H,
    int nEntropy)
  {
    svtkType* dataX = svtkType::SafeDownCast(valsX);
    svtkType* dataY = svtkType::SafeDownCast(valsY);

    if (dataX == nullptr || dataY == nullptr)
      return;

    double n = contingencyTab->GetValueByName(0, "Cardinality").ToDouble();
    double inv_n = 1. / n;

    // Container for derived values
    double* derivedVals = new double[nDerivedVals];

    // Calculate joint and conditional PDFs, and information entropies
    svtkIdType nRowCount = contingencyTab->GetNumberOfRows();
    for (int r = 1; r < nRowCount; ++r) // Skip first row which contains data set cardinality
    {
      // Find the pair of variables to which the key corresponds
      svtkIdType key = keys->GetValue(r);

      // Get values
      svtkStdString c1 = varX->GetValue(key);
      svtkStdString c2 = varY->GetValue(key);

      // Get primary statistics for (c1,c2) pair
      Tuple x(dataX->GetNumberOfComponents());
      Tuple y(dataX->GetNumberOfComponents());

      for (int c = 0; c < dataX->GetNumberOfComponents(); c++)
      {
        x[c] = dataX->GetComponent(r, c);
      }
      for (int c = 0; c < dataY->GetNumberOfComponents(); c++)
      {
        y[c] = dataY->GetComponent(r, c);
      }

      svtkIdType c = card->GetValue(r);

      // Get marginal PDF values and their product
      double p1 = marginalPDFs[c1][x];
      double p2 = marginalPDFs[c2][y];

      // Calculate P(c1,c2)
      derivedVals[0] = inv_n * c;

      // Calculate P(c2|c1)
      derivedVals[1] = derivedVals[0] / p1;

      // Calculate P(c1|c2)
      derivedVals[2] = derivedVals[0] / p2;

      // Store P(c1,c2), P(c2|c1), P(c1|c2) and use them to update H(X,Y), H(Y|X), H(X|Y)
      for (int j = 0; j < nEntropy; ++j)
      {
        // Store probabilities
        derivedCols[j]->SetValue(r, derivedVals[j]);

        // Update information entropies
        H[j][key] -= derivedVals[0] * log(derivedVals[j]);
      }

      // Calculate and store PMI(c1,c2)
      derivedVals[3] = log(derivedVals[0] / (p1 * p2));
      derivedCols[3]->SetValue(r, derivedVals[3]);
    }

    delete[] derivedVals;
  }

  // ----------------------------------------------------------------------
  static double SelectAssessFunctor(svtkTable* contingencyTab, svtkIdType pairKey,
    svtkAbstractArray* valsX, svtkAbstractArray* valsY, svtkStatisticsAlgorithm::AssessFunctor*& dfunc)
  {
    // Downcast columns to appropriate arrays for efficient data access
    svtkIdTypeArray* keys = svtkArrayDownCast<svtkIdTypeArray>(contingencyTab->GetColumnByName("Key"));
    svtkType* dataX = svtkType::SafeDownCast(contingencyTab->GetColumnByName("x"));
    svtkType* dataY = svtkType::SafeDownCast(contingencyTab->GetColumnByName("y"));

    svtkDoubleArray* pX_Y = svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName("P"));
    svtkDoubleArray* pYcX =
      svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName("Py|x"));
    svtkDoubleArray* pXcY =
      svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName("Px|y"));
    svtkDoubleArray* pmis = svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName("PMI"));

    // Verify that assess parameters have been properly obtained
    if (!pX_Y || !pYcX || !pXcY || !pmis)
    {
      svtkErrorWithObjectMacro(contingencyTab, "Missing derived values");
      return 0;
    }
    // Create parameter maps
    std::map<Tuple, PDF> pdfX_Y;
    std::map<Tuple, PDF> pdfYcX;
    std::map<Tuple, PDF> pdfXcY;
    std::map<Tuple, PDF> pmiX_Y;

    // Sanity check: joint CDF
    double cdf = 0.;

    // Loop over parameters table until the requested variables are found
    svtkIdType nRowCont = contingencyTab->GetNumberOfRows();
    for (int r = 1; r < nRowCont; ++r) // Skip first row which contains data set cardinality
    {
      // Find the pair of variables to which the key corresponds
      svtkIdType key = keys->GetValue(r);

      if (key != pairKey)
      {
        continue;
      }

      Tuple x(dataX->GetNumberOfComponents());
      Tuple y(dataX->GetNumberOfComponents());
      for (int c = 0; c < dataX->GetNumberOfComponents(); c++)
      {
        x[c] = dataX->GetComponent(r, c);
      }
      for (int c = 0; c < dataY->GetNumberOfComponents(); c++)
      {
        y[c] = dataY->GetComponent(r, c);
      }

      // Fill parameter maps
      // PDF(X,Y)
      double v = pX_Y->GetValue(r);
      pdfX_Y[x][y] = v;

      // Sanity check: update CDF
      cdf += v;

      // PDF(Y|X)
      v = pYcX->GetValue(r);
      pdfYcX[x][y] = v;

      // PDF(X|Y)
      v = pXcY->GetValue(r);
      pdfXcY[x][y] = v;

      // PMI(X,Y)
      v = pmis->GetValue(r);
      pmiX_Y[x][y] = v;
    } // for ( int r = 1; r < nRowCont; ++ r )

    // Sanity check: verify that CDF = 1
    if (fabs(cdf - 1.) <= 1.e-6)
    {
      dfunc = new BivariateContingenciesAndInformationFunctor<TypeSpec, svtkType>(
        valsX, valsY, pdfX_Y, pdfYcX, pdfXcY, pmiX_Y);
    }
    return cdf;
  }

private:
  std::map<svtkStdString, Counts> marginalCounts;
  std::map<svtkStdString, PDF> marginalPDFs;
};

template <>
class ContingencyImpl<svtkStdString, svtkStringArray>
{
  typedef svtkStringArray svtkType;

  typedef svtkStdString TypeSpec;
  typedef TypeSpec Tuple;

  typedef std::map<Tuple, svtkIdType> Counts;
  typedef std::map<Tuple, Counts> Table;

  typedef std::map<Tuple, double> PDF;

public:
  ContingencyImpl() = default;
  ~ContingencyImpl() = default;

  // ----------------------------------------------------------------------
  static void CalculateContingencyRow(
    svtkAbstractArray* valsX, svtkAbstractArray* valsY, svtkTable* contingencyTab, svtkIdType refRow)
  {
    // Calculate contingency table
    Table table;
    Count(table, valsX, valsY);

    // Store contingency table
    int row = contingencyTab->GetNumberOfRows();
    for (Table::iterator mit = table.begin(); mit != table.end(); ++mit)
    {
      for (Counts::iterator dit = mit->second.begin(); dit != mit->second.end(); ++dit)
      {
        contingencyTab->InsertNextBlankRow();

        contingencyTab->SetValue(row, 0, refRow);
        contingencyTab->SetValue(row, 1, mit->first);
        contingencyTab->SetValue(row, 2, dit->first);
        contingencyTab->SetValue(row, 3, dit->second);
        row++;
      }
    }
  }

  // ----------------------------------------------------------------------
  void ComputeMarginals(svtkIdTypeArray* keys, svtkStringArray* varX, svtkStringArray* varY,
    svtkAbstractArray* valsX, svtkAbstractArray* valsY, svtkIdTypeArray* card,
    svtkTable* contingencyTab)
  {
    svtkType* dataX = svtkType::SafeDownCast(valsX);
    svtkType* dataY = svtkType::SafeDownCast(valsY);

    if (dataX == nullptr || dataY == nullptr)
      return;

    int nRowSumm = varX->GetNumberOfTuples();
    if (nRowSumm != varY->GetNumberOfTuples())
      return;

    // Temporary counters, used to check that all pairs of variables have indeed the same number of
    // observations
    std::map<svtkIdType, svtkIdType> cardinalities;

    // Calculate marginal counts (marginal PDFs are calculated at storage time to avoid redundant
    // summations)
    std::map<svtkStdString, std::pair<svtkStdString, svtkStdString> > marginalToPair;

    marginalCounts.clear();

    svtkIdType nRowCont = contingencyTab->GetNumberOfRows();
    for (int r = 1; r < nRowCont; ++r) // Skip first row which contains data set cardinality
    {
      // Find the pair of variables to which the key corresponds
      svtkIdType key = keys->GetValue(r);

      if (key < 0 || key >= nRowSumm)
      {
        cerr << "Inconsistent input: dictionary does not have a row " << key
             << ". Cannot derive model." << endl;
        return;
      }

      svtkStdString c1 = varX->GetValue(key);
      svtkStdString c2 = varY->GetValue(key);

      if (marginalToPair.find(c1) == marginalToPair.end())
      {
        // c1 has not yet been used as a key... add it with (c1,c2) as the corresponding pair
        marginalToPair[c1].first = c1;
        marginalToPair[c1].second = c2;
      }

      if (marginalToPair.find(c2) == marginalToPair.end())
      {
        // c2 has not yet been used as a key... add it with (c1,c2) as the corresponding pair
        marginalToPair[c2].first = c1;
        marginalToPair[c2].second = c2;
      }

      Tuple x = dataX->GetValue(r);
      Tuple y = dataY->GetValue(r);
      svtkIdType c = card->GetValue(r);
      cardinalities[key] += c;

      if (marginalToPair[c1].first == c1 && marginalToPair[c1].second == c2)
      {
        marginalCounts[c1][x] += c;
      }

      if (marginalToPair[c2].first == c1 && marginalToPair[c2].second == c2)
      {
        marginalCounts[c2][y] += c;
      }
    }

    // Data set cardinality: unknown yet, pick the cardinality of the first pair and make sure all
    // other pairs have the same cardinality.
    svtkIdType n = cardinalities[0];
    for (std::map<svtkIdType, svtkIdType>::iterator iit = cardinalities.begin();
         iit != cardinalities.end(); ++iit)
    {
      if (iit->second != n)
      {
        cerr << "Inconsistent input: variable pairs do not have equal cardinalities: " << iit->first
             << " != " << n << ". Cannot derive model." << endl;
        return;
      }
    }

    // We have a unique value for the cardinality and can henceforth proceed
    contingencyTab->SetValueByName(0, "Cardinality", n);
  }

  // ----------------------------------------------------------------------
  void ComputePDFs(svtkMultiBlockDataSet* inMeta, svtkTable* contingencyTab)
  {
    // Resize output meta so marginal PDF tables can be appended
    unsigned int nBlocks = inMeta->GetNumberOfBlocks();
    inMeta->SetNumberOfBlocks(nBlocks + static_cast<unsigned int>(marginalCounts.size()));

    // Rows of the marginal PDF tables contain:
    // 0: variable value
    // 1: marginal cardinality
    // 2: marginal probability
    svtkVariantArray* row = svtkVariantArray::New();
    row->SetNumberOfValues(3);

    // Add marginal PDF tables as new blocks to the meta output starting at block nBlock
    // NB: block nBlock is kept for information entropy
    double n = contingencyTab->GetValueByName(0, "Cardinality").ToDouble();
    double inv_n = 1. / n;

    marginalPDFs.clear();

    for (std::map<svtkStdString, Counts>::iterator sit = marginalCounts.begin();
         sit != marginalCounts.end(); ++sit, ++nBlocks)
    {
      svtkTable* marginalTab = svtkTable::New();

      svtkStringArray* stringCol = svtkStringArray::New();
      stringCol->SetName(sit->first.c_str());
      marginalTab->AddColumn(stringCol);
      stringCol->Delete();

      svtkIdTypeArray* idTypeCol = svtkIdTypeArray::New();
      idTypeCol->SetName("Cardinality");
      marginalTab->AddColumn(idTypeCol);
      idTypeCol->Delete();

      svtkDoubleArray* doubleCol = svtkDoubleArray::New();
      doubleCol->SetName("P");
      marginalTab->AddColumn(doubleCol);
      doubleCol->Delete();

      double p;
      for (Counts::iterator xit = sit->second.begin(); xit != sit->second.end(); ++xit)
      {
        // Calculate and retain marginal PDF
        p = inv_n * xit->second;
        marginalPDFs[sit->first][xit->first] = p;

        // Insert marginal cardinalities and probabilities
        row->SetValue(0, xit->first);  // variable value
        row->SetValue(1, xit->second); // marginal cardinality
        row->SetValue(2, p);           // marginal probability
        marginalTab->InsertNextRow(row);
      }

      // Add marginal PDF block
      inMeta->GetMetaData(nBlocks)->Set(svtkCompositeDataSet::NAME(), sit->first.c_str());
      inMeta->SetBlock(nBlocks, marginalTab);

      // Clean up
      marginalTab->Delete();
    }

    row->Delete();
  }

  // ----------------------------------------------------------------------
  void ComputeDerivedValues(svtkIdTypeArray* keys, svtkStringArray* varX, svtkStringArray* varY,
    svtkAbstractArray* valsX, svtkAbstractArray* valsY, svtkIdTypeArray* card,
    svtkTable* contingencyTab, svtkDoubleArray** derivedCols, int nDerivedVals, Entropies* H,
    int nEntropy)
  {
    svtkType* dataX = svtkType::SafeDownCast(valsX);
    svtkType* dataY = svtkType::SafeDownCast(valsY);

    if (dataX == nullptr || dataY == nullptr)
      return;

    double n = contingencyTab->GetValueByName(0, "Cardinality").ToDouble();
    double inv_n = 1. / n;

    // Container for derived values
    double* derivedVals = new double[nDerivedVals];

    // Calculate joint and conditional PDFs, and information entropies
    svtkIdType nRowCount = contingencyTab->GetNumberOfRows();
    for (int r = 1; r < nRowCount; ++r) // Skip first row which contains data set cardinality
    {
      // Find the pair of variables to which the key corresponds
      svtkIdType key = keys->GetValue(r);

      // Get values
      svtkStdString c1 = varX->GetValue(key);
      svtkStdString c2 = varY->GetValue(key);

      // Get primary statistics for (c1,c2) pair
      Tuple x = dataX->GetValue(r);
      Tuple y = dataY->GetValue(r);
      svtkIdType c = card->GetValue(r);

      // Get marginal PDF values and their product
      double p1 = marginalPDFs[c1][x];
      double p2 = marginalPDFs[c2][y];

      // Calculate P(c1,c2)
      derivedVals[0] = inv_n * c;

      // Calculate P(c2|c1)
      derivedVals[1] = derivedVals[0] / p1;

      // Calculate P(c1|c2)
      derivedVals[2] = derivedVals[0] / p2;

      // Store P(c1,c2), P(c2|c1), P(c1|c2) and use them to update H(X,Y), H(Y|X), H(X|Y)
      for (int j = 0; j < nEntropy; ++j)
      {
        // Store probabilities
        derivedCols[j]->SetValue(r, derivedVals[j]);

        // Update information entropies
        H[j][key] -= derivedVals[0] * log(derivedVals[j]);
      }

      // Calculate and store PMI(c1,c2)
      derivedVals[3] = log(derivedVals[0] / (p1 * p2));
      derivedCols[3]->SetValue(r, derivedVals[3]);
    }

    delete[] derivedVals;
  }

  // ----------------------------------------------------------------------
  static double SelectAssessFunctor(svtkTable* contingencyTab, svtkIdType pairKey,
    svtkAbstractArray* valsX, svtkAbstractArray* valsY, svtkStatisticsAlgorithm::AssessFunctor*& dfunc)
  {
    // Downcast columns to appropriate arrays for efficient data access
    svtkIdTypeArray* keys = svtkArrayDownCast<svtkIdTypeArray>(contingencyTab->GetColumnByName("Key"));
    svtkType* dataX = svtkType::SafeDownCast(contingencyTab->GetColumnByName("x"));
    svtkType* dataY = svtkType::SafeDownCast(contingencyTab->GetColumnByName("y"));

    svtkDoubleArray* pX_Y = svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName("P"));
    svtkDoubleArray* pYcX =
      svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName("Py|x"));
    svtkDoubleArray* pXcY =
      svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName("Px|y"));
    svtkDoubleArray* pmis = svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName("PMI"));

    // Verify that assess parameters have been properly obtained
    if (!pX_Y || !pYcX || !pXcY || !pmis)
    {
      svtkErrorWithObjectMacro(contingencyTab, "Missing derived values");
      return 0;
    }
    // Create parameter maps
    std::map<Tuple, PDF> pdfX_Y;
    std::map<Tuple, PDF> pdfYcX;
    std::map<Tuple, PDF> pdfXcY;
    std::map<Tuple, PDF> pmiX_Y;

    // Sanity check: joint CDF
    double cdf = 0.;

    // Loop over parameters table until the requested variables are found
    svtkIdType nRowCont = contingencyTab->GetNumberOfRows();
    for (int r = 1; r < nRowCont; ++r) // Skip first row which contains data set cardinality
    {
      // Find the pair of variables to which the key corresponds
      svtkIdType key = keys->GetValue(r);

      if (key != pairKey)
      {
        continue;
      }

      Tuple x = dataX->GetValue(r);
      Tuple y = dataY->GetValue(r);

      // Fill parameter maps
      // PDF(X,Y)
      double v = pX_Y->GetValue(r);
      pdfX_Y[x][y] = v;

      // Sanity check: update CDF
      cdf += v;

      // PDF(Y|X)
      v = pYcX->GetValue(r);
      pdfYcX[x][y] = v;

      // PDF(X|Y)
      v = pXcY->GetValue(r);
      pdfXcY[x][y] = v;

      // PMI(X,Y)
      v = pmis->GetValue(r);
      pmiX_Y[x][y] = v;
    } // for ( int r = 1; r < nRowCont; ++ r )

    // Sanity check: verify that CDF = 1
    if (fabs(cdf - 1.) <= 1.e-6)
    {
      dfunc = new BivariateContingenciesAndInformationFunctor<TypeSpec, svtkType>(
        valsX, valsY, pdfX_Y, pdfYcX, pdfXcY, pmiX_Y);
    }
    return cdf;
  }

private:
  std::map<svtkStdString, Counts> marginalCounts;
  std::map<svtkStdString, PDF> marginalPDFs;
};

svtkObjectFactoryNewMacro(svtkContingencyStatistics);

// ----------------------------------------------------------------------
svtkContingencyStatistics::svtkContingencyStatistics()
{
  // This engine has 2 primary tables: summary and contingency table
  this->NumberOfPrimaryTables = 2;

  this->AssessNames->SetNumberOfValues(4);
  this->AssessNames->SetValue(0, "P");
  this->AssessNames->SetValue(1, "Py|x");
  this->AssessNames->SetValue(2, "Px|y");
  this->AssessNames->SetValue(3, "PMI");
};

// ----------------------------------------------------------------------
svtkContingencyStatistics::~svtkContingencyStatistics() = default;

// ----------------------------------------------------------------------
void svtkContingencyStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------
void svtkContingencyStatistics::Learn(
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

  typedef enum
  {
    None = 0,
    Double,
    Integer
  } Specialization;

  Specialization specialization = Integer;
  for (std::set<std::set<svtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    std::set<svtkStdString>::const_iterator it = rit->begin();
    svtkStdString colX = *it;
    if (!inData->GetColumnByName(colX))
    {
      svtkWarningMacro("InData table does not have a column " << colX << ". Ignoring this pair.");
      continue;
    }

    ++it;
    svtkStdString colY = *it;
    if (!inData->GetColumnByName(colY))
    {
      svtkWarningMacro("InData table does not have a column " << colY << ". Ignoring this pair.");
      continue;
    }

    svtkDataArray* dataX = svtkArrayDownCast<svtkDataArray>(inData->GetColumnByName(colX));
    svtkDataArray* dataY = svtkArrayDownCast<svtkDataArray>(inData->GetColumnByName(colY));

    if (dataX == nullptr || dataY == nullptr)
    {
      specialization = None;
      break;
    }

    if (svtkArrayDownCast<svtkDoubleArray>(dataX) || svtkArrayDownCast<svtkFloatArray>(dataX) ||
      svtkArrayDownCast<svtkDoubleArray>(dataY) || svtkArrayDownCast<svtkFloatArray>(dataY))
    {
      specialization = Double;
    }
  }

  // Summary table: assigns a unique key to each (variable X,variable Y) pair
  svtkTable* summaryTab = svtkTable::New();

  svtkStringArray* stringCol = svtkStringArray::New();
  stringCol->SetName("Variable X");
  summaryTab->AddColumn(stringCol);
  stringCol->Delete();

  stringCol = svtkStringArray::New();
  stringCol->SetName("Variable Y");
  summaryTab->AddColumn(stringCol);
  stringCol->Delete();

  // The actual contingency table, indexed by the key of the summary
  svtkTable* contingencyTab = svtkTable::New();

  svtkIdTypeArray* idTypeCol = svtkIdTypeArray::New();
  idTypeCol->SetName("Key");
  contingencyTab->AddColumn(idTypeCol);
  idTypeCol->Delete();

  svtkAbstractArray* abstractX;
  svtkAbstractArray* abstractY;
  switch (specialization)
  {
    case None:
      abstractX = svtkStringArray::New();
      abstractY = svtkStringArray::New();
      break;
    case Double:
      abstractX = svtkDoubleArray::New();
      abstractY = svtkDoubleArray::New();
      break;
    case Integer:
      abstractX = svtkLongArray::New();
      abstractY = svtkLongArray::New();
      break;
    default:
      svtkErrorMacro(
        "Invalid specialization, " << specialization << ", expected None, Double or Integer");
      return;
  }

  abstractX->SetName("x");
  contingencyTab->AddColumn(abstractX);
  abstractX->Delete();

  abstractY->SetName("y");
  contingencyTab->AddColumn(abstractY);
  abstractY->Delete();

  idTypeCol = svtkIdTypeArray::New();
  idTypeCol->SetName("Cardinality");
  contingencyTab->AddColumn(idTypeCol);
  idTypeCol->Delete();

  // Row to be used to insert into summary table
  svtkVariantArray* row2 = svtkVariantArray::New();
  row2->SetNumberOfValues(2);

  // Insert first row which will always contain the data set cardinality, with key -1
  // NB: The cardinality is calculated in derive mode ONLY, and is set to an invalid value of -1 in
  // learn mode to make it clear that it is not a correct value. This is an issue of database
  // normalization: including the cardinality to the other counts can lead to inconsistency, in
  // particular when the input meta table is calculated by something else than the learn mode (e.g.,
  // is specified by the user).
  svtkStdString zString = svtkStdString("");
  contingencyTab->InsertNextBlankRow();
  contingencyTab->SetValue(0, 0, -1);
  if (specialization == None)
  {
    contingencyTab->SetValue(0, 1, zString);
    contingencyTab->SetValue(0, 2, zString);
  }
  else
  {
    contingencyTab->SetValue(0, 1, 0);
    contingencyTab->SetValue(0, 2, 0);
  }
  contingencyTab->SetValue(0, 3, -1);

  // Loop over requests
  for (std::set<std::set<svtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    // Each request contains only one pair of column of interest (if there are others, they are
    // ignored)
    std::set<svtkStdString>::const_iterator it = rit->begin();
    svtkStdString colX = *it;
    if (!inData->GetColumnByName(colX))
    {
      svtkWarningMacro("InData table does not have a column " << colX << ". Ignoring this pair.");
      continue;
    }

    ++it;
    svtkStdString colY = *it;
    if (!inData->GetColumnByName(colY))
    {
      svtkWarningMacro("InData table does not have a column " << colY << ". Ignoring this pair.");
      continue;
    }

    // Create entry in summary for pair (colX,colY) and set its index to be the key
    // for (colX,colY) values in the contingency table
    row2->SetValue(0, colX);
    row2->SetValue(1, colY);
    int summaryRow = summaryTab->GetNumberOfRows();
    summaryTab->InsertNextRow(row2);

    svtkAbstractArray* valsX = inData->GetColumnByName(colX);
    svtkAbstractArray* valsY = inData->GetColumnByName(colY);

    svtkDataArray* dataX = svtkArrayDownCast<svtkDataArray>(valsX);
    svtkDataArray* dataY = svtkArrayDownCast<svtkDataArray>(valsY);
    switch (specialization)
    {
      case None:
        ContingencyImpl<svtkStdString, svtkStringArray>::CalculateContingencyRow(
          valsX, valsY, contingencyTab, summaryRow);
        break;
      case Double:
        ContingencyImpl<double, svtkDoubleArray>::CalculateContingencyRow(
          dataX, dataY, contingencyTab, summaryRow);
        break;
      case Integer:
        ContingencyImpl<long, svtkLongArray>::CalculateContingencyRow(
          dataX, dataY, contingencyTab, summaryRow);
        break;
      default:
        svtkErrorMacro(
          "Invalid specialization, " << specialization << ", expected None, Double or Integer");
        continue;
    }
  }

  // Finally set blocks of the output meta port
  outMeta->SetNumberOfBlocks(2);
  outMeta->GetMetaData(static_cast<unsigned>(0))->Set(svtkCompositeDataSet::NAME(), "Summary");
  outMeta->SetBlock(0, summaryTab);
  outMeta->GetMetaData(static_cast<unsigned>(1))
    ->Set(svtkCompositeDataSet::NAME(), "Contingency Table");
  outMeta->SetBlock(1, contingencyTab);

  // Clean up
  summaryTab->Delete();
  contingencyTab->Delete();
  row2->Delete();
}

// ----------------------------------------------------------------------
void svtkContingencyStatistics::Derive(svtkMultiBlockDataSet* inMeta)
{
  if (!inMeta || inMeta->GetNumberOfBlocks() < 2)
  {
    return;
  }

  svtkTable* summaryTab = svtkTable::SafeDownCast(inMeta->GetBlock(0));
  if (!summaryTab)
  {
    return;
  }

  svtkTable* contingencyTab = svtkTable::SafeDownCast(inMeta->GetBlock(1));
  if (!contingencyTab)
  {
    return;
  }

  int nEntropy = 3;
  svtkStdString entropyNames[] = { "H(X,Y)", "H(Y|X)", "H(X|Y)" };

  // Create table for derived meta statistics
  svtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  svtkDoubleArray* doubleCol;
  for (int j = 0; j < nEntropy; ++j)
  {
    if (!summaryTab->GetColumnByName(entropyNames[j]))
    {
      doubleCol = svtkDoubleArray::New();
      doubleCol->SetName(entropyNames[j]);
      doubleCol->SetNumberOfTuples(nRowSumm);
      summaryTab->AddColumn(doubleCol);
      doubleCol->Delete();
    }
  }

  // Create columns of derived statistics
  int nDerivedVals = 4;
  svtkStdString derivedNames[] = { "P", "Py|x", "Px|y", "PMI" };

  svtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  for (int j = 0; j < nDerivedVals; ++j)
  {
    if (!contingencyTab->GetColumnByName(derivedNames[j]))
    {
      doubleCol = svtkDoubleArray::New();
      doubleCol->SetName(derivedNames[j]);
      doubleCol->SetNumberOfTuples(nRowCont);
      contingencyTab->AddColumn(doubleCol);
      doubleCol->Delete();
    }
  }

  // Downcast columns to typed arrays for efficient data access
  svtkStringArray* varX =
    svtkArrayDownCast<svtkStringArray>(summaryTab->GetColumnByName("Variable X"));
  svtkStringArray* varY =
    svtkArrayDownCast<svtkStringArray>(summaryTab->GetColumnByName("Variable Y"));

  svtkIdTypeArray* keys = svtkArrayDownCast<svtkIdTypeArray>(contingencyTab->GetColumnByName("Key"));
  svtkIdTypeArray* card =
    svtkArrayDownCast<svtkIdTypeArray>(contingencyTab->GetColumnByName("Cardinality"));

  svtkAbstractArray* valsX = contingencyTab->GetColumnByName("x");
  svtkAbstractArray* valsY = contingencyTab->GetColumnByName("y");

  svtkDataArray* dataX = svtkArrayDownCast<svtkDataArray>(valsX);
  svtkDataArray* dataY = svtkArrayDownCast<svtkDataArray>(valsY);

  // Fill cardinality row (0) with invalid values for derived statistics
  for (int i = 0; i < nDerivedVals; ++i)
  {
    contingencyTab->SetValueByName(0, derivedNames[i], -1.);
  }

  std::vector<svtkDoubleArray*> derivedCols(nDerivedVals);

  for (int j = 0; j < nDerivedVals; ++j)
  {
    derivedCols[j] =
      svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName(derivedNames[j]));

    if (!derivedCols[j])
    {
      svtkErrorWithObjectMacro(contingencyTab, "Empty model column(s). Cannot derive model.\n");
      return;
    }
  }

  // Container for information entropies
  std::vector<Entropies> entropies(nEntropy);

  if (dataX == nullptr || dataY == nullptr)
  {
    ContingencyImpl<svtkStdString, svtkStringArray> impl;
    impl.ComputeMarginals(keys, varX, varY, valsX, valsY, card, contingencyTab);
    impl.ComputePDFs(inMeta, contingencyTab);
    impl.ComputeDerivedValues(keys, varX, varY, valsX, valsY, card, contingencyTab,
      derivedCols.data(), nDerivedVals, entropies.data(), nEntropy);
  }
  else if (dataX->GetDataType() == SVTK_DOUBLE)
  {
    ContingencyImpl<double, svtkDoubleArray> impl;
    impl.ComputeMarginals(keys, varX, varY, valsX, valsY, card, contingencyTab);
    impl.ComputePDFs(inMeta, contingencyTab);
    impl.ComputeDerivedValues(keys, varX, varY, valsX, valsY, card, contingencyTab,
      derivedCols.data(), nDerivedVals, entropies.data(), nEntropy);
  }
  else
  {
    ContingencyImpl<long, svtkLongArray> impl;
    impl.ComputeMarginals(keys, varX, varY, valsX, valsY, card, contingencyTab);
    impl.ComputePDFs(inMeta, contingencyTab);
    impl.ComputeDerivedValues(keys, varX, varY, valsX, valsY, card, contingencyTab,
      derivedCols.data(), nDerivedVals, entropies.data(), nEntropy);
  }

  // Store information entropies
  for (Entropies::iterator eit = entropies[0].begin(); eit != entropies[0].end(); ++eit)
  {
    summaryTab->SetValueByName(eit->first, entropyNames[0], eit->second);              // H(X,Y)
    summaryTab->SetValueByName(eit->first, entropyNames[1], entropies[1][eit->first]); // H(Y|X)
    summaryTab->SetValueByName(eit->first, entropyNames[2], entropies[2][eit->first]); // H(X|Y)
  }
}

// ----------------------------------------------------------------------
void svtkContingencyStatistics::Assess(
  svtkTable* inData, svtkMultiBlockDataSet* inMeta, svtkTable* outData)
{
  if (!inData)
  {
    return;
  }

  if (!inMeta)
  {
    return;
  }

  svtkTable* summaryTab = svtkTable::SafeDownCast(inMeta->GetBlock(0));
  if (!summaryTab)
  {
    return;
  }

  // Downcast columns to string arrays for efficient data access
  svtkStringArray* varX =
    svtkArrayDownCast<svtkStringArray>(summaryTab->GetColumnByName("Variable X"));
  svtkStringArray* varY =
    svtkArrayDownCast<svtkStringArray>(summaryTab->GetColumnByName("Variable Y"));

  // Loop over requests
  svtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  svtkIdType nRowData = inData->GetNumberOfRows();
  for (std::set<std::set<svtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    // Each request contains only one pair of column of interest (if there are others, they are
    // ignored)
    std::set<svtkStdString>::const_iterator it = rit->begin();
    svtkStdString varNameX = *it;
    if (!inData->GetColumnByName(varNameX))
    {
      svtkWarningMacro(
        "InData table does not have a column " << varNameX << ". Ignoring this pair.");
      continue;
    }

    ++it;
    svtkStdString varNameY = *it;
    if (!inData->GetColumnByName(varNameY))
    {
      svtkWarningMacro(
        "InData table does not have a column " << varNameY << ". Ignoring this pair.");
      continue;
    }

    // Find the summary key to which the pair (colX,colY) corresponds
    svtkIdType pairKey = -1;
    for (svtkIdType r = 0; r < nRowSumm && pairKey == -1; ++r)
    {
      if (varX->GetValue(r) == varNameX && varY->GetValue(r) == varNameY)
      {
        pairKey = r;
      }
    }
    if (pairKey < 0 || pairKey >= nRowSumm)
    {
      svtkErrorMacro("Inconsistent input: dictionary does not have a row "
        << pairKey << ". Cannot derive model.");
      return;
    }

    svtkStringArray* varNames = svtkStringArray::New();
    varNames->SetNumberOfValues(2);
    varNames->SetValue(0, varNameX);
    varNames->SetValue(1, varNameY);

    // Store names to be able to use SetValueByName which is faster than SetValue
    svtkIdType nv = this->AssessNames->GetNumberOfValues();
    std::vector<svtkStdString> names(nv);
    int columnOffset = outData->GetNumberOfColumns();
    for (svtkIdType v = 0; v < nv; ++v)
    {
      std::ostringstream assessColName;
      assessColName << this->AssessNames->GetValue(v) << "(" << varNameX << "," << varNameY << ")";

      names[v] = assessColName.str();

      svtkDoubleArray* assessValues = svtkDoubleArray::New();
      assessValues->SetName(names[v]);
      assessValues->SetNumberOfTuples(nRowData);
      outData->AddColumn(assessValues);
      assessValues->Delete();
    }

    // Select assess functor
    AssessFunctor* dfunc;
    this->SelectAssessFunctor(outData, inMeta, pairKey, varNames, dfunc);

    if (!dfunc)
    {
      // Functor selection did not work. Do nothing.
      svtkWarningMacro("AssessFunctors could not be allocated for column pair ("
        << varNameX << "," << varNameY << "). Ignoring it.");
      continue;
    }
    else
    {
      // Assess each entry of the column
      svtkDoubleArray* assessResult = svtkDoubleArray::New();
      for (svtkIdType r = 0; r < nRowData; ++r)
      {
        (*dfunc)(assessResult, r);
        for (svtkIdType v = 0; v < nv; ++v)
        {
          outData->SetValue(r, columnOffset + v, assessResult->GetValue(v));
        }
      }

      assessResult->Delete();
    }

    // Clean up
    delete dfunc;
    varNames->Delete(); // Do not delete earlier! Otherwise, dfunc will be wrecked
  }                     // rit
}

// ----------------------------------------------------------------------
void svtkContingencyStatistics::CalculatePValues(svtkTable* testTab)
{
  svtkIdTypeArray* dimCol = svtkArrayDownCast<svtkIdTypeArray>(testTab->GetColumn(0));

  // Test columns must be created first
  svtkDoubleArray* testChi2Col = svtkDoubleArray::New();  // Chi square p-value
  svtkDoubleArray* testChi2yCol = svtkDoubleArray::New(); // Chi square with Yates correction p-value

  // Fill this column
  svtkIdType n = dimCol->GetNumberOfTuples();
  testChi2Col->SetNumberOfTuples(n);
  testChi2yCol->SetNumberOfTuples(n);
  for (svtkIdType r = 0; r < n; ++r)
  {
    testChi2Col->SetTuple1(r, -1);
    testChi2yCol->SetTuple1(r, -1);
  }

  // Now add the column of invalid values to the output table
  testTab->AddColumn(testChi2Col);
  testTab->AddColumn(testChi2yCol);

  testChi2Col->SetName("P");
  testChi2yCol->SetName("P Yates");

  // Clean up
  testChi2Col->Delete();
  testChi2yCol->Delete();
}

// ----------------------------------------------------------------------
void svtkContingencyStatistics::Test(
  svtkTable* inData, svtkMultiBlockDataSet* inMeta, svtkTable* outMeta)
{
  if (!inMeta)
  {
    return;
  }

  svtkTable* summaryTab = svtkTable::SafeDownCast(inMeta->GetBlock(0));
  if (!summaryTab)
  {
    return;
  }

  svtkTable* contingencyTab = svtkTable::SafeDownCast(inMeta->GetBlock(1));
  if (!contingencyTab)
  {
    return;
  }

  if (!outMeta)
  {
    return;
  }

  if (!inData)
  {
    return;
  }

  // The test table, indexed by the key of the summary
  svtkTable* testTab = svtkTable::New();

  // Prepare columns for the test:
  // 0: dimension
  // 1: Chi square statistic
  // 2: Chi square statistic with Yates correction
  // 3: Chi square p-value
  // 4: Chi square with Yates correction p-value
  // NB: These are not added to the output table yet, for they will be filled individually first
  //     in order that R be invoked only once.
  svtkIdTypeArray* dimCol = svtkIdTypeArray::New();
  dimCol->SetName("d");

  svtkDoubleArray* chi2Col = svtkDoubleArray::New();
  chi2Col->SetName("Chi2");

  svtkDoubleArray* chi2yCol = svtkDoubleArray::New();
  chi2yCol->SetName("Chi2 Yates");

  // Downcast columns to typed arrays for efficient data access
  svtkStringArray* varX =
    svtkArrayDownCast<svtkStringArray>(summaryTab->GetColumnByName("Variable X"));
  svtkStringArray* varY =
    svtkArrayDownCast<svtkStringArray>(summaryTab->GetColumnByName("Variable Y"));
  svtkIdTypeArray* keys = svtkArrayDownCast<svtkIdTypeArray>(contingencyTab->GetColumnByName("Key"));
  svtkStringArray* valx = svtkArrayDownCast<svtkStringArray>(contingencyTab->GetColumnByName("x"));
  svtkStringArray* valy = svtkArrayDownCast<svtkStringArray>(contingencyTab->GetColumnByName("y"));
  svtkIdTypeArray* card =
    svtkArrayDownCast<svtkIdTypeArray>(contingencyTab->GetColumnByName("Cardinality"));

  // Loop over requests
  svtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  svtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  for (std::set<std::set<svtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    // Each request contains only one pair of column of interest (if there are others, they are
    // ignored)
    std::set<svtkStdString>::const_iterator it = rit->begin();
    svtkStdString varNameX = *it;
    if (!inData->GetColumnByName(varNameX))
    {
      svtkWarningMacro(
        "InData table does not have a column " << varNameX << ". Ignoring this pair.");
      continue;
    }

    ++it;
    svtkStdString varNameY = *it;
    if (!inData->GetColumnByName(varNameY))
    {
      svtkWarningMacro(
        "InData table does not have a column " << varNameY << ". Ignoring this pair.");
      continue;
    }

    // Find the summary key to which the pair (colX,colY) corresponds
    svtkIdType pairKey = -1;
    for (svtkIdType r = 0; r < nRowSumm && pairKey == -1; ++r)
    {
      if (varX->GetValue(r) == varNameX && varY->GetValue(r) == varNameY)
      {
        pairKey = r;
      }
    }
    if (pairKey < 0 || pairKey >= nRowSumm)
    {
      svtkErrorMacro(
        "Inconsistent input: dictionary does not have a row " << pairKey << ". Cannot test.");
      return;
    }

    // Start by fetching joint counts

    // Sanity check: make sure all counts sum to grand total
    svtkIdType n = card->GetValue(0);
    svtkIdType sumij = 0;

    // Loop over parameters table until the requested variables are found
    std::map<svtkStdString, StringCounts> oij;
    svtkStdString x, y;
    svtkIdType key, c;
    for (int r = 1; r < nRowCont; ++r) // Skip first row which contains data set cardinality
    {
      // Find the pair of variables to which the key corresponds
      key = keys->GetValue(r);

      // Only use entries in the contingency table that correspond to values we are interested in
      if (key != pairKey)
      {
        continue;
      }

      // Fill PDF and update CDF
      x = valx->GetValue(r);
      y = valy->GetValue(r);
      c = card->GetValue(r);
      oij[x][y] = c;
      sumij += c;
    } // for ( int r = 1; r < nRowCont; ++ r )

    // Sanity check: verify that sum = grand total
    if (sumij != n)
    {
      svtkWarningMacro("Inconsistent sum of counts and grand total for column pair "
        << varNameX << "," << varNameY << "): " << sumij << " <> " << n << ". Cannot test.");

      return;
    }

    // Now search for relevant marginal counts
    StringCounts ek[2];
    int foundCount = 0;
    for (unsigned int b = 2; b < inMeta->GetNumberOfBlocks() && foundCount < 2; ++b)
    {
      const char* name = inMeta->GetMetaData(b)->Get(svtkCompositeDataSet::NAME());
      int foundIndex = -1;
      if (!strcmp(name, varNameX))
      {
        // Found the marginal count of X
        foundIndex = 0;
        ++foundCount;
      }
      else if (!strcmp(name, varNameY))
      {
        // Found the marginal count of Y
        foundIndex = 1;
        ++foundCount;
      }

      if (foundIndex > -1)
      {
        // One relevant PDF was found
        svtkTable* marginalTab = svtkTable::SafeDownCast(inMeta->GetBlock(b));

        // Downcast columns to appropriate arrays for efficient data access
        svtkStringArray* vals = svtkArrayDownCast<svtkStringArray>(marginalTab->GetColumnByName(name));
        svtkIdTypeArray* marg =
          svtkArrayDownCast<svtkIdTypeArray>(marginalTab->GetColumnByName("Cardinality"));

        // Now iterate over all entries and fill count map
        svtkIdType nRow = marginalTab->GetNumberOfRows();
        for (svtkIdType r = 0; r < nRow; ++r)
        {
          ek[foundIndex][vals->GetValue(r)] = marg->GetValue(r);
        }
      }
    }

    // Eliminating the case where one or both marginal counts are not provided in the model
    if (ek[0].empty())
    {
      svtkErrorMacro(
        "Incomplete input: missing marginal count for " << varNameX << ". Cannot test.");
      return;
    }

    if (ek[1].empty())
    {
      svtkErrorMacro(
        "Incomplete input: missing marginal count for " << varNameY << ". Cannot test.");
      return;
    }

    // Now that we have all we need, let us calculate the test statistic

    // We must iterate over all possible independent instances, which might result in
    // an impossibly too large double loop, even if the actual occurrence table is
    // sparse. C'est la vie.
    double eij, delta;
    double chi2 = 0;  // chi square test statistic
    double chi2y = 0; // chi square test statistic with Yates correction
    for (StringCounts::iterator xit = ek[0].begin(); xit != ek[0].end(); ++xit)
    {
      for (StringCounts::iterator yit = ek[1].begin(); yit != ek[1].end(); ++yit)
      {
        // Expected count
        eij = static_cast<double>(xit->second * yit->second) / n;

        // Discrepancy
        delta = eij - oij[xit->first][yit->first];

        // Chi square contribution
        chi2 += delta * delta / eij;

        // Chi square contribution with Yates correction
        delta = fabs(delta) - .5;
        chi2y += delta * delta / eij;
      } // yit
    }   // xit

    // Degrees of freedom
    svtkIdType d = static_cast<svtkIdType>((ek[0].size() - 1) * (ek[1].size() - 1));

    // Insert variable name and calculated Jarque-Bera statistic
    // NB: R will be invoked only once at the end for efficiency
    dimCol->InsertNextTuple1(d);
    chi2Col->InsertNextTuple1(chi2);
    chi2yCol->InsertNextTuple1(chi2y);
  } // rit

  // Now, add the already prepared columns to the output table
  testTab->AddColumn(dimCol);
  testTab->AddColumn(chi2Col);
  testTab->AddColumn(chi2yCol);

  // Last phase: compute the p-values or assign invalid value if they cannot be computed
  this->CalculatePValues(testTab);

  // Finally set output table to test table
  outMeta->ShallowCopy(testTab);

  // Clean up
  dimCol->Delete();
  chi2Col->Delete();
  chi2yCol->Delete();
  testTab->Delete();
}

// ----------------------------------------------------------------------
void svtkContingencyStatistics::SelectAssessFunctor(svtkTable* svtkNotUsed(outData),
  svtkDataObject* svtkNotUsed(inMetaDO), svtkStringArray* svtkNotUsed(rowNames),
  AssessFunctor*& svtkNotUsed(dfunc))
{
  // This method is not implemented for contingency statistics, as its API does not allow
  // for the passing of necessary parameters.
}

// ----------------------------------------------------------------------
void svtkContingencyStatistics::SelectAssessFunctor(svtkTable* outData, svtkMultiBlockDataSet* inMeta,
  svtkIdType pairKey, svtkStringArray* rowNames, AssessFunctor*& dfunc)
{
  dfunc = nullptr;
  svtkTable* contingencyTab = svtkTable::SafeDownCast(inMeta->GetBlock(1));
  if (!contingencyTab)
  {
    return;
  }

  svtkStdString varNameX = rowNames->GetValue(0);
  svtkStdString varNameY = rowNames->GetValue(1);

  // Grab the data for the requested variables
  svtkAbstractArray* valsX = outData->GetColumnByName(varNameX);
  svtkAbstractArray* valsY = outData->GetColumnByName(varNameY);
  if (!valsX || !valsY)
  {
    return;
  }

  svtkDoubleArray* dubx = svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName("x"));
  svtkDoubleArray* duby = svtkArrayDownCast<svtkDoubleArray>(contingencyTab->GetColumnByName("y"));
  svtkLongArray* intx = svtkArrayDownCast<svtkLongArray>(contingencyTab->GetColumnByName("x"));
  svtkLongArray* inty = svtkArrayDownCast<svtkLongArray>(contingencyTab->GetColumnByName("y"));
  double cdf;
  if (dubx && duby)
  {
    cdf = ContingencyImpl<double, svtkDoubleArray>::SelectAssessFunctor(
      contingencyTab, pairKey, valsX, valsY, dfunc);
  }
  else if (intx && inty)
  {
    cdf = ContingencyImpl<long, svtkLongArray>::SelectAssessFunctor(
      contingencyTab, pairKey, valsX, valsY, dfunc);
  }
  else
  {
    cdf = ContingencyImpl<svtkStdString, svtkStringArray>::SelectAssessFunctor(
      contingencyTab, pairKey, valsX, valsY, dfunc);
  }

  if (fabs(cdf - 1.) > 1.e-6)
  {
    svtkWarningMacro(
      "Incorrect CDF for column pair:" << varNameX << "," << varNameY << "). Ignoring it.");
  }
}
