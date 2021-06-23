/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMREnzoParticlesReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkAMREnzoParticlesReader.h"
#include "svtkCellArray.h"
#include "svtkDataArray.h"
#include "svtkDataArraySelection.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

#include <cassert>
#include <vector>

#include "svtksys/SystemTools.hxx"

#define H5_USE_16_API
#include "svtk_hdf5.h" // for the HDF data loading engine

#include "svtkAMREnzoReaderInternal.h"

//------------------------------------------------------------------------------
//            HDF5 Utility Routines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Description:
// Finds the block index (blockIndx) within the HDF5 file associated with
// the given file index.
static bool FindBlockIndex(hid_t fileIndx, const int blockIdx, hid_t& rootIndx)
{
  // retrieve the contents of the root directory to look for a group
  // corresponding to the target block, if available, open that group
  hsize_t numbObjs;
  rootIndx = H5Gopen(fileIndx, "/");
  if (rootIndx < 0)
  {
    svtkGenericWarningMacro("Failed to open root node of particles file");
    return false;
  }

  bool found = false;
  H5Gget_num_objs(rootIndx, &numbObjs);
  for (int objIndex = 0; objIndex < static_cast<int>(numbObjs); objIndex++)
  {
    if (H5Gget_objtype_by_idx(rootIndx, objIndex) == H5G_GROUP)
    {
      int blckIndx;
      char blckName[65];
      H5Gget_objname_by_idx(rootIndx, objIndex, blckName, 64);

      // Is this the target block?
      if ((sscanf(blckName, "Grid%d", &blckIndx) == 1) && (blckIndx == blockIdx))
      {
        // located the target block
        rootIndx = H5Gopen(rootIndx, blckName);
        if (rootIndx < 0)
        {
          svtkGenericWarningMacro("Could not locate target block!\n");
        }
        found = true;
        break;
      }
    } // END if group
  }   // END for all objects
  return (found);
}

//------------------------------------------------------------------------------
// Description:
// Returns the double array
static void GetDoubleArrayByName(const hid_t rootIdx, const char* name, std::vector<double>& array)
{
  // turn off warnings
  void* pContext = nullptr;
  H5E_auto_t erorFunc;
  H5Eget_auto(&erorFunc, &pContext);
  H5Eset_auto(nullptr, nullptr);

  hid_t arrayIdx = H5Dopen(rootIdx, name);
  if (arrayIdx < 0)
  {
    svtkGenericWarningMacro("Cannot open array: " << name << "\n");
    return;
  }

  // turn warnings back on
  H5Eset_auto(erorFunc, pContext);
  pContext = nullptr;

  // get the number of particles
  hsize_t dimValus[3];
  hid_t spaceIdx = H5Dget_space(arrayIdx);
  H5Sget_simple_extent_dims(spaceIdx, dimValus, nullptr);
  int numbPnts = dimValus[0];

  array.resize(numbPnts);
  H5Dread(arrayIdx, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &array[0]);

  //  H5Dclose( spaceIdx );
  //  H5Dclose( arrayIdx );
}

//------------------------------------------------------------------------------
//          END of HDF5 Utility Routine definitions
//------------------------------------------------------------------------------

svtkStandardNewMacro(svtkAMREnzoParticlesReader);

//------------------------------------------------------------------------------
svtkAMREnzoParticlesReader::svtkAMREnzoParticlesReader()
{
  this->Internal = new svtkEnzoReaderInternal();
  this->ParticleType = -1; /* undefined particle type */
  this->Initialize();
}

//------------------------------------------------------------------------------
svtkAMREnzoParticlesReader::~svtkAMREnzoParticlesReader()
{
  delete this->Internal;
  this->Internal = nullptr;
}

