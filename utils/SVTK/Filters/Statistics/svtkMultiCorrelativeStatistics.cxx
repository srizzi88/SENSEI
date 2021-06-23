#include "svtkMultiCorrelativeStatistics.h"
#include "svtkMultiCorrelativeStatisticsAssessFunctor.h"

#include "svtkDataObject.h"
#include "svtkDataObjectCollection.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOrderStatistics.h"
#include "svtkStatisticsAlgorithmPrivate.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

#include <map>
#include <sstream>
#include <vector>

#define SVTK_MULTICORRELATIVE_KEYCOLUMN1 "Column1"
#define SVTK_MULTICORRELATIVE_KEYCOLUMN2 "Column2"
#define SVTK_MULTICORRELATIVE_ENTRIESCOL "Entries"
#define SVTK_MULTICORRELATIVE_AVERAGECOL "Mean"
#define SVTK_MULTICORRELATIVE_COLUMNAMES "Column"

svtkStandardNewMacro(svtkMultiCorrelativeStatistics);

// ----------------------------------------------------------------------
svtkMultiCorrelativeStatistics::svtkMultiCorrelativeStatistics()
{
  this->AssessNames->SetNumberOfValues(1);
  this->AssessNames->SetValue(0, "d^2"); // Squared Mahalanobis distance
  this->MedianAbsoluteDeviation = false;
}

// ----------------------------------------------------------------------
svtkMultiCorrelativeStatistics::~svtkMultiCorrelativeStatistics() = default;

// ----------------------------------------------------------------------
void svtkMultiCorrelativeStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------
static void svtkMultiCorrelativeInvertCholesky(std::vector<double*>& chol, std::vector<double>& inv)
{
  svtkIdType m = static_cast<svtkIdType>(chol.size());
  inv.resize(m * (m + 1) / 2);

  svtkIdType i, j, k;
  for (i = 0; i < m; ++i)
  {
    svtkIdType rsi = (i * (i + 1)) / 2; // start index of row i in inv.
    inv[rsi + i] = 1. / chol[i][i];
    for (j = i; j > 0;)
    {
      inv[rsi + (--j)] = 0.;
      for (k = j; k < i; ++k)
      {
        svtkIdType rsk = (k * (k + 1)) / 2;
        inv[rsi + j] -= chol[k][i] * inv[rsk + j];
      }
      inv[rsi + j] *= inv[rsi + i];
    }
  }
  // The result, stored in \a inv as a lower-triangular, row-major matrix, is
  // the inverse of the Cholesky decomposition given as input (stored as a
  // rectangular, column-major matrix in \a chol). Note that the super-diagonal
  // entries of \a chol are not zero as you would expect... we just cleverly
  // don't reference them.
}

// ----------------------------------------------------------------------
static void svtkMultiCorrelativeTransposeTriangular(std::vector<double>& a, svtkIdType m)
{
  std::vector<double> b(a.begin(), a.end());
  double* bp = &b[0];
  svtkIdType i, j;
  a.clear();
  double* v;
  for (i = 0; i < m; ++i)
  {
    v = bp + (i * (i + 3)) / 2; // pointer to i-th entry along diagonal (i.e., a(i,i)).
    for (j = i; j < m; ++j)
    {
      a.push_back(*v);
      v += (j + 1); // move down one row
    }
  }

  // Now, if a had previously contained: [ A B C D E F G H I J ], representing the
  // lower triangular matrix: A          or the upper triangular: A B D G
  // (row-major order)        B C           (column-major order)    C E H
  //                          D E F                                   F I
  //                          G H I J                                   J
  // It now contains [ A B D G C E H F I J ], representing
  // upper triangular matrix : A B D G   or the lower triangular: A
  // (row-major order)           C E H      (column-major order)  B C
  //                               F I                            D E F
  //                                 J                            G H I J
}

