/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOBJReaderGroups.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Verifies that svtkOBJReader properly handles the presence of
// groupIds
// .SECTION Description
//

#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkOBJReader.h"
#include "svtkTestUtilities.h"

// Read the specified file and check for expected number of groups
// Comments directly from the data file
static bool CheckOBJGroups(const std::string& filename, const int maxExpected)
{
  auto reader = svtkSmartPointer<svtkOBJReader>::New();
  reader->SetFileName(filename.c_str());
  reader->Update();

  svtkPolyData* data = reader->GetOutput();

  std::cerr << "Testing file:" << filename << std::endl
            << "Expecting " << maxExpected << " as max groupId" << std::endl;

  const char* comment = reader->GetComment();
  if (comment)
  {
    std::cerr << "Comment: " << comment << std::endl;
  }

  svtkFloatArray* groups =
    svtkFloatArray::SafeDownCast(data->GetCellData()->GetAbstractArray("GroupIds"));
  if (!groups)
  {
    std::cerr << "missing group id array" << std::endl;
    return false;
  }

  const svtkIdType nTuple = groups->GetNumberOfTuples();

  int maxGroupId = -1;

  for (svtkIdType i = 0; i < nTuple; ++i)
  {
    int thisGroup = static_cast<int>(round(groups->GetTuple(i)[0]));

    if (maxGroupId < thisGroup)
    {
      maxGroupId = thisGroup;
    }
  }

  if (maxExpected == maxGroupId)
  {
    return true;
  }

  std::cerr << "Error: found " << maxGroupId << " as max groupId" << std::endl;
  return false;
}

int TestOBJReaderGroups(int argc, char* argv[])
{
  // lambda for the testing
  auto doTesting = [&](int maxExpected, const char* dataName) -> bool {
    char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, dataName);
    std::string filename(fname);
    delete[] fname;

    return CheckOBJGroups(filename, maxExpected);
  };

  int nFailures = 0;

  if (!doTesting(0, "Data/objGroup_1a.obj"))
  {
    ++nFailures;
  }
  if (!doTesting(0, "Data/objGroup_1b.obj"))
  {
    ++nFailures;
  }
  if (!doTesting(1, "Data/objGroup_2a.obj"))
  {
    ++nFailures;
  }
  if (!doTesting(1, "Data/objGroup_2b.obj"))
  {
    ++nFailures;
  }

  std::cerr << "Test with " << nFailures << std::endl;

  return nFailures;
}
