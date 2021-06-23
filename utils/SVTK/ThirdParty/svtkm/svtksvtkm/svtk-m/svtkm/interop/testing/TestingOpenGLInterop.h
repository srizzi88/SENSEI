//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_interop_testing_TestingOpenGLInterop_h
#define svtk_m_interop_testing_TestingOpenGLInterop_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/Magnitude.h>

#include <svtkm/interop/TransferToOpenGL.h>

#include <svtkm/cont/testing/Testing.h>
// #include <svtkm/cont/testing/TestingGridGenerator.h>

#include <algorithm>
#include <iterator>
#include <random>
#include <vector>

namespace svtkm
{
namespace interop
{
namespace testing
{

/// This class has a single static member, Run, that tests the templated
/// DeviceAdapter for support for opengl interop.
///
template <class DeviceAdapterTag, class StorageTag = SVTKM_DEFAULT_STORAGE_TAG>
struct TestingOpenGLInterop
{
private:
  //fill the array with a collection of values and return it wrapped in
  //an svtkm array handle
  template <typename T>
  static svtkm::cont::ArrayHandle<T, StorageTag> FillArray(std::vector<T>& data, std::size_t length)
  {
    using iterator = typename std::vector<T>::iterator;
    //make sure the data array is exactly the right length
    data.clear();
    data.resize(length);
    svtkm::Id pos = 0;
    for (iterator i = data.begin(); i != data.end(); ++i, ++pos)
    {
      *i = TestValue(pos, T());
    }

    std::random_device rng;
    std::mt19937 urng(rng());
    std::shuffle(data.begin(), data.end(), urng);
    return svtkm::cont::make_ArrayHandle(data);
  }

  //Transfer the data in a svtkm ArrayHandle to open gl while making sure
  //we don't throw any errors
  template <typename ArrayHandleType>
  static void SafelyTransferArray(ArrayHandleType array, GLuint& handle)
  {
    try
    {
      svtkm::interop::BufferState state(handle);
      svtkm::interop::TransferToOpenGL(array, state, DeviceAdapterTag());
    }
    catch (svtkm::cont::ErrorBadAllocation& error)
    {
      std::cout << error.GetMessage() << std::endl;
      SVTKM_TEST_ASSERT(true == false,
                       "Got an unexpected Out Of Memory error transferring to openGL");
    }
    catch (svtkm::cont::ErrorBadValue& bvError)
    {
      std::cout << bvError.GetMessage() << std::endl;
      SVTKM_TEST_ASSERT(true == false, "Got an unexpected Bad Value error transferring to openGL");
    }

    // Test device adapter deduction:
    try
    {
      svtkm::interop::BufferState state(handle);
      svtkm::interop::TransferToOpenGL(array, state);
    }
    catch (svtkm::cont::ErrorBadAllocation& error)
    {
      std::cout << error.GetMessage() << std::endl;
      SVTKM_TEST_ASSERT(true == false,
                       "Got an unexpected Out Of Memory error transferring to openGL");
    }
    catch (svtkm::cont::ErrorBadValue& bvError)
    {
      std::cout << bvError.GetMessage() << std::endl;
      SVTKM_TEST_ASSERT(true == false, "Got an unexpected Bad Value error transferring to openGL");
    }
  }

  template <typename ArrayHandleType>
  static void SafelyTransferArray(ArrayHandleType array, GLuint& handle, GLenum type)
  {
    try
    {
      svtkm::interop::BufferState state(handle, type);
      svtkm::interop::TransferToOpenGL(array, state, DeviceAdapterTag());
    }
    catch (svtkm::cont::ErrorBadAllocation& error)
    {
      std::cout << error.GetMessage() << std::endl;
      SVTKM_TEST_ASSERT(true == false,
                       "Got an unexpected Out Of Memory error transferring to openGL");
    }
    catch (svtkm::cont::ErrorBadValue& bvError)
    {
      std::cout << bvError.GetMessage() << std::endl;
      SVTKM_TEST_ASSERT(true == false, "Got an unexpected Bad Value error transferring to openGL");
    }

    // Test device adapter deduction
    try
    {
      svtkm::interop::BufferState state(handle, type);
      svtkm::interop::TransferToOpenGL(array, state);
    }
    catch (svtkm::cont::ErrorBadAllocation& error)
    {
      std::cout << error.GetMessage() << std::endl;
      SVTKM_TEST_ASSERT(true == false,
                       "Got an unexpected Out Of Memory error transferring to openGL");
    }
    catch (svtkm::cont::ErrorBadValue& bvError)
    {
      std::cout << bvError.GetMessage() << std::endl;
      SVTKM_TEST_ASSERT(true == false, "Got an unexpected Bad Value error transferring to openGL");
    }
  }