// ----------------------------------------------------------------------
void svtkMultiCorrelativeAssessFunctor::operator()(svtkDoubleArray* result, svtkIdType row)
{
  svtkIdType m = static_cast<svtkIdType>(this->Columns.size());
  svtkIdType i, j;
  this->Tuple = this->EmptyTuple; // initialize Tuple to 0.0
  double* x = &this->Tuple[0];
  double* y;
  double* ci = &this->Factor[0];
  double v;
  for (i = 0; i < m; ++i)
  {
    v = this->Columns[i]->GetTuple(row)[0] - this->Center[i];
    y = x + i;
    for (j = i; j < m; ++j, ++ci, ++y)
    {
      (*y) += (*ci) * v;
    }
  }
  double r = 0.;
  y = x;
  for (i = 0; i < m; ++i, ++y)
  {
    r += (*y) * (*y);
  }

  result->SetNumberOfValues(1);
  // To report cumulance values instead of relative deviation, use this:
  // result->SetValue( 0, exp( -0.5 * r ) * pow( 0.5 * r, 0.5 * m - 2.0 ) * ( 0.5 * ( r + m ) - 1.0
  // ) / this->Normalization );
  result->SetValue(0, r);
}

// ----------------------------------------------------------------------
void svtkMultiCorrelativeStatistics::Aggregate(
  svtkDataObjectCollection* inMetaColl, svtkMultiBlockDataSet* outMeta)
{
  if (!outMeta)
  {
    return;
  }

  // Get hold of the first model (data object) in the collection
  svtkCollectionSimpleIterator it;
  inMetaColl->InitTraversal(it);
  svtkDataObject* inMetaDO = inMetaColl->GetNextDataObject(it);

  // Verify that the first input model is indeed contained in a multiblock data set
  svtkMultiBlockDataSet* inMeta = svtkMultiBlockDataSet::SafeDownCast(inMetaDO);
  if (!inMeta)
  {
    return;
  }

  // Verify that the first covariance matrix is indeed contained in a table
  svtkTable* inCov = svtkTable::SafeDownCast(inMeta->GetBlock(0));
  if (!inCov)
  {
    return;
  }

  svtkIdType nRow = inCov->GetNumberOfRows();
  if (!nRow)
  {
    // No statistics were calculated.
    return;
  }

  // Use this first model to initialize the aggregated one
  svtkTable* outCov = svtkTable::New();
  outCov->DeepCopy(inCov);

  // Now, loop over all remaining models and update aggregated each time
  while ((inMetaDO = inMetaColl->GetNextDataObject(it)))
  {
    // Verify that the current model is indeed contained in a multiblock data set
    inMeta = svtkMultiBlockDataSet::SafeDownCast(inMetaDO);
    if (!inMeta)
    {
      outCov->Delete();

      return;
    }

    // Verify that the current covariance matrix is indeed contained in a table
    inCov = svtkTable::SafeDownCast(inMeta->GetBlock(0));
    if (!inCov)
    {
      outCov->Delete();

      return;
    }

    if (inCov->GetNumberOfRows() != nRow)
    {
      // Models do not match
      outCov->Delete();

      return;
    }

    // Iterate over all model rows
    int inN, outN;
    double muFactor = 0.;
    double covFactor = 0.;
    std::vector<double> inMu, outMu;
    int j = 0;
    int k = 0;
    for (int r = 0; r < nRow; ++r)
    {
      // Verify that variable names match each other
      if (inCov->GetValueByName(r, SVTK_MULTICORRELATIVE_KEYCOLUMN1) !=
          outCov->GetValueByName(r, SVTK_MULTICORRELATIVE_KEYCOLUMN1) ||
        inCov->GetValueByName(r, SVTK_MULTICORRELATIVE_KEYCOLUMN2) !=
          outCov->GetValueByName(r, SVTK_MULTICORRELATIVE_KEYCOLUMN2))
      {
        // Models do not match
        outCov->Delete();

        return;
      }

      // Update each model parameter
      if (inCov->GetValueByName(r, SVTK_MULTICORRELATIVE_KEYCOLUMN1).ToString() == "Cardinality")
      {
        // Cardinality
        inN = inCov->GetValueByName(r, SVTK_MULTICORRELATIVE_ENTRIESCOL).ToInt();
        outN = outCov->GetValueByName(r, SVTK_MULTICORRELATIVE_ENTRIESCOL).ToInt();
        int totN = inN + outN;
        outCov->SetValueByName(r, SVTK_MULTICORRELATIVE_ENTRIESCOL, totN);
        muFactor = static_cast<double>(inN) / totN;
        covFactor = static_cast<double>(inN) * outN / totN;
      }
      else if (inCov->GetValueByName(r, SVTK_MULTICORRELATIVE_KEYCOLUMN2).ToString().empty())
      {
        // Mean
        inMu.push_back(inCov->GetValueByName(r, SVTK_MULTICORRELATIVE_ENTRIESCOL).ToDouble());
        outMu.push_back(outCov->GetValueByName(r, SVTK_MULTICORRELATIVE_ENTRIESCOL).ToDouble());
        outCov->SetValueByName(r, SVTK_MULTICORRELATIVE_ENTRIESCOL,
          outMu.back() + (inMu.back() - outMu.back()) * muFactor);
      }
      else
      {
        // M XY
        double inCovEntry = inCov->GetValueByName(r, SVTK_MULTICORRELATIVE_ENTRIESCOL).ToDouble();
        double outCovEntry = outCov->GetValueByName(r, SVTK_MULTICORRELATIVE_ENTRIESCOL).ToDouble();
        outCov->SetValueByName(r, SVTK_MULTICORRELATIVE_ENTRIESCOL,
          inCovEntry + outCovEntry + (inMu[j] - outMu[j]) * (inMu[k] - outMu[k]) * covFactor);
        ++k;
        if (k > j)
        {
          ++j;
          k = 0;
        }
      }
    }
  }

  // Replace covariance block of output model with updated one
  outMeta->SetBlock(0, outCov);

  // Clean up
  outCov->Delete();
}

