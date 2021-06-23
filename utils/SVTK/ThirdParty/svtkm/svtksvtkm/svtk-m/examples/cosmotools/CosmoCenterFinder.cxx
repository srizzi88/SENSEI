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

void TestCosmoCenterFinder(const char* fileName)
{
  std::cout << std::endl
            << "Testing Cosmology MBP Center Finder Filter on one halo " << fileName << std::endl;

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

  float* xLocation = new float[nParticles];
  float* yLocation = new float[nParticles];
  float* zLocation = new float[nParticles];
  std::cout << "Running MBP on " << nParticles << std::endl;

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

  // Output MBP particleId pairs array
  svtkm::Pair<svtkm::Id, svtkm::Float32> nxnResult;
  svtkm::Pair<svtkm::Id, svtkm::Float32> mxnResult;

  const svtkm::Float32 particleMass = 1.08413e+09f;
  svtkm::worklet::CosmoTools cosmoTools;

  {
    SVTKM_LOG_SCOPE(CosmoLogLevel, "Executing NxN");

    cosmoTools.RunMBPCenterFinderNxN(
      xLocArray, yLocArray, zLocArray, nParticles, particleMass, nxnResult);

    SVTKM_LOG_S(CosmoLogLevel,
               "NxN MPB = " << nxnResult.first << "  potential = " << nxnResult.second);
  }

  {
    SVTKM_LOG_SCOPE(CosmoLogLevel, "Executing MxN");
    cosmoTools.RunMBPCenterFinderMxN(
      xLocArray, yLocArray, zLocArray, nParticles, particleMass, mxnResult);

    SVTKM_LOG_S(CosmoLogLevel,
               "MxN MPB = " << mxnResult.first << "  potential = " << mxnResult.second);
  }

  if (nxnResult.first == mxnResult.first)
    std::cout << "FOUND CORRECT PARTICLE " << mxnResult.first << " with potential "
              << nxnResult.second << std::endl;
  else
    std::cout << "ERROR DID NOT FIND SAME PARTICLE" << std::endl;

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
// CosmoCenterFinder data.cosmotools
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

  TestCosmoCenterFinder(argv[1]);

  return 0;
}