//------------------------------------------------------------------------------
void svtkAMREnzoParticlesReader::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkAMREnzoParticlesReader::ReadMetaData()
{
  if (this->Initialized)
  {
    return;
  }

  if (!this->FileName)
  {
    svtkErrorMacro("No FileName set!");
    return;
  }

  this->Internal->SetFileName(this->FileName);
  std::string tempName(this->FileName);
  std::string bExtName(".boundary");
  std::string hExtName(".hierarchy");

  if (tempName.length() > hExtName.length() &&
    tempName.substr(tempName.length() - hExtName.length()) == hExtName)
  {
    this->Internal->MajorFileName = tempName.substr(0, tempName.length() - hExtName.length());
    this->Internal->HierarchyFileName = tempName;
    this->Internal->BoundaryFileName = this->Internal->MajorFileName + bExtName;
  }
  else if (tempName.length() > bExtName.length() &&
    tempName.substr(tempName.length() - bExtName.length()) == bExtName)
  {
    this->Internal->MajorFileName = tempName.substr(0, tempName.length() - bExtName.length());
    this->Internal->BoundaryFileName = tempName;
    this->Internal->HierarchyFileName = this->Internal->MajorFileName + hExtName;
  }
  else
  {
    svtkErrorMacro("Enzo file has invalid extension!");
    return;
  }

  this->Internal->DirectoryName = GetEnzoDirectory(this->Internal->MajorFileName.c_str());

  this->Internal->ReadMetaData();
  this->Internal->CheckAttributeNames();

  this->NumberOfBlocks = this->Internal->NumberOfBlocks;
  this->Initialized = true;

  this->SetupParticleDataSelections();
}

//------------------------------------------------------------------------------
svtkDataArray* svtkAMREnzoParticlesReader::GetParticlesTypeArray(const int blockIdx)
{

  svtkIntArray* array = svtkIntArray::New();
  if (this->ParticleDataArraySelection->ArrayExists("particle_type"))
  {
    this->Internal->LoadAttribute("particle_type", blockIdx);
    array->DeepCopy(this->Internal->DataArray);
  }
  return (array);
}

//------------------------------------------------------------------------------
bool svtkAMREnzoParticlesReader::CheckParticleType(const int idx, svtkIntArray* ptypes)
{
  assert("pre: particles type array should not be nullptr" && (ptypes != nullptr));

  if (ptypes->GetNumberOfTuples() > 0 &&
    this->ParticleDataArraySelection->ArrayExists("particle_type"))
  {
    int ptype = ptypes->GetValue(idx);
    if ((this->ParticleType == 0) || (ptype == this->ParticleType))
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return true;
  }
}

//------------------------------------------------------------------------------
svtkPolyData* svtkAMREnzoParticlesReader::GetParticles(const char* file, const int blockIdx)
{
  svtkPolyData* particles = svtkPolyData::New();
  svtkPoints* positions = svtkPoints::New();
  positions->SetDataTypeToDouble();
  svtkPointData* pdata = particles->GetPointData();

  hid_t fileIndx = H5Fopen(file, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileIndx < 0)
  {
    svtkErrorMacro("Failed opening particles file!");
    return nullptr;
  }

  hid_t rootIndx;
  if (!FindBlockIndex(fileIndx, blockIdx + 1, rootIndx))
  {
    svtkErrorMacro("Could not locate target block!");
    return nullptr;
  }

  //
  // Load the particles position arrays by name.
  // In Enzo the following arrays are available:
  //  ( 1 ) particle_position_i
  //  ( 2 ) tracer_particle_position_i
  //
  // where i \in {x,y,z}.
  std::vector<double> xcoords;
  std::vector<double> ycoords;
  std::vector<double> zcoords;

  // TODO: should we handle 2-D particle datasets?
  GetDoubleArrayByName(rootIndx, "particle_position_x", xcoords);
  GetDoubleArrayByName(rootIndx, "particle_position_y", ycoords);
  GetDoubleArrayByName(rootIndx, "particle_position_z", zcoords);

  svtkIntArray* particleTypes = svtkArrayDownCast<svtkIntArray>(this->GetParticlesTypeArray(blockIdx));

  assert("Coordinate arrays must have the same size: " && (xcoords.size() == ycoords.size()));
  assert("Coordinate arrays must have the same size: " && (ycoords.size() == zcoords.size()));

  int TotalNumberOfParticles = static_cast<int>(xcoords.size());
  positions->SetNumberOfPoints(TotalNumberOfParticles);

  svtkIdList* ids = svtkIdList::New();
  ids->SetNumberOfIds(TotalNumberOfParticles);

  svtkIdType NumberOfParticlesLoaded = 0;
  for (int i = 0; i < TotalNumberOfParticles; ++i)
  {
    if ((i % this->Frequency) == 0)
    {
      if (this->CheckLocation(xcoords[i], ycoords[i], zcoords[i]) &&
        this->CheckParticleType(i, particleTypes))
      {
        int pidx = NumberOfParticlesLoaded;
        ids->InsertId(pidx, i);
        positions->SetPoint(pidx, xcoords[i], ycoords[i], zcoords[i]);
        ++NumberOfParticlesLoaded;
      } // END if within requested region
    }   // END if within requested interval
  }     // END for all particles
  H5Gclose(rootIndx);
  H5Fclose(fileIndx);

  ids->SetNumberOfIds(NumberOfParticlesLoaded);
  ids->Squeeze();

  positions->SetNumberOfPoints(NumberOfParticlesLoaded);
  positions->Squeeze();

  particles->SetPoints(positions);
  positions->Delete();

  // Create CellArray consisting of a single polyvertex cell
  svtkCellArray* polyVertex = svtkCellArray::New();

  polyVertex->InsertNextCell(NumberOfParticlesLoaded);
  for (svtkIdType idx = 0; idx < NumberOfParticlesLoaded; ++idx)
    polyVertex->InsertCellPoint(idx);

  particles->SetVerts(polyVertex);
  polyVertex->Delete();

  // Release the particle types array
  particleTypes->Delete();

  int numArrays = this->ParticleDataArraySelection->GetNumberOfArrays();
  for (int i = 0; i < numArrays; ++i)
  {
    const char* name = this->ParticleDataArraySelection->GetArrayName(i);
    if (this->ParticleDataArraySelection->ArrayIsEnabled(name))
    {
      // Note: 0-based indexing is used for loading particles
      this->Internal->LoadAttribute(name, blockIdx);
      assert("pre: particle attribute size mismatch" &&
        (this->Internal->DataArray->GetNumberOfTuples() == TotalNumberOfParticles));

      svtkDataArray* array = this->Internal->DataArray->NewInstance();
      array->SetName(this->Internal->DataArray->GetName());
      array->SetNumberOfTuples(NumberOfParticlesLoaded);
      array->SetNumberOfComponents(this->Internal->DataArray->GetNumberOfComponents());

      svtkIdType numIds = ids->GetNumberOfIds();
      for (svtkIdType pidx = 0; pidx < numIds; ++pidx)
      {
        svtkIdType particleIdx = ids->GetId(pidx);
        int numComponents = array->GetNumberOfComponents();
        for (int k = 0; k < numComponents; ++k)
        {
          array->SetComponent(pidx, k, this->Internal->DataArray->GetComponent(particleIdx, k));
        } // END for all components
      }   // END for all ids of loaded particles
      pdata->AddArray(array);
      array->Delete();
    } // END if the array is supposed to be loaded
  }   // END for all particle arrays

  ids->Delete();
  return (particles);
}