// ----------------------------------------------------------------------
void svtkMultiCorrelativeStatistics::Learn(
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

  svtkTable* sparseCov = svtkTable::New();

  svtkStringArray* col1 = svtkStringArray::New();
  col1->SetName(SVTK_MULTICORRELATIVE_KEYCOLUMN1);
  sparseCov->AddColumn(col1);
  col1->Delete();

  svtkStringArray* col2 = svtkStringArray::New();
  col2->SetName(SVTK_MULTICORRELATIVE_KEYCOLUMN2);
  sparseCov->AddColumn(col2);
  col2->Delete();

  svtkDoubleArray* col3 = svtkDoubleArray::New();
  col3->SetName(SVTK_MULTICORRELATIVE_ENTRIESCOL);
  sparseCov->AddColumn(col3);
  col3->Delete();

  std::set<std::set<svtkStdString> >::const_iterator reqIt;
  std::set<svtkStdString>::const_iterator colIt;
  std::set<std::pair<svtkStdString, svtkDataArray*> > allColumns;
  std::map<std::pair<svtkIdType, svtkIdType>, svtkIdType> colPairs;
  std::map<std::pair<svtkIdType, svtkIdType>, svtkIdType>::iterator cpIt;
  std::map<svtkStdString, svtkIdType> colNameToIdx;
  std::vector<svtkDataArray*> colPtrs;

  // Populate a vector with pointers to columns of interest (i.e., columns from the input dataset
  // which have some statistics requested) and create a map from column names into this vector.
  // The first step is to create a set so that the vector entries will be sorted by name.
  for (reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end(); ++reqIt)
  {
    for (colIt = reqIt->begin(); colIt != reqIt->end(); ++colIt)
    {
      // Ignore invalid column names
      svtkDataArray* arr = svtkArrayDownCast<svtkDataArray>(inData->GetColumnByName(colIt->c_str()));
      if (arr)
      {
        allColumns.insert(std::pair<svtkStdString, svtkDataArray*>(*colIt, arr));
      }
    }
  }

  // Now make a map from input column name to output column index (colNameToIdx):
  svtkIdType i = 0;
  svtkIdType m = static_cast<svtkIdType>(allColumns.size());
  std::set<std::pair<svtkStdString, svtkDataArray*> >::const_iterator acIt;
  svtkStdString empty;
  col1->InsertNextValue("Cardinality");
  col2->InsertNextValue(empty);
  for (acIt = allColumns.begin(); acIt != allColumns.end(); ++acIt)
  {
    colNameToIdx[acIt->first] = i++;
    colPtrs.push_back(acIt->second);
    col1->InsertNextValue(acIt->second->GetName());
    col2->InsertNextValue(empty);
  }

  // Get a list of column pairs (across all requests) for which sums of squares will be computed.
  // This keeps us from computing the same covariance entry multiple times if several requests
  // contain common pairs of columns.
  i = m;

  // Loop over requests
  svtkIdType nRow = inData->GetNumberOfRows();
  for (reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end(); ++reqIt)
  {
    // For each column in the request:
    for (colIt = reqIt->begin(); colIt != reqIt->end(); ++colIt)
    {
      std::map<svtkStdString, svtkIdType>::iterator idxIt = colNameToIdx.find(*colIt);
      // Ignore invalid column names
      if (idxIt != colNameToIdx.end())
      {
        svtkIdType colA = idxIt->second;
        svtkStdString colAName = idxIt->first;
        std::set<svtkStdString>::const_iterator colIt2;
        for (colIt2 = colIt; colIt2 != reqIt->end(); ++colIt2)
        {
          idxIt = colNameToIdx.find(*colIt2);
          // Ignore invalid column names
          if (idxIt != colNameToIdx.end())
          { // Note that other requests may have inserted this entry.
            std::pair<svtkIdType, svtkIdType> entry(colA, idxIt->second);
            if (colPairs.find(entry) == colPairs.end())
            {
              // Point to the offset in col3 (below) for this column-pair sum:
              colPairs[entry] = -1;
            }
          }
        }
      }
    }
  }

  // Now insert the column pairs into col1 and col2 in the order in which they'll be evaluated.
  for (cpIt = colPairs.begin(); cpIt != colPairs.end(); ++cpIt)
  {
    cpIt->second = i++;
    col1->InsertNextValue(colPtrs[cpIt->first.first]->GetName());
    col2->InsertNextValue(colPtrs[cpIt->first.second]->GetName());
  }

  // Now (finally!) compute the covariance and column sums.
  // This uses the on-line algorithms for computing centered
  // moments and covariances from Philippe's SAND2008-6212 report.
  double* x;
  std::vector<double> v(m, 0.); // Values (v) for one observation

  // Storage pattern in primary statistics column:
  //  Row 0: cardinality of sample
  //  Rows 1 to m - 1: means of each variable
  //  Rows m to m + colPairs.size(): variances/covariances for each pair of variables
  col3->SetNumberOfTuples(static_cast<svtkIdType>(1 + m + colPairs.size()));
  col3->FillComponent(0, 0.);

  // Retrieve pointer to values and skip Cardinality entry
  double* rv = col3->GetPointer(0);
  *rv = static_cast<double>(nRow);
  ++rv;

  if (this->MedianAbsoluteDeviation)
  {
    // Computes the Median
    svtkNew<svtkTable> medianTable;
    this->ComputeMedian(inData, medianTable);
    // Sets the median
    x = rv;
    for (svtkIdType j = 0; j < m; ++j, ++x)
    {
      *x = medianTable->GetValue(1, j + 1).ToDouble();
    }

    // Computes the MAD inData (Median Absolute Deviation)
    svtkNew<svtkTable> inDataMAD;
    svtkIdType l = 0;
    // Iterate over column pairs
    for (cpIt = colPairs.begin(); cpIt != colPairs.end(); ++cpIt, ++x, ++l)
    {
      svtkIdType j = cpIt->first.first;
      svtkIdType k = cpIt->first.second;

      std::ostringstream nameStr;
      nameStr << "Cov{" << j << "," << k << "}";
      svtkNew<svtkDoubleArray> col;
      col->SetNumberOfTuples(nRow);
      col->SetName(nameStr.str().c_str());
      inDataMAD->AddColumn(col);
      // Iterate over rows
      for (i = 0; i < nRow; ++i)
      {
        inDataMAD->SetValue(i, l,
          (int)fabs((colPtrs[j]->GetTuple(i)[0] - rv[j]) * (colPtrs[k]->GetTuple(i)[0] - rv[k])));
      }
    }
    // Computes the MAD matrix
    svtkNew<svtkTable> MADTable;
    this->ComputeMedian(inDataMAD, MADTable);
    // Sets the MAD
    x = rv + m;
    // Iterate over column pairs
    for (l = 0, cpIt = colPairs.begin(); cpIt != colPairs.end(); ++cpIt, ++x, ++l)
    {
      *x = MADTable->GetValue(1, l + 1).ToDouble();
    }
  }
  else
  {
    // Iterate over rows
    for (i = 0; i < nRow; ++i)
    {
      // First fetch column values
      for (svtkIdType j = 0; j < m; ++j)
      {
        v[j] = colPtrs[j]->GetTuple(i)[0];
      }
      // Update column products. Equation 3.12 from the SAND report.
      x = rv + m;
      for (cpIt = colPairs.begin(); cpIt != colPairs.end(); ++cpIt, ++x)
      {
        // cpIt->first is a pair of indices into colPtrs used to specify (u,v) or (s,t)
        // cpIt->first.first is the index of u or s
        // cpIt->first.second is the index of v or t
        *x += (v[cpIt->first.first] - rv[cpIt->first.first]) * // \delta_{u,2,1} = s - \mu_{u,1}
          (v[cpIt->first.second] - rv[cpIt->first.second]) *   // \delta_{v,2,1} = t - \mu_{v,1}
          i / (i + 1.); // \frac{n_1 n_2}{n_1 + n_2} = \frac{n_1}{n_1 + 1}
      }
      // Update running column averages. Equation 1.1 from the SAND report.
      x = rv;
      for (svtkIdType j = 0; j < m; ++j, ++x)
      {
        *x += (v[j] - *x) / (i + 1);
      }
    }
  }

  outMeta->SetNumberOfBlocks(1);
  outMeta->SetBlock(0, sparseCov);
  outMeta->GetMetaData(static_cast<unsigned>(0))
    ->Set(svtkCompositeDataSet::NAME(), "Raw Sparse Covariance Data");
  sparseCov->Delete();
}

