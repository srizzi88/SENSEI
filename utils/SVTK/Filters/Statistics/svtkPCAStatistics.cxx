#include "svtkPCAStatistics.h"

#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiCorrelativeStatisticsAssessFunctor.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

#include <map>
#include <sstream>
#include <vector>

#include "svtk_eigen.h"
#include SVTK_EIGEN(Dense)

// To Do:
// - Add option to pre-multiply EigenVectors by normalization coeffs
// - In svtkPCAAssessFunctor, pre-multiply EigenVectors by normalization coeffs (if req)
// -

#define SVTK_PCA_NORMCOLUMN "PCA Cov Norm"
#define SVTK_PCA_COMPCOLUMN "PCA"

svtkObjectFactoryNewMacro(svtkPCAStatistics);

const char* svtkPCAStatistics::NormalizationSchemeEnumNames[NUM_NORMALIZATION_SCHEMES + 1] = {
  "None", "TriangleSpecified", "DiagonalSpecified", "DiagonalVariance", "InvalidNormalizationScheme"
};

const char* svtkPCAStatistics::BasisSchemeEnumNames[NUM_BASIS_SCHEMES + 1] = { "FullBasis",
  "FixedBasisSize", "FixedBasisEnergy", "InvalidBasisScheme" };

// ----------------------------------------------------------------------
void svtkPCAStatistics::GetEigenvalues(int request, svtkDoubleArray* eigenvalues)
{
  svtkSmartPointer<svtkMultiBlockDataSet> outputMetaDS = svtkMultiBlockDataSet::SafeDownCast(
    this->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));

  if (!outputMetaDS)
  {
    svtkErrorMacro(<< "nullptr dataset pointer!");
  }

  svtkSmartPointer<svtkTable> outputMeta =
    svtkTable::SafeDownCast(outputMetaDS->GetBlock(request + 1));

  if (!outputMetaDS)
  {
    svtkErrorMacro(<< "nullptr table pointer!");
  }

  svtkDoubleArray* meanCol = svtkArrayDownCast<svtkDoubleArray>(outputMeta->GetColumnByName("Mean"));
  svtkStringArray* rowNames =
    svtkArrayDownCast<svtkStringArray>(outputMeta->GetColumnByName("Column"));

  eigenvalues->SetNumberOfComponents(1);

  // Get values
  int eval = 0;
  for (svtkIdType i = 0; i < meanCol->GetNumberOfTuples(); i++)
  {
    std::stringstream ss;
    ss << "PCA " << eval;

    std::string rowName = rowNames->GetValue(i);
    if (rowName.compare(ss.str()) == 0)
    {
      eigenvalues->InsertNextValue(meanCol->GetValue(i));
      eval++;
    }
  }
}

// ----------------------------------------------------------------------
double svtkPCAStatistics::GetEigenvalue(int request, int i)
{
  svtkSmartPointer<svtkDoubleArray> eigenvalues = svtkSmartPointer<svtkDoubleArray>::New();
  this->GetEigenvalues(request, eigenvalues);
  return eigenvalues->GetValue(i);
}

// ----------------------------------------------------------------------
void svtkPCAStatistics::GetEigenvalues(svtkDoubleArray* eigenvalues)
{
  this->GetEigenvalues(0, eigenvalues);
}

// ----------------------------------------------------------------------
double svtkPCAStatistics::GetEigenvalue(int i)
{
  return this->GetEigenvalue(0, i);
}

// ----------------------------------------------------------------------
void svtkPCAStatistics::GetEigenvectors(int request, svtkDoubleArray* eigenvectors)
{
  // Count eigenvalues
  svtkSmartPointer<svtkDoubleArray> eigenvalues = svtkSmartPointer<svtkDoubleArray>::New();
  this->GetEigenvalues(request, eigenvalues);
  svtkIdType numberOfEigenvalues = eigenvalues->GetNumberOfTuples();

  svtkSmartPointer<svtkMultiBlockDataSet> outputMetaDS = svtkMultiBlockDataSet::SafeDownCast(
    this->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));

  if (!outputMetaDS)
  {
    svtkErrorMacro(<< "nullptr dataset pointer!");
  }

  svtkSmartPointer<svtkTable> outputMeta =
    svtkTable::SafeDownCast(outputMetaDS->GetBlock(request + 1));

  if (!outputMeta)
  {
    svtkErrorMacro(<< "nullptr table pointer!");
  }

  svtkDoubleArray* meanCol = svtkArrayDownCast<svtkDoubleArray>(outputMeta->GetColumnByName("Mean"));
  svtkStringArray* rowNames =
    svtkArrayDownCast<svtkStringArray>(outputMeta->GetColumnByName("Column"));

  eigenvectors->SetNumberOfComponents(numberOfEigenvalues);

  // Get vectors
  int eval = 0;
  for (svtkIdType i = 0; i < meanCol->GetNumberOfTuples(); i++)
  {
    std::stringstream ss;
    ss << "PCA " << eval;

    std::string rowName = rowNames->GetValue(i);
    if (rowName.compare(ss.str()) == 0)
    {
      std::vector<double> eigenvector;
      for (int val = 0; val < numberOfEigenvalues; val++)
      {
        // The first two columns will always be "Column" and "Mean", so start with the next one
        svtkDoubleArray* currentCol =
          svtkArrayDownCast<svtkDoubleArray>(outputMeta->GetColumn(val + 2));
        eigenvector.push_back(currentCol->GetValue(i));
      }

      eigenvectors->InsertNextTypedTuple(&eigenvector.front());
      eval++;
    }
  }
}