//------------------------------------------------------------------------------
void svtkAMREnzoParticlesReader::SetupParticleDataSelections()
{
  assert("pre: Intenal reader is nullptr" && (this->Internal != nullptr));

  unsigned int N = static_cast<unsigned int>(this->Internal->ParticleAttributeNames.size());
  for (unsigned int i = 0; i < N; ++i)
  {
    bool isParticleAttribute = svtksys::SystemTools::StringStartsWith(
      this->Internal->ParticleAttributeNames[i].c_str(), "particle_");

    if (isParticleAttribute)
    {
      this->ParticleDataArraySelection->AddArray(this->Internal->ParticleAttributeNames[i].c_str());
    }
  } // END for all particles attributes
  this->InitializeParticleDataSelections();
}

//------------------------------------------------------------------------------
int svtkAMREnzoParticlesReader::GetTotalNumberOfParticles()
{
  assert("Internal reader is null" && (this->Internal != nullptr));
  int numParticles = 0;
  for (int blockIdx = 0; blockIdx < this->NumberOfBlocks; ++blockIdx)
  {
    numParticles += this->Internal->Blocks[blockIdx].NumberOfParticles;
  }
  return (numParticles);
}

//------------------------------------------------------------------------------
svtkPolyData* svtkAMREnzoParticlesReader::ReadParticles(const int blkidx)
{
  // this->Internal->Blocks includes a pseudo block -- the roo as block #0
  int iBlockIdx = blkidx + 1;
  int NumParticles = this->Internal->Blocks[iBlockIdx].NumberOfParticles;

  if (NumParticles <= 0)
  {
    svtkPolyData* emptyParticles = svtkPolyData::New();
    assert("Cannot create particles dataset" && (emptyParticles != nullptr));
    return (emptyParticles);
  }

  std::string pfile = this->Internal->Blocks[iBlockIdx].ParticleFileName;
  if (pfile.empty())
  {
    svtkErrorMacro("No particles file found, string is empty!");
    return nullptr;
  }

  svtkPolyData* particles = this->GetParticles(pfile.c_str(), blkidx);
  return (particles);
}
