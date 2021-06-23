/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPathTracerMaterials.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can load a set of materials specification
// from disk and use them.

#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkOSPRayMaterialLibrary.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#include <string>

int TestPathTracerMaterialLibrary(int argc, char* argv[])
{
  // read an ospray material file
  const char* materialFile =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ospray_mats.json");
  svtkSmartPointer<svtkOSPRayMaterialLibrary> lib = svtkSmartPointer<svtkOSPRayMaterialLibrary>::New();
  cout << "Open " << materialFile << endl;
  lib->ReadFile(materialFile);
  cout << "Parsed file OK, now check for expected contents." << endl;
  std::set<std::string> mats = lib->GetMaterialNames();

  cout << "Materials are:" << endl;
  std::set<std::string>::iterator it = mats.begin();
  while (it != mats.end())
  {
    cout << *it << endl;
    ++it;
  }
  if (mats.find("Water") == mats.end())
  {
    cerr << "Problem, could not find expected material named water." << endl;
    return SVTK_ERROR;
  }
  cout << "Found Water material." << endl;
  if (lib->LookupImplName("Water") != "Glass")
  {
    cerr << "Problem, expected Water to be implemented by the Glass material." << endl;
    return SVTK_ERROR;
  }
  cout << "Water is the right type." << endl;
  if (lib->GetDoubleShaderVariable("Water", "attenuationColor").size() != 3)
  {
    cerr << "Problem, expected Water to have a 3 component variable called attentuationColor."
         << endl;
    return SVTK_ERROR;
  }
  cout << "Water has an expected variable." << endl;
  if (lib->GetTexture("Bumpy", "map_bump") == nullptr)
  {
    cerr << "Problem, expected Bumpy to have a texture called map_bump." << endl;
    return SVTK_ERROR;
  }
  cout << "Bumpy has a good texture too." << endl;

  // read a wavefront mtl file
  const char* materialFile2 =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ospray_mats.mtl");
  cout << "Open " << materialFile2 << endl;

  lib->ReadFile(materialFile2);
  cout << "Parsed file OK, now check for expected contents." << endl;

  mats = lib->GetMaterialNames();
  cout << "Materials are now:" << endl;
  it = mats.begin();
  while (it != mats.end())
  {
    cout << *it << endl;
    ++it;
  }

  auto ks = lib->GetDoubleShaderVariable("mat1", "Ks");
  if (ks[2] != 0.882353)
  {
    cerr << "Problem, could not find expected material mat1 ks component." << endl;
    return SVTK_ERROR;
  }

  if (mats.find("mat2") == mats.end())
  {
    cerr << "Problem, could not find expected material named mat2." << endl;
    return SVTK_ERROR;
  }
  if (lib->GetDoubleShaderVariable("mat2", "Kd").size() == 0)
  {
    cerr << "Problem, expected mat2 to have a variable called Kd." << endl;
    return SVTK_ERROR;
  }

  lib->RemoveAllShaderVariables("mat2");
  if (lib->GetDoubleShaderVariable("mat2", "Kd").size() > 0)
  {
    cerr << "Problem, expected mat2 to have Kd removed." << endl;
    return SVTK_ERROR;
  }

  cout << "mat2 has an expected variable." << endl;
  if (lib->GetTexture("mat2", "map_Kd") == nullptr)
  {
    cerr << "Problem, expected mat2 to have a texture called map_Kd." << endl;
    return SVTK_ERROR;
  }
  cout << "mat2 has a good texture too." << endl;

  lib->RemoveAllTextures("mat2");
  if (lib->GetTexture("mat2", "map_Kd") != nullptr)
  {
    cerr << "Problem, expected mat2 to have map_Kd removed." << endl;
    return SVTK_ERROR;
  }

  if (mats.find("mat3") == mats.end())
  {
    cerr << "Problem, could not find expected material named mat3." << endl;
    return SVTK_ERROR;
  }
  if (lib->LookupImplName("mat3") != "Metal")
  {
    cerr << "Problem, expected mat3 to be implemented by the Metal material." << endl;
    return SVTK_ERROR;
  }
  cout << "mat3 is the right type." << endl;

  cout << "We're all clear kid." << endl;

  // serialize and deserialize
  cout << "Serialize" << endl;
  const char* buf = lib->WriteBuffer();

  cout << "Deserialize" << endl;
  lib->ReadBuffer(buf);

  return 0;
}