// ----------------------------------------------------------------------
void svtkPCAStatistics::GetEigenvectors(svtkDoubleArray* eigenvectors)
{
  this->GetEigenvectors(0, eigenvectors);
}

// ----------------------------------------------------------------------
void svtkPCAStatistics::GetEigenvector(int request, int i, svtkDoubleArray* eigenvector)
{
  svtkSmartPointer<svtkDoubleArray> eigenvectors = svtkSmartPointer<svtkDoubleArray>::New();
  this->GetEigenvectors(request, eigenvectors);

  std::vector<double> evec(eigenvectors->GetNumberOfComponents());
  eigenvectors->GetTypedTuple(i, evec.data());

  eigenvector->Reset();
  eigenvector->Squeeze();
  eigenvector->SetNumberOfComponents(eigenvectors->GetNumberOfComponents());
  eigenvector->InsertNextTypedTuple(evec.data());
}

// ----------------------------------------------------------------------
void svtkPCAStatistics::GetEigenvector(int i, svtkDoubleArray* eigenvector)
{
  this->GetEigenvector(0, i, eigenvector);
}

// ======================================================== svtkPCAAssessFunctor
class svtkPCAAssessFunctor : public svtkMultiCorrelativeAssessFunctor
{
public:
  static svtkPCAAssessFunctor* New();

  svtkPCAAssessFunctor() = default;
  ~svtkPCAAssessFunctor() override = default;
  virtual bool InitializePCA(svtkTable* inData, svtkTable* reqModel, int normScheme, int basisScheme,
    int basisSize, double basisEnergy);

  void operator()(svtkDoubleArray* result, svtkIdType row) override;

  std::vector<double> EigenValues;
  std::vector<std::vector<double> > EigenVectors;
  svtkIdType BasisSize;
};

// ----------------------------------------------------------------------
svtkPCAAssessFunctor* svtkPCAAssessFunctor::New()
{
  return new svtkPCAAssessFunctor;
}

