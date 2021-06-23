/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAMReXGridReaderInternal.hpp

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAMReXGridReaderInternal
 *
 *
 *  Consists of the low-level AMReX Reader used by the svtkAMReXGridReader.
 *
 * @sa
 *  svtkAMReXGridReader
 */

#ifndef svtkAMReXGridReaderInternal_h
#define svtkAMReXGridReaderInternal_h
#ifndef __SVTK_WRAP__

#include <string>
#include <vector>

#include "svtkDataSet.h"

class svtkIndent;

//==============================================================================
//            I N T E R N A L   A M R e X     R E A D E R
//==============================================================================

// ----------------------------------------------------------------------------
//                     Class  RealDecriptor (begin)
// ----------------------------------------------------------------------------

/*
   floating point format specification (fmt):
    -   fmt[0] = # of bits per number
    -   fmt[1] = # of bits in exponent
    -   fmt[2] = # of bits in mantissa
    -   fmt[3] = start bit of sign
    -   fmt[4] = start bit of exponent
    -   fmt[5] = start bit of mantissa
    -   fmt[6] = high order mantissa bit (CRAY needs this)
    -   fmt[7] = bias of exponent

    64 11 52 0 1 12 0 1023 - IEEE Double

   byte order (ord) handles endianness (and defines size such as float or double)
    -   ord[0] = byte in 1st byte
    -   ord[1] = byte in 2nd byte
    -   ord[2] = byte in 3rd byte
    -   ord[3] = byte in 4th byte
    -   ...
*/

class RealDescriptor
{
public:
  RealDescriptor();
  RealDescriptor(const long* format, const int* order, int order_length);
  const long* format() const&;
  const std::vector<long>& formatarray() const&;
  const int* order() const&;
  const std::vector<int>& orderarray() const&;
  int numBytes() const;
  bool operator==(const RealDescriptor& rd) const;

private:
  std::vector<long> fr;
  std::vector<int> ord;
};

// ----------------------------------------------------------------------------
//                     Class  RealDecriptor ( end )
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                     Class  svtkAMReXGridHeader (begin)
// ----------------------------------------------------------------------------

class svtkAMReXGridHeader
{
public:
  std::string versionName;
  int variableNamesSize;
  std::vector<std::string> variableNames;
  int dim;
  double time;
  int finestLevel;
  std::vector<double> problemDomainLoEnd;
  std::vector<double> problemDomainHiEnd;
  std::vector<int> refinementRatio;
  std::vector<std::vector<std::vector<int> > > levelDomains;
  std::vector<int> levelSteps;
  std::vector<std::vector<double> > cellSize;
  int geometryCoord;
  int magicZero;
  std::vector<int> levelSize;
  std::vector<std::vector<std::vector<std::vector<double> > > > levelCells;
  std::vector<std::string> levelPrefix;
  std::vector<std::string> multiFabPrefix;
  bool debugHeader;

  svtkAMReXGridHeader();

  void PrintSelf(std::ostream& os, svtkIndent indent);
  void PrintSelfGenericHeader(std::ostream& os, svtkIndent indent);
  bool Parse(const std::string& headerData);
  bool ParseGenericHeader(const std::string& headerData);
};

// ----------------------------------------------------------------------------
//                     Class  svtkAMReXGridHeader ( end )
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                     Class  svtkAMReXGridLevelHeader (begin)
// ----------------------------------------------------------------------------