  //bring the data back from openGL and into a std vector. Will bind the
  //passed in handle to the default buffer type for the type T
  template <typename T>
  static std::vector<T> CopyGLBuffer(GLuint& handle, T t)
  {
    //get the type we used for this buffer.
    GLenum type = svtkm::interop::internal::BufferTypePicker(t);

    //bind the buffer to the guessed buffer type, this way
    //we can call CopyGLBuffer no matter what it the active buffer
    glBindBuffer(type, handle);

    //get the size of the buffer
    int bytesInBuffer = 0;
    glGetBufferParameteriv(type, GL_BUFFER_SIZE, &bytesInBuffer);
    const std::size_t size = (static_cast<std::size_t>(bytesInBuffer) / sizeof(T));

    //get the buffer contents and place it into a vector
    std::vector<T> data;
    data.resize(size);
    glGetBufferSubData(type, 0, bytesInBuffer, &data[0]);

    return data;
  }

  struct TransferFunctor
  {
    template <typename T>
    void operator()(const T t) const
    {
      const std::size_t Size = 10;
      GLuint GLHandle;
      //verify that T is able to be transfer to openGL.
      //than pull down the results from the array buffer and verify
      //that they match the handles contents
      std::vector<T> tempData;
      svtkm::cont::ArrayHandle<T, StorageTag> temp = FillArray(tempData, Size);

      //verify that the signature that doesn't have type works
      SafelyTransferArray(temp, GLHandle);

      GLboolean is_buffer;
      is_buffer = glIsBuffer(GLHandle);
      SVTKM_TEST_ASSERT(is_buffer == GL_TRUE, "OpenGL buffer not filled");

      std::vector<T> returnedValues = CopyGLBuffer(GLHandle, t);

      //verify the results match what is in the array handle
      temp.SyncControlArray();
      T* expectedValues = temp.GetStorage().GetArray();

      for (std::size_t i = 0; i < Size; ++i)
      {
        SVTKM_TEST_ASSERT(test_equal(*(expectedValues + i), returnedValues[i]),
                         "Array Handle failed to transfer properly");
      }

      temp.ReleaseResources();
      temp = FillArray(tempData, Size * 2);
      GLenum type = svtkm::interop::internal::BufferTypePicker(t);
      SafelyTransferArray(temp, GLHandle, type);
      is_buffer = glIsBuffer(GLHandle);
      SVTKM_TEST_ASSERT(is_buffer == GL_TRUE, "OpenGL buffer not filled");
      returnedValues = CopyGLBuffer(GLHandle, t);
      //verify the results match what is in the array handle
      temp.SyncControlArray();
      expectedValues = temp.GetStorage().GetArray();

      for (std::size_t i = 0; i < Size * 2; ++i)
      {
        SVTKM_TEST_ASSERT(test_equal(*(expectedValues + i), returnedValues[i]),
                         "Array Handle failed to transfer properly");
      }

      //verify this work for a constant value array handle
      T constantValue = TestValue(2, T()); //verified by die roll
      svtkm::cont::ArrayHandleConstant<T> constant(constantValue, static_cast<svtkm::Id>(Size));
      SafelyTransferArray(constant, GLHandle);
      is_buffer = glIsBuffer(GLHandle);
      SVTKM_TEST_ASSERT(is_buffer == GL_TRUE, "OpenGL buffer not filled");
      returnedValues = CopyGLBuffer(GLHandle, constantValue);
      for (std::size_t i = 0; i < Size; ++i)
      {
        SVTKM_TEST_ASSERT(test_equal(returnedValues[i], constantValue),
                         "Constant value array failed to transfer properly");
      }
    }
  };

