#include "svtkImageAlgorithm.h"
#include <svtkInformationVector.h>

class TestVectorFieldSource : public svtkImageAlgorithm
{
public:
  static TestVectorFieldSource* New();
  svtkTypeMacro(TestVectorFieldSource, svtkImageAlgorithm);
  void SetBoundingBox(double x0, double x1, double y0, double y1, double z0, double z1);
  void SetExtent(int xMin, int xMax, int yMin, int yMax, int zMin, int zMax);

protected:
  TestVectorFieldSource();
  ~TestVectorFieldSource();
  virtual int RequestInformation(svtkInformation* request, svtkInformationVector** inputInfoVectors,
    svtkInformationVector* outputInfoVector) override;
  void GetSpacing(double dx[3]);
  void GetSize(double dx[3]);
  virtual void ExecuteDataWithInformation(svtkDataObject* outData, svtkInformation* outInfo) override;

private:
  int Extent[6];
  double BoundingBox[6];
  int Spacing;
};