// ----------------------------------------------------------------------
bool svtkPCAAssessFunctor::InitializePCA(svtkTable* inData, svtkTable* reqModel, int normScheme,
  int basisScheme, int fixedBasisSize, double fixedBasisEnergy)
{
  if (!this->svtkMultiCorrelativeAssessFunctor::Initialize(
        inData, reqModel, false /* no Cholesky decomp */))
  {
    return false;
  }

  // Put the PCA basis into a matrix form we can use.
  svtkIdType m = reqModel->GetNumberOfColumns() - 2;
  svtkDoubleArray* evalm =
    svtkArrayDownCast<svtkDoubleArray>(reqModel->GetColumnByName(SVTK_MULTICORRELATIVE_AVERAGECOL));
  if (!evalm)
  {
    svtkGenericWarningMacro("No \"" SVTK_MULTICORRELATIVE_AVERAGECOL "\" column in request.");
    return false;
  }

  // Check that the derived model includes additional rows specifying the
  // normalization as required.
  svtkIdType mrmr = reqModel->GetNumberOfRows(); // actual number of rows
  svtkIdType ermr;                               // expected number of rows
  switch (normScheme)
  {
    case svtkPCAStatistics::NONE:
      // m+1 covariance/Cholesky rows, m eigenvector rows, no normalization factors
      ermr = 2 * m + 1;
      break;
    case svtkPCAStatistics::DIAGONAL_SPECIFIED:
    case svtkPCAStatistics::DIAGONAL_VARIANCE:
      // m+1 covariance/Cholesky rows, m eigenvector rows, 1 normalization factor row
      ermr = 2 * m + 2;
      break;
    case svtkPCAStatistics::TRIANGLE_SPECIFIED:
      // m+1 covariance/Cholesky rows, m eigenvector rows, m normalization factor rows
      ermr = 3 * m + 1;
      break;
    case svtkPCAStatistics::NUM_NORMALIZATION_SCHEMES:
    default:
    {
      svtkGenericWarningMacro(
        "The normalization scheme specified (" << normScheme << ") is invalid.");
      return false;
    }
  }

  // Allow derived classes to add rows, but never allow fewer than required.
  if (mrmr < ermr)
  {
    svtkGenericWarningMacro("Expected " << (2 * m + 1) << " or more columns in request but found "
                                       << reqModel->GetNumberOfRows() << ".");
    return false;
  }

  // OK, we got this far; we should succeed.
  svtkIdType i, j;
  double eigSum = 0.;
  for (i = 0; i < m; ++i)
  {
    double eigVal = evalm->GetValue(m + 1 + i);
    eigSum += eigVal;
    this->EigenValues.push_back(eigVal);
  }
  this->BasisSize = -1;
  switch (basisScheme)
  {
    case svtkPCAStatistics::NUM_BASIS_SCHEMES:
    default:
      svtkGenericWarningMacro("Unknown basis scheme " << basisScheme << ". Using FULL_BASIS.");
      SVTK_FALLTHROUGH;
    case svtkPCAStatistics::FULL_BASIS:
      this->BasisSize = m;
      break;
    case svtkPCAStatistics::FIXED_BASIS_SIZE:
      this->BasisSize = fixedBasisSize;
      break;
    case svtkPCAStatistics::FIXED_BASIS_ENERGY:
    {
      double frac = 0.;
      for (i = 0; i < m; ++i)
      {
        frac += this->EigenValues[i] / eigSum;
        if (frac > fixedBasisEnergy)
        {
          this->BasisSize = i + 1;
          break;
        }
      }
      if (this->BasisSize < 0)
      { // OK, it takes all the eigenvectors to approximate that well...
        this->BasisSize = m;
      }
    }
    break;
  }

  // FIXME: Offer mode to include normalization factors (none,diag,triang)?
  // Could be done here by pre-multiplying this->EigenVectors by factors.
  for (i = 0; i < this->BasisSize; ++i)
  {
    std::vector<double> evec;
    for (j = 0; j < m; ++j)
    {
      evec.push_back(reqModel->GetValue(m + 1 + i, j + 2).ToDouble());
    }
    this->EigenVectors.push_back(evec);
  }
  return true;
}

// ----------------------------------------------------------------------
void svtkPCAAssessFunctor::operator()(svtkDoubleArray* result, svtkIdType row)
{
  svtkIdType i;
  result->SetNumberOfValues(this->BasisSize);
  std::vector<std::vector<double> >::iterator it;
  svtkIdType m = this->GetNumberOfColumns();
  for (i = 0; i < m; ++i)
  {
    this->Tuple[i] = this->Columns[i]->GetTuple(row)[0] - this->Center[i];
  }
  i = 0;
  for (it = this->EigenVectors.begin(); it != this->EigenVectors.end(); ++it, ++i)
  {
    double cv = 0.;
    std::vector<double>::iterator tvit;
    std::vector<double>::iterator evit = this->Tuple.begin();
    for (tvit = it->begin(); tvit != it->end(); ++tvit, ++evit)
    {
      cv += (*evit) * (*tvit);
    }
    result->SetValue(i, cv);
  }
}

// ======================================================== svtkPCAStatistics
svtkPCAStatistics::svtkPCAStatistics()
{
  this->SetNumberOfInputPorts(4); // last port is for normalization coefficients.
  this->NormalizationScheme = NONE;
  this->BasisScheme = FULL_BASIS;
  this->FixedBasisSize = -1;
  this->FixedBasisEnergy = 1.;
}

// ----------------------------------------------------------------------
svtkPCAStatistics::~svtkPCAStatistics() = default;

// ----------------------------------------------------------------------
void svtkPCAStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "NormalizationScheme: " << this->GetNormalizationSchemeName(this->NormalizationScheme)
     << "\n";
  os << indent << "BasisScheme: " << this->GetBasisSchemeName(this->BasisScheme) << "\n";
  os << indent << "FixedBasisSize: " << this->FixedBasisSize << "\n";
  os << indent << "FixedBasisEnergy: " << this->FixedBasisEnergy << "\n";
}