class svtkAMReXGridLevelHeader
{
  enum Version
  {
    Undefined_v1 = 0,          // ---- undefined
    Version_v1 = 1,            // ---- auto converting version with headers
                               // ---- for each fab in the data files and
                               // ---- min and max values for each fab in the header
    NoFabHeader_v1 = 2,        // ---- no fab headers, no fab mins or maxes
    NoFabHeaderMinMax_v1 = 3,  // ---- no fab headers,
                               // ---- min and max values for each fab in the header
    NoFabHeaderFAMinMax_v1 = 4 // ---- no fab headers, no fab mins or maxes,
                               // ---- min and max values for each FabArray in the header
  };
  enum Ordering
  {
    NormalOrder = 1,
    ReverseOrder = 2
  };

public:
  int level;
  int dim;
  int levelVersion;
  int levelHow;
  int levelNumberOfComponents;
  int levelNumberOfGhostCells;
  int levelBoxArraySize;
  int levelMagicZero;
  std::vector<std::vector<std::vector<int> > > levelBoxArrays;
  int levelNumberOfFABOnDisk;
  std::string levelFabOnDiskPrefix;
  std::vector<std::string> levelFABFile;
  std::vector<long> levelFileOffset;
  std::vector<std::vector<double> > levelMinimumsFAB;
  std::vector<std::vector<double> > levelMaximumsFAB;
  std::vector<double> levelFABArrayMinimum;
  std::vector<double> levelFABArrayMaximum;
  int levelRealNumberOfBytes;
  int levelRealOrder;
  bool debugLevelHeader;

  svtkAMReXGridLevelHeader();
  void PrintSelf(std::ostream& os, svtkIndent indent);
  void PrintSelfLevelHeader(std::ostream& os, svtkIndent indent);
  bool Parse(int _level, int _dim, const std::string& headerData);
  bool ParseLevelHeader(int _level, int _dim, const std::string& headerData);
};

// ----------------------------------------------------------------------------
//                     Class  svtkAMReXGridLevelHeader ( end )
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                     Class  svtkAMReXGridReaderInternal (begin)
// ----------------------------------------------------------------------------

class svtkAMReXGridReaderInternal
{
public:
  svtkAMReXGridReaderInternal();
  ~svtkAMReXGridReaderInternal();
  void DestroyHeader();
  void DestroyLevelHeader();
  void PrintSelf(std::ostream& os, svtkIndent indent);
  void SetFileName(char* fName);

  void ReadMetaData();
  bool ReadHeader();
  bool ReadLevelHeader();
  int GetNumberOfLevels();
  int GetBlockLevel(const int blockIdx);
  int GetNumberOfBlocks();
  int GetBlockIndexWithinLevel(int blockIdx, int level);
  void GetBlockAttribute(const char* attribute, int blockIdx, svtkDataSet* pDataSet);
  int GetOffsetOfAttribute(const char* attribute);
  void ReadFAB(std::istream& is);
  int ReadVersion(std::istream& is);
  void ReadOrder(std::istream& is, std::vector<int>& ar);
  void PrintOrder(std::vector<int>& ar);
  void ReadFormat(std::istream& is, std::vector<long>& ar);
  void PrintFormat(std::vector<long>& ar);
  RealDescriptor* ReadRealDescriptor(std::istream& is);
  int ReadBoxArray(std::istream& is, int* boxArray, int* boxArrayDim);
  void PrintBoxArray(int* boxArray);
  int ReadNumberOfAttributes(std::istream& is);
  void ReadBlockAttribute(std::istream& is, int numberOfPoints, int size, char* buffer);
  void Convert(
    void* out, const void* in, long nitems, const RealDescriptor& ord, const RealDescriptor& ird);
  void PermuteOrder(
    void* out, const void* in, long nitems, const int* outord, const int* inord, int REALSIZE);

  bool headersAreRead;
  bool debugReader;
  std::string FileName;
  svtkAMReXGridHeader* Header;
  friend class svtkAMReXGridHeader;
  std::vector<svtkAMReXGridLevelHeader*> LevelHeader;
  friend class svtkAMReXGridLeveHeader;
};

// ----------------------------------------------------------------------------
//                     Class  svtkAMReXGridReaderInternal ( end )
// ----------------------------------------------------------------------------
#endif
#endif /* svtkAMReXGridReaderInternal_h */
// SVTK-HeaderTest-Exclude: svtkAMReXGridReaderInternal.h
