#include "svtkSmartPointer.h"
#include "svtkmDataArray.h"

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vector>

namespace
{

//-----------------------------------------------------------------------------
class TestError
{
public:
  TestError(const std::string& message, int line)
    : Message(message)
    , Line(line)
  {
  }

  void PrintMessage(std::ostream& out) const
  {
    out << "Error at line " << this->Line << ": " << this->Message << "\n";
  }

private:
  std::string Message;
  int Line;
};

#define RAISE_TEST_ERROR(msg) throw TestError((msg), __LINE__)

#define TEST_VERIFY(cond, msg)                                                                     \
  if (!(cond))                                                                                     \
  RAISE_TEST_ERROR((msg))

inline bool IsEqualFloat(double a, double b, double e = 1e-6f)
{
  return (std::abs(a - b) <= e);
}

//-----------------------------------------------------------------------------
template <typename ArrayHandleType>
void TestWithArrayHandle(const ArrayHandleType& svtkmArray)
{
  svtkSmartPointer<svtkDataArray> svtkArray;
  svtkArray.TakeReference(make_svtkmDataArray(svtkmArray));

  auto svtkmPortal = svtkmArray.GetPortalConstControl();

  svtkIdType length = svtkArray->GetNumberOfTuples();
  std::cout << "Length: " << length << "\n";
  TEST_VERIFY(length == svtkmArray.GetNumberOfValues(), "Array lengths don't match");

  int numberOfComponents = svtkArray->GetNumberOfComponents();
  std::cout << "Number of components: " << numberOfComponents << "\n";
  TEST_VERIFY(numberOfComponents ==
      internal::FlattenVec<typename ArrayHandleType::ValueType>::GetNumberOfComponents(
        svtkmPortal.Get(0)),
    "Number of components don't match");

  for (svtkIdType i = 0; i < length; ++i)
  {
    double tuple[9];
    svtkArray->GetTuple(i, tuple);
    auto val = svtkmPortal.Get(i);
    for (int j = 0; j < numberOfComponents; ++j)
    {
      auto comp = internal::FlattenVec<typename ArrayHandleType::ValueType>::GetComponent(val, j);
      TEST_VERIFY(IsEqualFloat(tuple[j], static_cast<double>(comp)), "values don't match");
      TEST_VERIFY(IsEqualFloat(svtkArray->GetComponent(i, j), static_cast<double>(comp)),
        "values don't match");
    }
  }
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int TestSVTKMDataArray(int, char*[])
try
{
  static const std::vector<double> testData = { 3.0, 6.0, 2.0, 5.0, 1.0, 0.0, 4.0 };

  std::cout << "Testing with Basic ArrayHandle\n";
  TestWithArrayHandle(svtkm::cont::make_ArrayHandle(testData));
  std::cout << "Passed\n";

  std::cout << "Testing with ArrayHandleConstant\n";
  TestWithArrayHandle(svtkm::cont::make_ArrayHandleConstant(
    svtkm::Vec<svtkm::Vec<float, 3>, 3>{ { 1.0f, 2.0f, 3.0f } }, 10));
  std::cout << "Passed\n";

  std::cout << "Testing with ArrayHandleUniformPointCoordinates\n";
  TestWithArrayHandle(svtkm::cont::ArrayHandleUniformPointCoordinates(svtkm::Id3{ 3 }));
  std::cout << "Passed\n";

  return EXIT_SUCCESS;
}
catch (const TestError& e)
{
  e.PrintMessage(std::cout);
  return 1;
}