// ----------------------------------------------------------------------
bool svtkPCAStatistics::SetParameter(const char* parameter, int svtkNotUsed(index), svtkVariant value)
{
  if (!strcmp(parameter, "NormalizationScheme"))
  {
    this->SetNormalizationScheme(value.ToInt());

    return true;
  }

  if (!strcmp(parameter, "BasisScheme"))
  {
    this->SetBasisScheme(value.ToInt());

    return true;
  }

  if (!strcmp(parameter, "FixedBasisSize"))
  {
    this->SetFixedBasisSize(value.ToInt());

    return true;
  }

  if (!strcmp(parameter, "FixedBasisEnergy"))
  {
    this->SetFixedBasisEnergy(value.ToDouble());

    return true;
  }

  return false;
}

// ----------------------------------------------------------------------
const char* svtkPCAStatistics::GetNormalizationSchemeName(int schemeIndex)
{
  if (schemeIndex < 0 || schemeIndex > NUM_NORMALIZATION_SCHEMES)
  {
    return svtkPCAStatistics::NormalizationSchemeEnumNames[NUM_NORMALIZATION_SCHEMES];
  }
  return svtkPCAStatistics::NormalizationSchemeEnumNames[schemeIndex];
}

// ----------------------------------------------------------------------
void svtkPCAStatistics::SetNormalizationSchemeByName(const char* schemeName)
{
  for (int i = 0; i < NUM_NORMALIZATION_SCHEMES; ++i)
  {
    if (!strcmp(svtkPCAStatistics::NormalizationSchemeEnumNames[i], schemeName))
    {
      this->SetNormalizationScheme(i);
      return;
    }
  }
  svtkErrorMacro("Invalid normalization scheme name \"" << schemeName << "\" provided.");
}

// ----------------------------------------------------------------------
svtkTable* svtkPCAStatistics::GetSpecifiedNormalization()
{
  return svtkTable::SafeDownCast(this->GetInputDataObject(3, 0));
}

void svtkPCAStatistics::SetSpecifiedNormalization(svtkTable* normSpec)
{
  this->SetInputData(3, normSpec);
}

// ----------------------------------------------------------------------
const char* svtkPCAStatistics::GetBasisSchemeName(int schemeIndex)
{
  if (schemeIndex < 0 || schemeIndex > NUM_BASIS_SCHEMES)
  {
    return svtkPCAStatistics::BasisSchemeEnumNames[NUM_BASIS_SCHEMES];
  }
  return svtkPCAStatistics::BasisSchemeEnumNames[schemeIndex];
}

// ----------------------------------------------------------------------
void svtkPCAStatistics::SetBasisSchemeByName(const char* schemeName)
{
  for (int i = 0; i < NUM_BASIS_SCHEMES; ++i)
  {
    if (!strcmp(svtkPCAStatistics::BasisSchemeEnumNames[i], schemeName))
    {
      this->SetBasisScheme(i);
      return;
    }
  }
  svtkErrorMacro("Invalid basis scheme name \"" << schemeName << "\" provided.");
}

// ----------------------------------------------------------------------
int svtkPCAStatistics::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 3)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return this->Superclass::FillInputPortInformation(port, info);
}

