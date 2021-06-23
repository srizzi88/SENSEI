#include "senseiConfig.h"
#include "SVTKHistogram.h"
#include "Error.h"

#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstring>
#include <errno.h>

#include <svtkSmartPointer.h>
#include <svtkUnsignedCharArray.h>
#ifdef ENABLE_SVTK_GENERIC_ARRAYS
#include <svtkAOSDataArrayTemplate.h>
#include <svtkArrayDispatch.h>
#include <svtkCompositeDataIterator.h>
#include <svtkCompositeDataSet.h>
#include <svtkDataArray.h>
#include <svtkDataObject.h>
#include <svtkFieldData.h>
#include <svtkObjectFactory.h>
#else
#include <svtkDataArrayDispatcher.h>
#endif

namespace sensei
{
// Private worker for Histogram method. Computes the local Histogram on
// array (passed to operator()). To be used with svtkArrayDispatch.
//
// Inputs:
// range: Global range of data
// bins: Number of Histogram bins
// array: Local data.
//
// Outputs:
// Histogram: The Histogram of the local data.
#ifdef ENABLE_SVTK_GENERIC_ARRAYS
struct SVTKHistogram::Internals
{
  const double *Range;
  int Bins;
  std::vector<unsigned int> Histogram;

  Internals(const double *range, int bins) :
    Range(range), Bins(bins), Histogram(bins,0.0) {}

  template <typename ArrayT>
  void operator()(ArrayT *array)
  {
    assert(array);
    assert(array->GetNumberOfComponents() == 1);

    typedef typename ArrayT::ValueType ValueType;

    ValueType width = static_cast<ValueType>((this->Range[1]
      - this->Range[0]) / this->Bins);

    ValueType min = static_cast<ValueType>(this->Range[0]);
    svtkIdType numTuples = array->GetNumberOfTuples();

    // + 1 to store val == max. These will be moved to this last bin before
    // returning (Avoids having to branch in the loop below);
    this->Histogram.resize(this->Bins + 1, 0);
    for (svtkIdType tIdx = 0; tIdx < numTuples; ++tIdx)
      {
      int bin = static_cast<int>((array->GetComponent(tIdx, 0) - min) / width);
      ++this->Histogram[bin];
      }

    // Merge the last two bins (the last is only when val == max)
    this->Histogram[this->Bins-1] += this->Histogram[this->Bins];
    this->Histogram.resize(this->Bins);
  }
};
#else
struct SVTKHistogram::Internals
{
  svtkUnsignedCharArray* GhostArray;
  const double *Range;
  int Bins;
  std::vector<unsigned int> Histogram;

  Internals(const double *range, int bins) :
    GhostArray(NULL), Range(range), Bins(bins), Histogram(bins,0.0) {}

  template <typename T>
  void operator()(const svtkDataArrayDispatcherPointer<T>& array)
  {
    assert(array.NumberOfComponents == 1);

    typedef T ValueType;
    ValueType width = static_cast<ValueType>((this->Range[1] - this->Range[0]) / this->Bins);
    ValueType min = static_cast<ValueType>(this->Range[0]);
    svtkIdType numTuples = array.NumberOfTuples;

    // + 1 to store val == max. These will be moved to this last bin before
    // returning (Avoids having to branch in the loop below);
    this->Histogram.resize(this->Bins + 1, 0);
    if (this->GhostArray)
      {
      for (svtkIdType tIdx = 0; tIdx < numTuples; ++tIdx)
        {
        if (this->GhostArray->GetValue(tIdx) == 0)
          {
          int bin = static_cast<int>((array.RawPointer[tIdx] - min) / width);
          ++this->Histogram[bin];
          }
        }
      }
    else
      {
      for (svtkIdType tIdx = 0; tIdx < numTuples; ++tIdx)
        {
        int bin = static_cast<int>((array.RawPointer[tIdx] - min) / width);
        ++this->Histogram[bin];
        }
      }

    // Merge the last two bins (the last is only when val == max)
    this->Histogram[this->Bins-1] += this->Histogram[this->Bins];
    this->Histogram.resize(this->Bins);
  }
};

// Compute array range by skipping ghost elements.
class ComponentRangeWorker
{
public:
  ComponentRangeWorker(svtkUnsignedCharArray* ghostArray) :
    GhostArray(ghostArray)
    {
    this->Range[0] = svtkTypeTraits<double>::Max();
    this->Range[1] = svtkTypeTraits<double>::Min();
    }

  template <typename T>
  void operator()(const svtkDataArrayDispatcherPointer<T>& array)
    {
    assert(array.NumberOfComponents == 1);
    svtkIdType numTuples = array.NumberOfTuples;
    for (svtkIdType cc=0; cc < numTuples; cc++)
      {
      if (this->GhostArray->GetValue(cc) == 0)
        {
        this->Range[0] = std::min(this->Range[0], static_cast<double>(array.RawPointer[cc]));
        this->Range[1] = std::max(this->Range[1], static_cast<double>(array.RawPointer[cc]));
        }
      }
    }