// ----------------------------------------------------------------------
static void svtkMultiCorrelativeCholesky(std::vector<double*>& a, svtkIdType m)
{
  // First define some macros to make the Cholevsky decomposition algorithm legible:
#ifdef A
#undef A
#endif
#ifdef L
#undef L
#endif
#define A(i, j) ((j) >= (i) ? a[j][i] : a[i][j])
#define L(i, j) a[j][(i) + 1]

  // Then perform decomposition
  double tmp;
  for (svtkIdType i = 0; i < m; ++i)
  {
    L(i, i) = A(i, i);
    for (svtkIdType k = 0; k < i; ++k)
    {
      tmp = L(i, k);
      L(i, i) -= tmp * tmp;
    }
    L(i, i) = sqrt(L(i, i));
    for (svtkIdType j = i + 1; j < m; ++j)
    {
      L(j, i) = A(j, i);
      for (svtkIdType k = 0; k < i; ++k)
      {
        L(j, i) -= L(j, k) * L(i, k);
      }
      L(j, i) /= L(i, i);
    }
  }
}

// ----------------------------------------------------------------------
void svtkMultiCorrelativeStatistics::Derive(svtkMultiBlockDataSet* outMeta)
{
  svtkTable* sparseCov;
  svtkStringArray* col1;
  svtkStringArray* col2;
  svtkDoubleArray* col3;
  if (!outMeta || !(sparseCov = svtkTable::SafeDownCast(outMeta->GetBlock(0))) ||
    !(col1 = svtkArrayDownCast<svtkStringArray>(
        sparseCov->GetColumnByName(SVTK_MULTICORRELATIVE_KEYCOLUMN1))) ||
    !(col2 = svtkArrayDownCast<svtkStringArray>(
        sparseCov->GetColumnByName(SVTK_MULTICORRELATIVE_KEYCOLUMN2))) ||
    !(col3 = svtkArrayDownCast<svtkDoubleArray>(
        sparseCov->GetColumnByName(SVTK_MULTICORRELATIVE_ENTRIESCOL))))
  {
    return;
  }

  std::set<std::set<svtkStdString> >::const_iterator reqIt;
  std::set<svtkStdString>::const_iterator colIt;
  std::map<std::pair<svtkIdType, svtkIdType>, svtkIdType> colPairs;
  std::map<svtkStdString, svtkIdType> colNameToIdx;
  // Reconstruct information about the computed sums from the raw data.
  // The first entry is always the sample size
  double n = col3->GetValue(0);
  svtkIdType m = 0;
  svtkIdType i;
  svtkIdType ncol3 = col3->GetNumberOfTuples();
  for (i = 1; i < ncol3 && col2->GetValue(i).empty(); ++i, ++m)
  {
    colNameToIdx[col1->GetValue(i)] = m;
  }
  for (; i < ncol3; ++i)
  {
    std::pair<svtkIdType, svtkIdType> idxs(
      colNameToIdx[col1->GetValue(i)], colNameToIdx[col2->GetValue(i)]);
    colPairs[idxs] = i - 1;
  }
  double* rv = col3->GetPointer(0) + 1;

  // Create an output table for each request and fill it in using the col3 array (the first table in
  // outMeta and which is presumed to exist upon entry to Derive).
  // Note that these tables are normalized by the number of samples.
  outMeta->SetNumberOfBlocks(1 + static_cast<unsigned int>(this->Internals->Requests.size()));

  // Keep track of last current block
  unsigned int b = 1;

  // Loop over requests
  double scale = 1. / (n - 1); // n -1 for unbiased variance estimators
  for (reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end();
       ++reqIt, ++b)
  {
    svtkStringArray* colNames = svtkStringArray::New();
    colNames->SetName(SVTK_MULTICORRELATIVE_COLUMNAMES);
    svtkDoubleArray* colAvgs = svtkDoubleArray::New();
    colAvgs->SetName(SVTK_MULTICORRELATIVE_AVERAGECOL);
    std::vector<svtkDoubleArray*> covCols;
    std::vector<double*> covPtrs;
    std::vector<int> covIdxs;
    std::ostringstream reqNameStr;
    reqNameStr << "Cov(";
    // For each column in the request:
    for (colIt = reqIt->begin(); colIt != reqIt->end(); ++colIt)
    {
      std::map<svtkStdString, svtkIdType>::iterator idxIt = colNameToIdx.find(*colIt);
      // Ignore invalid column names
      if (idxIt != colNameToIdx.end())
      {
        // Create a new column for the covariance matrix output
        covIdxs.push_back(idxIt->second);
        colNames->InsertNextValue(*colIt);
        svtkDoubleArray* arr = svtkDoubleArray::New();
        arr->SetName(colIt->c_str());
        covCols.push_back(arr);

        if (colIt == reqIt->begin())
        {
          reqNameStr << *colIt;
        }
        else
        {
          reqNameStr << "," << *colIt;
        }
      }
    } // colIt

    reqNameStr << ")";
    covCols.push_back(colAvgs);
    colNames->InsertNextValue(
      "Cholesky"); // Need extra row for lower-triangular Cholesky decomposition

    // We now have the total number of columns in the output
    // Allocate memory for the correct number of rows and fill in values
    svtkIdType reqCovSize = colNames->GetNumberOfTuples();
    colAvgs->SetNumberOfTuples(reqCovSize);

    // Prepare covariance table and store it as last current block
    svtkTable* covariance = svtkTable::New();
    covariance->AddColumn(colNames);
    covariance->AddColumn(colAvgs);
    outMeta->GetMetaData(b)->Set(svtkCompositeDataSet::NAME(), reqNameStr.str().c_str());
    outMeta->SetBlock(b, covariance);

    // Clean up
    covariance->Delete();
    colNames->Delete();
    colAvgs->Delete();

    svtkIdType j = 0;
    for (std::vector<svtkDoubleArray*>::iterator arrIt = covCols.begin(); arrIt != covCols.end();
         ++arrIt, ++j)
    {
      (*arrIt)->SetNumberOfTuples(reqCovSize);
      (*arrIt)->FillComponent(0, 0.);
      double* x = (*arrIt)->GetPointer(0);
      covPtrs.push_back(x);
      if (*arrIt != colAvgs)
      {
        // Column is part of covariance matrix
        covariance->AddColumn(*arrIt);
        (*arrIt)->Delete();
        for (svtkIdType k = 0; k <= j; ++k, ++x)
        {
          *x = rv[colPairs[std::pair<svtkIdType, svtkIdType>(covIdxs[k], covIdxs[j])]] * scale;
        }
      }
      else // if ( *arrIt != colAvgs )
      {
        // Column holds averages
        for (svtkIdType k = 0; k < reqCovSize - 1; ++k, ++x)
        {
          *x = rv[covIdxs[k]];
        }
        *x = static_cast<double>(n);
      }
    } // arrIt, j
    svtkMultiCorrelativeCholesky(covPtrs, reqCovSize - 1);
  } //  reqIt, b
}