// ----------------------------------------------------------------------
static void svtkPCAStatisticsNormalizeSpec(svtkVariantArray* normData, Eigen::MatrixXd& cov,
  svtkTable* normSpec, svtkTable* reqModel, bool triangle)
{
  svtkIdType i, j;
  svtkIdType m = reqModel->GetNumberOfColumns() - 2;
  std::map<svtkStdString, svtkIdType> colNames;
  // Get a list of columns of interest for this request
  for (i = 0; i < m; ++i)
  {
    colNames[reqModel->GetColumn(i + 2)->GetName()] = i;
  }
  // Turn normSpec into a useful array.
  std::map<std::pair<svtkIdType, svtkIdType>, double> factor;
  svtkIdType n = normSpec->GetNumberOfRows();
  for (svtkIdType r = 0; r < n; ++r)
  {
    std::map<svtkStdString, svtkIdType>::iterator it;
    if ((it = colNames.find(normSpec->GetValue(r, 0).ToString())) == colNames.end())
    {
      continue;
    }
    i = it->second;
    if ((it = colNames.find(normSpec->GetValue(r, 1).ToString())) == colNames.end())
    {
      continue;
    }
    j = it->second;
    if (j < i)
    {
      svtkIdType tmp = i;
      i = j;
      j = tmp;
    }
    factor[std::pair<svtkIdType, svtkIdType>(i, j)] = normSpec->GetValue(r, 2).ToDouble();
  }
  // Now normalize cov, recording any missing factors along the way.
  std::ostringstream missing;
  bool gotMissing = false;
  std::map<std::pair<svtkIdType, svtkIdType>, double>::iterator fit;
  if (triangle)
  { // Normalization factors are provided for the upper triangular portion of the covariance matrix.
    for (i = 0; i < m; ++i)
    {
      for (j = i; j < m; ++j)
      {
        double v;
        fit = factor.find(std::pair<svtkIdType, svtkIdType>(i, j));
        if (fit == factor.end())
        {
          v = 1.;
          gotMissing = true;
          missing << "(" << reqModel->GetColumn(i + 2)->GetName() << ","
                  << reqModel->GetColumn(j + 2)->GetName() << ") ";
        }
        else
        {
          v = fit->second;
        }
        normData->InsertNextValue(v);
        cov(i, j) /= v;
        if (i != j)
        { // don't normalize diagonal entries twice
          cov(j, i) /= v;
        }
      }
    }
  }
  else
  { // Only diagonal normalization factors are supplied. Off-diagonals are the product of diagonals.
    for (i = 0; i < m; ++i)
    {
      double v;
      double vsq;
      fit = factor.find(std::pair<svtkIdType, svtkIdType>(i, i));
      if (fit == factor.end())
      {
        vsq = v = 1.;
        gotMissing = true;
        missing << "(" << reqModel->GetColumn(i + 2)->GetName() << ","
                << reqModel->GetColumn(i + 2)->GetName() << ") ";
      }
      else
      {
        vsq = fit->second;
        v = sqrt(vsq);
      }
      normData->InsertNextValue(vsq);
      // normalization factor applied up and to the left.
      for (j = 0; j < i; ++j)
      {
        cov(i, j) /= v;
        cov(j, i) /= v;
      }
      // normalization factor applied down and to the right.
      for (j = i + 1; j < m; ++j)
      {
        cov(i, j) /= v;
        cov(j, i) /= v;
      }
      cov(i, i) /= vsq;
    }
  }
  if (gotMissing)
  {
    svtkGenericWarningMacro(
      "The following normalization factors were expected but not provided: " << missing.str());
  }
}

// ----------------------------------------------------------------------
static void svtkPCAStatisticsNormalizeVariance(svtkVariantArray* normData, Eigen::MatrixXd& cov)
{
  svtkIdType i, j;
  svtkIdType m = cov.rows();
  for (i = 0; i < m; ++i)
  {
    normData->InsertNextValue(cov(i, i));
    double norm = sqrt(cov(i, i));
    // normalization factor applied down and to the right.
    for (j = i + 1; j < m; ++j)
    {
      cov(i, j) /= norm;
      cov(j, i) /= norm;
    }
    // normalization factor applied up and to the left.
    for (j = 0; j < i; ++j)
    {
      cov(i, j) /= norm;
      cov(j, i) /= norm;
    }
    cov(i, i) = 1.;
  }
}