  void GetRange(double r[2])
    {
    std::copy(this->Range, this->Range+2, r);
    }
private:
  double Range[2];
  svtkUnsignedCharArray* GhostArray;
};
#endif

// --------------------------------------------------------------------------
SVTKHistogram::SVTKHistogram()
{
  this->Range[0] = SVTK_DOUBLE_MAX;
  this->Range[1] = SVTK_DOUBLE_MIN;
  this->Worker = NULL;
}

// --------------------------------------------------------------------------
SVTKHistogram::~SVTKHistogram()
{
  delete this->Worker;
}

// --------------------------------------------------------------------------
void SVTKHistogram::AddRange(svtkDataArray* da,
  svtkUnsignedCharArray* ghostArray)
{
#ifdef ENABLE_SVTK_GENERIC_ARRAYS
  (void)ghostArray;
  if (da)
    {
    double crange[2];
    da->GetRange(crange);
    this->Range[0] = std::min(this->Range[0], crange[0]);
    this->Range[1] = std::max(this->Range[1], crange[1]);
    }
#else
  double crange[2] = { svtkTypeTraits<double>::Max(), svtkTypeTraits<double>::Min() };
  if (da && ghostArray)
    {
    ComponentRangeWorker worker(ghostArray);
    svtkDataArrayDispatcher<ComponentRangeWorker> dispatcher(worker);
    dispatcher.Go(da);
    worker.GetRange(crange);
    }
  else if (da)
    {
    da->GetRange(crange);
    }
  this->Range[0] = std::min(this->Range[0], crange[0]);
  this->Range[1] = std::max(this->Range[1], crange[1]);
#endif
}

// --------------------------------------------------------------------------
void SVTKHistogram::Compute(svtkDataArray* da,
  svtkUnsignedCharArray* ghostArray)
{
  if (da)
    {
#ifdef ENABLE_SVTK_GENERIC_ARRAYS
    (void)ghostArray;
    svtkArrayDispatch::Dispatch::Execute(da, *this->Worker);
#else
    this->Worker->GhostArray = ghostArray;
    svtkDataArrayDispatcher<Internals> dispatcher(*this->Worker);
    dispatcher.Go(da);
    this->Worker->GhostArray = NULL;
#endif
    }
}

// --------------------------------------------------------------------------
void SVTKHistogram::PreCompute(MPI_Comm comm, int bins)
{
  double g_range[2];
  // Find the global max/min
  MPI_Allreduce(&this->Range[0], &g_range[0], 1, MPI_DOUBLE, MPI_MIN, comm);
  MPI_Allreduce(&this->Range[1], &g_range[1], 1, MPI_DOUBLE, MPI_MAX, comm);
  this->Range[0] = g_range[0];
  this->Range[1] = g_range[1];
  this->Worker = new Internals(this->Range, bins);
}

// --------------------------------------------------------------------------
void SVTKHistogram::PostCompute(MPI_Comm comm, int nBins, int step,
  double time, const std::string &meshName, const std::string &arrayName,
  const std::string &fileName)
{
  std::vector<unsigned int> gHist(nBins, 0);

  MPI_Reduce(&this->Worker->Histogram[0], &gHist[0],
    nBins, MPI_UNSIGNED, MPI_SUM, 0, comm);

  int rank = 0;
  MPI_Comm_rank(comm, &rank);

  if (rank == 0)
    {
    // if there was an error range is initialized to [DOUBLE_MAX, DOUBLE_MIN]
    if (this->Range[0] >= this->Range[1])
      {
      SENSEI_ERROR("Invalid histgram range ["
        << this->Range[0] << " - " << this->Range[1] << "]")
      MPI_Abort(comm, -1);
      return;
      }

    if (fileName.empty())
      {
      // print the histogram nBins, range of each bin and count.
      int origPrec = cout.precision();
      std::cout.precision(4);

      std::cout << "Histogram mesh \"" << meshName << "\" data array \""
        << arrayName << "\" step " << step << " time " << time << std::endl;

      double width = (this->Range[1] - this->Range[0]) / nBins;
      for (int i = 0; i < nBins; ++i)
        {
        const int wid = 15;
        std::cout << std::scientific << std::setw(wid) << std::right << this->Range[0] + i*width
          << " - " << std::setw(wid) << std::left << this->Range[0] + (i+1)*width
          << ": " << std::fixed << gHist[i] << std::endl;
        }

      std::cout.precision(origPrec);
      }
    else
      {
      char fname[1024] = {'\0'};
      snprintf(fname, 1024, "%s_%s_%s_%d.txt", fileName.c_str(),
        meshName.c_str(), arrayName.c_str(), step);

      FILE *file = fopen(fname, "w");
      if (!file)
        {
        char *estr = strerror(errno);
        SENSEI_ERROR("Failed to open \"" << fname << "\""
          << std::endl << estr)
        MPI_Abort(comm, -1);
        return;
        }

      fprintf(file, "step : %d\n", step);
      fprintf(file, "time : %0.6g\n", time);
      fprintf(file, "num bins : %d\n", nBins);
      fprintf(file, "range : %0.6g %0.6g\n", this->Range[0], this->Range[1]);
      fprintf(file, "bin edges : ");
      double width = (this->Range[1] - this->Range[0]) / nBins;
      for (int i = 0; i < nBins + 1; ++i)
        fprintf(file, "%0.6g ", this->Range[0] + i*width);
      fprintf(file, "\n");
      fprintf(file, "counts : ");
      for (int i = 0; i < nBins; ++i)
        fprintf(file, "%d ", gHist[i]);
      fprintf(file, "\n");
      fclose(file);
      }

    // cache the last result, the simulation can access it
    this->Worker->Histogram = gHist;
    }
}

// --------------------------------------------------------------------------
int SVTKHistogram::GetHistogram(MPI_Comm comm, double &min, double &max,
  std::vector<unsigned int> &bins)
{
  if (!this->Worker)
    return -1;

  int rank = 0;
  MPI_Comm_rank(comm, &rank);

  if (rank == 0)
    {
    min = this->Range[0];
    max = this->Range[1];
    bins = this->Worker->Histogram;
    }

  return 0;
}

}