  // struct TransferGridFunctor
  // {
  //   GLuint CoordGLHandle;
  //   GLuint MagnitudeGLHandle;

  //   template <typename GridType>
  //   void operator()(const GridType)
  //   {
  //   //verify we are able to be transfer both coordinates and indices to openGL.
  //   //than pull down the results from the array buffer and verify
  //   //that they match the handles contents
  //   svtkm::cont::testing::TestGrid<GridType,
  //                                StorageTag,
  //                                DeviceAdapterTag> grid(64);

  //   svtkm::cont::ArrayHandle<svtkm::FloatDefault,
  //                          StorageTag,
  //                          DeviceAdapterTag> magnitudeHandle;

  //   svtkm::cont::DispatcherMapField< svtkm::worklet::Magnitude,
  //                                  DeviceAdapterTag> dispatcher;
  //   dispatcher.Invoke(grid->GetPointCoordinates(), magnitudeHandle);

  //   //transfer to openGL 3 handles and catch any errors
  //   //
  //   SafelyTransferArray(grid->GetPointCoordinates(),this->CoordGLHandle);
  //   SafelyTransferArray(magnitudeHandle,this->MagnitudeGLHandle);

  //   //verify all 3 handles are actually handles
  //   bool  is_buffer = glIsBuffer(this->CoordGLHandle);
  //   SVTKM_TEST_ASSERT(is_buffer==GL_TRUE,
  //                   "Coordinates OpenGL buffer not filled");

  //   is_buffer = glIsBuffer(this->MagnitudeGLHandle);
  //   SVTKM_TEST_ASSERT(is_buffer==GL_TRUE,
  //                   "Magnitude OpenGL buffer not filled");

  //   //now that everything is openGL we have one task left.
  //   //transfer everything back to the host and compare it to the
  //   //computed values.
  //   std::vector<svtkm::Vec<svtkm::FloatDefault,3>> GLReturnedCoords = CopyGLBuffer(
  //                                       this->CoordGLHandle, svtkm::Vec<svtkm::FloatDefault,3>());
  //   std::vector<svtkm::FloatDefault> GLReturneMags = CopyGLBuffer(
  //                                       this->MagnitudeGLHandle,svtkm::FloatDefault());

  //   for (svtkm::Id pointIndex = 0;
  //        pointIndex < grid->GetNumberOfPoints();
  //        pointIndex++)
  //     {
  //     svtkm::Vec<svtkm::FloatDefault,3> pointCoordinateExpected = grid.GetPointCoordinates(
  //                                                                   pointIndex);
  //     svtkm::Vec<svtkm::FloatDefault,3> pointCoordinatesReturned =  GLReturnedCoords[pointIndex];
  //     SVTKM_TEST_ASSERT(test_equal(pointCoordinateExpected,
  //                                pointCoordinatesReturned),
  //                     "Got bad coordinate from OpenGL buffer.");

  //     svtkm::FloatDefault magnitudeValue = GLReturneMags[pointIndex];
  //     svtkm::FloatDefault magnitudeExpected =
  //         sqrt(svtkm::Dot(pointCoordinateExpected, pointCoordinateExpected));
  //     SVTKM_TEST_ASSERT(test_equal(magnitudeValue, magnitudeExpected),
  //                     "Got bad magnitude from OpenGL buffer.");
  //     }
  //   }
  // };

public:
  SVTKM_CONT static int Run(int argc, char* argv[])
  {
    std::cout << "TestingOpenGLInterop Run() " << std::endl;

    //verify that we can transfer basic arrays and constant value arrays to opengl
    svtkm::testing::Testing::TryTypes(TransferFunctor(), argc, argv);

    //verify that openGL interop works with all grid types in that we can
    //transfer coordinates / verts and properties to openGL
    // svtkm::cont::testing::GridTesting::TryAllGridTypes(
    //                              TransferGridFunctor(),
    //                              svtkm::testing::Testing::CellCheckAlwaysTrue(),
    //                              StorageTag(),
    //                              DeviceAdapterTag() );

    return 0;
  }
};
}
}
}

#endif //svtk_m_interop_testing_TestingOpenGLInterop_h
