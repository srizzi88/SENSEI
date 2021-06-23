//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Initialize.h>

#include <svtkm/io/reader/SVTKDataSetReader.h>
#include <svtkm/io/writer/SVTKDataSetWriter.h>

#include <svtkm/worklet/CosmoTools.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static const svtkm::cont::LogLevel CosmoLogLevel = svtkm::cont::LogLevel::UserFirst;

void TestCosmoHaloFinder(const char* fileName)
{
  std::cout << std::endl
            << "Testing Cosmology Halo Finder and MBP Center Finder " << fileName << std::endl;

  // Open the file for reading
  std::ifstream inFile(fileName);
  if (inFile.fail())
  {
    std::cout << "File does not exist " << fileName << std::endl;
    return;
  }

  // Read in number of particles and locations
  int nParticles;
  inFile >> nParticles;
  std::size_t size = static_cast<std::size_t>(nParticles);

  float* xLocation = new float[size];
  float* yLocation = new float[size];
  float* zLocation = new float[size];
  std::cout << "Running Halo Finder on " << nParticles << std::endl;

  for (svtkm::Id p = 0; p < nParticles; p++)
  {
    inFile >> xLocation[p] >> yLocation[p] >> zLocation[p];
  }

  svtkm::cont::ArrayHandle<svtkm::Float32> xLocArray =
    svtkm::cont::make_ArrayHandle<svtkm::Float32>(xLocation, nParticles);
  svtkm::cont::ArrayHandle<svtkm::Float32> yLocArray =
    svtkm::cont::make_ArrayHandle<svtkm::Float32>(yLocation, nParticles);
  svtkm::cont::ArrayHandle<svtkm::Float32> zLocArray =
    svtkm::cont::make_ArrayHandle<svtkm::Float32>(zLocation, nParticles);

  // Output halo id, mbp id and min potential per particle
  svtkm::cont::ArrayHandle<svtkm::Id> resultHaloId;
  svtkm::cont::ArrayHandle<svtkm::Id> resultMBP;
  svtkm::cont::ArrayHandle<svtkm::Float32> resultPot;

  // Create the worklet and run it
  svtkm::Id minHaloSize = 20;
  svtkm::Float32 linkingLength = 0.2f;
  svtkm::Float32 particleMass = 1.08413e+09f;

  {
    SVTKM_LOG_SCOPE(CosmoLogLevel, "Executing HaloFinder");

    svtkm::worklet::CosmoTools cosmoTools;
    cosmoTools.RunHaloFinder(xLocArray,
                             yLocArray,
                             zLocArray,
                             nParticles,
                             particleMass,
                             minHaloSize,
                             linkingLength,
                             resultHaloId,
                             resultMBP,
                             resultPot);
  }

  xLocArray.ReleaseResources();
  yLocArray.ReleaseResources();
  zLocArray.ReleaseResources();

  delete[] xLocation;
  delete[] yLocation;
  delete[] zLocation;
}

/////////////////////////////////////////////////////////////////////
//
// Form of the input file in ASCII
// Line 1: number of particles in the file
// Line 2+: (float) xLoc (float) yLoc (float) zLoc
//
// CosmoHaloFinder data.cosmotools
//
/////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  svtkm::cont::SetLogLevelName(CosmoLogLevel, "Cosmo");
  svtkm::cont::SetStderrLogLevel(CosmoLogLevel);

  auto opts = svtkm::cont::InitializeOptions::DefaultAnyDevice;
  svtkm::cont::InitializeResult config = svtkm::cont::Initialize(argc, argv, opts);

  if (argc < 2)
  {
    std::cout << "Usage: " << std::endl << "$ " << argv[0] << " <input_file>" << std::endl;
    std::cout << config.Usage << std::endl;
    return 1;
  }

#ifndef SVTKM_ENABLE_LOGGING
  std::cout << "Warning: turn on SVTKm_ENABLE_LOGGING CMake option to turn on timing." << std::endl;
#endif

  TestCosmoHaloFinder(argv[1]);

  return 0;
}