// ----------------------------------------------------------------------
void svtkMultiCorrelativeStatistics::Assess(
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

  // For each request, add a column to the output data related to the probability
  // of observing each input datum with respect to the model in the request
  // NB: Column names of the metadata and input data are assumed to match
  // The output columns will be named "this->AssessNames->GetValue(0)(A,B,C)",
  // where "A", "B", and "C" are the column names specified in the per-request metadata tables.
  svtkIdType nRow = inData->GetNumberOfRows();
  int nb = static_cast<int>(inMeta->GetNumberOfBlocks());
  AssessFunctor* dfunc = nullptr;
  for (int req = 1; req < nb; ++req)
  {
    svtkTable* reqModel = svtkTable::SafeDownCast(inMeta->GetBlock(req));
    if (!reqModel)
    {
      // Silently skip invalid entries
      // NB: The assessValues column is left in output data even when empty
      continue;
    }

    this->SelectAssessFunctor(inData, reqModel, nullptr, dfunc);
    svtkMultiCorrelativeAssessFunctor* mcfunc =
      static_cast<svtkMultiCorrelativeAssessFunctor*>(dfunc);
    if (!mcfunc)
    {
      svtkWarningMacro("Request " << req - 1 << " could not be accommodated. Skipping.");
      delete dfunc;
      continue;
    }

    // Create the outData columns
    int nv = this->AssessNames->GetNumberOfValues();
    std::vector<svtkStdString> names(nv);
    for (int v = 0; v < nv; ++v)
    {
      std::ostringstream assessColName;
      assessColName << this->AssessNames->GetValue(v) << "(";
      for (int i = 0; i < mcfunc->GetNumberOfColumns(); ++i)
      {
        if (i > 0)
        {
          assessColName << ",";
        }
        assessColName << mcfunc->GetColumn(i)->GetName();
      }
      assessColName << ")";

      // Storing names to be able to use SetValueByName which is faster than SetValue
      svtkDoubleArray* assessValues = svtkDoubleArray::New();
      names[v] = assessColName.str().c_str();
      assessValues->SetName(names[v]);
      assessValues->SetNumberOfTuples(nRow);
      outData->AddColumn(assessValues);
      assessValues->Delete();
    }

    // Assess each entry of the column
    svtkDoubleArray* assessResult = svtkDoubleArray::New();
    for (svtkIdType r = 0; r < nRow; ++r)
    {
      (*dfunc)(assessResult, r);
      for (int v = 0; v < nv; ++v)
      {
        outData->SetValueByName(r, names[v], assessResult->GetValue(v));
      }
    }

    assessResult->Delete();

    delete dfunc;
  }
}

