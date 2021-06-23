#ifndef sensei_SVTKHistogram_h
#define sensei_SVTKHistogram_h

class svtkUnsignedCharArray;
class svtkDataArray;

#include <mpi.h>
#include <string>
#include <vector>

namespace sensei
{

class SVTKHistogram
{
public:
    SVTKHistogram();
    ~SVTKHistogram();

    void AddRange(svtkDataArray* da, svtkUnsignedCharArray* ghostArray);

    // compute the global min and max
    void PreCompute(MPI_Comm comm, int bins);

    // do the local histgram calculation
    void Compute(svtkDataArray* da, svtkUnsignedCharArray* ghostArray);

    // do the reduction, write the result to a file, or cout.
    // the result is cached on rank 0.
    void PostCompute(MPI_Comm comm, int nBins, int step, double time,
      const std::string &meshName, const std::string &arrayName,
      const std::string &fileName);

    // return the last computed results on rank 0
    int GetHistogram(MPI_Comm comm, double &min, double &max,
      std::vector<unsigned int> &bins);

private:
  double Range[2];
  struct Internals;
  Internals *Worker;
};

}

#endif