// ----------------------------------------------------------------------
void svtkPCAStatistics::Derive(svtkMultiBlockDataSet* inMeta)
{
  if (!inMeta)
  {
    return;
  }

  // Use the parent class to compute a covariance matrix for each request.
  this->Superclass::Derive(inMeta);

  // Now that we have the covariance matrices, compute the SVD of each.
  svtkIdType nb = static_cast<svtkIdType>(inMeta->GetNumberOfBlocks());
  for (svtkIdType b = 1; b < nb; ++b)
  {
    svtkTable* reqModel = svtkTable::SafeDownCast(inMeta->GetBlock(b));
    if (!reqModel)
    {
      continue;
    }
    svtkIdType m = reqModel->GetNumberOfColumns() - 2;
    Eigen::MatrixXd cov(m, m);
    // Fill the cov array with values from the svtkTable
    svtkIdType i, j;
    for (j = 2; j < 2 + m; ++j)
    {
      for (i = 0; i < j - 1; ++i)
      {
        cov(i, j - 2) = reqModel->GetValue(i, j).ToDouble();
      }
    }
    // Fill in the lower triangular portion of the matrix
    for (j = 0; j < m - 1; ++j)
    {
      for (i = j; i < m; ++i)
      {
        cov(i, j) = cov(j, i);
      }
    }
    // If normalization of the covariance array is requested, perform it:
    svtkVariantArray* normData = svtkVariantArray::New();
    switch (this->NormalizationScheme)
    {
      case TRIANGLE_SPECIFIED:
      case DIAGONAL_SPECIFIED:
        svtkPCAStatisticsNormalizeSpec(normData, cov, this->GetSpecifiedNormalization(), reqModel,
          this->NormalizationScheme == TRIANGLE_SPECIFIED);
        break;
      case DIAGONAL_VARIANCE:
        svtkPCAStatisticsNormalizeVariance(normData, cov);
        break;
      case NONE:
      case NUM_NORMALIZATION_SCHEMES:
      default:
        // do nothing
        break;
    }
    Eigen::BDCSVD<Eigen::MatrixXd> svd(cov, Eigen::ComputeFullU);
    const Eigen::MatrixXd& u = svd.matrixU();
    const Eigen::MatrixXd& s = svd.singularValues();
    svtkVariantArray* row = svtkVariantArray::New();
    // row->SetNumberOfComponents( m + 2 );
    // row->SetNumberOfTuples( 1 );
    row->SetNumberOfComponents(1);
    row->SetNumberOfTuples(m + 2);
    for (i = 0; i < m; ++i)
    {
      std::ostringstream pcaCompName;
      pcaCompName << SVTK_PCA_COMPCOLUMN << " " << i;
      row->SetValue(0, pcaCompName.str().c_str());
      row->SetValue(1, s(i));
      for (j = 0; j < m; ++j)
      {
        // transpose the matrix so basis is row vectors (and thus
        // eigenvalues are to the left of their eigenvectors):
        row->SetValue(j + 2, u(j, i));
      }
      reqModel->InsertNextRow(row);
    }
    // Now insert the subset of the normalization data we used to
    // process this request at the bottom of the results.
    switch (this->NormalizationScheme)
    {
      case TRIANGLE_SPECIFIED:
        for (i = 0; i < m; ++i)
        {
          std::ostringstream normCompName;
          normCompName << SVTK_PCA_NORMCOLUMN << " " << i;
          row->SetValue(0, normCompName.str().c_str());
          row->SetValue(1, 0.);
          for (j = 0; j < i; ++j)
          {
            row->SetValue(j + 2, 0.);
          }
          for (; j < m; ++j)
          {
            row->SetValue(j + 2, normData->GetValue(j));
          }
          reqModel->InsertNextRow(row);
        }
        break;
      case DIAGONAL_SPECIFIED:
      case DIAGONAL_VARIANCE:
      {
        row->SetValue(0, SVTK_PCA_NORMCOLUMN);
        row->SetValue(1, 0.);
        for (j = 0; j < m; ++j)
        {
          row->SetValue(j + 2, normData->GetValue(j));
        }
        reqModel->InsertNextRow(row);
      }
      break;
      case NONE:
      case NUM_NORMALIZATION_SCHEMES:
      default:
        // do nothing
        break;
    }
    normData->Delete();
    row->Delete();
  }
}
// Use the invalid value of -1 for p-values if R is absent
svtkDoubleArray* svtkPCAStatistics::CalculatePValues(
  svtkIdTypeArray* svtkNotUsed(dimCol), svtkDoubleArray* statCol)
{
  // A column must be created first
  svtkDoubleArray* testCol = svtkDoubleArray::New();

  // Fill this column
  svtkIdType n = statCol->GetNumberOfTuples();
  testCol->SetNumberOfTuples(n);
  for (svtkIdType r = 0; r < n; ++r)
  {
    testCol->SetTuple1(r, -1);
  }

  return testCol;
}

// ----------------------------------------------------------------------
void svtkPCAStatistics::Test(svtkTable* inData, svtkMultiBlockDataSet* inMeta, svtkTable* outMeta)

