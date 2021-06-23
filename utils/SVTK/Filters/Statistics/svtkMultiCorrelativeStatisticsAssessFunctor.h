#ifndef svtkMultiCorrelativeStatisticsAssessFunctor_h
#define svtkMultiCorrelativeStatisticsAssessFunctor_h

#include "svtkStatisticsAlgorithm.h"

#include <vector>

class svtkDataArray;
class svtkTable;

#define SVTK_MULTICORRELATIVE_KEYCOLUMN1 "Column1"
#define SVTK_MULTICORRELATIVE_KEYCOLUMN2 "Column2"
#define SVTK_MULTICORRELATIVE_ENTRIESCOL "Entries"
#define SVTK_MULTICORRELATIVE_AVERAGECOL "Mean"
#define SVTK_MULTICORRELATIVE_COLUMNAMES "Column"

class svtkMultiCorrelativeAssessFunctor : public svtkStatisticsAlgorithm::AssessFunctor
{
public:
  static svtkMultiCorrelativeAssessFunctor* New();

  svtkMultiCorrelativeAssessFunctor() {}
  ~svtkMultiCorrelativeAssessFunctor() override {}
  virtual bool Initialize(svtkTable* inData, svtkTable* reqModel, bool cholesky = true);

  void operator()(svtkDoubleArray* result, svtkIdType row) override;

  svtkIdType GetNumberOfColumns() { return static_cast<svtkIdType>(this->Columns.size()); }
  svtkDataArray* GetColumn(svtkIdType colIdx) { return this->Columns[colIdx]; }

  std::vector<svtkDataArray*> Columns; // Source of data
  double* Center;             // Offset per column (usu. to re-center the data about the mean)
  std::vector<double> Factor; // Weights per column
  // double Normalization; // Scale factor for the volume under a multivariate Gaussian used to
  // normalize the CDF
  std::vector<double> Tuple; // Place to store product of detrended input tuple and Cholesky inverse
  std::vector<double> EmptyTuple; // Used to quickly initialize Tuple for each datum
};

#endif // svtkMultiCorrelativeStatisticsAssessFunctor_h
// SVTK-HeaderTest-Exclude: svtkMultiCorrelativeStatisticsAssessFunctor.h
