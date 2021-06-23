/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAMREnzoReaderInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAMREnzoReaderInternal
 *
 *
 * Consists of the low-level Enzo Reader used by the svtkAMREnzoReader.
 *
 * @sa
 * svtkAMREnzoReader svtkAMREnzoParticlesReader
 */

#ifndef svtkAMREnzoReaderInternal_h
#define svtkAMREnzoReaderInternal_h

#include "svtksys/SystemTools.hxx"

#include <cassert> // for assert()
#include <string>  // for STL string
#include <vector>  // for STL vector

class svtkDataArray;
class svtkDataSet;

/*****************************************************************************
 *
 * Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
 * Produced at the Lawrence Livermore National Laboratory
 * LLNL-CODE-400124
 * All rights reserved.
 *
 * This file was adapted from the VisIt Enzo reader (avtEnzoFileFormat). For
 * details, see https://visit.llnl.gov/.  The full copyright notice is contained
 * in the file COPYRIGHT located at the root of the VisIt distribution or at
 * http://www.llnl.gov/visit/copyright.html.
 *
 *****************************************************************************/

static std::string GetEnzoDirectory(const char* path)
{
  return (svtksys::SystemTools::GetFilenamePath(std::string(path)));
}

// ----------------------------------------------------------------------------
//                       Class svtkEnzoReaderBlock (begin)
// ----------------------------------------------------------------------------

class svtkEnzoReaderBlock
{
public:
  svtkEnzoReaderBlock() { this->Init(); }
  ~svtkEnzoReaderBlock() { this->Init(); }
  svtkEnzoReaderBlock(const svtkEnzoReaderBlock& other) { this->DeepCopy(&other); }
  svtkEnzoReaderBlock& operator=(const svtkEnzoReaderBlock& other)
  {
    this->DeepCopy(&other);
    return *this;
  }

  int Index;
  int Level;
  int ParentId;
  std::vector<int> ChildrenIds;

  int MinParentWiseIds[3];
  int MaxParentWiseIds[3];
  int MinLevelBasedIds[3];
  int MaxLevelBasedIds[3];

  int NumberOfParticles;
  int NumberOfDimensions;
  int BlockCellDimensions[3];
  int BlockNodeDimensions[3];

  double MinBounds[3];
  double MaxBounds[3];
  double SubdivisionRatio[3];

  std::string BlockFileName;
  std::string ParticleFileName;

  void Init();
  void DeepCopy(const svtkEnzoReaderBlock* other);
  void GetParentWiseIds(std::vector<svtkEnzoReaderBlock>& blocks);
  void GetLevelBasedIds(std::vector<svtkEnzoReaderBlock>& blocks);
};

// ----------------------------------------------------------------------------
//                       Class svtkEnzoReaderBlock ( end )
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                     Class  svtkEnzoReaderInternal (begin)
// ----------------------------------------------------------------------------

class svtkEnzoReaderInternal
{
public:
  svtkEnzoReaderInternal();
  ~svtkEnzoReaderInternal();

  // number of all svtkDataSet (svtkImageData / svtkRectilinearGrid / svtkPolyData)
  // objects that have been SUCCESSFULLY extracted and inserted to the output
  // svtkMultiBlockDataSet (including rectilinear blocks and particle sets)
  int NumberOfMultiBlocks;

  int NumberOfDimensions;
  int NumberOfLevels;
  int NumberOfBlocks;
  int ReferenceBlock;
  int CycleIndex;
  char* FileName;
  double DataTime;
  svtkDataArray* DataArray;
  //  svtkAMREnzoReader * TheReader;

  std::string DirectoryName;
  std::string MajorFileName;
  std::string BoundaryFileName;
  std::string HierarchyFileName;
  std::vector<std::string> BlockAttributeNames;
  std::vector<std::string> ParticleAttributeNames;
  std::vector<std::string> TracerParticleAttributeNames;
  std::vector<svtkEnzoReaderBlock> Blocks;

  void Init();
  void ReleaseDataArray();
  void SetFileName(char* fileName) { this->FileName = fileName; }
  void ReadMetaData();
  void GetAttributeNames();
  void CheckAttributeNames();
  void ReadBlockStructures();
  void ReadGeneralParameters();
  void DetermineRootBoundingBox();
  int LoadAttribute(const char* attribute, int blockIdx);
  int GetBlockAttribute(const char* attribute, int blockIdx, svtkDataSet* pDataSet);
  std::string GetBaseDirectory(const char* path) { return GetEnzoDirectory(path); }
};

// ----------------------------------------------------------------------------
//                     Class  svtkEnzoReaderInternal ( end )
// ----------------------------------------------------------------------------

#endif /* svtkAMREnzoReaderInternal_h */
// SVTK-HeaderTest-Exclude: svtkAMREnzoReaderInternal.h