// ----------------------------------------------------------------------
void svtkMultiCorrelativeStatistics::ComputeMedian(svtkTable* inData, svtkTable* outData)
{
  svtkOrderStatistics* orderStats = this->CreateOrderStatisticsInstance();
  svtkNew<svtkTable> inOrderStats;
  orderStats->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, inOrderStats);
  for (svtkIdType i = 0; i < inData->GetNumberOfColumns(); ++i)
  {
    inOrderStats->AddColumn(inData->GetColumn(i));
    orderStats->AddColumn(inData->GetColumn(i)->GetName());
  }
  orderStats->SetNumberOfIntervals(2);
  orderStats->SetLearnOption(true);
  orderStats->SetDeriveOption(true);
  orderStats->SetTestOption(false);
  orderStats->SetAssessOption(false);
  orderStats->Update();

  // Get the Median
  svtkMultiBlockDataSet* outputOrderStats = svtkMultiBlockDataSet::SafeDownCast(
    orderStats->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));
  outData->ShallowCopy(
    svtkTable::SafeDownCast(outputOrderStats->GetBlock(outputOrderStats->GetNumberOfBlocks() - 1)));

  orderStats->Delete();
}

// ----------------------------------------------------------------------
svtkOrderStatistics* svtkMultiCorrelativeStatistics::CreateOrderStatisticsInstance()
{
  return svtkOrderStatistics::New();
}

