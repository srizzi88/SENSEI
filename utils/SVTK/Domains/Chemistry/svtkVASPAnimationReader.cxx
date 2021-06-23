/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVASPAnimationReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkVASPAnimationReader.h"

#include "svtkDataSetAttributes.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMolecule.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkVectorOperators.h"

#include <svtksys/FStream.hxx>
#include <svtksys/RegularExpression.hxx>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

typedef svtksys::RegularExpression RegEx;

namespace
{

template <typename T>
bool parse(const std::string& str, T& result)
{
  if (!str.empty())
  {
    std::istringstream tmp(str);
    tmp >> result;
    return !tmp.fail();
  }
  return false;
}

} // end anon namespace

svtkStandardNewMacro(svtkVASPAnimationReader);

//------------------------------------------------------------------------------
void svtkVASPAnimationReader::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkVASPAnimationReader::svtkVASPAnimationReader()
  : FileName(nullptr)
  , TimeParser(new RegEx("^ *time *= *([0-9EeDd.+-]+) *$"))
  ,                                              // time = (timeVal)
  LatticeParser(new RegEx("^ *([0-9EeDd.+-]+) +" // Set of 3 floats
                          "([0-9EeDd.+-]+) +"
                          "([0-9EeDd.+-]+) *$"))
  , AtomCountParser(new RegEx("^ *([0-9]+) *$"))
  ,                                           // Just a single integer
  AtomParser(new RegEx("^ *[0-9]+ +"          // Atom index
                       "([0-9]+) +"           // Atomic number
                       "[A-Za-z]+ +"          // Element symbol
                       "([0-9EeDd.+-]+) +"    // X coordinate
                       "([0-9EeDd.+-]+) +"    // Y coordinate
                       "([0-9EeDd.+-]+) +"    // Z coordinate
                       "([0-9EeDd.+-]+) +"    // Radius
                       "([0-9EeDd.+-]+) *$")) // Kinetic energy
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
svtkVASPAnimationReader::~svtkVASPAnimationReader()
{
  this->SetFileName(nullptr);
  delete this->TimeParser;
  delete this->LatticeParser;
  delete this->AtomCountParser;
  delete this->AtomParser;
}

//------------------------------------------------------------------------------
int svtkVASPAnimationReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outInfos)
{
  svtkInformation* outInfo = outInfos->GetInformationObject(0);

  svtkMolecule* output = svtkMolecule::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  assert(output);

  svtksys::ifstream in(this->FileName);
  if (!in)
  {
    svtkErrorMacro("Could not open file for reading: " << (this->FileName ? this->FileName : ""));
    return 1;
  }

  // Advance to the selected timestep:
  size_t stepIdx = this->SelectTimeStepIndex(outInfo);
  double time = 0.;
  for (size_t i = 0; i <= stepIdx; ++i) // <= to read the "time=" line
  {
    if (!this->NextTimeStep(in, time))
    {
      svtkErrorMacro("Error -- attempting to read timestep #"
        << (stepIdx + 1) << " but encountered a parsing error at timestep #" << (i + 1) << ".");
      return 1;
    }
  }

  if (this->ReadMolecule(in, output))
  {
    output->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), time);
  }
  else
  {
    output->Initialize();
  }

  return 1;
}

//------------------------------------------------------------------------------
int svtkVASPAnimationReader::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outInfos)
{
  svtksys::ifstream in(this->FileName);
  if (!in)
  {
    svtkErrorMacro("Could not open file for reading: " << (this->FileName ? this->FileName : ""));
    return 1;
  }

  // Scan the file for timesteps:
  double time;
  std::vector<double> times;
  double timeRange[2] = { SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN };
  while (this->NextTimeStep(in, time))
  {
    times.push_back(time);
    timeRange[0] = std::min(timeRange[0], time);
    timeRange[1] = std::max(timeRange[1], time);
  }

  if (!times.empty())
  {
    svtkInformation* outInfo = outInfos->GetInformationObject(0);
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    outInfo->Set(
      svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0], static_cast<int>(times.size()));
  }

  return 1;
}

//------------------------------------------------------------------------------
bool svtkVASPAnimationReader::NextTimeStep(std::istream& in, double& time)
{
  std::string line;
  while (std::getline(in, line))
  {
    if (this->TimeParser->find(line))
    {
      // Parse timestamp:
      if (!parse(this->TimeParser->match(1), time))
      {
        svtkErrorMacro("Error parsing time information from line: " << line);
        return false;
      }
      return true;
    }
  }

  return false;
}

//------------------------------------------------------------------------------
size_t svtkVASPAnimationReader::SelectTimeStepIndex(svtkInformation* info)
{
  if (!info->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()) ||
    !info->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    return 0;
  }

  double* times = info->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int nTimes = info->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double t = info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  double resultDiff = SVTK_DOUBLE_MAX;
  size_t result = 0;
  for (int i = 0; i < nTimes; ++i)
  {
    double diff = std::fabs(times[i] - t);
    if (diff < resultDiff)
    {
      resultDiff = diff;
      result = static_cast<size_t>(i);
    }
  }

  return result;
}

