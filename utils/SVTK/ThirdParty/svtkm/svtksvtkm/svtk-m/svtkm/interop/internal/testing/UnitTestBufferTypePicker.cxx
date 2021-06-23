//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/testing/Testing.h>

#include <GL/glew.h>
#include <svtkm/interop/internal/BufferTypePicker.h>

namespace
{
void TestBufferTypePicker()
{
  //just verify that certain types match
  GLenum type;
  using svtkmUint = unsigned int;
  using T = svtkm::FloatDefault;

  type = svtkm::interop::internal::BufferTypePicker(svtkm::Id());
  SVTKM_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = svtkm::interop::internal::BufferTypePicker(int());
  SVTKM_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = svtkm::interop::internal::BufferTypePicker(svtkmUint());
  SVTKM_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");

  type = svtkm::interop::internal::BufferTypePicker(svtkm::Vec<T, 4>());
  SVTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = svtkm::interop::internal::BufferTypePicker(svtkm::Vec<T, 3>());
  SVTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = svtkm::interop::internal::BufferTypePicker(svtkm::FloatDefault());
  SVTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = svtkm::interop::internal::BufferTypePicker(float());
  SVTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = svtkm::interop::internal::BufferTypePicker(double());
  SVTKM_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
}
}

int UnitTestBufferTypePicker(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestBufferTypePicker, argc, argv);
}