{
  if (!inMeta)
  {
    return;
  }

  if (!outMeta)
  {
    return;
  }

  // Prepare columns for the test:
  // 0: (derived) model block index
  // 1: multivariate Srivastava skewness
  // 2: multivariate Srivastava kurtosis
  // 3: multivariate Jarque-Bera-Srivastava statistic
  // 4: multivariate Jarque-Bera-Srivastava p-value (calculated only if R is available, filled with
  // -1 otherwise) 5: number of degrees of freedom of Chi square distribution NB: These are not
  // added to the output table yet, for they will be filled individually first
  //     in order that R be invoked only once.
  svtkIdTypeArray* blockCol = svtkIdTypeArray::New();
  blockCol->SetName("Block");

  svtkDoubleArray* bS1Col = svtkDoubleArray::New();
  bS1Col->SetName("Srivastava Skewness");

  svtkDoubleArray* bS2Col = svtkDoubleArray::New();
  bS2Col->SetName("Srivastava Kurtosis");

  svtkDoubleArray* statCol = svtkDoubleArray::New();
  statCol->SetName("Jarque-Bera-Srivastava");

  svtkIdTypeArray* dimCol = svtkIdTypeArray::New();
  dimCol->SetName("d");

  // Retain data cardinality to check that models are applicable
  svtkIdType nRowData = inData->GetNumberOfRows();

  // Now iterate over model blocks
  unsigned int nBlocks = inMeta->GetNumberOfBlocks();
  for (unsigned int b = 1; b < nBlocks; ++b)
  {
    svtkTable* derivedTab = svtkTable::SafeDownCast(inMeta->GetBlock(b));

    // Silently ignore empty blocks
    if (!derivedTab)
    {
      continue;
    }

    // Figure out dimensionality; it is assumed that the 2 first columns
    // are what they should be: namely, Column and Mean.
    int p = derivedTab->GetNumberOfColumns() - 2;

    // Return informative message when cardinalities do not match.
    if (derivedTab->GetValueByName(p, "Mean").ToInt() != nRowData)
    {
      svtkWarningMacro("Inconsistent input: input data has "
        << nRowData << " rows but primary model has cardinality "
        << derivedTab->GetValueByName(p, "Mean").ToInt() << " for block " << b << ". Cannot test.");
      continue;
    }

    // Create and fill entries of name and mean vectors
    std::vector<svtkStdString> varNameX(p);
    std::vector<double> mX(p);
    for (int i = 0; i < p; ++i)
    {
      varNameX[i] = derivedTab->GetValueByName(i, "Column").ToString();
      mX[i] = derivedTab->GetValueByName(i, "Mean").ToDouble();
    }

    // Create and fill entries of eigenvalue vector and change of basis matrix
    std::vector<double> wX(p);
    std::vector<double> P(p * p);
    for (int i = 0; i < p; ++i)
    {
      // Skip p + 1 (Means and Cholesky) rows and 1 column (Column)
      wX[i] = derivedTab->GetValue(i + p + 1, 1).ToDouble();

      for (int j = 0; j < p; ++j)
      {
        // Skip p + 1 (Means and Cholesky) rows and 2 columns (Column and Mean)
        P[p * i + j] = derivedTab->GetValue(i + p + 1, j + 2).ToDouble();
      }
    }

    // Now iterate over all observations
    double tmp, t;
    std::vector<double> x(p);
    std::vector<double> sum3(p);
    std::vector<double> sum4(p);
    for (int i = 0; i < p; ++i)
    {
      sum3[i] = 0.;
      sum4[i] = 0.;
    }
    for (svtkIdType r = 0; r < nRowData; ++r)
    {
      // Read and center observation
      for (int i = 0; i < p; ++i)
      {
        x[i] = inData->GetValueByName(r, varNameX[i]).ToDouble() - mX[i];
      }

      // Now calculate skewness and kurtosis per component
      for (int i = 0; i < p; ++i)
      {
        // Transform coordinate into eigencoordinates
        t = 0.;
        for (int j = 0; j < p; ++j)
        {
          // Pij = P[p*i+j]
          t += P[p * i + j] * x[j];
        }

        // Update third and fourth order sums for each eigencoordinate
        tmp = t * t;
        sum3[i] += tmp * t;
        sum4[i] += tmp * tmp;
      }
    }

    // Finally calculate moments by normalizing sums with corresponding eigenvalues and powers
    double bS1 = 0.;
    double bS2 = 0.;
    for (int i = 0; i < p; ++i)
    {
      tmp = wX[i] * wX[i];
      if (tmp != 0.0)
      {
        bS1 += sum3[i] * sum3[i] / (tmp * wX[i]);
        bS2 += sum4[i] / tmp;
      }
    }
    bS1 /= (nRowData * nRowData * p);
    bS2 /= (nRowData * p);

    // Finally, calculate Jarque-Bera-Srivastava statistic
    tmp = bS2 - 3.;
    double jbs = static_cast<double>(nRowData * p) * (bS1 / 6. + (tmp * tmp) / 24.);

    // Insert variable name and calculated Jarque-Bera-Srivastava statistic
    blockCol->InsertNextValue(b);
    bS1Col->InsertNextTuple1(bS1);
    bS2Col->InsertNextTuple1(bS2);
    statCol->InsertNextTuple1(jbs);
    dimCol->InsertNextTuple1(p + 1);
  } // b

  // Now, add the already prepared columns to the output table
  outMeta->AddColumn(blockCol);
  outMeta->AddColumn(bS1Col);
  outMeta->AddColumn(bS2Col);
  outMeta->AddColumn(statCol);
  outMeta->AddColumn(dimCol);

  // Last phase: compute the p-values or assign invalid value if they cannot be computed
  svtkDoubleArray* testCol = this->CalculatePValues(dimCol, statCol);

  // The test column name can only be set after the column has been obtained from R
  testCol->SetName("P");
  // Now add the column of invalid values to the output table
  outMeta->AddColumn(testCol);
  // Clean up
  testCol->Delete();

  // Clean up
  blockCol->Delete();
  bS1Col->Delete();
  bS2Col->Delete();
  statCol->Delete();
  dimCol->Delete();
}
// ----------------------------------------------------------------------
void svtkPCAStatistics::Assess(svtkTable* inData, svtkMultiBlockDataSet* inMeta, svtkTable* outData)
{
  if (!inData)
  {
    return;
  }

  if (!inMeta)
  {
    return;
  }

  // For each request, add a column to the output data related to the likelihood of each input datum
  // wrt the model in the request. Column names of the metadata and input data are assumed to match.
  // The output columns will be named "RelDevSq(A,B,C)" where "A", "B", and "C" are the column names
  // specified in the per-request metadata tables.
  svtkIdType nRow = inData->GetNumberOfRows();
  int nb = static_cast<int>(inMeta->GetNumberOfBlocks());
  AssessFunctor* dfunc = nullptr;
  for (int req = 1; req < nb; ++req)
  {
    svtkTable* reqModel = svtkTable::SafeDownCast(inMeta->GetBlock(req));
    if (!reqModel)
    { // silently skip invalid entries. Note we leave assessValues column in output data even when
      // it's empty.
      continue;
    }

    this->SelectAssessFunctor(inData, reqModel, nullptr, dfunc);

    svtkPCAAssessFunctor* pcafunc = static_cast<svtkPCAAssessFunctor*>(dfunc);
    if (!pcafunc)
    {
      svtkWarningMacro("Request " << req - 1 << " could not be accommodated. Skipping.");
      delete dfunc;
      continue;
    }

    // Create an array to hold the assess values for all the input data
    std::vector<double*> assessValues;
    int comp;
    for (comp = 0; comp < pcafunc->BasisSize; ++comp)
    {
      std::ostringstream reqNameStr;
      reqNameStr << SVTK_PCA_COMPCOLUMN << "{";
      for (int i = 0; i < pcafunc->GetNumberOfColumns(); ++i)
      {
        if (i > 0)
        {
          reqNameStr << ",";
        }
        reqNameStr << pcafunc->GetColumn(i)->GetName();
      }
      reqNameStr << "}(" << comp << ")";
      svtkDoubleArray* arr = svtkDoubleArray::New();
      arr->SetName(reqNameStr.str().c_str());
      arr->SetNumberOfTuples(nRow);
      outData->AddColumn(arr);
      arr->Delete();
      assessValues.push_back(arr->GetPointer(0));
    }

    // Something to hold assessed values for a single input datum
    svtkDoubleArray* singleResult = svtkDoubleArray::New();
    // Loop over all the input data and assess each datum:
    for (svtkIdType row = 0; row < nRow; ++row)
    {
      (*dfunc)(singleResult, row);
      for (comp = 0; comp < pcafunc->BasisSize; ++comp)
      {
        assessValues[comp][row] = singleResult->GetValue(comp);
      }
    }
    delete dfunc;
    singleResult->Delete();
  }
}

// ----------------------------------------------------------------------
void svtkPCAStatistics::SelectAssessFunctor(svtkTable* inData, svtkDataObject* inMetaDO,
  svtkStringArray* svtkNotUsed(rowNames), AssessFunctor*& dfunc)
{
  dfunc = nullptr;
  svtkTable* reqModel = svtkTable::SafeDownCast(inMetaDO);
  if (!reqModel)
  {
    return;
  }

  svtkPCAAssessFunctor* pcafunc = svtkPCAAssessFunctor::New();
  if (!pcafunc->InitializePCA(inData, reqModel, this->NormalizationScheme, this->BasisScheme,
        this->FixedBasisSize, this->FixedBasisEnergy))
  {
    delete pcafunc;
    return;
  }

  dfunc = pcafunc;
}