//------------------------------------------------------------------------------
bool svtkVASPAnimationReader::ReadMolecule(std::istream& in, svtkMolecule* molecule)
{
  // Note: The leading time = XXXX line has already been read.
  std::string line;

  svtkVector3d lattice[3];
  for (size_t i = 0; i < 3; ++i)
  {
    if (!std::getline(in, line))
    {
      svtkErrorMacro("Error reading line " << (i + 1)
                                          << " of the lattice "
                                             "specification. Unexpected EOF.");
      return false;
    }
    if (!this->LatticeParser->find(line))
    {
      svtkErrorMacro("Error reading line " << (i + 1)
                                          << " of the lattice "
                                             "specification. Expected three floats: "
                                          << line);
      return false;
    }
    if (!parse(this->LatticeParser->match(1), lattice[i][0]))
    {
      svtkErrorMacro("Error reading line " << (i + 1)
                                          << " of the lattice "
                                             "specification. X component is not parsable: "
                                          << this->LatticeParser->match(1));
      return false;
    }
    if (!parse(this->LatticeParser->match(2), lattice[i][1]))
    {
      svtkErrorMacro("Error reading line " << (i + 1)
                                          << " of the lattice "
                                             "specification. Y component is not parsable: "
                                          << this->LatticeParser->match(2));
      return false;
    }
    if (!parse(this->LatticeParser->match(3), lattice[i][2]))
    {
      svtkErrorMacro("Error reading line " << (i + 1)
                                          << " of the lattice "
                                             "specification. Z component is not parsable: "
                                          << this->LatticeParser->match(3));
      return false;
    }
  }

  molecule->SetLattice(lattice[0], lattice[1], lattice[2]);
  molecule->SetLatticeOrigin(svtkVector3d(0.));

  // Next line should be the number of atoms in the molecule:
  if (!std::getline(in, line))
  {
    svtkErrorMacro("Unexpected EOF while parsing atom count.");
    return false;
  }
  if (!this->AtomCountParser->find(line))
  {
    svtkErrorMacro("Error parsing atom count from line: " << line);
    return false;
  }
  svtkIdType numAtoms;
  if (!parse(this->AtomCountParser->match(1), numAtoms))
  {
    svtkErrorMacro("Error parsing atom count as integer: " << this->AtomCountParser->match(1));
    return false;
  }

  // Create some attribute arrays to store the radii and energy.
  svtkNew<svtkFloatArray> radii;
  radii->SetName("radii");
  radii->SetNumberOfTuples(numAtoms);

  svtkNew<svtkFloatArray> kineticEnergies;
  kineticEnergies->SetName("kinetic_energy");
  kineticEnergies->SetNumberOfTuples(numAtoms);

  // Atoms are next:
  for (svtkIdType atomIdx = 0; atomIdx < numAtoms; ++atomIdx)
  {
    if (!std::getline(in, line))
    {
      svtkErrorMacro("Unexpected EOF while parsing atom at index " << atomIdx);
      return false;
    }
    if (!this->AtomParser->find(line))
    {
      svtkErrorMacro("Malformed atom specification: " << line);
      return false;
    }

    unsigned short atomicNumber;
    if (!parse(this->AtomParser->match(1), atomicNumber))
    {
      svtkErrorMacro(
        "Error parsing atomic number '" << this->AtomParser->match(1) << "' from line: " << line);
      return false;
    }

    svtkVector3f position;
    if (!parse(this->AtomParser->match(2), position[0]))
    {
      svtkErrorMacro(
        "Error parsing x coordinate '" << this->AtomParser->match(2) << "' from line: " << line);
      return false;
    }
    if (!parse(this->AtomParser->match(3), position[1]))
    {
      svtkErrorMacro(
        "Error parsing y coordinate '" << this->AtomParser->match(3) << "' from line: " << line);
      return false;
    }
    if (!parse(this->AtomParser->match(4), position[2]))
    {
      svtkErrorMacro(
        "Error parsing z coordinate '" << this->AtomParser->match(4) << "' from line: " << line);
      return false;
    }

    float radius;
    if (!parse(this->AtomParser->match(5), radius))
    {
      svtkErrorMacro(
        "Error parsing radius '" << this->AtomParser->match(5) << "' from line: " << line);
      return false;
    }

    float kineticEnergy;
    if (!parse(this->AtomParser->match(6), kineticEnergy))
    {
      svtkErrorMacro(
        "Error parsing kinetic energy '" << this->AtomParser->match(6) << "' from line: " << line);
      return false;
    }

    molecule->AppendAtom(atomicNumber, position);
    radii->SetTypedComponent(atomIdx, 0, radius);
    kineticEnergies->SetTypedComponent(atomIdx, 0, kineticEnergy);
  }

  svtkDataSetAttributes* atomData = molecule->GetVertexData();
  atomData->AddArray(radii);
  atomData->AddArray(kineticEnergies);

  return true;
}