// ----------------------------------------------------------------------
svtkMultiCorrelativeAssessFunctor* svtkMultiCorrelativeAssessFunctor::New()
{
  return new svtkMultiCorrelativeAssessFunctor;
}

// ----------------------------------------------------------------------
bool svtkMultiCorrelativeAssessFunctor::Initialize(
  svtkTable* inData, svtkTable* reqModel, bool cholesky)
{
  svtkDoubleArray* avgs =
    svtkArrayDownCast<svtkDoubleArray>(reqModel->GetColumnByName(SVTK_MULTICORRELATIVE_AVERAGECOL));
  if (!avgs)
  {
    svtkGenericWarningMacro(
      "Multicorrelative request without a \"" SVTK_MULTICORRELATIVE_AVERAGECOL "\" column");
    return false;
  }
  svtkStringArray* name =
    svtkArrayDownCast<svtkStringArray>(reqModel->GetColumnByName(SVTK_MULTICORRELATIVE_COLUMNAMES));
  if (!name)
  {
    svtkGenericWarningMacro(
      "Multicorrelative request without a \"" SVTK_MULTICORRELATIVE_COLUMNAMES "\" column");
    return false;
  }

  // Storage for input data columns
  std::vector<svtkDataArray*> cols;

  // Storage for Cholesky matrix columns
  // NB: Only the lower triangle is significant
  std::vector<double*> chol;
  svtkIdType m = reqModel->GetNumberOfColumns() - 2;
  svtkIdType i;
  for (i = 0; i < m; ++i)
  {
    svtkStdString colname(name->GetValue(i));
    svtkDataArray* arr = svtkArrayDownCast<svtkDataArray>(inData->GetColumnByName(colname.c_str()));
    if (!arr)
    {
      svtkGenericWarningMacro(
        "Multicorrelative input data needs a \"" << colname.c_str() << "\" column");
      return false;
    }
    cols.push_back(arr);
    svtkDoubleArray* dar =
      svtkArrayDownCast<svtkDoubleArray>(reqModel->GetColumnByName(colname.c_str()));
    if (!dar)
    {
      svtkGenericWarningMacro(
        "Multicorrelative request needs a \"" << colname.c_str() << "\" column");
      return false;
    }
    chol.push_back(dar->GetPointer(1));
  }

  // OK, if we made it this far, we will succeed
  this->Columns = cols;
  this->Center = avgs->GetPointer(0);
  this->Tuple.resize(m);
  this->EmptyTuple = std::vector<double>(m, 0.);
  if (cholesky)
  {
    // Store the inverse of chol in this->Factor, F
    svtkMultiCorrelativeInvertCholesky(chol, this->Factor);
    // transpose F to make it easier to use in the () operator
    svtkMultiCorrelativeTransposeTriangular(this->Factor, m);
  }
#if 0
  // Compute the normalization factor to turn X * F * F' * X' into a cumulance
  if ( m % 2 == 0 )
  {
    this->Normalization = 1.0;
    for ( i = m / 2 - 1; i > 1; -- i )
    {
      this->Normalization *= i;
    }
  }
  else
  {
    this->Normalization = sqrt( svtkMath::Pi() ) / ( 1 << ( m / 2 ) );
    for ( i = m - 2; i > 1; i -= 2 )
    {
      this->Normalization *= i;
    }
  }
#endif // 0

  return true;
}

// ----------------------------------------------------------------------
void svtkMultiCorrelativeStatistics::SelectAssessFunctor(svtkTable* inData, svtkDataObject* inMetaDO,
  svtkStringArray* svtkNotUsed(rowNames), AssessFunctor*& dfunc)
{
  dfunc = nullptr;
  svtkTable* reqModel = svtkTable::SafeDownCast(inMetaDO);
  if (!reqModel)
  {
    return;
  }

  svtkMultiCorrelativeAssessFunctor* mcfunc = svtkMultiCorrelativeAssessFunctor::New();
  if (!mcfunc->Initialize(inData, reqModel))
  {
    delete mcfunc;
    return;
  }
  dfunc = mcfunc;
}
