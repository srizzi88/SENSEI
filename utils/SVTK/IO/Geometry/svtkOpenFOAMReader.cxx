/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenFOAMReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Thanks to Terry Jordan of SAIC at the National Energy
// Technology Laboratory who developed this class.
// Please address all comments to Terry Jordan (terry.jordan@sa.netl.doe.gov)
//
// Token-based FoamFile format lexer/parser,
// performance/stability/compatibility enhancements, gzipped file
// support, lagrangian field support, variable timestep support,
// builtin cell-to-point filter, pointField support, polyhedron
// decomposition support, OF 1.5 extended format support, multiregion
// support, old mesh format support, parallelization support for
// decomposed cases in conjunction with svtkPOpenFOAMReader, et. al. by
// Takuya Oshima of Niigata University, Japan (oshima@eng.niigata-u.ac.jp)
//
// * GUI Based selection of mesh regions and fields available in the case
// * Minor bug fixes / Strict memory allocation checks
// * Minor performance enhancements
// by Philippose Rajan (sarith@rocketmail.com)

// Hijack the CRC routine of zlib to omit CRC check for gzipped files
// (on OSes other than Windows where the mechanism doesn't work due
// to pre-bound DLL symbols) if set to 1, or not (set to 0). Affects
// performance by about 3% - 4%.
#define SVTK_FOAMFILE_OMIT_CRCCHECK 0

// The input/output buffer sizes for zlib in bytes.
#define SVTK_FOAMFILE_INBUFSIZE (16384)
#define SVTK_FOAMFILE_OUTBUFSIZE (131072)
#define SVTK_FOAMFILE_INCLUDE_STACK_SIZE (10)

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS 1
// No strtoll on msvc:
#define strtoll _strtoi64
#endif

#if SVTK_FOAMFILE_OMIT_CRCCHECK
#define ZLIB_INTERNAL
#endif

// for possible future extension of linehead-aware directives
#define SVTK_FOAMFILE_RECOGNIZE_LINEHEAD 0

#include "svtkOpenFOAMReader.h"

#include "svtk_zlib.h"
#include "svtksys/RegularExpression.hxx"
#include "svtksys/SystemTools.hxx"
#include <sstream>
#include <vector>

#include "svtkAssume.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkCollection.h"
#include "svtkConvexPointSet.h"
#include "svtkDataArraySelection.h"
#include "svtkDirectory.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkHexahedron.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkPyramid.h"
#include "svtkQuad.h"
#include "svtkSortDataArray.h"
#include "svtkStdString.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkTetra.h"
#include "svtkTriangle.h"
#include "svtkTypeInt32Array.h"
#include "svtkTypeInt64Array.h"
#include "svtkTypeTraits.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVertex.h"
#include "svtkWedge.h"
#include <svtksys/SystemTools.hxx>

#if !(defined(_WIN32) && !defined(__CYGWIN__) || defined(__LIBCATAMOUNT__))
// for getpwnam() / getpwuid()
#include <pwd.h>
#include <sys/types.h>
// for getuid()
#include <unistd.h>
#endif
// for fabs()
#include <cmath>
// for isalnum() / isspace() / isdigit()
#include <cctype>

#include <typeinfo>
#include <vector>

#if SVTK_FOAMFILE_OMIT_CRCCHECK
uLong ZEXPORT crc32(uLong, const Bytef*, uInt)
{
  return 0;
}
#endif

svtkStandardNewMacro(svtkOpenFOAMReader);

namespace
{

// Given a data array and a flag indicating whether 64 bit labels are used,
// lookup and return a single element in the array. The data array must
// be either a svtkTypeInt32Array or svtkTypeInt64Array.
svtkTypeInt64 GetLabelValue(svtkDataArray* array, svtkIdType idx, bool use64BitLabels)
{
  if (!use64BitLabels)
  {
    svtkTypeInt64 result =
      static_cast<svtkTypeInt64>(static_cast<svtkTypeInt32Array*>(array)->GetValue(idx));
    assert(result >= -1); // some arrays store -1 == 'uninitialized'.
    return result;
  }
  else
  {
    svtkTypeInt64 result = static_cast<svtkTypeInt64Array*>(array)->GetValue(idx);
    assert(result >= -1); // some arrays store -1 == 'uninitialized'.
    return result;
  }
}

// Setter analogous to the above getter.
void SetLabelValue(svtkDataArray* array, svtkIdType idx, svtkTypeInt64 value, bool use64BitLabels)
{
  if (!use64BitLabels)
  {
    assert(static_cast<svtkTypeInt32>(value) >= 0);
    static_cast<svtkTypeInt32Array*>(array)->SetValue(idx, static_cast<svtkTypeInt32>(value));
  }
  else
  {
    assert(value >= 0);
    static_cast<svtkTypeInt64Array*>(array)->SetValue(idx, value);
  }
}

// Similar to above, but increments the specified value by one.
void IncrementLabelValue(svtkDataArray* array, svtkIdType idx, bool use64BitLabels)
{
  if (!use64BitLabels)
  {
    svtkTypeInt32 val = static_cast<svtkTypeInt32Array*>(array)->GetValue(idx);
    assert(val + 1 >= 0);
    static_cast<svtkTypeInt32Array*>(array)->SetValue(idx, val + 1);
  }
  else
  {
    svtkTypeInt64 val = static_cast<svtkTypeInt64Array*>(array)->GetValue(idx);
    assert(val + 1 >= 0);
    static_cast<svtkTypeInt64Array*>(array)->SetValue(idx, val + 1);
  }
}

// Another helper for appending an id to a list
void AppendLabelValue(svtkDataArray* array, svtkTypeInt64 val, bool use64BitLabels)
{
  if (!use64BitLabels)
  {
    assert(static_cast<svtkTypeInt32>(val) >= 0);
    static_cast<svtkTypeInt32Array*>(array)->InsertNextValue(static_cast<svtkTypeInt32>(val));
  }
  else
  {
    assert(val >= 0);
    static_cast<svtkTypeInt64Array*>(array)->InsertNextValue(val);
  }
}

// Another 64/32 bit label helper. Given a void* c-array, set/get element idx.
// The array must be of svtkTypeInt32 or svtkTypeInt64.
void SetLabelValue(void* array, size_t idx, svtkTypeInt64 value, bool use64BitLabels)
{
  if (!use64BitLabels)
  {
    assert(static_cast<svtkTypeInt32>(value) >= 0);
    static_cast<svtkTypeInt32*>(array)[idx] = static_cast<svtkTypeInt32>(value);
  }
  else
  {
    assert(value >= 0);
    static_cast<svtkTypeInt64*>(array)[idx] = value;
  }
}
svtkTypeInt64 GetLabelValue(const void* array, size_t idx, bool use64BitLabels)
{
  if (!use64BitLabels)
  {
    svtkTypeInt64 result = static_cast<svtkTypeInt64>(static_cast<const svtkTypeInt32*>(array)[idx]);
    assert(result >= 0);
    return result;
  }
  else
  {
    svtkTypeInt64 result = static_cast<const svtkTypeInt64*>(array)[idx];
    assert(result >= 0);
    return result;
  }
}

} // end anon namespace

// forward declarations
template <typename T>
struct svtkFoamArrayVector : public std::vector<T*>
{
private:
  typedef std::vector<T*> Superclass;

public:
  ~svtkFoamArrayVector()
  {
    for (size_t arrayI = 0; arrayI < Superclass::size(); arrayI++)
    {
      if (Superclass::operator[](arrayI))
      {
        Superclass::operator[](arrayI)->Delete();
      }
    }
  }
};

typedef svtkFoamArrayVector<svtkDataArray> svtkFoamLabelArrayVector;
typedef svtkFoamArrayVector<svtkIntArray> svtkFoamIntArrayVector;
typedef svtkFoamArrayVector<svtkFloatArray> svtkFoamFloatArrayVector;

struct svtkFoamLabelVectorVector;
template <typename ArrayT>
struct svtkFoamLabelVectorVectorImpl;
typedef svtkFoamLabelVectorVectorImpl<svtkTypeInt32Array> svtkFoamLabel32VectorVector;
typedef svtkFoamLabelVectorVectorImpl<svtkTypeInt64Array> svtkFoamLabel64VectorVector;

struct svtkFoamError;
struct svtkFoamToken;
struct svtkFoamFileStack;
struct svtkFoamFile;
struct svtkFoamIOobject;
template <typename T>
struct svtkFoamReadValue;
struct svtkFoamEntryValue;
struct svtkFoamEntry;
struct svtkFoamDict;

//-----------------------------------------------------------------------------
// class svtkOpenFOAMReaderPrivate
// the reader core of svtkOpenFOAMReader
class svtkOpenFOAMReaderPrivate : public svtkObject
{
public:
  static svtkOpenFOAMReaderPrivate* New();
  svtkTypeMacro(svtkOpenFOAMReaderPrivate, svtkObject);

  svtkDoubleArray* GetTimeValues() { return this->TimeValues; }
  svtkGetMacro(TimeStep, int);
  svtkSetMacro(TimeStep, int);
  const svtkStdString& GetRegionName() const { return this->RegionName; }

  // gather timestep information
  bool MakeInformationVector(
    const svtkStdString&, const svtkStdString&, const svtkStdString&, svtkOpenFOAMReader*);
  // read mesh/fields and create dataset
  int RequestData(svtkMultiBlockDataSet*, bool, bool, bool);
  void SetTimeValue(const double);
  int MakeMetaDataAtTimeStep(svtkStringArray*, svtkStringArray*, svtkStringArray*, const bool);
  void SetupInformation(
    const svtkStdString&, const svtkStdString&, const svtkStdString&, svtkOpenFOAMReaderPrivate*);

private:
  struct svtkFoamBoundaryEntry
  {
    enum bt
    {
      PHYSICAL = 1,   // patch, wall
      PROCESSOR = 2,  // processor
      GEOMETRICAL = 0 // symmetryPlane, wedge, cyclic, empty, etc.
    };
    svtkStdString BoundaryName;
    svtkIdType NFaces, StartFace, AllBoundariesStartFace;
    bool IsActive;
    bt BoundaryType;
  };

  struct svtkFoamBoundaryDict : public std::vector<svtkFoamBoundaryEntry>
  {
    // we need to keep the path to time directory where the current mesh
    // is read from, since boundaryDict may be accessed multiple times
    // at a timestep for patch selections
    svtkStdString TimeDir;
  };

  svtkOpenFOAMReader* Parent;

  // case and region
  svtkStdString CasePath;
  svtkStdString RegionName;
  svtkStdString ProcessorName;

  // time information
  svtkDoubleArray* TimeValues;
  int TimeStep;
  int TimeStepOld;
  svtkStringArray* TimeNames;

  int InternalMeshSelectionStatus;
  int InternalMeshSelectionStatusOld;

  // filenames / directories
  svtkStringArray* VolFieldFiles;
  svtkStringArray* PointFieldFiles;
  svtkStringArray* LagrangianFieldFiles;
  svtkStringArray* PolyMeshPointsDir;
  svtkStringArray* PolyMeshFacesDir;

  // for mesh construction
  svtkIdType NumCells;
  svtkIdType NumPoints;
  svtkDataArray* FaceOwner;

  // for cell-to-point interpolation
  svtkPolyData* AllBoundaries;
  svtkDataArray* AllBoundariesPointMap;
  svtkDataArray* InternalPoints;

  // for caching mesh
  svtkUnstructuredGrid* InternalMesh;
  svtkMultiBlockDataSet* BoundaryMesh;
  svtkFoamLabelArrayVector* BoundaryPointMap;
  svtkFoamBoundaryDict BoundaryDict;
  svtkMultiBlockDataSet* PointZoneMesh;
  svtkMultiBlockDataSet* FaceZoneMesh;
  svtkMultiBlockDataSet* CellZoneMesh;

  // for polyhedra handling
  int NumTotalAdditionalCells;
  svtkIdTypeArray* AdditionalCellIds;
  svtkIntArray* NumAdditionalCells;
  svtkFoamLabelArrayVector* AdditionalCellPoints;

  // constructor and destructor are kept private
  svtkOpenFOAMReaderPrivate();
  ~svtkOpenFOAMReaderPrivate() override;

  svtkOpenFOAMReaderPrivate(const svtkOpenFOAMReaderPrivate&) = delete;
  void operator=(const svtkOpenFOAMReaderPrivate&) = delete;

  // clear mesh construction
  void ClearInternalMeshes();
  void ClearBoundaryMeshes();
  void ClearMeshes();

  svtkStdString RegionPath() const
  {
    return (this->RegionName.empty() ? "" : "/") + this->RegionName;
  }
  svtkStdString TimePath(const int timeI) const
  {
    return this->CasePath + this->TimeNames->GetValue(timeI);
  }
  svtkStdString TimeRegionPath(const int timeI) const
  {
    return this->TimePath(timeI) + this->RegionPath();
  }
  svtkStdString CurrentTimePath() const { return this->TimePath(this->TimeStep); }
  svtkStdString CurrentTimeRegionPath() const { return this->TimeRegionPath(this->TimeStep); }
  svtkStdString CurrentTimeRegionMeshPath(svtkStringArray* dir) const
  {
    return this->CasePath + dir->GetValue(this->TimeStep) + this->RegionPath() + "/polyMesh/";
  }
  svtkStdString RegionPrefix() const
  {
    return this->RegionName + (this->RegionName.empty() ? "" : "/");
  }

  // search time directories for mesh
  void AppendMeshDirToArray(svtkStringArray*, const svtkStdString&, const int);
  void PopulatePolyMeshDirArrays();

  // search a time directory for field objects
  void GetFieldNames(const svtkStdString&, const bool, svtkStringArray*, svtkStringArray*);
  void SortFieldFiles(svtkStringArray*, svtkStringArray*, svtkStringArray*);
  void LocateLagrangianClouds(svtkStringArray*, const svtkStdString&);

  // read controlDict
  bool ListTimeDirectoriesByControlDict(svtkFoamDict* dict);
  bool ListTimeDirectoriesByInstances();

  // read mesh files
  svtkFloatArray* ReadPointsFile();
  svtkFoamLabelVectorVector* ReadFacesFile(const svtkStdString&);
  svtkFoamLabelVectorVector* ReadOwnerNeighborFiles(const svtkStdString&, svtkFoamLabelVectorVector*);
  bool CheckFacePoints(svtkFoamLabelVectorVector*);

  // create mesh
  void InsertCellsToGrid(svtkUnstructuredGrid*, const svtkFoamLabelVectorVector*,
    const svtkFoamLabelVectorVector*, svtkFloatArray*, svtkIdTypeArray*, svtkDataArray*);
  svtkUnstructuredGrid* MakeInternalMesh(
    const svtkFoamLabelVectorVector*, const svtkFoamLabelVectorVector*, svtkFloatArray*);
  void InsertFacesToGrid(svtkPolyData*, const svtkFoamLabelVectorVector*, svtkIdType, svtkIdType,
    svtkDataArray*, svtkIdList*, svtkDataArray*, const bool);
  template <typename T1, typename T2>
  bool ExtendArray(T1*, svtkIdType);
  svtkMultiBlockDataSet* MakeBoundaryMesh(const svtkFoamLabelVectorVector*, svtkFloatArray*);
  void SetBlockName(svtkMultiBlockDataSet*, unsigned int, const char*);
  void TruncateFaceOwner();

  // move additional points for decomposed cells
  svtkPoints* MoveInternalMesh(svtkUnstructuredGrid*, svtkFloatArray*);
  void MoveBoundaryMesh(svtkMultiBlockDataSet*, svtkFloatArray*);

  // cell-to-point interpolator
  void InterpolateCellToPoint(
    svtkFloatArray*, svtkFloatArray*, svtkPointSet*, svtkDataArray*, svtkTypeInt64);

  // read and create cell/point fields
  void ConstructDimensions(svtkStdString*, svtkFoamDict*);
  bool ReadFieldFile(svtkFoamIOobject*, svtkFoamDict*, const svtkStdString&, svtkDataArraySelection*);
  svtkFloatArray* FillField(svtkFoamEntry*, svtkIdType, svtkFoamIOobject*, const svtkStdString&);
  void GetVolFieldAtTimeStep(svtkUnstructuredGrid*, svtkMultiBlockDataSet*, const svtkStdString&);
  void GetPointFieldAtTimeStep(svtkUnstructuredGrid*, svtkMultiBlockDataSet*, const svtkStdString&);
  void AddArrayToFieldData(svtkDataSetAttributes*, svtkDataArray*, const svtkStdString&);

  // create lagrangian mesh/fields
  svtkMultiBlockDataSet* MakeLagrangianMesh();

  // create point/face/cell zones
  svtkFoamDict* GatherBlocks(const char*, bool);
  bool GetPointZoneMesh(svtkMultiBlockDataSet*, svtkPoints*);
  bool GetFaceZoneMesh(svtkMultiBlockDataSet*, const svtkFoamLabelVectorVector*, svtkPoints*);
  bool GetCellZoneMesh(svtkMultiBlockDataSet*, const svtkFoamLabelVectorVector*,
    const svtkFoamLabelVectorVector*, svtkPoints*);
};

svtkStandardNewMacro(svtkOpenFOAMReaderPrivate);

//-----------------------------------------------------------------------------
// struct svtkFoamLabelVectorVector
struct svtkFoamLabelVectorVector
{
  typedef std::vector<svtkTypeInt64> CellType;

  virtual ~svtkFoamLabelVectorVector() = default;
  virtual size_t GetLabelSize() const = 0; // in bytes
  virtual void ResizeBody(svtkIdType bodyLength) = 0;
  virtual void* WritePointer(svtkIdType i, svtkIdType bodyI, svtkIdType number) = 0;
  virtual void SetIndex(svtkIdType i, svtkIdType bodyI) = 0;
  virtual void SetValue(svtkIdType bodyI, svtkTypeInt64 value) = 0;
  virtual void InsertValue(svtkIdType bodyI, svtkTypeInt64 value) = 0;
  virtual const void* operator[](svtkIdType i) const = 0;
  virtual svtkIdType GetSize(svtkIdType i) const = 0;
  virtual void GetCell(svtkIdType i, CellType& cell) const = 0;
  virtual void SetCell(svtkIdType i, const CellType& cell) = 0;
  virtual svtkIdType GetNumberOfElements() const = 0;
  virtual svtkDataArray* GetIndices() = 0;
  virtual svtkDataArray* GetBody() = 0;

  bool Is64Bit() const { return this->GetLabelSize() == 8; }
};

template <typename ArrayT>
struct svtkFoamLabelVectorVectorImpl : public svtkFoamLabelVectorVector
{
private:
  ArrayT* Indices;
  ArrayT* Body;

public:
  typedef ArrayT LabelArrayType;
  typedef typename ArrayT::ValueType LabelType;

  ~svtkFoamLabelVectorVectorImpl() override
  {
    this->Indices->Delete();
    this->Body->Delete();
  }

  // Construct from base class:
  svtkFoamLabelVectorVectorImpl(const svtkFoamLabelVectorVector& ivv)
    : Indices(nullptr)
    , Body(nullptr)
  {
    assert(
      "LabelVectorVectors use the same label width." && this->GetLabelSize() == ivv.GetLabelSize());

    typedef svtkFoamLabelVectorVectorImpl<LabelArrayType> ThisType;
    const ThisType& ivvCast = static_cast<const ThisType&>(ivv);

    this->Indices = ivvCast.Indices;
    this->Body = ivvCast.Body;
    this->Indices->Register(nullptr); // ref count the copy
    this->Body->Register(nullptr);
  }

  svtkFoamLabelVectorVectorImpl(const svtkFoamLabelVectorVectorImpl<ArrayT>& ivv)
    : Indices(ivv.Indices)
    , Body(ivv.Body)
  {
    this->Indices->Register(0); // ref count the copy
    this->Body->Register(0);
  }

  svtkFoamLabelVectorVectorImpl()
    : Indices(LabelArrayType::New())
    , Body(LabelArrayType::New())
  {
  }

  svtkFoamLabelVectorVectorImpl(svtkIdType nElements, svtkIdType bodyLength)
    : Indices(LabelArrayType::New())
    , Body(LabelArrayType::New())
  {
    this->Indices->SetNumberOfValues(nElements + 1);
    this->Body->SetNumberOfValues(bodyLength);
  }

  size_t GetLabelSize() const override { return sizeof(LabelType); }

  // note that svtkIntArray::Resize() allocates (current size + new
  // size) bytes if current size < new size until 2010-06-27
  // cf. commit c869c3d5875f503e757b64f2fd1ec349aee859bf
  void ResizeBody(svtkIdType bodyLength) override { this->Body->Resize(bodyLength); }

  void* WritePointer(svtkIdType i, svtkIdType bodyI, svtkIdType number) override
  {
    return this->Body->WritePointer(*this->Indices->GetPointer(i) = bodyI, number);
  }

  void SetIndex(svtkIdType i, svtkIdType bodyI) override
  {
    this->Indices->SetValue(i, static_cast<LabelType>(bodyI));
  }

  void SetValue(svtkIdType bodyI, svtkTypeInt64 value) override
  {
    this->Body->SetValue(bodyI, static_cast<LabelType>(value));
  }

  void InsertValue(svtkIdType bodyI, svtkTypeInt64 value) override
  {
    this->Body->InsertValue(bodyI, value);
  }

  const void* operator[](svtkIdType i) const override
  {
    return this->Body->GetPointer(this->Indices->GetValue(i));
  }

  svtkIdType GetSize(svtkIdType i) const override
  {
    return this->Indices->GetValue(i + 1) - this->Indices->GetValue(i);
  }

  void GetCell(svtkIdType cellId, CellType& cell) const override
  {
    LabelType cellStart = this->Indices->GetValue(cellId);
    LabelType cellSize = this->Indices->GetValue(cellId + 1) - cellStart;
    cell.resize(cellSize);
    for (svtkIdType i = 0; i < cellSize; ++i)
    {
      cell[i] = this->Body->GetValue(cellStart + i);
    }
  }

  void SetCell(svtkIdType cellId, const CellType& cell) override
  {
    LabelType cellStart = this->Indices->GetValue(cellId);
    LabelType cellSize = this->Indices->GetValue(cellId + 1) - cellStart;
    for (svtkIdType i = 0; i < cellSize; ++i)
    {
      this->Body->SetValue(cellStart + i, cell[i]);
    }
  }

  svtkIdType GetNumberOfElements() const override { return this->Indices->GetNumberOfTuples() - 1; }

  svtkDataArray* GetIndices() override { return this->Indices; }

  svtkDataArray* GetBody() override { return this->Body; }
};

//-----------------------------------------------------------------------------
// class svtkFoamError
// class for exception-carrying object
struct svtkFoamError : public svtkStdString
{
private:
  typedef svtkStdString Superclass;

public:
  // a super-easy way to make use of operator<<()'s defined in
  // std::ostringstream class
  template <class T>
  svtkFoamError& operator<<(const T& t)
  {
    std::ostringstream os;
    os << t;
    this->Superclass::operator+=(os.str());
    return *this;
  }
};

//-----------------------------------------------------------------------------
// class svtkFoamToken
// token class which also works as container for list types
// - a word token is treated as a string token for simplicity
// - handles only atomic types. Handling of list types are left to the
//   derived classes.
struct svtkFoamToken
{
public:
  enum tokenType
  {
    // undefined type
    UNDEFINED,
    // atomic types
    PUNCTUATION,
    LABEL,
    SCALAR,
    STRING,
    IDENTIFIER,
    // svtkObject-derived list types
    STRINGLIST,
    LABELLIST,
    SCALARLIST,
    VECTORLIST,
    // original list types
    LABELLISTLIST,
    ENTRYVALUELIST,
    BOOLLIST,
    EMPTYLIST,
    DICTIONARY,
    // error state
    TOKEN_ERROR
  };

  // Bitwidth of labels.
  enum labelType
  {
    NO_LABEL_TYPE = 0, // Used for assertions.
    INT32,
    INT64
  };

protected:
  tokenType Type;
  labelType LabelType;
  union {
    char Char;
    svtkTypeInt64 Int;
    double Double;
    svtkStdString* String;
    svtkObjectBase* VtkObjectPtr;
    // svtkObject-derived list types
    svtkDataArray* LabelListPtr;
    svtkFloatArray *ScalarListPtr, *VectorListPtr;
    svtkStringArray* StringListPtr;
    // original list types
    svtkFoamLabelVectorVector* LabelListListPtr;
    std::vector<svtkFoamEntryValue*>* EntryValuePtrs;
    svtkFoamDict* DictPtr;
  };

  void Clear()
  {
    if (this->Type == STRING || this->Type == IDENTIFIER)
    {
      delete this->String;
    }
  }

  void AssignData(const svtkFoamToken& value)
  {
    switch (value.Type)
    {
      case PUNCTUATION:
        this->Char = value.Char;
        break;
      case LABEL:
        this->Int = value.Int;
        break;
      case SCALAR:
        this->Double = value.Double;
        break;
      case STRING:
      case IDENTIFIER:
        this->String = new svtkStdString(*value.String);
        break;
      case UNDEFINED:
      case STRINGLIST:
      case LABELLIST:
      case SCALARLIST:
      case VECTORLIST:
      case LABELLISTLIST:
      case ENTRYVALUELIST:
      case BOOLLIST:
      case EMPTYLIST:
      case DICTIONARY:
      case TOKEN_ERROR:
        break;
    }
  }

public:
  svtkFoamToken()
    : Type(UNDEFINED)
    , LabelType(NO_LABEL_TYPE)
  {
  }
  svtkFoamToken(const svtkFoamToken& value)
    : Type(value.Type)
    , LabelType(value.LabelType)
  {
    this->AssignData(value);
  }
  ~svtkFoamToken() { this->Clear(); }

  tokenType GetType() const { return this->Type; }

  void SetLabelType(labelType type) { this->LabelType = type; }
  labelType GetLabelType() const { return this->LabelType; }

  template <typename T>
  bool Is() const;
  template <typename T>
  T To() const;
#if defined(_MSC_VER)
  // workaround for Win32-64ids-nmake70
  template <>
  bool Is<svtkTypeInt32>() const;
  template <>
  bool Is<svtkTypeInt64>() const;
  template <>
  bool Is<float>() const;
  template <>
  bool Is<double>() const;
  template <>
  svtkTypeInt32 To<svtkTypeInt32>() const;
  template <>
  svtkTypeInt64 To<svtkTypeInt64>() const;
  template <>
  float To<float>() const;
  template <>
  double To<double>() const;
#endif

  // workaround for SunOS-CC5.6-dbg
  svtkTypeInt64 ToInt() const
  {
    assert("Label type not set!" && this->LabelType != NO_LABEL_TYPE);
    return this->Int;
  }

  // workaround for SunOS-CC5.6-dbg
  float ToFloat() const
  {
    return this->Type == LABEL ? static_cast<float>(this->Int) : static_cast<float>(this->Double);
  }

  svtkStdString ToString() const { return *this->String; }
  svtkStdString ToIdentifier() const { return *this->String; }

  void SetBad()
  {
    this->Clear();
    this->Type = TOKEN_ERROR;
  }
  void SetIdentifier(const svtkStdString& idString)
  {
    this->operator=(idString);
    this->Type = IDENTIFIER;
  }

  void operator=(const char value)
  {
    this->Clear();
    this->Type = PUNCTUATION;
    this->Char = value;
  }
  void operator=(const svtkTypeInt32 value)
  {
    this->Clear();

    assert("Label type not set!" && this->LabelType != NO_LABEL_TYPE);
    if (this->LabelType == INT64)
    {
      svtkGenericWarningMacro("Setting a 64 bit label from a 32 bit integer.");
    }

    this->Type = LABEL;
    this->Int = static_cast<svtkTypeInt32>(value);
  }
  void operator=(const svtkTypeInt64 value)
  {
    this->Clear();

    assert("Label type not set!" && this->LabelType != NO_LABEL_TYPE);
    if (this->LabelType == INT32)
    {
      svtkGenericWarningMacro("Setting a 32 bit label from a 64 bit integer. "
                             "Precision loss may occur.");
    }

    this->Type = LABEL;
    this->Int = value;
  }
  void operator=(const double value)
  {
    this->Clear();
    this->Type = SCALAR;
    this->Double = value;
  }
  void operator=(const char* value)
  {
    this->Clear();
    this->Type = STRING;
    this->String = new svtkStdString(value);
  }
  void operator=(const svtkStdString& value)
  {
    this->Clear();
    this->Type = STRING;
    this->String = new svtkStdString(value);
  }
  svtkFoamToken& operator=(const svtkFoamToken& value)
  {
    this->Clear();
    this->Type = value.Type;
    this->LabelType = value.LabelType;
    this->AssignData(value);
    return *this;
  }
  bool operator==(const char value) const
  {
    return this->Type == PUNCTUATION && this->Char == value;
  }
  bool operator==(const svtkTypeInt32 value) const
  {
    assert("Label type not set!" && this->LabelType != NO_LABEL_TYPE);
    return this->Type == LABEL && this->Int == static_cast<svtkTypeInt64>(value);
  }
  bool operator==(const svtkTypeInt64 value) const
  {
    assert("Label type not set!" && this->LabelType != NO_LABEL_TYPE);
    return this->Type == LABEL && this->Int == value;
  }
  bool operator==(const svtkStdString& value) const
  {
    return this->Type == STRING && *this->String == value;
  }
  bool operator!=(const svtkStdString& value) const
  {
    return this->Type != STRING || *this->String != value;
  }
  bool operator!=(const char value) const { return !this->operator==(value); }

  friend std::ostringstream& operator<<(std::ostringstream& str, const svtkFoamToken& value)
  {
    switch (value.GetType())
    {
      case TOKEN_ERROR:
        str << "badToken (an unexpected EOF?)";
        break;
      case PUNCTUATION:
        str << value.Char;
        break;
      case LABEL:
        assert("Label type not set!" && value.LabelType != NO_LABEL_TYPE);
        if (value.LabelType == INT32)
        {
          str << static_cast<svtkTypeInt32>(value.Int);
        }
        else
        {
          str << value.Int;
        }
        break;
      case SCALAR:
        str << value.Double;
        break;
      case STRING:
      case IDENTIFIER:
        str << *value.String;
        break;
      case UNDEFINED:
      case STRINGLIST:
      case LABELLIST:
      case SCALARLIST:
      case VECTORLIST:
      case LABELLISTLIST:
      case ENTRYVALUELIST:
      case BOOLLIST:
      case EMPTYLIST:
      case DICTIONARY:
        break;
    }
    return str;
  }
};

template <>
inline bool svtkFoamToken::Is<char>() const
{
  // masquerade for bool
  return this->Type == LABEL;
}

template <>
inline bool svtkFoamToken::Is<svtkTypeInt32>() const
{
  assert("Label type not set!" && this->LabelType != NO_LABEL_TYPE);
  return this->Type == LABEL && this->LabelType == INT32;
}

template <>
inline bool svtkFoamToken::Is<svtkTypeInt64>() const
{
  assert("Label type not set!" && this->LabelType != NO_LABEL_TYPE);
  return this->Type == LABEL;
}

template <>
inline bool svtkFoamToken::Is<float>() const
{
  return this->Type == LABEL || this->Type == SCALAR;
}

template <>
inline bool svtkFoamToken::Is<double>() const
{
  return this->Type == SCALAR;
}

template <>
inline char svtkFoamToken::To<char>() const
{
  return static_cast<char>(this->Int);
}

template <>
inline svtkTypeInt32 svtkFoamToken::To<svtkTypeInt32>() const
{
  assert("Label type not set!" && this->LabelType != NO_LABEL_TYPE);
  if (this->LabelType == INT64)
  {
    svtkGenericWarningMacro("Casting 64 bit label to int32. Precision loss "
                           "may occur.");
  }
  return static_cast<svtkTypeInt32>(this->Int);
}

template <>
inline svtkTypeInt64 svtkFoamToken::To<svtkTypeInt64>() const
{
  assert("Label type not set!" && this->LabelType != NO_LABEL_TYPE);
  return this->Int;
}

template <>
inline float svtkFoamToken::To<float>() const
{
  return this->Type == LABEL ? static_cast<float>(this->Int) : static_cast<float>(this->Double);
}

template <>
inline double svtkFoamToken::To<double>() const
{
  return this->Type == LABEL ? static_cast<double>(this->Int) : this->Double;
}

//-----------------------------------------------------------------------------
// class svtkFoamFileStack
// list of variables that have to be saved when a file is included.
struct svtkFoamFileStack
{
protected:
  svtkOpenFOAMReader* Reader;
  svtkStdString FileName;
  FILE* File;
  bool IsCompressed;
  z_stream Z;
  int ZStatus;
  int LineNumber;
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
  bool WasNewline;
#endif

  // buffer pointers. using raw pointers for performance reason.
  unsigned char* Inbuf;
  unsigned char* Outbuf;
  unsigned char* BufPtr;
  unsigned char* BufEndPtr;

  svtkFoamFileStack(svtkOpenFOAMReader* reader)
    : Reader(reader)
    , FileName()
    , File(nullptr)
    , IsCompressed(false)
    , ZStatus(Z_OK)
    , LineNumber(0)
    ,
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
    WasNewline(true)
    ,
#endif
    Inbuf(nullptr)
    , Outbuf(nullptr)
    , BufPtr(nullptr)
    , BufEndPtr(nullptr)
  {
    this->Z.zalloc = Z_NULL;
    this->Z.zfree = Z_NULL;
    this->Z.opaque = Z_NULL;
  }

  void Reset()
  {
    // this->FileName = "";
    this->File = nullptr;
    this->IsCompressed = false;
    // this->ZStatus = Z_OK;
    this->Z.zalloc = Z_NULL;
    this->Z.zfree = Z_NULL;
    this->Z.opaque = Z_NULL;
    // this->LineNumber = 0;
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
    this->WasNewline = true;
#endif

    this->Inbuf = nullptr;
    this->Outbuf = nullptr;
    // this->BufPtr = nullptr;
    // this->BufEndPtr = nullptr;
  }

public:
  const svtkStdString& GetFileName() const { return this->FileName; }
  int GetLineNumber() const { return this->LineNumber; }
  svtkOpenFOAMReader* GetReader() const { return this->Reader; }
};

//-----------------------------------------------------------------------------
// class svtkFoamFile
// read and tokenize the input.
struct svtkFoamFile : public svtkFoamFileStack
{
private:
  typedef svtkFoamFileStack Superclass;

public:
  // #inputMode values
  enum inputModes
  {
    INPUT_MODE_MERGE,
    INPUT_MODE_OVERWRITE,
    INPUT_MODE_PROTECT,
    INPUT_MODE_WARN,
    INPUT_MODE_ERROR
  };

private:
  inputModes InputMode;

  // inclusion handling
  svtkFoamFileStack* Stack[SVTK_FOAMFILE_INCLUDE_STACK_SIZE];
  int StackI;
  svtkStdString CasePath;

  // declare and define as private
  svtkFoamFile() = delete;
  bool InflateNext(unsigned char* buf, int requestSize, int* readSize = nullptr);
  int NextTokenHead();
  // hacks to keep exception throwing / recursive codes out-of-line to make
  // putBack(), getc() and readExpecting() inline expandable
  void ThrowDuplicatedPutBackException();
  void ThrowUnexpectedEOFException();
  void ThrowUnexpectedNondigitCharExecption(const int c);
  void ThrowUnexpectedTokenException(const char, const int c);
  int ReadNext();

  void PutBack(const int c)
  {
    if (--this->Superclass::BufPtr < this->Superclass::Outbuf)
    {
      this->ThrowDuplicatedPutBackException();
    }
    *this->Superclass::BufPtr = static_cast<unsigned char>(c);
  }

  // get a character
  int Getc()
  {
    return this->Superclass::BufPtr == this->Superclass::BufEndPtr ? this->ReadNext()
                                                                   : *this->Superclass::BufPtr++;
  }

  svtkFoamError StackString()
  {
    std::ostringstream os;
    if (this->StackI > 0)
    {
      os << "\n included";

      for (int stackI = this->StackI - 1; stackI >= 0; stackI--)
      {
        os << " from line " << this->Stack[stackI]->GetLineNumber() << " of "
           << this->Stack[stackI]->GetFileName() << "\n";
      }
      os << ": ";
    }
    return svtkFoamError() << os.str();
  }

  bool CloseIncludedFile()
  {
    if (this->StackI == 0)
    {
      return false;
    }
    this->Clear();
    this->StackI--;
    // use the default bitwise assignment operator
    this->Superclass::operator=(*this->Stack[this->StackI]);
    delete this->Stack[this->StackI];
    return true;
  }

  void Clear()
  {
    if (this->Superclass::IsCompressed)
    {
      inflateEnd(&this->Superclass::Z);
    }

    delete[] this->Superclass::Inbuf;
    delete[] this->Superclass::Outbuf;
    this->Superclass::Inbuf = this->Superclass::Outbuf = nullptr;

    if (this->Superclass::File)
    {
      fclose(this->Superclass::File);
      this->Superclass::File = nullptr;
    }
    // don't reset the line number so that the last line number is
    // retained after close
    // lineNumber_ = 0;
  }

  //! Return file name (part beyond last /)
  svtkStdString ExtractName(const svtkStdString& path) const
  {
#if defined(_WIN32)
    const svtkStdString pathFindSeparator = "/\\", pathSeparator = "\\";
#else
    const svtkStdString pathFindSeparator = "/", pathSeparator = "/";
#endif
    svtkStdString::size_type pos = path.find_last_of(pathFindSeparator);
    if (pos == svtkStdString::npos)
    {
      // no slash
      return path;
    }
    else if (pos + 1 == path.size())
    {
      // final trailing slash
      svtkStdString::size_type endPos = pos;
      pos = path.find_last_of(pathFindSeparator, pos - 1);
      if (pos == svtkStdString::npos)
      {
        // no further slash
        return path.substr(0, endPos);
      }
      else
      {
        return path.substr(pos + 1, endPos - pos - 1);
      }
    }
    else
    {
      return path.substr(pos + 1, svtkStdString::npos);
    }
  }

  //! Return directory path name (part before last /)
  svtkStdString ExtractPath(const svtkStdString& path) const
  {
#if defined(_WIN32)
    const svtkStdString pathFindSeparator = "/\\", pathSeparator = "\\";
#else
    const svtkStdString pathFindSeparator = "/", pathSeparator = "/";
#endif
    const svtkStdString::size_type pos = path.find_last_of(pathFindSeparator);
    return pos == svtkStdString::npos ? svtkStdString(".") + pathSeparator : path.substr(0, pos + 1);
  }

public:
  svtkFoamFile(const svtkStdString& casePath, svtkOpenFOAMReader* reader)
    : svtkFoamFileStack(reader)
    , InputMode(INPUT_MODE_ERROR)
    , StackI(0)
    , CasePath(casePath)
  {
  }
  ~svtkFoamFile() { this->Close(); }

  inputModes GetInputMode() const { return this->InputMode; }
  svtkStdString GetCasePath() const { return this->CasePath; }
  svtkStdString GetFilePath() const { return this->ExtractPath(this->FileName); }

  svtkStdString ExpandPath(const svtkStdString& pathIn, const svtkStdString& defaultPath)
  {
    svtkStdString expandedPath;
    bool isExpanded = false, wasPathSeparator = true;
    const size_t nChars = pathIn.length();
    for (size_t charI = 0; charI < nChars;)
    {
      char c = pathIn[charI];
      switch (c)
      {
        case '$': // $-variable expansion
        {
          svtkStdString variable;
          while (++charI < nChars && (isalnum(pathIn[charI]) || pathIn[charI] == '_'))
          {
            variable += pathIn[charI];
          }
          if (variable == "FOAM_CASE") // discard path until the variable
          {
            expandedPath = this->CasePath;
            wasPathSeparator = true;
            isExpanded = true;
          }
          else if (variable == "FOAM_CASENAME")
          {
            // FOAM_CASENAME is the final directory name from CasePath
            expandedPath += this->ExtractName(this->CasePath);
            wasPathSeparator = false;
            isExpanded = true;
          }
          else
          {
            const char* value = getenv(variable.c_str());
            if (value != nullptr)
            {
              expandedPath += value;
            }
            const svtkStdString::size_type len = expandedPath.length();
            if (len > 0)
            {
              const char c2 = expandedPath[len - 1];
              wasPathSeparator = (c2 == '/' || c2 == '\\');
            }
            else
            {
              wasPathSeparator = false;
            }
          }
        }
        break;
        case '~': // home directory expansion
          // not using svtksys::SystemTools::ConvertToUnixSlashes() for
          // a bit better handling of "~"
          if (wasPathSeparator)
          {
            svtkStdString userName;
            while (++charI < nChars && (pathIn[charI] != '/' && pathIn[charI] != '\\') &&
              pathIn[charI] != '$')
            {
              userName += pathIn[charI];
            }
            if (userName.empty())
            {
              const char* homePtr = getenv("HOME");
              if (homePtr == nullptr)
              {
#if defined(_WIN32) && !defined(__CYGWIN__) || defined(__LIBCATAMOUNT__)
                expandedPath = "";
#else
                const struct passwd* pwentry = getpwuid(getuid());
                if (pwentry == nullptr)
                {
                  throw this->StackString() << "Home directory path not found";
                }
                expandedPath = pwentry->pw_dir;
#endif
              }
              else
              {
                expandedPath = homePtr;
              }
            }
            else
            {
#if defined(_WIN32) && !defined(__CYGWIN__) || defined(__LIBCATAMOUNT__)
              const char* homePtr = getenv("HOME");
              expandedPath = this->ExtractPath(homePtr ? homePtr : "") + userName;
#else
              if (userName == "OpenFOAM")
              {
                // so far only "~/.OpenFOAM" expansion is supported
                const char* homePtr = getenv("HOME");
                if (homePtr == nullptr)
                {
                  expandedPath = "";
                }
                else
                {
                  expandedPath = svtkStdString(homePtr) + "/.OpenFOAM";
                }
              }
              else
              {
                const struct passwd* pwentry = getpwnam(userName.c_str());
                if (pwentry == nullptr)
                {
                  throw this->StackString()
                    << "Home directory for user " << userName.c_str() << " not found";
                }
                expandedPath = pwentry->pw_dir;
              }
#endif
            }
            wasPathSeparator = false;
            isExpanded = true;
            break;
          }
          SVTK_FALLTHROUGH;
        default:
          wasPathSeparator = (c == '/' || c == '\\');
          expandedPath += c;
          charI++;
      }
    }
    if (isExpanded || expandedPath.substr(0, 1) == "/" || expandedPath.substr(0, 1) == "\\")
    {
      return expandedPath;
    }
    else
    {
      return defaultPath + expandedPath;
    }
  }

  void IncludeFile(const svtkStdString& includedFileName, const svtkStdString& defaultPath)
  {
    if (this->StackI >= SVTK_FOAMFILE_INCLUDE_STACK_SIZE)
    {
      throw this->StackString() << "Exceeded maximum #include recursions of "
                                << SVTK_FOAMFILE_INCLUDE_STACK_SIZE;
    }
    // use the default bitwise copy constructor
    this->Stack[this->StackI++] = new svtkFoamFileStack(*this);
    this->Superclass::Reset();

    this->Open(this->ExpandPath(includedFileName, defaultPath));
  }

  // the tokenizer
  // returns true if success, false if encountered EOF
  bool Read(svtkFoamToken& token)
  {
    token.SetLabelType(
      this->Reader->GetUse64BitLabels() ? svtkFoamToken::INT64 : svtkFoamToken::INT32);
    // expanded the outermost loop in nextTokenHead() for performance
    int c;
    while (isspace(c = this->Getc())) // isspace() accepts -1 as EOF
    {
      if (c == '\n')
      {
        ++this->Superclass::LineNumber;
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
        this->Superclass::WasNewline = true;
#endif
      }
    }
    if (c == '/')
    {
      this->PutBack(c);
      c = this->NextTokenHead();
    }
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
    if (c != '#')
    {
      this->Superclass::WasNewline = false;
    }
#endif

    const int MAXLEN = 1024;
    char buf[MAXLEN + 1];
    int charI = 0;
    switch (c)
    {
      case '(':
      case ')':
        // high-priority punctuation token
        token = static_cast<char>(c);
        return true;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '0':
      case '-':
        // undetermined number token
        do
        {
          buf[charI++] = static_cast<unsigned char>(c);
        } while (isdigit(c = this->Getc()) && charI < MAXLEN);
        if (c != '.' && c != 'e' && c != 'E' && charI < MAXLEN && c != EOF)
        {
          // label token
          buf[charI] = '\0';
          if (this->Reader->GetUse64BitLabels())
          {
            token = static_cast<svtkTypeInt64>(strtoll(buf, nullptr, 10));
          }
          else
          {
            token = static_cast<svtkTypeInt32>(strtol(buf, nullptr, 10));
          }
          this->PutBack(c);
          return true;
        }
        SVTK_FALLTHROUGH;
      case '.':
        // scalar token
        if (c == '.' && charI < MAXLEN)
        {
          // read decimal fraction part
          buf[charI++] = static_cast<unsigned char>(c);
          while (isdigit(c = this->Getc()) && charI < MAXLEN)
          {
            buf[charI++] = static_cast<unsigned char>(c);
          }
        }
        if ((c == 'e' || c == 'E') && charI < MAXLEN)
        {
          // read exponent part
          buf[charI++] = static_cast<unsigned char>(c);
          if (((c = this->Getc()) == '+' || c == '-') && charI < MAXLEN)
          {
            buf[charI++] = static_cast<unsigned char>(c);
            c = this->Getc();
          }
          while (isdigit(c) && charI < MAXLEN)
          {
            buf[charI++] = static_cast<unsigned char>(c);
            c = this->Getc();
          }
        }
        if (charI == 1 && buf[0] == '-')
        {
          token = '-';
          this->PutBack(c);
          return true;
        }
        buf[charI] = '\0';
        token = strtod(buf, nullptr);
        this->PutBack(c);
        break;
      case ';':
      case '{':
      case '}':
      case '[':
      case ']':
      case ':':
      case ',':
      case '=':
      case '+':
      case '*':
      case '/':
        // low-priority punctuation token
        token = static_cast<char>(c);
        return true;
      case '"':
      {
        // string token
        bool wasEscape = false;
        while ((c = this->Getc()) != EOF && charI < MAXLEN)
        {
          if (c == '\\' && !wasEscape)
          {
            wasEscape = true;
            continue;
          }
          else if (c == '"' && !wasEscape)
          {
            break;
          }
          else if (c == '\n')
          {
            ++this->Superclass::LineNumber;
            if (!wasEscape)
            {
              throw this->StackString() << "Unescaped newline in string constant";
            }
          }
          buf[charI++] = static_cast<unsigned char>(c);
          wasEscape = false;
        }
        buf[charI] = '\0';
        token = buf;
      }
      break;
      case EOF:
        // end of file
        token.SetBad();
        return false;
      case '$':
      {
        svtkFoamToken identifierToken;
        if (!this->Read(identifierToken))
        {
          throw this->StackString() << "Unexpected EOF reading identifier";
        }
        if (identifierToken.GetType() != svtkFoamToken::STRING)
        {
          throw this->StackString() << "Expected a word, found " << identifierToken;
        }
        token.SetIdentifier(identifierToken.ToString());
        return true;
      }
      case '#':
      {
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
        // the OpenFOAM #-directives can indeed be placed in the
        // middle of a line
        if (!this->Superclass::WasNewline)
        {
          throw this->StackString() << "Encountered #-directive in the middle of a line";
        }
        this->Superclass::WasNewline = false;
#endif
        // read directive
        svtkFoamToken directiveToken;
        if (!this->Read(directiveToken))
        {
          throw this->StackString() << "Unexpected EOF reading directive";
        }
        if (directiveToken == "include")
        {
          svtkFoamToken fileNameToken;
          if (!this->Read(fileNameToken))
          {
            throw this->StackString() << "Unexpected EOF reading filename";
          }
          this->IncludeFile(fileNameToken.ToString(), this->ExtractPath(this->FileName));
        }
        else if (directiveToken == "includeIfPresent")
        {
          svtkFoamToken fileNameToken;
          if (!this->Read(fileNameToken))
          {
            throw this->StackString() << "Unexpected EOF reading filename";
          }

          // special treatment since the file is allowed to be missing
          const svtkStdString fullName =
            this->ExpandPath(fileNameToken.ToString(), this->ExtractPath(this->FileName));

          FILE* fh = svtksys::SystemTools::Fopen(fullName, "rb");
          if (fh)
          {
            fclose(fh);

            this->IncludeFile(fileNameToken.ToString(), this->ExtractPath(this->FileName));
          }
        }
        else if (directiveToken == "inputMode")
        {
          svtkFoamToken modeToken;
          if (!this->Read(modeToken))
          {
            throw this->StackString() << "Unexpected EOF reading inputMode specifier";
          }
          if (modeToken == "merge" || modeToken == "default")
          {
            this->InputMode = INPUT_MODE_MERGE;
          }
          else if (modeToken == "overwrite")
          {
            this->InputMode = INPUT_MODE_OVERWRITE;
          }
          else if (modeToken == "protect")
          {
            // not properly supported - treat like "merge" for now
            // this->InputMode = INPUT_MODE_PROTECT;
            this->InputMode = INPUT_MODE_MERGE;
          }
          else if (modeToken == "warn")
          {
            // not properly supported - treat like "error" for now
            // this->InputMode = INPUT_MODE_WARN;
            this->InputMode = INPUT_MODE_ERROR;
          }
          else if (modeToken == "error")
          {
            this->InputMode = INPUT_MODE_ERROR;
          }
          else
          {
            throw this->StackString() << "Expected one of inputMode specifiers "
                                         "(merge, overwrite, protect, warn, error, default), found "
                                      << modeToken;
          }
        }
        else if (directiveToken == '{')
        {
          // '#{' verbatim/code block. swallow everything until a closing '#}'
          // This hopefully matches the first one...
          while (true)
          {
            c = this->NextTokenHead();
            if (c == EOF)
            {
              throw this->StackString() << "Unexpected EOF while skipping over #{ directive";
            }
            else if (c == '#')
            {
              c = this->Getc();
              if (c == '/')
              {
                this->PutBack(c);
              }
              else if (c == '}')
              {
                break;
              }
            }
          }
        }
        else
        {
          throw this->StackString() << "Unsupported directive " << directiveToken;
        }
        return this->Read(token);
      }
      default:
        // parses as a word token, but gives the STRING type for simplicity
        int inBrace = 0;
        do
        {
          if (c == '(')
          {
            inBrace++;
          }
          else if (c == ')' && --inBrace == -1)
          {
            break;
          }
          buf[charI++] = static_cast<unsigned char>(c);
          // valid characters that constitutes a word
          // cf. src/OpenFOAM/primitives/strings/word/wordI.H
        } while ((c = this->Getc()) != EOF && !isspace(c) && c != '"' && c != '/' && c != ';' &&
          c != '{' && c != '}' && charI < MAXLEN);
        buf[charI] = '\0';
        token = buf;
        this->PutBack(c);
    }

    if (c == EOF)
    {
      this->ThrowUnexpectedEOFException();
    }
    if (charI == MAXLEN)
    {
      throw this->StackString() << "Exceeded maximum allowed length of " << MAXLEN << " chars";
    }
    return true;
  }

  void Open(const svtkStdString& fileName)
  {
    // reset line number to indicate the beginning of the file when an
    // exception is thrown
    this->Superclass::LineNumber = 0;
    this->Superclass::FileName = fileName;

    if (this->Superclass::File)
    {
      throw this->StackString() << "File already opened within this object";
    }

    if ((this->Superclass::File = svtksys::SystemTools::Fopen(this->Superclass::FileName, "rb")) ==
      nullptr)
    {
      throw this->StackString() << "Can't open";
    }

    unsigned char zMagic[2];
    if (fread(zMagic, 1, 2, this->Superclass::File) == 2 && zMagic[0] == 0x1f && zMagic[1] == 0x8b)
    {
      // gzip-compressed format
      this->Superclass::Z.avail_in = 0;
      this->Superclass::Z.next_in = Z_NULL;
      // + 32 to automatically recognize gzip format
      if (inflateInit2(&this->Superclass::Z, 15 + 32) == Z_OK)
      {
        this->Superclass::IsCompressed = true;
        this->Superclass::Inbuf = new unsigned char[SVTK_FOAMFILE_INBUFSIZE];
      }
      else
      {
        fclose(this->Superclass::File);
        this->Superclass::File = nullptr;
        throw this->StackString() << "Can't init zstream "
                                  << (this->Superclass::Z.msg ? this->Superclass::Z.msg : "");
      }
    }
    else
    {
      // uncompressed format
      this->Superclass::IsCompressed = false;
    }
    rewind(this->Superclass::File);

    this->Superclass::ZStatus = Z_OK;
    this->Superclass::Outbuf = new unsigned char[SVTK_FOAMFILE_OUTBUFSIZE + 1];
    this->Superclass::BufPtr = this->Superclass::Outbuf + 1;
    this->Superclass::BufEndPtr = this->Superclass::BufPtr;
    this->Superclass::LineNumber = 1;
  }

  void Close()
  {
    while (this->CloseIncludedFile())
      ;
    this->Clear();
  }

  // gzread with buffering handling
  int Read(unsigned char* buf, const int len)
  {
    int readlen;
    const int buflen = static_cast<int>(this->Superclass::BufEndPtr - this->Superclass::BufPtr);
    if (len > buflen)
    {
      memcpy(buf, this->Superclass::BufPtr, buflen);
      this->InflateNext(buf + buflen, len - buflen, &readlen);
      if (readlen >= 0)
      {
        readlen += buflen;
      }
      else
      {
        if (buflen == 0) // return EOF
        {
          readlen = -1;
        }
        else
        {
          readlen = buflen;
        }
      }
      this->Superclass::BufPtr = this->Superclass::BufEndPtr;
    }
    else
    {
      memcpy(buf, this->Superclass::BufPtr, len);
      this->Superclass::BufPtr += len;
      readlen = len;
    }
    for (int i = 0; i < readlen; i++)
    {
      if (buf[i] == '\n')
      {
        this->Superclass::LineNumber++;
      }
    }
    return readlen;
  }

  void ReadExpecting(const char expected)
  {
    // skip prepending invalid chars
    // expanded the outermost loop in nextTokenHead() for performance
    int c;
    while (isspace(c = this->Getc())) // isspace() accepts -1 as EOF
    {
      if (c == '\n')
      {
        ++this->Superclass::LineNumber;
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
        this->Superclass::WasNewline = true;
#endif
      }
    }
    if (c == '/')
    {
      this->PutBack(c);
      c = this->NextTokenHead();
    }
    if (c != expected)
    {
      this->ThrowUnexpectedTokenException(expected, c);
    }
  }

  void ReadExpecting(const char* str)
  {
    svtkFoamToken t;
    if (!this->Read(t) || t != str)
    {
      throw this->StackString() << "Expected string \"" << str << "\", found " << t;
    }
  }

  svtkTypeInt64 ReadIntValue();
  template <typename FloatType>
  FloatType ReadFloatValue();
};

int svtkFoamFile::ReadNext()
{
  if (!this->InflateNext(this->Superclass::Outbuf + 1, SVTK_FOAMFILE_OUTBUFSIZE))
  {
    return this->CloseIncludedFile() ? this->Getc() : EOF;
  }
  return *this->Superclass::BufPtr++;
}

// specialized for reading an integer value.
// not using the standard strtol() for speed reason.
svtkTypeInt64 svtkFoamFile::ReadIntValue()
{
  // skip prepending invalid chars
  // expanded the outermost loop in nextTokenHead() for performance
  int c;
  while (isspace(c = this->Getc())) // isspace() accepts -1 as EOF
  {
    if (c == '\n')
    {
      ++this->Superclass::LineNumber;
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
      this->Superclass::WasNewline = true;
#endif
    }
  }
  if (c == '/')
  {
    this->PutBack(c);
    c = this->NextTokenHead();
  }

  // leading sign?
  const bool negNum = (c == '-');
  if (negNum || c == '+')
  {
    c = this->Getc();
    if (c == '\n')
    {
      ++this->Superclass::LineNumber;
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
      this->Superclass::WasNewline = true;
#endif
    }
  }

  if (!isdigit(c)) // isdigit() accepts -1 as EOF
  {
    if (c == EOF)
    {
      this->ThrowUnexpectedEOFException();
    }
    else
    {
      this->ThrowUnexpectedNondigitCharExecption(c);
    }
  }

  svtkTypeInt64 num = c - '0';
  while (isdigit(c = this->Getc()))
  {
    num = 10 * num + c - '0';
  }

  if (c == EOF)
  {
    this->ThrowUnexpectedEOFException();
  }
  this->PutBack(c);

  return negNum ? -num : num;
}

// extremely simplified high-performing string to floating point
// conversion code based on
// ParaView3/SVTK/Utilities/svtksqlite/svtk_sqlite3.c
template <typename FloatType>
FloatType svtkFoamFile::ReadFloatValue()
{
  // skip prepending invalid chars
  // expanded the outermost loop in nextTokenHead() for performance
  int c;
  while (isspace(c = this->Getc())) // isspace() accepts -1 as EOF
  {
    if (c == '\n')
    {
      ++this->Superclass::LineNumber;
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
      this->Superclass::WasNewline = true;
#endif
    }
  }
  if (c == '/')
  {
    this->PutBack(c);
    c = this->NextTokenHead();
  }

  // leading sign?
  const bool negNum = (c == '-');
  if (negNum || c == '+')
  {
    c = this->Getc();
    if (c == '\n')
    {
      ++this->Superclass::LineNumber;
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
      this->Superclass::WasNewline = true;
#endif
    }
  }

  if (!isdigit(c) && c != '.') // Attention: isdigit() accepts EOF
  {
    this->ThrowUnexpectedNondigitCharExecption(c);
  }

  double num = 0;

  // read integer part (before '.')
  if (c != '.')
  {
    num = c - '0';
    while (isdigit(c = this->Getc()))
    {
      num = num * 10.0 + (c - '0');
    }
  }

  // read decimal part (after '.')
  if (c == '.')
  {
    double divisor = 1.0;

    while (isdigit(c = this->Getc()))
    {
      num = num * 10.0 + (c - '0');
      divisor *= 10.0;
    }
    num /= divisor;
  }

  // read exponent part
  if (c == 'E' || c == 'e')
  {
    int esign = 1;
    int eval = 0;
    double scale = 1.0;

    c = this->Getc();
    if (c == '-')
    {
      esign = -1;
      c = this->Getc();
    }
    else if (c == '+')
    {
      c = this->Getc();
    }

    while (isdigit(c))
    {
      eval = eval * 10 + (c - '0');
      c = this->Getc();
    }

    // fast exponent multiplication!
    while (eval >= 64)
    {
      scale *= 1.0e+64;
      eval -= 64;
    }
    while (eval >= 16)
    {
      scale *= 1.0e+16;
      eval -= 16;
    }
    while (eval >= 4)
    {
      scale *= 1.0e+4;
      eval -= 4;
    }
    while (eval >= 1)
    {
      scale *= 1.0e+1;
      eval -= 1;
    }

    if (esign < 0)
    {
      num /= scale;
    }
    else
    {
      num *= scale;
    }
  }

  if (c == EOF)
  {
    this->ThrowUnexpectedEOFException();
  }
  this->PutBack(c);

  return static_cast<FloatType>(negNum ? -num : num);
}

// hacks to keep exception throwing code out-of-line to make
// putBack() and readExpecting() inline expandable
void svtkFoamFile::ThrowUnexpectedEOFException()
{
  throw this->StackString() << "Unexpected EOF";
}

void svtkFoamFile::ThrowUnexpectedNondigitCharExecption(const int c)
{
  throw this->StackString() << "Expected a number, found a non-digit character "
                            << static_cast<char>(c);
}

void svtkFoamFile::ThrowUnexpectedTokenException(const char expected, const int c)
{
  svtkFoamError sstr;
  sstr << this->StackString() << "Expected punctuation token '" << expected << "', found ";
  if (c == EOF)
  {
    sstr << "EOF";
  }
  else
  {
    sstr << static_cast<char>(c);
  }
  throw sstr;
}

void svtkFoamFile::ThrowDuplicatedPutBackException()
{
  throw this->StackString() << "Attempted duplicated putBack()";
}

bool svtkFoamFile::InflateNext(unsigned char* buf, int requestSize, int* readSize)
{
  if (readSize)
  {
    *readSize = -1; // Set to an error state for early returns
  }
  size_t size;
  if (this->Superclass::IsCompressed)
  {
    if (this->Superclass::ZStatus != Z_OK)
    {
      return false;
    }
    this->Superclass::Z.next_out = buf;
    this->Superclass::Z.avail_out = requestSize;

    do
    {
      if (this->Superclass::Z.avail_in == 0)
      {
        this->Superclass::Z.next_in = this->Superclass::Inbuf;
        this->Superclass::Z.avail_in = static_cast<uInt>(
          fread(this->Superclass::Inbuf, 1, SVTK_FOAMFILE_INBUFSIZE, this->Superclass::File));
        if (ferror(this->Superclass::File))
        {
          throw this->StackString() << "Fread failed";
        }
      }
      this->Superclass::ZStatus = inflate(&this->Superclass::Z, Z_NO_FLUSH);
      if (this->Superclass::ZStatus == Z_STREAM_END
#if SVTK_FOAMFILE_OMIT_CRCCHECK
        // the dummy CRC function causes data error when finalizing
        // so we have to proceed even when a data error is detected
        || this->Superclass::ZStatus == Z_DATA_ERROR
#endif
      )
      {
        break;
      }
      if (this->Superclass::ZStatus != Z_OK)
      {
        throw this->StackString() << "Inflation failed: "
                                  << (this->Superclass::Z.msg ? this->Superclass::Z.msg : "");
      }
    } while (this->Superclass::Z.avail_out > 0);

    size = requestSize - this->Superclass::Z.avail_out;
  }
  else
  {
    // not compressed
    size = fread(buf, 1, requestSize, this->Superclass::File);
  }

  if (size <= 0)
  {
    // retain the current location bufPtr_ to the end of the buffer so that
    // getc() returns EOF again when called next time
    return false;
  }
  // size > 0
  // reserve the first byte for getback char
  this->Superclass::BufPtr = this->Superclass::Outbuf + 1;
  this->Superclass::BufEndPtr = this->Superclass::BufPtr + size;
  if (readSize)
  { // Cast size_t to int -- since requestSize is int, this should be ok.
    *readSize = static_cast<int>(size);
  }
  return true;
}

// get next semantically valid character
int svtkFoamFile::NextTokenHead()
{
  for (;;)
  {
    int c;
    while (isspace(c = this->Getc())) // isspace() accepts -1 as EOF
    {
      if (c == '\n')
      {
        ++this->Superclass::LineNumber;
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
        this->Superclass::WasNewline = true;
#endif
      }
    }
    if (c == '/')
    {
      if ((c = this->Getc()) == '/')
      {
        while ((c = this->Getc()) != EOF && c != '\n')
          ;
        if (c == EOF)
        {
          return c;
        }
        ++this->Superclass::LineNumber;
#if SVTK_FOAMFILE_RECOGNIZE_LINEHEAD
        this->Superclass::WasNewline = true;
#endif
      }
      else if (c == '*')
      {
        for (;;)
        {
          while ((c = this->Getc()) != EOF && c != '*')
          {
            if (c == '\n')
            {
              ++this->Superclass::LineNumber;
            }
          }
          if (c == EOF)
          {
            return c;
          }
          else if ((c = this->Getc()) == '/')
          {
            break;
          }
          this->PutBack(c);
        }
      }
      else
      {
        this->PutBack(c); // may be an EOF
        return '/';
      }
    }
    else // may be an EOF
    {
      return c;
    }
  }
#if defined(__hpux)
  return EOF; // this line should not be executed; workaround for HP-UXia64-aCC
#endif
}

//-----------------------------------------------------------------------------
// class svtkFoamIOobject
// holds file handle, file format, name of the object the file holds and
// type of the object.
struct svtkFoamIOobject : public svtkFoamFile
{
private:
  typedef svtkFoamFile Superclass;

public:
  enum fileFormat
  {
    UNDEFINED,
    ASCII,
    BINARY
  };

private:
  fileFormat Format;
  svtkStdString ObjectName;
  svtkStdString HeaderClassName;
  svtkFoamError E;

  bool Use64BitLabels;
  bool Use64BitFloats;

  // inform IO object if lagrangian/positions has extra data (OF 1.4 - 2.4)
  const bool LagrangianPositionsExtraData;

  void ReadHeader(); // defined later

  // Disallow default bitwise copy/assignment constructor
  svtkFoamIOobject(const svtkFoamIOobject&) = delete;
  void operator=(const svtkFoamIOobject&) = delete;
  svtkFoamIOobject() = delete;

public:
  svtkFoamIOobject(const svtkStdString& casePath, svtkOpenFOAMReader* reader)
    : svtkFoamFile(casePath, reader)
    , Format(UNDEFINED)
    , E()
    , Use64BitLabels(reader->GetUse64BitLabels())
    , Use64BitFloats(reader->GetUse64BitFloats())
    , LagrangianPositionsExtraData(static_cast<bool>(!reader->GetPositionsIsIn13Format()))
  {
  }
  ~svtkFoamIOobject() { this->Close(); }

  bool Open(const svtkStdString& file)
  {
    try
    {
      this->Superclass::Open(file);
    }
    catch (svtkFoamError& e)
    {
      this->E = e;
      return false;
    }

    try
    {
      this->ReadHeader();
    }
    catch (svtkFoamError& e)
    {
      this->Superclass::Close();
      this->E = e;
      return false;
    }
    return true;
  }

  void Close()
  {
    this->Superclass::Close();
    this->Format = UNDEFINED;
    this->ObjectName.erase();
    this->HeaderClassName.erase();
    this->E.erase();
    this->Use64BitLabels = this->Reader->GetUse64BitLabels();
    this->Use64BitFloats = this->Reader->GetUse64BitFloats();
  }
  fileFormat GetFormat() const { return this->Format; }
  const svtkStdString& GetClassName() const { return this->HeaderClassName; }
  const svtkStdString& GetObjectName() const { return this->ObjectName; }
  const svtkFoamError& GetError() const { return this->E; }
  void SetError(const svtkFoamError& e) { this->E = e; }
  bool GetUse64BitLabels() const { return this->Use64BitLabels; }
  bool GetUse64BitFloats() const { return this->Use64BitFloats; }
  bool GetLagrangianPositionsExtraData() const { return this->LagrangianPositionsExtraData; }
};

//-----------------------------------------------------------------------------
// workarounding class for older compilers (gcc-3.3.x and possibly older)
template <typename T>
struct svtkFoamReadValue
{
public:
  static T ReadValue(svtkFoamIOobject& io);
};

template <>
inline char svtkFoamReadValue<char>::ReadValue(svtkFoamIOobject& io)
{
  return static_cast<char>(io.ReadIntValue());
}

template <>
inline svtkTypeInt32 svtkFoamReadValue<svtkTypeInt32>::ReadValue(svtkFoamIOobject& io)
{
  return static_cast<svtkTypeInt32>(io.ReadIntValue());
}

template <>
inline svtkTypeInt64 svtkFoamReadValue<svtkTypeInt64>::ReadValue(svtkFoamIOobject& io)
{
  return io.ReadIntValue();
}

template <>
inline float svtkFoamReadValue<float>::ReadValue(svtkFoamIOobject& io)
{
  return io.ReadFloatValue<float>();
}

template <>
inline double svtkFoamReadValue<double>::ReadValue(svtkFoamIOobject& io)
{
  return io.ReadFloatValue<double>();
}

//-----------------------------------------------------------------------------
// class svtkFoamEntryValue
// a class that represents a value of a dictionary entry that corresponds to
// its keyword. note that an entry can have more than one value.
struct svtkFoamEntryValue : public svtkFoamToken
{
private:
  typedef svtkFoamToken Superclass;

  bool IsUniform;
  bool Managed;
  const svtkFoamEntry* UpperEntryPtr;

  svtkFoamEntryValue() = delete;
  svtkObjectBase* ToSVTKObject() { return this->Superclass::VtkObjectPtr; }
  void Clear();
  void ReadList(svtkFoamIOobject& io);

public:
  // reads primitive int/float lists
  template <typename listT, typename primitiveT>
  class listTraits
  {
    listT* Ptr;

  public:
    listTraits()
      : Ptr(listT::New())
    {
    }
    listT* GetPtr() { return this->Ptr; }
    void ReadUniformValues(svtkFoamIOobject& io, const svtkIdType size)
    {
      primitiveT value = svtkFoamReadValue<primitiveT>::ReadValue(io);
      for (svtkIdType i = 0; i < size; i++)
      {
        this->Ptr->SetValue(i, value);
      }
    }
    void ReadAsciiList(svtkFoamIOobject& io, const svtkIdType size)
    {
      for (svtkIdType i = 0; i < size; i++)
      {
        this->Ptr->SetValue(i, svtkFoamReadValue<primitiveT>::ReadValue(io));
      }
    }
    void ReadBinaryList(svtkFoamIOobject& io, const int size)
    {
      typedef typename listT::ValueType ListValueType;
      if (typeid(ListValueType) == typeid(primitiveT))
      {
        io.Read(reinterpret_cast<unsigned char*>(this->Ptr->GetPointer(0)),
          static_cast<int>(size * sizeof(primitiveT)));
      }
      else
      {
        svtkDataArray* fileData =
          svtkDataArray::CreateDataArray(svtkTypeTraits<primitiveT>::SVTKTypeID());
        fileData->SetNumberOfComponents(this->Ptr->GetNumberOfComponents());
        fileData->SetNumberOfTuples(this->Ptr->GetNumberOfTuples());
        io.Read(reinterpret_cast<unsigned char*>(fileData->GetVoidPointer(0)),
          static_cast<int>(size * sizeof(primitiveT)));
        this->Ptr->DeepCopy(fileData);
        fileData->Delete();
      }
    }
    void ReadValue(svtkFoamIOobject&, svtkFoamToken& currToken)
    {
      if (!currToken.Is<primitiveT>())
      {
        throw svtkFoamError() << "Expected an integer or a (, found " << currToken;
      }
      this->Ptr->InsertNextValue(currToken.To<primitiveT>());
    }
  };

  // reads rank 1 lists of types vector, sphericalTensor, symmTensor
  // and tensor. if isPositions is true it reads Cloud type of data as
  // particle positions. cf. (the positions format)
  // src/lagrangian/basic/particle/particleIO.C - writePosition()
  template <typename listT, typename primitiveT, int nComponents, bool isPositions = false>
  class vectorListTraits
  {
    listT* Ptr;

  public:
    vectorListTraits()
      : Ptr(listT::New())
    {
      this->Ptr->SetNumberOfComponents(nComponents);
    }
    listT* GetPtr() { return this->Ptr; }
    void ReadUniformValues(svtkFoamIOobject& io, const svtkIdType size)
    {
      io.ReadExpecting('(');
      primitiveT vectorValue[nComponents];
      for (int j = 0; j < nComponents; j++)
      {
        vectorValue[j] = svtkFoamReadValue<primitiveT>::ReadValue(io);
      }
      for (svtkIdType i = 0; i < size; i++)
      {
        this->Ptr->SetTuple(i, vectorValue);
      }
      io.ReadExpecting(')');
      if (isPositions)
      {
        // skip label celli
        svtkFoamReadValue<int>::ReadValue(io);
      }
    }
    void ReadAsciiList(svtkFoamIOobject& io, const svtkIdType size)
    {
      typedef typename listT::ValueType ListValueType;
      for (svtkIdType i = 0; i < size; i++)
      {
        io.ReadExpecting('(');
        ListValueType* vectorTupleI = this->Ptr->GetPointer(nComponents * i);
        for (int j = 0; j < nComponents; j++)
        {
          vectorTupleI[j] = static_cast<ListValueType>(svtkFoamReadValue<primitiveT>::ReadValue(io));
        }
        io.ReadExpecting(')');
        if (isPositions)
        {
          // skip label celli
          svtkFoamReadValue<svtkTypeInt64>::ReadValue(io);
        }
      }
    }
    void ReadBinaryList(svtkFoamIOobject& io, const int size)
    {
      if (isPositions) // lagrangian/positions (class Cloud)
      {
        // xyz (3*scalar) + celli (label)
        // in OpenFOAM 1.4 -> 2.4 also had facei (label) and stepFraction (scalar)

        const unsigned labelSize = (io.GetUse64BitLabels() ? 8 : 4);
        const unsigned tupleLength = (sizeof(primitiveT) * nComponents + labelSize +
          (io.GetLagrangianPositionsExtraData() ? (labelSize + sizeof(primitiveT)) : 0));

        // MSVC doesn't support variable-sized stack arrays (JAN-2017)
        // memory management via std::vector
        std::vector<unsigned char> bufferContainer;
        bufferContainer.resize(tupleLength);
        primitiveT* buffer = reinterpret_cast<primitiveT*>(&bufferContainer[0]);

        for (int i = 0; i < size; i++)
        {
          io.ReadExpecting('(');
          io.Read(reinterpret_cast<unsigned char*>(buffer), tupleLength);
          io.ReadExpecting(')');
          this->Ptr->SetTuple(i, buffer);
        }
      }
      else
      {
        typedef typename listT::ValueType ListValueType;

        // Compiler hint for better unrolling:
        SVTK_ASSUME(this->Ptr->GetNumberOfComponents() == nComponents);

        const int tupleLength = sizeof(primitiveT) * nComponents;
        primitiveT buffer[nComponents];
        for (int i = 0; i < size; i++)
        {
          const int readLength = io.Read(reinterpret_cast<unsigned char*>(buffer), tupleLength);
          if (readLength != tupleLength)
          {
            throw svtkFoamError() << "Failed to read tuple " << i << " of " << size << ": Expected "
                                 << tupleLength << " bytes, got " << readLength << " bytes.";
          }
          for (int c = 0; c < nComponents; ++c)
          {
            this->Ptr->SetTypedComponent(i, c, static_cast<ListValueType>(buffer[c]));
          }
        }
      }
    }
    void ReadValue(svtkFoamIOobject& io, svtkFoamToken& currToken)
    {
      if (currToken != '(')
      {
        throw svtkFoamError() << "Expected '(', found " << currToken;
      }
      primitiveT v[nComponents];
      for (int j = 0; j < nComponents; j++)
      {
        v[j] = svtkFoamReadValue<primitiveT>::ReadValue(io);
      }
      this->Ptr->InsertNextTuple(v);
      io.ReadExpecting(')');
    }
  };

  svtkFoamEntryValue(const svtkFoamEntry* upperEntryPtr)
    : svtkFoamToken()
    , IsUniform(false)
    , Managed(true)
    , UpperEntryPtr(upperEntryPtr)
  {
  }
  svtkFoamEntryValue(svtkFoamEntryValue&, const svtkFoamEntry*);
  ~svtkFoamEntryValue() { this->Clear(); }

  void SetEmptyList()
  {
    this->Clear();
    this->IsUniform = false;
    this->Superclass::Type = EMPTYLIST;
  }
  bool GetIsUniform() const { return this->IsUniform; }
  int Read(svtkFoamIOobject& io);
  void ReadDictionary(svtkFoamIOobject& io, const svtkFoamToken& firstKeyword);
  const svtkDataArray& LabelList() const { return *this->Superclass::LabelListPtr; }
  svtkDataArray& LabelList() { return *this->Superclass::LabelListPtr; }
  const svtkFoamLabelVectorVector& LabelListList() const
  {
    return *this->Superclass::LabelListListPtr;
  }
  const svtkFloatArray& ScalarList() const { return *this->Superclass::ScalarListPtr; }
  svtkFloatArray& ScalarList() { return *this->Superclass::ScalarListPtr; }
  const svtkFloatArray& VectorList() const { return *this->Superclass::VectorListPtr; }
  const svtkFoamDict& Dictionary() const { return *this->Superclass::DictPtr; }
  svtkFoamDict& Dictionary() { return *this->Superclass::DictPtr; }

  void* Ptr()
  {
    this->Managed = false; // returned pointer will not be deleted by the d'tor
    // all list pointers are in a single union
    return (void*)this->Superclass::LabelListPtr;
  }

  svtkStdString ToString() const
  {
    return this->Superclass::Type == STRING ? this->Superclass::ToString() : svtkStdString();
  }
  float ToFloat() const
  {
    return this->Superclass::Type == SCALAR || this->Superclass::Type == LABEL
      ? this->Superclass::To<float>()
      : 0.0F;
  }
  double ToDouble() const
  {
    return this->Superclass::Type == SCALAR || this->Superclass::Type == LABEL
      ? this->Superclass::To<double>()
      : 0.0;
  }
  // TODO is it ok to always use a 64bit int here?
  svtkTypeInt64 ToInt() const
  {
    return this->Superclass::Type == LABEL ? this->Superclass::To<svtkTypeInt64>() : 0;
  }

  // the following two are for an exceptional expression of
  // `LABEL{LABELorSCALAR}' without type prefix (e. g. `2{-0}' in
  // mixedRhoE B.C. in rhopSonicFoam/shockTube)
  void MakeLabelList(const svtkTypeInt64 labelValue, const svtkIdType size)
  {
    assert("Label type not set!" && this->GetLabelType() != NO_LABEL_TYPE);
    this->Superclass::Type = LABELLIST;
    if (this->LabelType == INT32)
    {
      svtkTypeInt32Array* array = svtkTypeInt32Array::New();
      array->SetNumberOfValues(size);
      for (svtkIdType i = 0; i < size; ++i)
      {
        array->SetValue(i, static_cast<svtkTypeInt32>(labelValue));
      }
      this->Superclass::LabelListPtr = array;
    }
    else
    {
      svtkTypeInt64Array* array = svtkTypeInt64Array::New();
      array->SetNumberOfValues(size);
      for (svtkIdType i = 0; i < size; ++i)
      {
        array->SetValue(i, labelValue);
      }
      this->Superclass::LabelListPtr = array;
    }
  }

  void MakeScalarList(const float scalarValue, const svtkIdType size)
  {
    this->Superclass::ScalarListPtr = svtkFloatArray::New();
    this->Superclass::Type = SCALARLIST;
    this->Superclass::ScalarListPtr->SetNumberOfValues(size);
    for (int i = 0; i < size; i++)
    {
      this->Superclass::ScalarListPtr->SetValue(i, scalarValue);
    }
  }

  // reads dimensionSet
  void ReadDimensionSet(svtkFoamIOobject& io)
  {
    assert("Label type not set!" && this->GetLabelType() != NO_LABEL_TYPE);
    const int nDims = 7;
    this->Superclass::Type = LABELLIST;
    if (this->LabelType == INT32)
    {
      svtkTypeInt32Array* array = svtkTypeInt32Array::New();
      array->SetNumberOfValues(nDims);
      for (svtkIdType i = 0; i < nDims; ++i)
      {
        array->SetValue(i, svtkFoamReadValue<svtkTypeInt32>::ReadValue(io));
      }
      this->Superclass::LabelListPtr = array;
    }
    else
    {
      svtkTypeInt64Array* array = svtkTypeInt64Array::New();
      array->SetNumberOfValues(nDims);
      for (svtkIdType i = 0; i < nDims; ++i)
      {
        array->SetValue(i, svtkFoamReadValue<svtkTypeInt64>::ReadValue(io));
      }
      this->Superclass::LabelListPtr = array;
    }
    io.ReadExpecting(']');
  }

  template <svtkFoamToken::tokenType listType, typename traitsT>
  void ReadNonuniformList(svtkFoamIOobject& io);

  // reads a list of labelLists. requires size prefix of the listList
  // to be present. size of each sublist must also be present in the
  // stream if the format is binary.
  void ReadLabelListList(svtkFoamIOobject& io)
  {
    assert("Label type not set!" && this->GetLabelType() != NO_LABEL_TYPE);
    bool use64BitLabels = this->LabelType == INT64;
    svtkFoamToken currToken;
    currToken.SetLabelType(this->LabelType);
    if (!io.Read(currToken))
    {
      throw svtkFoamError() << "Unexpected EOF";
    }
    if (currToken.GetType() == svtkFoamToken::LABEL)
    {
      const svtkTypeInt64 sizeI = currToken.To<svtkTypeInt64>();
      if (sizeI < 0)
      {
        throw svtkFoamError() << "List size must not be negative: size = " << sizeI;
      }

      // gives initial guess for list size
      if (use64BitLabels)
      {
        this->LabelListListPtr = new svtkFoamLabel64VectorVector(sizeI, 4 * sizeI);
      }
      else
      {
        this->LabelListListPtr = new svtkFoamLabel32VectorVector(sizeI, 4 * sizeI);
      }
      this->Superclass::Type = LABELLISTLIST;
      io.ReadExpecting('(');
      svtkIdType bodyI = 0;
      for (int i = 0; i < sizeI; i++)
      {
        if (!io.Read(currToken))
        {
          throw svtkFoamError() << "Unexpected EOF";
        }
        if (currToken.GetType() == svtkFoamToken::LABEL)
        {
          const svtkTypeInt64 sizeJ = currToken.To<svtkTypeInt64>();
          if (sizeJ < 0)
          {
            throw svtkFoamError() << "List size must not be negative: size = " << sizeJ;
          }

          void* listI = this->LabelListListPtr->WritePointer(i, bodyI, sizeJ);

          if (io.GetFormat() == svtkFoamIOobject::ASCII)
          {
            io.ReadExpecting('(');
            for (int j = 0; j < sizeJ; j++)
            {
              SetLabelValue(
                listI, j, svtkFoamReadValue<svtkTypeInt64>::ReadValue(io), use64BitLabels);
            }
            io.ReadExpecting(')');
          }
          else
          {
            if (sizeJ > 0) // avoid invalid reference to labelListI.at(0)
            {
              io.ReadExpecting('(');
              io.Read(reinterpret_cast<unsigned char*>(listI),
                static_cast<int>(sizeJ * this->LabelListListPtr->GetLabelSize()));
              io.ReadExpecting(')');
            }
          }
          bodyI += sizeJ;
        }
        else if (currToken == '(')
        {
          this->Superclass::LabelListListPtr->SetIndex(i, bodyI);
          while (io.Read(currToken) && currToken != ')')
          {
            if (currToken.GetType() != svtkFoamToken::LABEL)
            {
              throw svtkFoamError() << "Expected an integer, found " << currToken;
            }
            this->Superclass::LabelListListPtr->InsertValue(bodyI++, currToken.To<int>());
          }
        }
        else
        {
          throw svtkFoamError() << "Expected integer or '(', found " << currToken;
        }
      }
      // set the next index of the last element to calculate the last
      // subarray size
      this->Superclass::LabelListListPtr->SetIndex(sizeI, bodyI);
      // shrink to the actually used size
      this->Superclass::LabelListListPtr->ResizeBody(bodyI);
      io.ReadExpecting(')');
    }
    else
    {
      throw svtkFoamError() << "Expected integer, found " << currToken;
    }
  }

  // reads compact list of labels.
  void ReadCompactIOLabelList(svtkFoamIOobject& io)
  {
    if (io.GetFormat() != svtkFoamIOobject::BINARY)
    {
      this->ReadLabelListList(io);
      return;
    }

    assert("Label type not set!" && this->GetLabelType() != NO_LABEL_TYPE);
    bool use64BitLabels = this->LabelType == INT64;

    if (use64BitLabels)
    {
      this->LabelListListPtr = new svtkFoamLabel64VectorVector;
    }
    else
    {
      this->LabelListListPtr = new svtkFoamLabel32VectorVector;
    }
    this->Superclass::Type = LABELLISTLIST;
    for (int arrayI = 0; arrayI < 2; arrayI++)
    {
      svtkFoamToken currToken;
      if (!io.Read(currToken))
      {
        throw svtkFoamError() << "Unexpected EOF";
      }
      if (currToken.GetType() == svtkFoamToken::LABEL)
      {
        svtkTypeInt64 sizeI = currToken.To<svtkTypeInt64>();
        if (sizeI < 0)
        {
          throw svtkFoamError() << "List size must not be negative: size = " << sizeI;
        }
        if (sizeI > 0) // avoid invalid reference
        {
          svtkDataArray* array = (arrayI == 0 ? this->Superclass::LabelListListPtr->GetIndices()
                                             : this->Superclass::LabelListListPtr->GetBody());
          array->SetNumberOfValues(static_cast<svtkIdType>(sizeI));
          io.ReadExpecting('(');
          io.Read(reinterpret_cast<unsigned char*>(array->GetVoidPointer(0)),
            static_cast<int>(sizeI * array->GetDataTypeSize()));
          io.ReadExpecting(')');
        }
      }
      else
      {
        throw svtkFoamError() << "Expected integer, found " << currToken;
      }
    }
  }

  bool ReadField(svtkFoamIOobject& io)
  {
    try
    {
      // lagrangian labels (cf. gnemdFoam/nanoNozzle)
      if (io.GetClassName() == "labelField")
      {
        assert("Label type not set!" && this->GetLabelType() != NO_LABEL_TYPE);
        if (this->LabelType == INT64)
        {
          this->ReadNonuniformList<LABELLIST, listTraits<svtkTypeInt64Array, svtkTypeInt64> >(io);
        }
        else
        {
          this->ReadNonuniformList<LABELLIST, listTraits<svtkTypeInt32Array, svtkTypeInt32> >(io);
        }
      }
      // lagrangian scalars

      else if (io.GetClassName() == "scalarField")
      {
        if (io.GetUse64BitFloats())
        {
          this->ReadNonuniformList<SCALARLIST, listTraits<svtkFloatArray, double> >(io);
        }
        else
        {
          this->ReadNonuniformList<SCALARLIST, listTraits<svtkFloatArray, float> >(io);
        }
      }
      else if (io.GetClassName() == "sphericalTensorField")
      {
        if (io.GetUse64BitFloats())
        {
          this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, double, 1, false> >(
            io);
        }
        else
        {
          this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, float, 1, false> >(
            io);
        }
      }
      // polyMesh/points, lagrangian vectors

      else if (io.GetClassName() == "vectorField")
      {
        if (io.GetUse64BitFloats())
        {
          this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, double, 3, false> >(
            io);
        }
        else
        {
          this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, float, 3, false> >(
            io);
        }
      }
      else if (io.GetClassName() == "symmTensorField")
      {
        if (io.GetUse64BitFloats())
        {
          this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, double, 6, false> >(
            io);
        }
        else
        {
          this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, float, 6, false> >(
            io);
        }
      }
      else if (io.GetClassName() == "tensorField")
      {
        if (io.GetUse64BitFloats())
        {
          this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, double, 9, false> >(
            io);
        }
        else
        {
          this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, float, 9, false> >(
            io);
        }
      }
      else
      {
        throw svtkFoamError() << "Non-supported field type " << io.GetClassName();
      }
    }
    catch (svtkFoamError& e)
    {
      io.SetError(e);
      return false;
    }
    return true;
  }
};

// I'm removing this method because it looks like a hack and is preventing
// well-formed datasets from loading. This method overrides ReadBinaryList
// for float data by assuming that the data is really stored as doubles, and
// converting each value to float as it's read. Leaving this here in case
// someone knows why it exists...
//
// specialization for reading double precision binary into svtkFloatArray.
// Must precede ReadNonuniformList() below (HP-UXia64-aCC).
// template<>
// void svtkFoamEntryValue::listTraits<svtkFloatArray, float>::ReadBinaryList(
//    svtkFoamIOobject& io, const int size)
//{
//  for (int i = 0; i < size; i++)
//    {
//    double buffer;
//    io.Read(reinterpret_cast<unsigned char *>(&buffer), sizeof(double));
//    this->Ptr->SetValue(i, static_cast<float>(buffer));
//    }
//}

// generic reader for nonuniform lists. requires size prefix of the
// list to be present in the stream if the format is binary.
template <svtkFoamToken::tokenType listType, typename traitsT>
void svtkFoamEntryValue::ReadNonuniformList(svtkFoamIOobject& io)
{
  svtkFoamToken currToken;
  if (!io.Read(currToken))
  {
    throw svtkFoamError() << "Unexpected EOF";
  }
  traitsT list;
  this->Superclass::Type = listType;
  this->Superclass::VtkObjectPtr = list.GetPtr();
  if (currToken.Is<svtkTypeInt64>())
  {
    const svtkTypeInt64 size = currToken.To<svtkTypeInt64>();
    if (size < 0)
    {
      throw svtkFoamError() << "List size must not be negative: size = " << size;
    }
    list.GetPtr()->SetNumberOfTuples(size);
    if (io.GetFormat() == svtkFoamIOobject::ASCII)
    {
      if (!io.Read(currToken))
      {
        throw svtkFoamError() << "Unexpected EOF";
      }
      // some objects have lists with only one element enclosed by {}
      // e. g. simpleFoam/pitzDaily3Blocks/constant/polyMesh/faceZones
      if (currToken == '{')
      {
        list.ReadUniformValues(io, size);
        io.ReadExpecting('}');
        return;
      }
      else if (currToken != '(')
      {
        throw svtkFoamError() << "Expected '(', found " << currToken;
      }
      list.ReadAsciiList(io, size);
      io.ReadExpecting(')');
    }
    else
    {
      if (size > 0)
      {
        // read parentheses only when size > 0
        io.ReadExpecting('(');
        list.ReadBinaryList(io, static_cast<int>(size));
        io.ReadExpecting(')');
      }
    }
  }
  else if (currToken == '(')
  {
    while (io.Read(currToken) && currToken != ')')
    {
      list.ReadValue(io, currToken);
    }
    list.GetPtr()->Squeeze();
  }
  else
  {
    throw svtkFoamError() << "Expected integer or '(', found " << currToken;
  }
}

//-----------------------------------------------------------------------------
// class svtkFoamEntry
// a class that represents an entry of a dictionary. note that an
// entry can have more than one value.
struct svtkFoamEntry : public std::vector<svtkFoamEntryValue*>
{
private:
  typedef std::vector<svtkFoamEntryValue*> Superclass;
  svtkStdString Keyword;
  svtkFoamDict* UpperDictPtr;

  svtkFoamEntry() = delete;

public:
  svtkFoamEntry(svtkFoamDict* upperDictPtr)
    : UpperDictPtr(upperDictPtr)
  {
  }
  svtkFoamEntry(const svtkFoamEntry& entry, svtkFoamDict* upperDictPtr)
    : Superclass(entry.size())
    , Keyword(entry.GetKeyword())
    , UpperDictPtr(upperDictPtr)
  {
    for (size_t valueI = 0; valueI < entry.size(); valueI++)
    {
      this->Superclass::operator[](valueI) = new svtkFoamEntryValue(*entry[valueI], this);
    }
  }

  ~svtkFoamEntry() { this->Clear(); }

  void Clear()
  {
    for (size_t i = 0; i < this->Superclass::size(); i++)
    {
      delete this->Superclass::operator[](i);
    }
    this->Superclass::clear();
  }
  const svtkStdString& GetKeyword() const { return this->Keyword; }
  void SetKeyword(const svtkStdString& keyword) { this->Keyword = keyword; }
  const svtkFoamEntryValue& FirstValue() const { return *this->Superclass::operator[](0); }
  svtkFoamEntryValue& FirstValue() { return *this->Superclass::operator[](0); }
  const svtkDataArray& LabelList() const { return this->FirstValue().LabelList(); }
  svtkDataArray& LabelList() { return this->FirstValue().LabelList(); }
  const svtkFoamLabelVectorVector& LabelListList() const
  {
    return this->FirstValue().LabelListList();
  }
  const svtkFloatArray& ScalarList() const { return this->FirstValue().ScalarList(); }
  svtkFloatArray& ScalarList() { return this->FirstValue().ScalarList(); }
  const svtkFloatArray& VectorList() const { return this->FirstValue().VectorList(); }
  const svtkFoamDict& Dictionary() const { return this->FirstValue().Dictionary(); }
  svtkFoamDict& Dictionary() { return this->FirstValue().Dictionary(); }
  void* Ptr() { return this->FirstValue().Ptr(); }
  const svtkFoamDict* GetUpperDictPtr() const { return this->UpperDictPtr; }

  svtkStdString ToString() const
  {
    return !this->empty() ? this->FirstValue().ToString() : svtkStdString();
  }
  float ToFloat() const { return !this->empty() ? this->FirstValue().ToFloat() : 0.0F; }
  double ToDouble() const { return !this->empty() ? this->FirstValue().ToDouble() : 0.0; }
  svtkTypeInt64 ToInt() const { return !this->empty() ? this->FirstValue().ToInt() : 0; }

  void ReadDictionary(svtkFoamIOobject& io)
  {
    this->Superclass::push_back(new svtkFoamEntryValue(this));
    this->Superclass::back()->ReadDictionary(io, svtkFoamToken());
  }

  // read values of an entry
  void Read(svtkFoamIOobject& io);
};

//-----------------------------------------------------------------------------
// class svtkFoamDict
// a class that holds a FoamFile data structure
struct svtkFoamDict : public std::vector<svtkFoamEntry*>
{
private:
  typedef std::vector<svtkFoamEntry*> Superclass;

  svtkFoamToken Token;
  const svtkFoamDict* UpperDictPtr;

  svtkFoamDict(const svtkFoamDict&) = delete;

public:
  svtkFoamDict(const svtkFoamDict* upperDictPtr = nullptr)
    : Superclass()
    , Token()
    , UpperDictPtr(upperDictPtr)
  {
  }
  svtkFoamDict(const svtkFoamDict& dict, const svtkFoamDict* upperDictPtr)
    : Superclass(dict.size())
    , Token()
    , UpperDictPtr(upperDictPtr)
  {
    if (dict.GetType() == svtkFoamToken::DICTIONARY)
    {
      for (size_t entryI = 0; entryI < dict.size(); entryI++)
      {
        this->operator[](entryI) = new svtkFoamEntry(*dict[entryI], this);
      }
    }
  }

  ~svtkFoamDict()
  {
    if (this->Token.GetType() == svtkFoamToken::UNDEFINED)
    {
      for (size_t i = 0; i < this->Superclass::size(); i++)
      {
        delete this->operator[](i);
      }
    }
  }

  svtkFoamToken::labelType GetLabelType() const { return this->Token.GetLabelType(); }

  void SetLabelType(svtkFoamToken::labelType lt) { this->Token.SetLabelType(lt); }

  svtkFoamToken::tokenType GetType() const
  {
    return this->Token.GetType() == svtkFoamToken::UNDEFINED ? svtkFoamToken::DICTIONARY
                                                            : this->Token.GetType();
  }
  const svtkFoamToken& GetToken() const { return this->Token; }
  const svtkFoamDict* GetUpperDictPtr() const { return this->UpperDictPtr; }
  svtkFoamEntry* Lookup(const svtkStdString& keyword, bool regex = false) const
  {
    if (this->Token.GetType() == svtkFoamToken::UNDEFINED)
    {
      int lastMatch = -1;
      for (size_t i = 0; i < this->Superclass::size(); i++)
      {
        svtksys::RegularExpression rex;
        if (this->operator[](i)->GetKeyword() == keyword) // found
        {
          return this->operator[](i);
        }
        else if (regex && rex.compile(this->operator[](i)->GetKeyword()) && rex.find(keyword) &&
          rex.start(0) == 0 && rex.end(0) == keyword.size())
        {
          // regular expression matches full keyword
          lastMatch = static_cast<int>(i);
        }
      }
      if (lastMatch >= 0)
      {
        return this->operator[](lastMatch);
      }
    }

    // not found
    return nullptr;
  }

  // reads a FoamFile or a subdictionary. if the stream to be read is
  // a subdictionary the preceding '{' is assumed to have already been
  // thrown away.
  bool Read(svtkFoamIOobject& io, const bool isSubDictionary = false,
    const svtkFoamToken& firstToken = svtkFoamToken())
  {
    try
    {
      svtkFoamToken currToken;
      if (firstToken.GetType() == svtkFoamToken::UNDEFINED)
      {
        // read the first token
        if (!io.Read(currToken))
        {
          throw svtkFoamError() << "Unexpected EOF";
        }

        if (isSubDictionary)
        {
          // the following if clause is for an exceptional expression
          // of `LABEL{LABELorSCALAR}' without type prefix
          // (e. g. `2{-0}' in mixedRhoE B.C. in
          // rhopSonicFoam/shockTube)
          if (currToken.GetType() == svtkFoamToken::LABEL ||
            currToken.GetType() == svtkFoamToken::SCALAR)
          {
            this->Token = currToken;
            io.ReadExpecting('}');
            return true;
          }
          // return as empty dictionary

          else if (currToken == '}')
          {
            return true;
          }
        }
        else
        {
          // list of dictionaries is read as a usual dictionary
          // polyMesh/boundary, point/face/cell-Zones
          if (currToken.GetType() == svtkFoamToken::LABEL)
          {
            io.ReadExpecting('(');
            if (currToken.To<svtkTypeInt64>() > 0)
            {
              if (!io.Read(currToken))
              {
                throw svtkFoamError() << "Unexpected EOF";
              }
              // continue to read as a usual dictionary
            }
            else // return as empty dictionary

            {
              io.ReadExpecting(')');
              return true;
            }
          }
          // some boundary files does not have the number of boundary
          // patches (e.g. settlingFoam/tank3D). in this case we need to
          // explicitly read the file as a dictionary.

          else if (currToken == '(' && io.GetClassName() == "polyBoundaryMesh") // polyMesh/boundary

          {
            if (!io.Read(currToken)) // read the first keyword

            {
              throw svtkFoamError() << "Unexpected EOF";
            }
            if (currToken == ')') // return as empty dictionary

            {
              return true;
            }
          }
        }
      }
      // if firstToken is given as string read the following stream as
      // subdictionary

      else if (firstToken.GetType() == svtkFoamToken::STRING)
      {
        this->Superclass::push_back(new svtkFoamEntry(this));
        this->Superclass::back()->SetKeyword(firstToken.ToString());
        this->Superclass::back()->ReadDictionary(io);
        if (!io.Read(currToken) || currToken == '}' || currToken == ')')
        {
          return true;
        }
      }
      else // quite likely an identifier

      {
        currToken = firstToken;
      }

      if (currToken == ';' || currToken.GetType() == svtkFoamToken::STRING ||
        currToken.GetType() == svtkFoamToken::IDENTIFIER)
      {
        // general dictionary
        do
        {
          if (currToken.GetType() == svtkFoamToken::STRING)
          {
            svtkFoamEntry* previousEntry = this->Lookup(currToken.ToString());
            if (previousEntry != nullptr)
            {
              if (io.GetInputMode() == svtkFoamFile::INPUT_MODE_MERGE)
              {
                if (previousEntry->FirstValue().GetType() == svtkFoamToken::DICTIONARY)
                {
                  io.ReadExpecting('{');
                  previousEntry->FirstValue().Dictionary().Read(io, true);
                }
                else
                {
                  previousEntry->Clear();
                  previousEntry->Read(io);
                }
              }
              else if (io.GetInputMode() == svtkFoamFile::INPUT_MODE_OVERWRITE)
              {
                previousEntry->Clear();
                previousEntry->Read(io);
              }
              else // INPUT_MODE_ERROR
              {
                throw svtkFoamError()
                  << "Found duplicated entries with keyword " << currToken.ToString();
              }
            }
            else
            {
              this->Superclass::push_back(new svtkFoamEntry(this));
              this->Superclass::back()->SetKeyword(currToken.ToString());
              this->Superclass::back()->Read(io);
            }

            if (currToken == "FoamFile")
            {
              // delete the FoamFile header subdictionary entry
              delete this->Superclass::back();
              this->Superclass::pop_back();
            }
            else if (currToken == "include")
            {
              // include the named file. Exiting the included file at
              // EOF will be handled automatically by
              // svtkFoamFile::closeIncludedFile()
              if (this->Superclass::back()->FirstValue().GetType() != svtkFoamToken::STRING)
              {
                throw svtkFoamError() << "Expected string as the file name to be included, found "
                                     << this->Superclass::back()->FirstValue();
              }
              const svtkStdString includeFileName(this->Superclass::back()->ToString());
              delete this->Superclass::back();
              this->Superclass::pop_back();
              io.IncludeFile(includeFileName, io.GetFilePath());
            }
          }
          else if (currToken.GetType() == svtkFoamToken::IDENTIFIER)
          {
            // substitute identifier
            const svtkStdString identifier(currToken.ToIdentifier());

            for (const svtkFoamDict* uDictPtr = this;;)
            {
              const svtkFoamEntry* identifiedEntry = uDictPtr->Lookup(identifier);

              if (identifiedEntry != nullptr)
              {
                if (identifiedEntry->FirstValue().GetType() != svtkFoamToken::DICTIONARY)
                {
                  throw svtkFoamError()
                    << "Expected dictionary for substituting entry " << identifier;
                }
                const svtkFoamDict& identifiedDict = identifiedEntry->FirstValue().Dictionary();
                for (size_t entryI = 0; entryI < identifiedDict.size(); entryI++)
                {
                  // I think #inputMode handling should be done here
                  // as well, but the genuine FoamFile parser for OF
                  // 1.5 does not seem to be doing it.
                  this->Superclass::push_back(new svtkFoamEntry(*identifiedDict[entryI], this));
                }
                break;
              }
              else
              {
                uDictPtr = uDictPtr->GetUpperDictPtr();
                if (uDictPtr == nullptr)
                {
                  throw svtkFoamError() << "Substituting entry " << identifier << " not found";
                }
              }
            }
          }
          // skip empty entry only with ';'
        } while (io.Read(currToken) &&
          (currToken.GetType() == svtkFoamToken::STRING ||
            currToken.GetType() == svtkFoamToken::IDENTIFIER || currToken == ';'));

        if (currToken.GetType() == svtkFoamToken::TOKEN_ERROR || currToken == '}' ||
          currToken == ')')
        {
          return true;
        }
        throw svtkFoamError() << "Expected keyword, closing brace, ';' or EOF, found " << currToken;
      }
      throw svtkFoamError() << "Expected keyword or identifier, found " << currToken;
    }
    catch (svtkFoamError& e)
    {
      if (isSubDictionary)
      {
        throw;
      }
      else
      {
        io.SetError(e);
        return false;
      }
    }
  }
};

void svtkFoamIOobject::ReadHeader()
{
  svtkFoamToken firstToken;
  firstToken.SetLabelType(
    this->Reader->GetUse64BitLabels() ? svtkFoamToken::INT64 : svtkFoamToken::INT32);

  this->Superclass::ReadExpecting("FoamFile");
  this->Superclass::ReadExpecting('{');

  svtkFoamDict headerDict;
  headerDict.SetLabelType(firstToken.GetLabelType());
  // throw exception in case of error
  headerDict.Read(*this, true, svtkFoamToken());

  const svtkFoamEntry* formatEntry = headerDict.Lookup("format");
  if (formatEntry == nullptr)
  {
    throw svtkFoamError() << "format entry (binary/ascii) not found in FoamFile header";
  }
  // case does matter (e. g. "BINARY" is treated as ascii)
  // cf. src/OpenFOAM/db/IOstreams/IOstreams/IOstream.C
  this->Format = (formatEntry->ToString() == "binary" ? BINARY : ASCII);

  // Newer (binary) files have 'arch' entry with "label=(32|64) scalar=(32|64)"
  // If this entry does not exist, or is incomplete, uses the fallback values
  // that come from the reader (defined in constructor and Close)
  const svtkFoamEntry* archEntry = headerDict.Lookup("arch");
  if (archEntry)
  {
    const svtkStdString archValue = archEntry->ToString();
    svtksys::RegularExpression re;

    if (re.compile("^.*label *= *(32|64).*$") && re.find(archValue.c_str()))
    {
      this->Use64BitLabels = ("64" == re.match(1));
    }
    if (re.compile("^.*scalar *= *(32|64).*$") && re.find(archValue.c_str()))
    {
      this->Use64BitFloats = ("64" == re.match(1));
    }
  }

  const svtkFoamEntry* classEntry = headerDict.Lookup("class");
  if (classEntry == nullptr)
  {
    throw svtkFoamError() << "class name not found in FoamFile header";
  }
  this->HeaderClassName = classEntry->ToString();

  const svtkFoamEntry* objectEntry = headerDict.Lookup("object");
  if (objectEntry == nullptr)
  {
    throw svtkFoamError() << "object name not found in FoamFile header";
  }
  this->ObjectName = objectEntry->ToString();
}

svtkFoamEntryValue::svtkFoamEntryValue(svtkFoamEntryValue& value, const svtkFoamEntry* upperEntryPtr)
  : svtkFoamToken(value)
  , IsUniform(value.GetIsUniform())
  , Managed(true)
  , UpperEntryPtr(upperEntryPtr)
{
  switch (this->Superclass::Type)
  {
    case VECTORLIST:
    {
      svtkFloatArray* fa = svtkFloatArray::SafeDownCast(value.ToSVTKObject());
      if (fa->GetNumberOfComponents() == 6)
      {
        // create deepcopies for svtkObjects to avoid duplicated mainpulation
        svtkFloatArray* newfa = svtkFloatArray::New();
        newfa->DeepCopy(fa);
        this->Superclass::VtkObjectPtr = newfa;
      }
      else
      {
        this->Superclass::VtkObjectPtr = value.ToSVTKObject();
        this->Superclass::VtkObjectPtr->Register(nullptr);
      }
    }
    break;
    case LABELLIST:
    case SCALARLIST:
    case STRINGLIST:
      this->Superclass::VtkObjectPtr = value.ToSVTKObject();
      this->Superclass::VtkObjectPtr->Register(nullptr);
      break;
    case LABELLISTLIST:
      assert("Label type not set!" && this->GetLabelType() != NO_LABEL_TYPE);
      if (this->LabelType == INT32)
      {
        this->LabelListListPtr = new svtkFoamLabel32VectorVector(*value.LabelListListPtr);
      }
      else
      {
        this->LabelListListPtr = new svtkFoamLabel64VectorVector(*value.LabelListListPtr);
      }
      break;
    case ENTRYVALUELIST:
    {
      const size_t nValues = value.EntryValuePtrs->size();
      this->EntryValuePtrs = new std::vector<svtkFoamEntryValue*>(nValues);
      for (size_t valueI = 0; valueI < nValues; valueI++)
      {
        this->EntryValuePtrs->operator[](valueI) =
          new svtkFoamEntryValue(*value.EntryValuePtrs->operator[](valueI), upperEntryPtr);
      }
    }
    break;
    case DICTIONARY:
      // UpperEntryPtr is null when called from svtkFoamDict constructor
      if (this->UpperEntryPtr != nullptr)
      {
        this->DictPtr = new svtkFoamDict(*value.DictPtr, this->UpperEntryPtr->GetUpperDictPtr());
        this->DictPtr->SetLabelType(value.GetLabelType());
      }
      else
      {
        this->DictPtr = nullptr;
      }
      break;
    case BOOLLIST:
      break;
    case EMPTYLIST:
      break;
    case UNDEFINED:
    case PUNCTUATION:
    case LABEL:
    case SCALAR:
    case STRING:
    case IDENTIFIER:
    case TOKEN_ERROR:
    default:
      break;
  }
}

void svtkFoamEntryValue::Clear()
{
  if (this->Managed)
  {
    switch (this->Superclass::Type)
    {
      case LABELLIST:
      case SCALARLIST:
      case VECTORLIST:
      case STRINGLIST:
        this->VtkObjectPtr->Delete();
        break;
      case LABELLISTLIST:
        delete this->LabelListListPtr;
        break;
      case ENTRYVALUELIST:
        for (size_t valueI = 0; valueI < this->EntryValuePtrs->size(); valueI++)
        {
          delete this->EntryValuePtrs->operator[](valueI);
        }
        delete this->EntryValuePtrs;
        break;
      case DICTIONARY:
        delete this->DictPtr;
        break;
      case UNDEFINED:
      case PUNCTUATION:
      case LABEL:
      case SCALAR:
      case STRING:
      case IDENTIFIER:
      case TOKEN_ERROR:
      case BOOLLIST:
      case EMPTYLIST:
      default:
        break;
    }
  }
}

// general-purpose list reader - guess the type of the list and read
// it. only supports ascii format and assumes the preceding '(' has
// already been thrown away.  the reader supports nested list with
// variable lengths (e. g. `((token token) (token token token)).'
// also supports compound of tokens and lists (e. g. `((token token)
// token)') only if a list comes as the first value.
void svtkFoamEntryValue::ReadList(svtkFoamIOobject& io)
{
  assert("Label type not set!" && this->GetLabelType() != NO_LABEL_TYPE);
  svtkFoamToken currToken;
  currToken.SetLabelType(this->LabelType);
  io.Read(currToken);

  // initial guess of the list type
  if (currToken.GetType() == this->Superclass::LABEL)
  {
    // if the first token is of type LABEL it might be either an element of
    // a labelList or the size of a sublist so proceed to the next token
    svtkFoamToken nextToken;
    nextToken.SetLabelType(this->LabelType);
    if (!io.Read(nextToken))
    {
      throw svtkFoamError() << "Unexpected EOF";
    }
    if (nextToken.GetType() == this->Superclass::LABEL)
    {
      if (this->LabelType == INT32)
      {
        svtkTypeInt32Array* array = svtkTypeInt32Array::New();
        array->InsertNextValue(currToken.To<svtkTypeInt32>());
        array->InsertNextValue(nextToken.To<svtkTypeInt32>());
        this->Superclass::LabelListPtr = array;
      }
      else
      {
        svtkTypeInt64Array* array = svtkTypeInt64Array::New();
        array->InsertNextValue(currToken.To<svtkTypeInt64>());
        array->InsertNextValue(nextToken.To<svtkTypeInt64>());
        this->Superclass::LabelListPtr = array;
      }
      this->Superclass::Type = LABELLIST;
    }
    else if (nextToken.GetType() == this->Superclass::SCALAR)
    {
      this->Superclass::ScalarListPtr = svtkFloatArray::New();
      this->Superclass::ScalarListPtr->InsertNextValue(currToken.To<float>());
      this->Superclass::ScalarListPtr->InsertNextValue(nextToken.To<float>());
      this->Superclass::Type = SCALARLIST;
    }
    else if (nextToken == '(') // list of list: read recursively
    {
      this->Superclass::EntryValuePtrs = new std::vector<svtkFoamEntryValue*>;
      this->Superclass::EntryValuePtrs->push_back(new svtkFoamEntryValue(this->UpperEntryPtr));
      this->Superclass::EntryValuePtrs->back()->SetLabelType(this->LabelType);
      this->Superclass::EntryValuePtrs->back()->ReadList(io);
      this->Superclass::Type = ENTRYVALUELIST;
    }
    else if (nextToken == ')') // list with only one label element
    {
      if (this->LabelType == INT32)
      {
        svtkTypeInt32Array* array = svtkTypeInt32Array::New();
        array->SetNumberOfValues(1);
        array->SetValue(0, currToken.To<svtkTypeInt32>());
        this->Superclass::LabelListPtr = array;
      }
      else
      {
        svtkTypeInt64Array* array = svtkTypeInt64Array::New();
        array->SetNumberOfValues(1);
        array->SetValue(0, currToken.To<svtkTypeInt64>());
        this->Superclass::LabelListPtr = array;
      }
      this->Superclass::Type = LABELLIST;
      return;
    }
    else
    {
      throw svtkFoamError() << "Expected number, '(' or ')', found " << nextToken;
    }
  }
  else if (currToken.GetType() == this->Superclass::SCALAR)
  {
    this->Superclass::ScalarListPtr = svtkFloatArray::New();
    this->Superclass::ScalarListPtr->InsertNextValue(currToken.To<float>());
    this->Superclass::Type = SCALARLIST;
  }
  // if the first word is a string we have to read another token to determine
  // if the first word is a keyword for the following dictionary
  else if (currToken.GetType() == this->Superclass::STRING)
  {
    svtkFoamToken nextToken;
    nextToken.SetLabelType(this->LabelType);
    if (!io.Read(nextToken))
    {
      throw svtkFoamError() << "Unexpected EOF";
    }
    if (nextToken.GetType() == this->Superclass::STRING) // list of strings
    {
      this->Superclass::StringListPtr = svtkStringArray::New();
      this->Superclass::StringListPtr->InsertNextValue(currToken.ToString());
      this->Superclass::StringListPtr->InsertNextValue(nextToken.ToString());
      this->Superclass::Type = STRINGLIST;
    }
    // dictionary with the already read stringToken as the first keyword
    else if (nextToken == '{')
    {
      if (currToken.ToString().empty())
      {
        throw "Empty string is invalid as a keyword for dictionary entry";
      }
      this->ReadDictionary(io, currToken);
      // the dictionary read as list has the entry terminator ';' so
      // we have to skip it
      return;
    }
    else if (nextToken == ')') // list with only one string element
    {
      this->Superclass::StringListPtr = svtkStringArray::New();
      this->Superclass::StringListPtr->SetNumberOfValues(1);
      this->Superclass::StringListPtr->SetValue(0, currToken.ToString());
      this->Superclass::Type = STRINGLIST;
      return;
    }
    else
    {
      throw svtkFoamError() << "Expected string, '{' or ')', found " << nextToken;
    }
  }
  // list of lists or dictionaries: read recursively
  else if (currToken == '(' || currToken == '{')
  {
    this->Superclass::EntryValuePtrs = new std::vector<svtkFoamEntryValue*>;
    this->Superclass::EntryValuePtrs->push_back(new svtkFoamEntryValue(this->UpperEntryPtr));
    this->Superclass::EntryValuePtrs->back()->SetLabelType(this->LabelType);
    if (currToken == '(')
    {
      this->Superclass::EntryValuePtrs->back()->ReadList(io);
    }
    else // currToken == '{'
    {
      this->Superclass::EntryValuePtrs->back()->ReadDictionary(io, svtkFoamToken());
    }
    // read all the following values as arbitrary entryValues
    // the alphaContactAngle b.c. in multiphaseInterFoam/damBreak4phase
    // reaquires this treatment (reading by readList() is not enough)
    do
    {
      this->Superclass::EntryValuePtrs->push_back(new svtkFoamEntryValue(this->UpperEntryPtr));
      this->Superclass::EntryValuePtrs->back()->Read(io);
    } while (*this->Superclass::EntryValuePtrs->back() != ')' &&
      *this->Superclass::EntryValuePtrs->back() != '}' &&
      *this->Superclass::EntryValuePtrs->back() != ';');

    if (*this->Superclass::EntryValuePtrs->back() != ')')
    {
      throw svtkFoamError() << "Expected ')' before " << *this->Superclass::EntryValuePtrs->back();
    }

    // delete ')'
    delete this->Superclass::EntryValuePtrs->back();
    this->EntryValuePtrs->pop_back();
    this->Superclass::Type = ENTRYVALUELIST;
    return;
  }
  else if (currToken == ')') // empty list
  {
    this->Superclass::Type = EMPTYLIST;
    return;
  }
  // FIXME: may (or may not) need identifier handling

  while (io.Read(currToken) && currToken != ')')
  {
    if (this->Superclass::Type == LABELLIST)
    {
      if (currToken.GetType() == this->Superclass::SCALAR)
      {
        // switch to scalarList
        // LabelListPtr and ScalarListPtr are packed into a single union so
        // we need a temporary pointer
        svtkFloatArray* slPtr = svtkFloatArray::New();
        svtkIdType size = this->Superclass::LabelListPtr->GetNumberOfTuples();
        slPtr->SetNumberOfValues(size + 1);
        for (int i = 0; i < size; i++)
        {
          slPtr->SetValue(
            i, static_cast<float>(GetLabelValue(this->LabelListPtr, i, this->LabelType == INT64)));
        }
        this->LabelListPtr->Delete();
        slPtr->SetValue(size, currToken.To<float>());
        // copy after LabelListPtr is deleted
        this->Superclass::ScalarListPtr = slPtr;
        this->Superclass::Type = SCALARLIST;
      }
      else if (currToken.GetType() == this->Superclass::LABEL)
      {
        assert("Label type not set!" && currToken.GetLabelType() != NO_LABEL_TYPE);
        if (currToken.GetLabelType() == INT32)
        {
          assert(svtkTypeInt32Array::FastDownCast(this->LabelListPtr) != nullptr);
          static_cast<svtkTypeInt32Array*>(this->LabelListPtr)
            ->InsertNextValue(currToken.To<svtkTypeInt32>());
        }
        else
        {
          assert(svtkTypeInt64Array::FastDownCast(this->LabelListPtr) != nullptr);
          static_cast<svtkTypeInt64Array*>(this->LabelListPtr)
            ->InsertNextValue(currToken.To<svtkTypeInt64>());
        }
      }
      else
      {
        throw svtkFoamError() << "Expected a number, found " << currToken;
      }
    }
    else if (this->Superclass::Type == this->Superclass::SCALARLIST)
    {
      if (currToken.Is<float>())
      {
        this->Superclass::ScalarListPtr->InsertNextValue(currToken.To<float>());
      }
      else if (currToken == '(')
      {
        svtkGenericWarningMacro("Found a list containing scalar data followed "
                               "by a nested list, but this reader only "
                               "supports nested lists that precede all "
                               "scalars. Discarding nested list data.");
        svtkFoamEntryValue tmp(this->UpperEntryPtr);
        tmp.SetLabelType(this->LabelType);
        tmp.ReadList(io);
      }
      else
      {
        throw svtkFoamError() << "Expected a number, found " << currToken;
      }
    }
    else if (this->Superclass::Type == this->Superclass::STRINGLIST)
    {
      if (currToken.GetType() == this->Superclass::STRING)
      {
        this->Superclass::StringListPtr->InsertNextValue(currToken.ToString());
      }
      else
      {
        throw svtkFoamError() << "Expected a string, found " << currToken;
      }
    }
    else if (this->Superclass::Type == this->Superclass::ENTRYVALUELIST)
    {
      if (currToken.GetType() == this->Superclass::LABEL)
      {
        // skip the number of elements to make things simple
        if (!io.Read(currToken))
        {
          throw svtkFoamError() << "Unexpected EOF";
        }
      }
      if (currToken != '(')
      {
        throw svtkFoamError() << "Expected '(', found " << currToken;
      }
      this->Superclass::EntryValuePtrs->push_back(new svtkFoamEntryValue(this->UpperEntryPtr));
      this->Superclass::EntryValuePtrs->back()->ReadList(io);
    }
    else
    {
      throw svtkFoamError() << "Unexpected token " << currToken;
    }
  }

  if (this->Superclass::Type == this->Superclass::LABELLIST)
  {
    this->Superclass::LabelListPtr->Squeeze();
  }
  else if (this->Superclass::Type == this->Superclass::SCALARLIST)
  {
    this->Superclass::ScalarListPtr->Squeeze();
  }
  else if (this->Superclass::Type == this->Superclass::STRINGLIST)
  {
    this->Superclass::StringListPtr->Squeeze();
  }
}

// a list of dictionaries is actually read as a dictionary
void svtkFoamEntryValue::ReadDictionary(svtkFoamIOobject& io, const svtkFoamToken& firstKeyword)
{
  this->Superclass::DictPtr = new svtkFoamDict(this->UpperEntryPtr->GetUpperDictPtr());
  this->DictPtr->SetLabelType(io.GetUse64BitLabels() ? svtkFoamToken::INT64 : svtkFoamToken::INT32);
  this->Superclass::Type = this->Superclass::DICTIONARY;
  this->Superclass::DictPtr->Read(io, true, firstKeyword);
}

// guess the type of the given entry value and read it
// return value: 0 if encountered end of entry (';') during parsing
// composite entry value, 1 otherwise
int svtkFoamEntryValue::Read(svtkFoamIOobject& io)
{
  this->SetLabelType(io.GetUse64BitLabels() ? svtkFoamToken::INT64 : svtkFoamToken::INT32);

  svtkFoamToken currToken;
  currToken.SetLabelType(this->LabelType);
  if (!io.Read(currToken))
  {
    throw svtkFoamError() << "Unexpected EOF";
  }

  if (currToken == '{')
  {
    this->ReadDictionary(io, svtkFoamToken());
    return 1;
  }
  // for reading sublist from svtkFoamEntryValue::readList() or there
  // are cases where lists without the (non)uniform keyword appear
  // (e. g. coodles/pitsDaily/0/U, uniformFixedValue b.c.)
  else if (currToken == '(')
  {
    this->ReadList(io);
    return 1;
  }
  else if (currToken == '[')
  {
    this->ReadDimensionSet(io);
    return 1;
  }
  else if (currToken == "uniform")
  {
    if (!io.Read(currToken))
    {
      throw svtkFoamError() << "Expected a uniform value or a list, found unexpected EOF";
    }
    if (currToken == '(')
    {
      this->ReadList(io);
    }
    else if (currToken == ';')
    {
      this->Superclass::operator=("uniform");
      return 0;
    }
    else if (currToken.GetType() == this->Superclass::LABEL ||
      currToken.GetType() == this->Superclass::SCALAR ||
      currToken.GetType() == this->Superclass::STRING)
    {
      this->Superclass::operator=(currToken);
    }
    else // unexpected punctuation token
    {
      throw svtkFoamError() << "Expected number, string or (, found " << currToken;
    }
    this->IsUniform = true;
  }
  else if (currToken == "nonuniform")
  {
    if (!io.Read(currToken))
    {
      throw svtkFoamError() << "Expected list type specifier, found EOF";
    }
    this->IsUniform = false;
    if (currToken == "List<scalar>")
    {
      if (io.GetUse64BitFloats())
      {
        this->ReadNonuniformList<SCALARLIST, listTraits<svtkFloatArray, double> >(io);
      }
      else
      {
        this->ReadNonuniformList<SCALARLIST, listTraits<svtkFloatArray, float> >(io);
      }
    }
    else if (currToken == "List<sphericalTensor>")
    {
      if (io.GetUse64BitFloats())
      {
        this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, double, 1, false> >(
          io);
      }
      else
      {
        this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, float, 1, false> >(io);
      }
    }
    else if (currToken == "List<vector>")
    {
      if (io.GetUse64BitFloats())
      {
        this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, double, 3, false> >(
          io);
      }
      else
      {
        this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, float, 3, false> >(io);
      }
    }
    else if (currToken == "List<symmTensor>")
    {
      if (io.GetUse64BitFloats())
      {
        this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, double, 6, false> >(
          io);
      }
      else
      {
        this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, float, 6, false> >(io);
      }
    }
    else if (currToken == "List<tensor>")
    {
      if (io.GetUse64BitFloats())
      {
        this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, double, 9, false> >(
          io);
      }
      else
      {
        this->ReadNonuniformList<VECTORLIST, vectorListTraits<svtkFloatArray, float, 9, false> >(io);
      }
    }
    // List<bool> may or may not be read as List<label>,
    // this needs checking but not many bool fields in use
    else if (currToken == "List<label>" || currToken == "List<bool>")
    {
      assert("Label type not set!" && this->GetLabelType() != NO_LABEL_TYPE);
      if (this->LabelType == INT64)
      {
        this->ReadNonuniformList<LABELLIST, listTraits<svtkTypeInt64Array, svtkTypeInt64> >(io);
      }
      else
      {
        this->ReadNonuniformList<LABELLIST, listTraits<svtkTypeInt32Array, svtkTypeInt32> >(io);
      }
    }
    // an empty list doesn't have a list type specifier
    else if (currToken.GetType() == this->Superclass::LABEL && currToken.To<svtkTypeInt64>() == 0)
    {
      this->Superclass::Type = this->Superclass::EMPTYLIST;
      if (io.GetFormat() == svtkFoamIOobject::ASCII)
      {
        io.ReadExpecting('(');
        io.ReadExpecting(')');
      }
    }
    else if (currToken == ';')
    {
      this->Superclass::operator=("nonuniform");
      return 0;
    }
    else
    {
      throw svtkFoamError() << "Unsupported nonuniform list type " << currToken;
    }
  }
  // turbulentTemperatureCoupledBaffleMixed boundary condition
  // uses lists without a uniform/nonuniform keyword
  else if (currToken == "List<scalar>")
  {
    this->IsUniform = false;
    if (io.GetUse64BitFloats())
    {
      this->ReadNonuniformList<SCALARLIST, listTraits<svtkFloatArray,
                                                      double> >(io);
    }
    else
    {
      this->ReadNonuniformList<SCALARLIST, listTraits<svtkFloatArray,
                                                      float> >(io);
    }
  }
  // zones have list without a uniform/nonuniform keyword
  else if (currToken == "List<label>")
  {
    this->IsUniform = false;
    assert("Label type not set!" && this->GetLabelType() != NO_LABEL_TYPE);
    if (this->LabelType == INT64)
    {
      this->ReadNonuniformList<LABELLIST, listTraits<svtkTypeInt64Array, svtkTypeInt64> >(io);
    }
    else
    {
      this->ReadNonuniformList<LABELLIST, listTraits<svtkTypeInt32Array, svtkTypeInt32> >(io);
    }
  }
  else if (currToken == "List<bool>")
  {
    // List<bool> is read as a list of bytes (binary) or ints (ascii)
    // - primary location is the flipMap entry in faceZones
    this->IsUniform = false;
    this->ReadNonuniformList<BOOLLIST, listTraits<svtkCharArray, char> >(io);
  }
  else if (currToken.GetType() == this->Superclass::PUNCTUATION ||
    currToken.GetType() == this->Superclass::LABEL ||
    currToken.GetType() == this->Superclass::SCALAR ||
    currToken.GetType() == this->Superclass::STRING ||
    currToken.GetType() == this->Superclass::IDENTIFIER)
  {
    this->Superclass::operator=(currToken);
  }
  return 1;
}

// read values of an entry
void svtkFoamEntry::Read(svtkFoamIOobject& io)
{
  for (;;)
  {
    this->Superclass::push_back(new svtkFoamEntryValue(this));
    if (!this->Superclass::back()->Read(io))
    {
      break;
    }

    if (this->Superclass::size() >= 2)
    {
      svtkFoamEntryValue& secondLastValue =
        *this->Superclass::operator[](this->Superclass::size() - 2);
      if (secondLastValue.GetType() == svtkFoamToken::LABEL)
      {
        svtkFoamEntryValue& lastValue = *this->Superclass::back();

        // a zero-sized nonuniform list without prefixing "nonuniform"
        // keyword nor list type specifier (i. e. `0()';
        // e. g. simpleEngine/0/polyMesh/pointZones) requires special
        // care (one with nonuniform prefix is treated within
        // svtkFoamEntryValue::read()). still this causes erroneous
        // behavior for `0 nonuniform 0()' but this should be extremely
        // rare
        if (lastValue.GetType() == svtkFoamToken::EMPTYLIST && secondLastValue == 0)
        {
          delete this->Superclass::back();
          this->Superclass::pop_back(); // delete the last value
          // mark new last value as empty
          this->Superclass::back()->SetEmptyList();
        }
        // for an exceptional expression of `LABEL{LABELorSCALAR}' without
        // type prefix (e. g. `2{-0}' in mixedRhoE B.C. in
        // rhopSonicFoam/shockTube)
        else if (lastValue.GetType() == svtkFoamToken::DICTIONARY)
        {
          if (lastValue.Dictionary().GetType() == svtkFoamToken::LABEL)
          {
            const svtkTypeInt64 asize = secondLastValue.To<svtkTypeInt64>();
            const svtkTypeInt64 value = lastValue.Dictionary().GetToken().ToInt();
            // delete last two values
            delete this->Superclass::back();
            this->Superclass::pop_back();
            delete this->Superclass::back();
            this->Superclass::pop_back();
            // make new labelList
            this->Superclass::push_back(new svtkFoamEntryValue(this));
            this->Superclass::back()->MakeLabelList(value, asize);
          }
          else if (lastValue.Dictionary().GetType() == svtkFoamToken::SCALAR)
          {
            const svtkTypeInt64 asize = secondLastValue.To<svtkTypeInt64>();
            const float value = lastValue.Dictionary().GetToken().ToFloat();
            // delete last two values
            delete this->Superclass::back();
            this->Superclass::pop_back();
            delete this->Superclass::back();
            this->Superclass::pop_back();
            // make new labelList
            this->Superclass::push_back(new svtkFoamEntryValue(this));
            this->Superclass::back()->MakeScalarList(value, asize);
          }
        }
      }
    }

    if (this->Superclass::back()->GetType() == svtkFoamToken::IDENTIFIER)
    {
      // substitute identifier
      const svtkStdString identifier(this->Superclass::back()->ToIdentifier());
      delete this->Superclass::back();
      this->Superclass::pop_back();

      for (const svtkFoamDict* uDictPtr = this->UpperDictPtr;;)
      {
        const svtkFoamEntry* identifiedEntry = uDictPtr->Lookup(identifier);

        if (identifiedEntry != nullptr)
        {
          for (size_t valueI = 0; valueI < identifiedEntry->size(); valueI++)
          {
            this->Superclass::push_back(
              new svtkFoamEntryValue(*identifiedEntry->operator[](valueI), this));
            this->back()->SetLabelType(
              io.GetUse64BitLabels() ? svtkFoamToken::INT64 : svtkFoamToken::INT32);
          }
          break;
        }
        else
        {
          uDictPtr = uDictPtr->GetUpperDictPtr();
          if (uDictPtr == nullptr)
          {
            throw svtkFoamError() << "substituting entry " << identifier << " not found";
          }
        }
      }
    }
    else if (*this->Superclass::back() == ';')
    {
      delete this->Superclass::back();
      this->Superclass::pop_back();
      break;
    }
    else if (this->Superclass::back()->GetType() == svtkFoamToken::DICTIONARY)
    {
      // subdictionary is not suffixed by an entry terminator ';'
      break;
    }
    else if (*this->Superclass::back() == '}' || *this->Superclass::back() == ')')
    {
      throw svtkFoamError() << "Unmatched " << *this->Superclass::back();
    }
  }
}

//-----------------------------------------------------------------------------
// svtkOpenFOAMReaderPrivate constructor and destructor
svtkOpenFOAMReaderPrivate::svtkOpenFOAMReaderPrivate()
{
  // DATA TIMES
  this->TimeStep = 0;
  this->TimeStepOld = -1;
  this->TimeValues = svtkDoubleArray::New();
  this->TimeNames = svtkStringArray::New();

  // selection
  this->InternalMeshSelectionStatus = 0;
  this->InternalMeshSelectionStatusOld = 0;

  // DATA COUNTS
  this->NumCells = 0;
  this->NumPoints = 0;

  this->VolFieldFiles = svtkStringArray::New();
  this->PointFieldFiles = svtkStringArray::New();
  this->LagrangianFieldFiles = svtkStringArray::New();
  this->PolyMeshPointsDir = svtkStringArray::New();
  this->PolyMeshFacesDir = svtkStringArray::New();

  // for creating cell-to-point translated data
  this->BoundaryPointMap = nullptr;
  this->AllBoundaries = nullptr;
  this->AllBoundariesPointMap = nullptr;
  this->InternalPoints = nullptr;

  // for caching mesh
  this->InternalMesh = nullptr;
  this->BoundaryMesh = nullptr;
  this->BoundaryPointMap = nullptr;
  this->FaceOwner = nullptr;
  this->PointZoneMesh = nullptr;
  this->FaceZoneMesh = nullptr;
  this->CellZoneMesh = nullptr;

  // for decomposing polyhedra
  this->NumAdditionalCells = nullptr;
  this->AdditionalCellIds = nullptr;
  this->NumAdditionalCells = nullptr;
  this->AdditionalCellPoints = nullptr;

  this->NumTotalAdditionalCells = 0;
  this->Parent = nullptr;
}

svtkOpenFOAMReaderPrivate::~svtkOpenFOAMReaderPrivate()
{
  this->TimeValues->Delete();
  this->TimeNames->Delete();

  this->PolyMeshPointsDir->Delete();
  this->PolyMeshFacesDir->Delete();
  this->VolFieldFiles->Delete();
  this->PointFieldFiles->Delete();
  this->LagrangianFieldFiles->Delete();

  this->ClearMeshes();
}

void svtkOpenFOAMReaderPrivate::ClearInternalMeshes()
{
  if (this->FaceOwner != nullptr)
  {
    this->FaceOwner->Delete();
    this->FaceOwner = nullptr;
  }
  if (this->InternalMesh != nullptr)
  {
    this->InternalMesh->Delete();
    this->InternalMesh = nullptr;
  }
  if (this->AdditionalCellIds != nullptr)
  {
    this->AdditionalCellIds->Delete();
    this->AdditionalCellIds = nullptr;
  }
  if (this->NumAdditionalCells != nullptr)
  {
    this->NumAdditionalCells->Delete();
    this->NumAdditionalCells = nullptr;
  }
  delete this->AdditionalCellPoints;
  this->AdditionalCellPoints = nullptr;

  if (this->PointZoneMesh != nullptr)
  {
    this->PointZoneMesh->Delete();
    this->PointZoneMesh = nullptr;
  }
  if (this->FaceZoneMesh != nullptr)
  {
    this->FaceZoneMesh->Delete();
    this->FaceZoneMesh = nullptr;
  }
  if (this->CellZoneMesh != nullptr)
  {
    this->CellZoneMesh->Delete();
    this->CellZoneMesh = nullptr;
  }
}

void svtkOpenFOAMReaderPrivate::ClearBoundaryMeshes()
{
  if (this->BoundaryMesh != nullptr)
  {
    this->BoundaryMesh->Delete();
    this->BoundaryMesh = nullptr;
  }

  delete this->BoundaryPointMap;
  this->BoundaryPointMap = nullptr;

  if (this->InternalPoints != nullptr)
  {
    this->InternalPoints->Delete();
    this->InternalPoints = nullptr;
  }
  if (this->AllBoundaries != nullptr)
  {
    this->AllBoundaries->Delete();
    this->AllBoundaries = nullptr;
  }
  if (this->AllBoundariesPointMap != nullptr)
  {
    this->AllBoundariesPointMap->Delete();
    this->AllBoundariesPointMap = nullptr;
  }
}

void svtkOpenFOAMReaderPrivate::ClearMeshes()
{
  this->ClearInternalMeshes();
  this->ClearBoundaryMeshes();
}

void svtkOpenFOAMReaderPrivate::SetTimeValue(const double requestedTime)
{
  const svtkIdType nTimeValues = this->TimeValues->GetNumberOfTuples();

  if (nTimeValues > 0)
  {
    int minTimeI = 0;
    double minTimeDiff = fabs(this->TimeValues->GetValue(0) - requestedTime);
    for (int timeI = 1; timeI < nTimeValues; timeI++)
    {
      const double timeDiff(fabs(this->TimeValues->GetValue(timeI) - requestedTime));
      if (timeDiff < minTimeDiff)
      {
        minTimeI = timeI;
        minTimeDiff = timeDiff;
      }
    }
    this->SetTimeStep(minTimeI); // set Modified() if TimeStep changed
  }
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReaderPrivate::SetupInformation(const svtkStdString& casePath,
  const svtkStdString& regionName, const svtkStdString& procName, svtkOpenFOAMReaderPrivate* master)
{
  // copy parent, path and timestep information from master
  this->CasePath = casePath;
  this->RegionName = regionName;
  this->ProcessorName = procName;
  this->Parent = master->Parent;
  this->TimeValues->Delete();
  this->TimeValues = master->TimeValues;
  this->TimeValues->Register(nullptr);
  this->TimeNames->Delete();
  this->TimeNames = master->TimeNames;
  this->TimeNames->Register(nullptr);

  this->PopulatePolyMeshDirArrays();
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReaderPrivate::GetFieldNames(const svtkStdString& tempPath, const bool isLagrangian,
  svtkStringArray* cellObjectNames, svtkStringArray* pointObjectNames)
{
  // open the directory and get num of files
  svtkDirectory* directory = svtkDirectory::New();
  if (!directory->Open(tempPath.c_str()))
  {
    // no data
    directory->Delete();
    return;
  }

  // loop over all files and locate valid fields
  svtkIdType nFieldFiles = directory->GetNumberOfFiles();
  for (svtkIdType j = 0; j < nFieldFiles; j++)
  {
    const svtkStdString fieldFile(directory->GetFile(j));
    const size_t len = fieldFile.length();

    // excluded extensions cf. src/OpenFOAM/OSspecific/Unix/Unix.C
    if (!directory->FileIsDirectory(fieldFile.c_str()) && fieldFile.substr(len - 1) != "~" &&
      (len < 4 ||
        (fieldFile.substr(len - 4) != ".bak" && fieldFile.substr(len - 4) != ".BAK" &&
          fieldFile.substr(len - 4) != ".old")) &&
      (len < 5 || fieldFile.substr(len - 5) != ".save"))
    {
      svtkFoamIOobject io(this->CasePath, this->Parent);
      if (io.Open(tempPath + "/" + fieldFile)) // file exists and readable
      {
        const svtkStdString& cn = io.GetClassName();
        if (isLagrangian)
        {
          if (cn == "labelField" || cn == "scalarField" || cn == "vectorField" ||
            cn == "sphericalTensorField" || cn == "symmTensorField" || cn == "tensorField")
          {
            // real file name
            this->LagrangianFieldFiles->InsertNextValue(fieldFile);
            // object name
            pointObjectNames->InsertNextValue(io.GetObjectName());
          }
        }
        else
        {
          if (cn == "volScalarField" || cn == "pointScalarField" || cn == "volVectorField" ||
            cn == "pointVectorField" || cn == "volSphericalTensorField" ||
            cn == "pointSphericalTensorField" || cn == "volSymmTensorField" ||
            cn == "pointSymmTensorField" || cn == "volTensorField" || cn == "pointTensorField")
          {
            if (cn.substr(0, 3) == "vol")
            {
              // real file name
              this->VolFieldFiles->InsertNextValue(fieldFile);
              // object name
              cellObjectNames->InsertNextValue(io.GetObjectName());
            }
            else
            {
              this->PointFieldFiles->InsertNextValue(fieldFile);
              pointObjectNames->InsertNextValue(io.GetObjectName());
            }
          }
        }
        io.Close();
      }
    }
  }
  // inserted objects are squeezed later in SortFieldFiles()
  directory->Delete();
}

//-----------------------------------------------------------------------------
// locate laglangian clouds
void svtkOpenFOAMReaderPrivate::LocateLagrangianClouds(
  svtkStringArray* lagrangianObjectNames, const svtkStdString& timePath)
{
  svtkDirectory* directory = svtkDirectory::New();
  if (directory->Open((timePath + this->RegionPath() + "/lagrangian").c_str()))
  {
    // search for sub-clouds (OF 1.5 format)
    svtkIdType nFiles = directory->GetNumberOfFiles();
    bool isSubCloud = false;
    for (svtkIdType fileI = 0; fileI < nFiles; fileI++)
    {
      const svtkStdString fileNameI(directory->GetFile(fileI));
      if (fileNameI != "." && fileNameI != ".." && directory->FileIsDirectory(fileNameI.c_str()))
      {
        svtkFoamIOobject io(this->CasePath, this->Parent);
        const svtkStdString subCloudName(this->RegionPrefix() + "lagrangian/" + fileNameI);
        const svtkStdString subCloudFullPath(timePath + "/" + subCloudName);
        // lagrangian positions. there are many concrete class names
        // e. g. Cloud<parcel>, basicKinematicCloud etc.
        if ((io.Open(subCloudFullPath + "/positions") ||
              io.Open(subCloudFullPath + "/positions.gz")) &&
          io.GetClassName().find("Cloud") != svtkStdString::npos &&
          io.GetObjectName() == "positions")
        {
          isSubCloud = true;
          // a lagrangianPath has to be in a bit different format from
          // subCloudName to make the "lagrangian" reserved path
          // component and a mesh region with the same name
          // distinguishable later
          const svtkStdString subCloudPath(this->RegionName + "/lagrangian/" + fileNameI);
          if (this->Parent->LagrangianPaths->LookupValue(subCloudPath) == -1)
          {
            this->Parent->LagrangianPaths->InsertNextValue(subCloudPath);
          }
          this->GetFieldNames(subCloudFullPath, true, nullptr, lagrangianObjectNames);
          this->Parent->PatchDataArraySelection->AddArray(subCloudName.c_str());
        }
      }
    }
    // if there's no sub-cloud then OF < 1.5 format
    if (!isSubCloud)
    {
      svtkFoamIOobject io(this->CasePath, this->Parent);
      const svtkStdString cloudName(this->RegionPrefix() + "lagrangian");
      const svtkStdString cloudFullPath(timePath + "/" + cloudName);
      if ((io.Open(cloudFullPath + "/positions") || io.Open(cloudFullPath + "/positions.gz")) &&
        io.GetClassName().find("Cloud") != svtkStdString::npos && io.GetObjectName() == "positions")
      {
        const svtkStdString cloudPath(this->RegionName + "/lagrangian");
        if (this->Parent->LagrangianPaths->LookupValue(cloudPath) == -1)
        {
          this->Parent->LagrangianPaths->InsertNextValue(cloudPath);
        }
        this->GetFieldNames(cloudFullPath, true, nullptr, lagrangianObjectNames);
        this->Parent->PatchDataArraySelection->AddArray(cloudName.c_str());
      }
    }
    this->Parent->LagrangianPaths->Squeeze();
  }
  directory->Delete();
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReaderPrivate::SortFieldFiles(
  svtkStringArray* selections, svtkStringArray* files, svtkStringArray* objects)
{
  objects->Squeeze();
  files->Squeeze();
  svtkSortDataArray::Sort(objects, files);
  for (int nameI = 0; nameI < objects->GetNumberOfValues(); nameI++)
  {
    selections->InsertNextValue(objects->GetValue(nameI));
  }
  objects->Delete();
}

//-----------------------------------------------------------------------------
// create field data lists and cell/point array selection lists
int svtkOpenFOAMReaderPrivate::MakeMetaDataAtTimeStep(svtkStringArray* cellSelectionNames,
  svtkStringArray* pointSelectionNames, svtkStringArray* lagrangianSelectionNames,
  const bool listNextTimeStep)
{
  // Read the patches from the boundary file into selection array
  if (this->PolyMeshFacesDir->GetValue(this->TimeStep) != this->BoundaryDict.TimeDir ||
    this->Parent->PatchDataArraySelection->GetMTime() != this->Parent->PatchSelectionMTimeOld)
  {
    this->BoundaryDict.clear();
    this->BoundaryDict.TimeDir = this->PolyMeshFacesDir->GetValue(this->TimeStep);

    const bool isSubRegion = !this->RegionName.empty();
    svtkFoamDict* boundaryDict = this->GatherBlocks("boundary", isSubRegion);
    if (boundaryDict == nullptr)
    {
      if (isSubRegion)
      {
        return 0;
      }
    }
    else
    {
      // Add the internal mesh by default always
      const svtkStdString internalMeshName(this->RegionPrefix() + "internalMesh");
      this->Parent->PatchDataArraySelection->AddArray(internalMeshName.c_str());
      this->InternalMeshSelectionStatus =
        this->Parent->GetPatchArrayStatus(internalMeshName.c_str());

      // iterate through each entry in the boundary file
      svtkTypeInt64 allBoundariesNextStartFace = 0;
      this->BoundaryDict.resize(boundaryDict->size());
      for (size_t i = 0; i < boundaryDict->size(); i++)
      {
        svtkFoamEntry* boundaryEntryI = boundaryDict->operator[](i);
        const svtkFoamEntry* nFacesEntry = boundaryEntryI->Dictionary().Lookup("nFaces");
        if (nFacesEntry == nullptr)
        {
          svtkErrorMacro(<< "nFaces entry not found in boundary entry "
                        << boundaryEntryI->GetKeyword().c_str());
          delete boundaryDict;
          return 0;
        }
        svtkTypeInt64 nFaces = nFacesEntry->ToInt();

        // extract name of the current patch for insertion
        const svtkStdString& boundaryNameI = boundaryEntryI->GetKeyword();

        // create BoundaryDict entry
        svtkFoamBoundaryEntry& BoundaryEntryI = this->BoundaryDict[i];
        BoundaryEntryI.NFaces = nFaces;
        BoundaryEntryI.BoundaryName = boundaryNameI;
        const svtkFoamEntry* startFaceEntry = boundaryEntryI->Dictionary().Lookup("startFace");
        if (startFaceEntry == nullptr)
        {
          svtkErrorMacro(<< "startFace entry not found in boundary entry "
                        << boundaryEntryI->GetKeyword().c_str());
          delete boundaryDict;
          return 0;
        }
        BoundaryEntryI.StartFace = startFaceEntry->ToInt();
        const svtkFoamEntry* typeEntry = boundaryEntryI->Dictionary().Lookup("type");
        if (typeEntry == nullptr)
        {
          svtkErrorMacro(<< "type entry not found in boundary entry "
                        << boundaryEntryI->GetKeyword().c_str());
          delete boundaryDict;
          return 0;
        }
        BoundaryEntryI.AllBoundariesStartFace = allBoundariesNextStartFace;
        const svtkStdString typeNameI(typeEntry->ToString());
        // if the basic type of the patch is one of the following the
        // point-filtered values at patches are overridden by patch values
        if (typeNameI == "patch" || typeNameI == "wall")
        {
          BoundaryEntryI.BoundaryType = svtkFoamBoundaryEntry::PHYSICAL;
          allBoundariesNextStartFace += nFaces;
        }
        else if (typeNameI == "processor")
        {
          BoundaryEntryI.BoundaryType = svtkFoamBoundaryEntry::PROCESSOR;
          allBoundariesNextStartFace += nFaces;
        }
        else
        {
          BoundaryEntryI.BoundaryType = svtkFoamBoundaryEntry::GEOMETRICAL;
        }
        BoundaryEntryI.IsActive = false;

        // always hide processor patches for decomposed cases to keep
        // svtkAppendCompositeDataLeaves happy
        if (!this->ProcessorName.empty() &&
          BoundaryEntryI.BoundaryType == svtkFoamBoundaryEntry::PROCESSOR)
        {
          continue;
        }
        const svtkStdString selectionName(this->RegionPrefix() + boundaryNameI);
        if (this->Parent->PatchDataArraySelection->ArrayExists(selectionName.c_str()))
        {
          // Mark boundary if selected for display
          if (this->Parent->GetPatchArrayStatus(selectionName.c_str()))
          {
            BoundaryEntryI.IsActive = true;
          }
        }
        else
        {
          // add patch to list with selection status turned off:
          // the patch is added to list even if its size is zero
          this->Parent->PatchDataArraySelection->DisableArray(selectionName.c_str());
        }
      }

      delete boundaryDict;
    }
  }

  // Add scalars and vectors to metadata
  svtkStdString timePath(this->CurrentTimePath());
  // do not do "RemoveAllArrays()" to accumulate array selections
  // this->CellDataArraySelection->RemoveAllArrays();
  this->VolFieldFiles->Initialize();
  this->PointFieldFiles->Initialize();
  svtkStringArray* cellObjectNames = svtkStringArray::New();
  svtkStringArray* pointObjectNames = svtkStringArray::New();
  this->GetFieldNames(timePath + this->RegionPath(), false, cellObjectNames, pointObjectNames);

  this->LagrangianFieldFiles->Initialize();
  if (listNextTimeStep)
  {
    this->Parent->LagrangianPaths->Initialize();
  }
  svtkStringArray* lagrangianObjectNames = svtkStringArray::New();
  this->LocateLagrangianClouds(lagrangianObjectNames, timePath);

  // if the requested timestep is 0 then we also look at the next
  // timestep to add extra objects that don't exist at timestep 0 into
  // selection lists. Note the ObjectNames array will be recreated in
  // RequestData() so we don't have to worry about duplicated fields.
  if (listNextTimeStep && this->TimeValues->GetNumberOfTuples() >= 2 && this->TimeStep == 0)
  {
    const svtkStdString timePath2(this->TimePath(1));
    this->GetFieldNames(timePath2 + this->RegionPath(), false, cellObjectNames, pointObjectNames);
    // if lagrangian clouds were not found at timestep 0
    if (this->Parent->LagrangianPaths->GetNumberOfTuples() == 0)
    {
      this->LocateLagrangianClouds(lagrangianObjectNames, timePath2);
    }
  }

  // sort array names
  this->SortFieldFiles(cellSelectionNames, this->VolFieldFiles, cellObjectNames);
  this->SortFieldFiles(pointSelectionNames, this->PointFieldFiles, pointObjectNames);
  this->SortFieldFiles(lagrangianSelectionNames, this->LagrangianFieldFiles, lagrangianObjectNames);

  return 1;
}

//-----------------------------------------------------------------------------
// list time directories according to controlDict
bool svtkOpenFOAMReaderPrivate::ListTimeDirectoriesByControlDict(svtkFoamDict* dictPtr)
{
  svtkFoamDict& dict = *dictPtr;

  const svtkFoamEntry* startTimeEntry = dict.Lookup("startTime");
  if (startTimeEntry == nullptr)
  {
    svtkErrorMacro(<< "startTime entry not found in controlDict");
    return false;
  }
  // using double to precisely handle time values
  const double startTime = startTimeEntry->ToDouble();

  const svtkFoamEntry* endTimeEntry = dict.Lookup("endTime");
  if (endTimeEntry == nullptr)
  {
    svtkErrorMacro(<< "endTime entry not found in controlDict");
    return false;
  }
  const double endTime = endTimeEntry->ToDouble();

  const svtkFoamEntry* deltaTEntry = dict.Lookup("deltaT");
  if (deltaTEntry == nullptr)
  {
    svtkErrorMacro(<< "deltaT entry not found in controlDict");
    return false;
  }
  const double deltaT = deltaTEntry->ToDouble();

  const svtkFoamEntry* writeIntervalEntry = dict.Lookup("writeInterval");
  if (writeIntervalEntry == nullptr)
  {
    svtkErrorMacro(<< "writeInterval entry not found in controlDict");
    return false;
  }
  const double writeInterval = writeIntervalEntry->ToDouble();

  const svtkFoamEntry* timeFormatEntry = dict.Lookup("timeFormat");
  if (timeFormatEntry == nullptr)
  {
    svtkErrorMacro(<< "timeFormat entry not found in controlDict");
    return false;
  }
  const svtkStdString timeFormat(timeFormatEntry->ToString());

  const svtkFoamEntry* timePrecisionEntry = dict.Lookup("timePrecision");
  svtkTypeInt64 timePrecision // default is 6
    = (timePrecisionEntry != nullptr ? timePrecisionEntry->ToInt() : 6);

  // calculate the time step increment based on type of run
  const svtkFoamEntry* writeControlEntry = dict.Lookup("writeControl");
  if (writeControlEntry == nullptr)
  {
    svtkErrorMacro(<< "writeControl entry not found in controlDict");
    return false;
  }
  const svtkStdString writeControl(writeControlEntry->ToString());
  double timeStepIncrement;
  if (writeControl == "timeStep")
  {
    timeStepIncrement = writeInterval * deltaT;
  }
  else if (writeControl == "runTime" || writeControl == "adjustableRunTime")
  {
    timeStepIncrement = writeInterval;
  }
  else
  {
    svtkErrorMacro(<< "Time step can't be determined because writeControl is"
                     " set to "
                  << writeControl.c_str());
    return false;
  }

  // calculate how many timesteps there should be
  const double tempResult = (endTime - startTime) / timeStepIncrement;
  // +0.5 to round up
  const int tempNumTimeSteps = static_cast<int>(tempResult + 0.5) + 1;

  // make sure time step dir exists
  svtkDirectory* test = svtkDirectory::New();
  this->TimeValues->Initialize();
  this->TimeNames->Initialize();

  // determine time name based on Foam::Time::timeName()
  // cf. src/OpenFOAM/db/Time/Time.C
  std::ostringstream parser;
#ifdef _MSC_VER
  bool correctExponent = true;
#endif
  if (timeFormat == "general")
  {
    parser.setf(std::ios_base::fmtflags(0), std::ios_base::floatfield);
  }
  else if (timeFormat == "fixed")
  {
    parser.setf(std::ios_base::fmtflags(std::ios_base::fixed), std::ios_base::floatfield);
#ifdef _MSC_VER
    correctExponent = false;
#endif
  }
  else if (timeFormat == "scientific")
  {
    parser.setf(std::ios_base::fmtflags(std::ios_base::scientific), std::ios_base::floatfield);
  }
  else
  {
    svtkWarningMacro("Warning: unsupported time format. Assuming general.");
    parser.setf(std::ios_base::fmtflags(0), std::ios_base::floatfield);
  }
  parser.precision(timePrecision);

  for (int i = 0; i < tempNumTimeSteps; i++)
  {
    parser.str("");
    const double tempStep = i * timeStepIncrement + startTime;
    parser << tempStep; // stringstream doesn't require ends
#ifdef _MSC_VER
    // workaround for format difference in MSVC++:
    // remove an extra 0 from exponent
    if (correctExponent)
    {
      svtkStdString tempStr(parser.str());
      svtkStdString::size_type pos = tempStr.find('e');
      if (pos != svtkStdString::npos && tempStr.length() >= pos + 3 && tempStr[pos + 2] == '0')
      {
        tempStr.erase(pos + 2, 1);
        parser.str(tempStr);
      }
    }
#endif
    // Add the time steps that actually exist to steps
    // allows the run to be stopped short of controlDict spec
    // allows for removal of timesteps
    if (test->Open((this->CasePath + parser.str()).c_str()))
    {
      this->TimeValues->InsertNextValue(tempStep);
      this->TimeNames->InsertNextValue(parser.str());
    }
    // necessary for reading the case/0 directory whatever the timeFormat is
    // based on Foam::Time::operator++() cf. src/OpenFOAM/db/Time/Time.C
    else if ((fabs(tempStep) < 1.0e-14L) // 10*SMALL
      && test->Open((this->CasePath + svtkStdString("0")).c_str()))
    {
      this->TimeValues->InsertNextValue(tempStep);
      this->TimeNames->InsertNextValue(svtkStdString("0"));
    }
  }
  test->Delete();
  this->TimeValues->Squeeze();
  this->TimeNames->Squeeze();

  if (this->TimeValues->GetNumberOfTuples() == 0)
  {
    // set the number of timesteps to 1 if the constant subdirectory exists
    test = svtkDirectory::New();
    if (test->Open((this->CasePath + "constant").c_str()))
    {
      parser.str("");
      parser << startTime;
      this->TimeValues->InsertNextValue(startTime);
      this->TimeValues->Squeeze();
      this->TimeNames->InsertNextValue(parser.str());
      this->TimeNames->Squeeze();
    }
    test->Delete();
  }
  return true;
}

//-----------------------------------------------------------------------------
// list time directories by searching all valid time instances in a
// case directory
bool svtkOpenFOAMReaderPrivate::ListTimeDirectoriesByInstances()
{
  // open the case directory
  svtkDirectory* test = svtkDirectory::New();
  if (!test->Open(this->CasePath.c_str()))
  {
    test->Delete();
    svtkErrorMacro(<< "Can't open directory " << this->CasePath.c_str());
    return false;
  }

  const bool ignore0Dir = this->Parent->GetSkipZeroTime();

  // search all the directories in the case directory and detect
  // directories with names convertible to numbers
  this->TimeValues->Initialize();
  this->TimeNames->Initialize();
  svtkIdType nFiles = test->GetNumberOfFiles();
  for (svtkIdType i = 0; i < nFiles; i++)
  {
    const svtkStdString dir = test->GetFile(i);
    int isTimeDir = test->FileIsDirectory(dir.c_str());

    // optionally ignore 0/ directory
    if (ignore0Dir && dir == "0")
    {
      isTimeDir = false;
    }

    // check if the name is convertible to a number
    for (size_t j = 0; j < dir.length() && isTimeDir; ++j)
    {
      const char c = dir[j];
      isTimeDir = (isdigit(c) || c == '+' || c == '-' || c == '.' || c == 'e' || c == 'E');
    }
    if (!isTimeDir)
    {
      continue;
    }

    // convert to a number
    char* endptr;
    double timeValue = strtod(dir.c_str(), &endptr);
    // check if the value really was converted to a number
    if (timeValue == 0.0 && endptr == dir.c_str())
    {
      continue;
    }

    // add to the instance list
    this->TimeValues->InsertNextValue(timeValue);
    this->TimeNames->InsertNextValue(dir);
  }
  test->Delete();

  this->TimeValues->Squeeze();
  this->TimeNames->Squeeze();

  if (this->TimeValues->GetNumberOfTuples() > 1)
  {
    // sort the detected time directories
    svtkSortDataArray::Sort(this->TimeValues, this->TimeNames);

    // if there are duplicated timeValues found, remove duplicates
    // (e.g. "0" and "0.000")
    for (svtkIdType timeI = 1; timeI < this->TimeValues->GetNumberOfTuples(); timeI++)
    {
      // compare by exact match
      if (this->TimeValues->GetValue(timeI - 1) == this->TimeValues->GetValue(timeI))
      {
        svtkWarningMacro(<< "Different time directories with the same time value "
                        << this->TimeNames->GetValue(timeI - 1).c_str() << " and "
                        << this->TimeNames->GetValue(timeI).c_str() << " found. "
                        << this->TimeNames->GetValue(timeI).c_str() << " will be ignored.");
        this->TimeValues->RemoveTuple(timeI);
        // svtkStringArray does not have RemoveTuple()
        for (svtkIdType timeJ = timeI + 1; timeJ < this->TimeNames->GetNumberOfTuples(); timeJ++)
        {
          this->TimeNames->SetValue(timeJ - 1, this->TimeNames->GetValue(timeJ));
        }
        this->TimeNames->Resize(this->TimeNames->GetNumberOfTuples() - 1);
      }
    }
  }

  if (this->TimeValues->GetNumberOfTuples() == 0)
  {
    // set the number of timesteps to 1 if the constant subdirectory exists
    test = svtkDirectory::New();
    if (test->Open((this->CasePath + "constant").c_str()))
    {
      this->TimeValues->InsertNextValue(0.0);
      this->TimeValues->Squeeze();
      this->TimeNames->InsertNextValue("constant");
      this->TimeNames->Squeeze();
    }
    test->Delete();
  }

  return true;
}

//-----------------------------------------------------------------------------
// gather the necessary information to create a path to the data
bool svtkOpenFOAMReaderPrivate::MakeInformationVector(const svtkStdString& casePath,
  const svtkStdString& controlDictPath, const svtkStdString& procName, svtkOpenFOAMReader* parent)
{
  this->CasePath = casePath;
  this->ProcessorName = procName;
  this->Parent = parent;

  // list timesteps (skip parsing controlDict entirely if
  // ListTimeStepsByControlDict is false)
  bool ret = false; // tentatively set to false to suppress warning by older compilers

  int listByControlDict = this->Parent->GetListTimeStepsByControlDict();
  if (listByControlDict)
  {
    svtkFoamIOobject io(this->CasePath, this->Parent);

    // open and check if controlDict is readable
    if (!io.Open(controlDictPath))
    {
      svtkErrorMacro(<< "Error opening " << io.GetFileName().c_str() << ": "
                    << io.GetError().c_str());
      return false;
    }
    svtkFoamDict dict;
    if (!dict.Read(io))
    {
      svtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of "
                    << io.GetFileName().c_str() << ": " << io.GetError().c_str());
      return false;
    }
    if (dict.GetType() != svtkFoamToken::DICTIONARY)
    {
      svtkErrorMacro(<< "The file type of " << io.GetFileName().c_str() << " is not a dictionary");
      return false;
    }

    const svtkFoamEntry* writeControlEntry = dict.Lookup("writeControl");
    if (writeControlEntry == nullptr)
    {
      svtkErrorMacro(<< "writeControl entry not found in " << io.GetFileName().c_str());
      return false;
    }
    const svtkStdString writeControl(writeControlEntry->ToString());

    // empty if not found
    const svtkFoamEntry* adjustTimeStepEntry = dict.Lookup("adjustTimeStep");
    const svtkStdString adjustTimeStep =
      adjustTimeStepEntry == nullptr ? svtkStdString() : adjustTimeStepEntry->ToString();

    // list time directories according to controlDict if (adjustTimeStep
    // writeControl) == (off, timeStep) or (on, adjustableRunTime); list
    // by time instances in the case directory otherwise (different behavior
    // from paraFoam)
    // valid switching words cf. src/OpenFOAM/db/Switch/Switch.C
    if ((((adjustTimeStep == "off" || adjustTimeStep == "no" || adjustTimeStep == "n" ||
            adjustTimeStep == "false" || adjustTimeStep.empty()) &&
           writeControl == "timeStep") ||
          ((adjustTimeStep == "on" || adjustTimeStep == "yes" || adjustTimeStep == "y" ||
             adjustTimeStep == "true") &&
            writeControl == "adjustableRunTime")))
    {
      ret = this->ListTimeDirectoriesByControlDict(&dict);
    }
    else
    {
      // cannot list by controlDict, fall through to below
      listByControlDict = false;
    }
  }

  if (!listByControlDict)
  {
    ret = this->ListTimeDirectoriesByInstances();
  }

  if (!ret)
  {
    return ret;
  }

  // does not seem to be required even if number of timesteps reduced
  // upon refresh since ParaView rewinds TimeStep to 0, but for precaution
  if (this->TimeValues->GetNumberOfTuples() > 0)
  {
    if (this->TimeStep >= this->TimeValues->GetNumberOfTuples())
    {
      this->SetTimeStep(static_cast<int>(this->TimeValues->GetNumberOfTuples() - 1));
    }
  }
  else
  {
    this->SetTimeStep(0);
  }

  this->PopulatePolyMeshDirArrays();
  return ret;
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReaderPrivate::AppendMeshDirToArray(
  svtkStringArray* polyMeshDir, const svtkStdString& path, const int timeI)
{
  svtkFoamIOobject io(this->CasePath, this->Parent);

  if (io.Open(path) || io.Open(path + ".gz"))
  {
    io.Close();
    // set points/faces location to current timesteps value
    polyMeshDir->SetValue(timeI, this->TimeNames->GetValue(timeI));
  }
  else
  {
    if (timeI != 0)
    {
      // set points/faces location to previous timesteps value
      polyMeshDir->SetValue(timeI, polyMeshDir->GetValue(timeI - 1));
    }
    else
    {
      // set points/faces to constant
      polyMeshDir->SetValue(timeI, "constant");
    }
  }
}

//-----------------------------------------------------------------------------
// create a Lookup Table containing the location of the points
// and faces files for each time steps mesh
void svtkOpenFOAMReaderPrivate::PopulatePolyMeshDirArrays()
{
  // initialize size to number of timesteps
  svtkIdType nSteps = this->TimeValues->GetNumberOfTuples();
  this->PolyMeshPointsDir->SetNumberOfValues(nSteps);
  this->PolyMeshFacesDir->SetNumberOfValues(nSteps);

  // loop through each timestep
  for (int i = 0; i < static_cast<int>(nSteps); i++)
  {
    // create the path to the timestep
    svtkStdString polyMeshPath = this->TimeRegionPath(i) + "/polyMesh/";
    AppendMeshDirToArray(this->PolyMeshPointsDir, polyMeshPath + "points", i);
    AppendMeshDirToArray(this->PolyMeshFacesDir, polyMeshPath + "faces", i);
  }
}

//-----------------------------------------------------------------------------
// read the points file into a svtkFloatArray
svtkFloatArray* svtkOpenFOAMReaderPrivate::ReadPointsFile()
{
  // path to points file
  const svtkStdString pointPath =
    this->CurrentTimeRegionMeshPath(this->PolyMeshPointsDir) + "points";

  svtkFoamIOobject io(this->CasePath, this->Parent);
  if (!(io.Open(pointPath) || io.Open(pointPath + ".gz")))
  {
    svtkErrorMacro(<< "Error opening " << io.GetFileName().c_str() << ": " << io.GetError().c_str());
    return nullptr;
  }

  svtkFloatArray* pointArray = nullptr;

  try
  {
    svtkFoamEntryValue dict(nullptr);

    if (io.GetUse64BitFloats())
    {
      dict.ReadNonuniformList<svtkFoamToken::VECTORLIST,
        svtkFoamEntryValue::vectorListTraits<svtkFloatArray, double, 3, false> >(io);
    }
    else
    {
      dict.ReadNonuniformList<svtkFoamToken::VECTORLIST,
        svtkFoamEntryValue::vectorListTraits<svtkFloatArray, float, 3, false> >(io);
    }

    pointArray = static_cast<svtkFloatArray*>(dict.Ptr());
  }
  catch (svtkFoamError& e)
  { // Something is horribly wrong.
    svtkErrorMacro("Mesh points data are neither 32 nor 64 bit, or some other "
                  "parse error occurred while reading points. Failed at line "
      << io.GetLineNumber() << " of " << io.GetFileName().c_str() << ": " << e.c_str());
    return nullptr;
  }

  assert(pointArray);

  // set the number of points
  this->NumPoints = pointArray->GetNumberOfTuples();

  return pointArray;
}

//-----------------------------------------------------------------------------
// read the faces into a svtkFoamLabelVectorVector
svtkFoamLabelVectorVector* svtkOpenFOAMReaderPrivate::ReadFacesFile(const svtkStdString& facePathIn)
{
  const svtkStdString facePath(facePathIn + "faces");

  svtkFoamIOobject io(this->CasePath, this->Parent);
  if (!(io.Open(facePath) || io.Open(facePath + ".gz")))
  {
    svtkErrorMacro(<< "Error opening " << io.GetFileName().c_str() << ": " << io.GetError().c_str()
                  << ". If you are trying to read a parallel "
                     "decomposed case, set Case Type to Decomposed Case.");
    return nullptr;
  }

  svtkFoamEntryValue dict(nullptr);
  dict.SetLabelType(this->Parent->Use64BitLabels ? svtkFoamToken::INT64 : svtkFoamToken::INT32);
  try
  {
    if (io.GetClassName() == "faceCompactList")
    {
      dict.ReadCompactIOLabelList(io);
    }
    else
    {
      dict.ReadLabelListList(io);
    }
  }
  catch (svtkFoamError& e)
  {
    svtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of "
                  << io.GetFileName().c_str() << ": " << e.c_str());
    return nullptr;
  }
  return static_cast<svtkFoamLabelVectorVector*>(dict.Ptr());
}

//-----------------------------------------------------------------------------
// read the owner and neighbor file and create cellFaces
svtkFoamLabelVectorVector* svtkOpenFOAMReaderPrivate::ReadOwnerNeighborFiles(
  const svtkStdString& ownerNeighborPath, svtkFoamLabelVectorVector* facePoints)
{
  bool use64BitLabels = this->Parent->Use64BitLabels;

  svtkFoamIOobject io(this->CasePath, this->Parent);
  svtkStdString ownerPath(ownerNeighborPath + "owner");
  if (io.Open(ownerPath) || io.Open(ownerPath + ".gz"))
  {
    svtkFoamEntryValue ownerDict(nullptr);
    ownerDict.SetLabelType(use64BitLabels ? svtkFoamToken::INT64 : svtkFoamToken::INT32);
    try
    {
      if (use64BitLabels)
      {
        ownerDict.ReadNonuniformList<svtkFoamEntryValue::LABELLIST,
          svtkFoamEntryValue::listTraits<svtkTypeInt64Array, svtkTypeInt64> >(io);
      }
      else
      {
        ownerDict.ReadNonuniformList<svtkFoamEntryValue::LABELLIST,
          svtkFoamEntryValue::listTraits<svtkTypeInt32Array, svtkTypeInt32> >(io);
      }
    }
    catch (svtkFoamError& e)
    {
      svtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of "
                    << io.GetFileName().c_str() << ": " << e.c_str());
      return nullptr;
    }

    io.Close();

    const svtkStdString neighborPath(ownerNeighborPath + "neighbour");
    if (!(io.Open(neighborPath) || io.Open(neighborPath + ".gz")))
    {
      svtkErrorMacro(<< "Error opening " << io.GetFileName().c_str() << ": "
                    << io.GetError().c_str());
      return nullptr;
    }

    svtkFoamEntryValue neighborDict(nullptr);
    neighborDict.SetLabelType(use64BitLabels ? svtkFoamToken::INT64 : svtkFoamToken::INT32);
    try
    {
      if (use64BitLabels)
      {
        neighborDict.ReadNonuniformList<svtkFoamEntryValue::LABELLIST,
          svtkFoamEntryValue::listTraits<svtkTypeInt64Array, svtkTypeInt64> >(io);
      }
      else
      {
        neighborDict.ReadNonuniformList<svtkFoamEntryValue::LABELLIST,
          svtkFoamEntryValue::listTraits<svtkTypeInt32Array, svtkTypeInt32> >(io);
      }
    }
    catch (svtkFoamError& e)
    {
      svtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of "
                    << io.GetFileName().c_str() << ": " << e.c_str());
      return nullptr;
    }

    this->FaceOwner = static_cast<svtkDataArray*>(ownerDict.Ptr());
    svtkDataArray& faceOwner = *this->FaceOwner;
    svtkDataArray& faceNeighbor = neighborDict.LabelList();

    const svtkIdType nFaces = faceOwner.GetNumberOfTuples();
    const svtkIdType nNeiFaces = faceNeighbor.GetNumberOfTuples();

    if (nFaces < nNeiFaces)
    {
      svtkErrorMacro(<< "Numbers of owner faces " << nFaces
                    << " must be equal or larger than number of neighbor faces " << nNeiFaces);
      return nullptr;
    }

    if (nFaces != facePoints->GetNumberOfElements())
    {
      svtkWarningMacro(<< "Numbers of faces in faces " << facePoints->GetNumberOfElements()
                      << " and owner " << nFaces << " does not match");
      return nullptr;
    }

    // add the face numbers to the correct cell cf. Terry's code and
    // src/OpenFOAM/meshes/primitiveMesh/primitiveMeshCells.C
    // find the number of cells
    svtkTypeInt64 nCells = -1;
    for (int faceI = 0; faceI < nNeiFaces; faceI++)
    {
      const svtkTypeInt64 ownerCell = GetLabelValue(&faceOwner, faceI, use64BitLabels);
      if (nCells < ownerCell) // max(nCells, faceOwner[i])
      {
        nCells = ownerCell;
      }

      // we do need to take neighbor faces into account since all the
      // surrounding faces of a cell can be neighbors for a valid mesh
      const svtkTypeInt64 neighborCell = GetLabelValue(&faceNeighbor, faceI, use64BitLabels);
      if (nCells < neighborCell) // max(nCells, faceNeighbor[i])
      {
        nCells = neighborCell;
      }
    }

    for (svtkIdType faceI = nNeiFaces; faceI < nFaces; faceI++)
    {
      const svtkTypeInt64 ownerCell = GetLabelValue(&faceOwner, faceI, use64BitLabels);
      if (nCells < ownerCell) // max(nCells, faceOwner[i])
      {
        nCells = ownerCell;
      }
    }
    nCells++;

    if (nCells == 0)
    {
      svtkWarningMacro(<< "The mesh contains no cells");
    }

    // set the number of cells
    this->NumCells = static_cast<svtkIdType>(nCells);

    // create cellFaces with the length of the body undetermined
    svtkFoamLabelVectorVector* cells;
    if (use64BitLabels)
    {
      cells = new svtkFoamLabel64VectorVector(nCells, 1);
    }
    else
    {
      cells = new svtkFoamLabel32VectorVector(nCells, 1);
    }

    // count number of faces for each cell
    svtkDataArray* cellIndices = cells->GetIndices();
    for (int cellI = 0; cellI <= nCells; cellI++)
    {
      SetLabelValue(cellIndices, cellI, 0, use64BitLabels);
    }
    svtkIdType nTotalCellFaces = 0;
    svtkIdType cellIndexOffset = 1; // offset +1
    for (int faceI = 0; faceI < nNeiFaces; faceI++)
    {
      const svtkTypeInt64 ownerCell = GetLabelValue(&faceOwner, faceI, use64BitLabels);
      // simpleFoam/pitzDaily3Blocks has faces with owner cell number -1
      if (ownerCell >= 0)
      {
        IncrementLabelValue(cellIndices, cellIndexOffset + ownerCell, use64BitLabels);
        nTotalCellFaces++;
      }

      const svtkTypeInt64 neighborCell = GetLabelValue(&faceNeighbor, faceI, use64BitLabels);
      if (neighborCell >= 0)
      {
        IncrementLabelValue(cellIndices, cellIndexOffset + neighborCell, use64BitLabels);
        nTotalCellFaces++;
      }
    }

    for (svtkIdType faceI = nNeiFaces; faceI < nFaces; faceI++)
    {
      const svtkTypeInt64 ownerCell = GetLabelValue(&faceOwner, faceI, use64BitLabels);
      if (ownerCell >= 0)
      {
        IncrementLabelValue(cellIndices, cellIndexOffset + ownerCell, use64BitLabels);
        nTotalCellFaces++;
      }
    }
    cellIndexOffset = 0; // revert offset +1

    // allocate cellFaces. To reduce the numbers of new/delete operations we
    // allocate memory space for all faces linearly
    cells->ResizeBody(nTotalCellFaces);

    // accumulate the number of cellFaces to create cellFaces indices
    // and copy them to a temporary array
    svtkDataArray* tmpFaceIndices;
    if (use64BitLabels)
    {
      tmpFaceIndices = svtkTypeInt64Array::New();
    }
    else
    {
      tmpFaceIndices = svtkTypeInt32Array::New();
    }
    tmpFaceIndices->SetNumberOfValues(nCells + 1);
    SetLabelValue(tmpFaceIndices, 0, 0, use64BitLabels);
    for (svtkIdType cellI = 1; cellI <= nCells; cellI++)
    {
      svtkTypeInt64 curCellSize = GetLabelValue(cellIndices, cellI, use64BitLabels);
      svtkTypeInt64 lastCellSize = GetLabelValue(cellIndices, cellI - 1, use64BitLabels);
      svtkTypeInt64 curCellOffset = lastCellSize + curCellSize;
      SetLabelValue(cellIndices, cellI, curCellOffset, use64BitLabels);
      SetLabelValue(tmpFaceIndices, cellI, curCellOffset, use64BitLabels);
    }

    // add face numbers to cell-faces list
    svtkDataArray* cellFacesList = cells->GetBody();
    for (svtkIdType faceI = 0; faceI < nNeiFaces; faceI++)
    {
      // must be a signed int
      const svtkTypeInt64 ownerCell = GetLabelValue(&faceOwner, faceI, use64BitLabels);

      // simpleFoam/pitzDaily3Blocks has faces with owner cell number -1
      if (ownerCell >= 0)
      {
        svtkTypeInt64 tempFace = GetLabelValue(tmpFaceIndices, ownerCell, use64BitLabels);
        SetLabelValue(cellFacesList, tempFace, faceI, use64BitLabels);
        ++tempFace;
        SetLabelValue(tmpFaceIndices, ownerCell, tempFace, use64BitLabels);
      }

      svtkTypeInt64 neighborCell = GetLabelValue(&faceNeighbor, faceI, use64BitLabels);
      if (neighborCell >= 0)
      {
        svtkTypeInt64 tempFace = GetLabelValue(tmpFaceIndices, neighborCell, use64BitLabels);
        SetLabelValue(cellFacesList, tempFace, faceI, use64BitLabels);
        ++tempFace;
        SetLabelValue(tmpFaceIndices, neighborCell, tempFace, use64BitLabels);
      }
    }

    for (svtkIdType faceI = nNeiFaces; faceI < nFaces; faceI++)
    {
      // must be a signed int
      svtkTypeInt64 ownerCell = GetLabelValue(&faceOwner, faceI, use64BitLabels);

      // simpleFoam/pitzDaily3Blocks has faces with owner cell number -1
      if (ownerCell >= 0)
      {
        svtkTypeInt64 tempFace = GetLabelValue(tmpFaceIndices, ownerCell, use64BitLabels);
        SetLabelValue(cellFacesList, tempFace, faceI, use64BitLabels);
        ++tempFace;
        SetLabelValue(tmpFaceIndices, ownerCell, tempFace, use64BitLabels);
      }
    }
    tmpFaceIndices->Delete();

    return cells;
  }
  else // if owner does not exist look for cells
  {
    svtkStdString cellsPath(ownerNeighborPath + "cells");
    if (!(io.Open(cellsPath) || io.Open(cellsPath + ".gz")))
    {
      svtkErrorMacro(<< "Error opening " << io.GetFileName().c_str() << ": "
                    << io.GetError().c_str());
      return nullptr;
    }

    svtkFoamEntryValue cellsDict(nullptr);
    cellsDict.SetLabelType(use64BitLabels ? svtkFoamToken::INT64 : svtkFoamToken::INT32);
    try
    {
      cellsDict.ReadLabelListList(io);
    }
    catch (svtkFoamError& e)
    {
      svtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of "
                    << io.GetFileName().c_str() << ": " << e.c_str());
      return nullptr;
    }

    svtkFoamLabelVectorVector* cells = static_cast<svtkFoamLabelVectorVector*>(cellsDict.Ptr());
    this->NumCells = cells->GetNumberOfElements();
    svtkIdType nFaces = facePoints->GetNumberOfElements();

    // create face owner list
    if (use64BitLabels)
    {
      this->FaceOwner = svtkTypeInt64Array::New();
    }
    else
    {
      this->FaceOwner = svtkTypeInt32Array::New();
    }

    this->FaceOwner->SetNumberOfTuples(nFaces);
    this->FaceOwner->FillComponent(0, -1);

    svtkFoamLabelVectorVector::CellType cellFaces;
    for (svtkIdType cellI = 0; cellI < this->NumCells; cellI++)
    {
      cells->GetCell(cellI, cellFaces);
      for (size_t faceI = 0; faceI < cellFaces.size(); faceI++)
      {
        svtkTypeInt64 f = cellFaces[faceI];
        if (f < 0 || f >= nFaces) // make sure the face number is valid
        {
          svtkErrorMacro("Face number " << f << " in cell " << cellI
                                       << " exceeds the number of faces " << nFaces);
          this->FaceOwner->Delete();
          this->FaceOwner = nullptr;
          delete cells;
          return nullptr;
        }

        svtkTypeInt64 owner = GetLabelValue(this->FaceOwner, f, use64BitLabels);
        if (owner == -1 || owner > cellI)
        {
          SetLabelValue(this->FaceOwner, f, cellI, use64BitLabels);
        }
      }
    }

    // check for unused faces
    for (int faceI = 0; faceI < nFaces; faceI++)
    {
      svtkTypeInt64 f = GetLabelValue(this->FaceOwner, faceI, use64BitLabels);
      if (f == -1)
      {
        svtkErrorMacro(<< "Face " << faceI << " is not used");
        this->FaceOwner->Delete();
        this->FaceOwner = nullptr;
        delete cells;
        return nullptr;
      }
    }
    return cells;
  }
}

//-----------------------------------------------------------------------------
bool svtkOpenFOAMReaderPrivate::CheckFacePoints(svtkFoamLabelVectorVector* facePoints)
{
  svtkIdType nFaces = facePoints->GetNumberOfElements();

  svtkFoamLabelVectorVector::CellType face;
  for (svtkIdType faceI = 0; faceI < nFaces; faceI++)
  {
    facePoints->GetCell(faceI, face);
    if (face.size() < 3)
    {
      svtkErrorMacro(<< "Face " << faceI << " has only " << face.size()
                    << " points which is not enough to constitute a face"
                       " (a face must have at least 3 points)");
      return false;
    }

    for (size_t pointI = 0; pointI < face.size(); pointI++)
    {
      svtkTypeInt64 p = face[pointI];
      if (p < 0 || p >= this->NumPoints)
      {
        svtkErrorMacro(<< "The point number " << p << " at face number " << faceI
                      << " is out of range for " << this->NumPoints << " points");
        return false;
      }
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
// determine cell shape and insert the cell into the mesh
// hexahedron, prism, pyramid, tetrahedron and decompose polyhedron
void svtkOpenFOAMReaderPrivate::InsertCellsToGrid(svtkUnstructuredGrid* internalMesh,
  const svtkFoamLabelVectorVector* cellsFaces, const svtkFoamLabelVectorVector* facesPoints,
  svtkFloatArray* pointArray, svtkIdTypeArray* additionalCells, svtkDataArray* cellList)
{
  bool use64BitLabels = this->Parent->Use64BitLabels;

  svtkIdType maxNPoints = 256; // assume max number of points per cell
  svtkIdList* cellPoints = svtkIdList::New();
  cellPoints->SetNumberOfIds(maxNPoints);
  // assume max number of nPoints per face + points per cell
  svtkIdType maxNPolyPoints = 1024;
  svtkIdList* polyPoints = svtkIdList::New();
  polyPoints->SetNumberOfIds(maxNPolyPoints);

  svtkIdType nCells = (cellList == nullptr ? this->NumCells : cellList->GetNumberOfTuples());
  int nAdditionalPoints = 0;
  this->NumTotalAdditionalCells = 0;

  // alias
  const svtkFoamLabelVectorVector& facePoints = *facesPoints;

  svtkFoamLabelVectorVector::CellType cellFaces;

  svtkSmartPointer<svtkIdTypeArray> arrayId;
  if (cellList)
  {
    // create array holding cell id only on zone mesh
    arrayId = svtkSmartPointer<svtkIdTypeArray>::New();
    arrayId->SetName("CellId");
    arrayId->SetNumberOfTuples(nCells);
    internalMesh->GetCellData()->AddArray(arrayId);
  }

  for (svtkIdType cellI = 0; cellI < nCells; cellI++)
  {
    svtkIdType cellId;
    if (cellList == nullptr)
    {
      cellId = cellI;
    }
    else
    {
      cellId = GetLabelValue(cellList, cellI, use64BitLabels);
      if (cellId >= this->NumCells)
      {
        svtkWarningMacro(<< "cellLabels id " << cellId << " exceeds the number of cells " << nCells
                        << ". Inserting an empty cell.");
        internalMesh->InsertNextCell(SVTK_EMPTY_CELL, 0, cellPoints->GetPointer(0));
        continue;
      }
      arrayId->SetValue(cellI, cellId);
    }

    cellsFaces->GetCell(cellId, cellFaces);

    // determine type of the cell
    // cf. src/OpenFOAM/meshes/meshShapes/cellMatcher/{hex|prism|pyr|tet}-
    // Matcher.C
    int cellType = SVTK_POLYHEDRON; // Fallback value
    if (cellFaces.size() == 6)
    {
      size_t j = 0;
      for (; j < cellFaces.size(); j++)
      {
        if (facePoints.GetSize(cellFaces[j]) != 4)
        {
          break;
        }
      }
      if (j == cellFaces.size())
      {
        cellType = SVTK_HEXAHEDRON;
      }
    }
    else if (cellFaces.size() == 5)
    {
      int nTris = 0, nQuads = 0;
      for (size_t j = 0; j < cellFaces.size(); j++)
      {
        svtkIdType nPoints = facePoints.GetSize(cellFaces[j]);
        if (nPoints == 3)
        {
          nTris++;
        }
        else if (nPoints == 4)
        {
          nQuads++;
        }
        else
        {
          break;
        }
      }
      if (nTris == 2 && nQuads == 3)
      {
        cellType = SVTK_WEDGE;
      }
      else if (nTris == 4 && nQuads == 1)
      {
        cellType = SVTK_PYRAMID;
      }
    }
    else if (cellFaces.size() == 4)
    {
      size_t j = 0;
      for (; j < cellFaces.size(); j++)
      {
        if (facePoints.GetSize(cellFaces[j]) != 3)
        {
          break;
        }
      }
      if (j == cellFaces.size())
      {
        cellType = SVTK_TETRA;
      }
    }

    // Not a known (standard) primitive mesh-shape
    if (cellType == SVTK_POLYHEDRON)
    {
      size_t nPoints = 0;
      for (size_t j = 0; j < cellFaces.size(); j++)
      {
        nPoints += facePoints.GetSize(cellFaces[j]);
      }
      if (nPoints == 0)
      {
        cellType = SVTK_EMPTY_CELL;
      }
    }

    // Cell shape constructor based on the one implementd by Terry
    // Jordan, with lots of improvements. Not as elegant as the one in
    // OpenFOAM but it's simple and works reasonably fast.

    // OFhex | svtkHexahedron
    if (cellType == SVTK_HEXAHEDRON)
    {
      // get first face in correct order
      svtkTypeInt64 cellBaseFaceId = cellFaces[0];
      svtkFoamLabelVectorVector::CellType face0Points;
      facePoints.GetCell(cellBaseFaceId, face0Points);

      if (GetLabelValue(this->FaceOwner, cellBaseFaceId, use64BitLabels) == cellId)
      {
        // if it is an owner face flip the points
        for (int j = 0; j < 4; j++)
        {
          cellPoints->SetId(j, face0Points[3 - j]);
        }
      }
      else
      {
        // add base face to cell points
        for (int j = 0; j < 4; j++)
        {
          cellPoints->SetId(j, face0Points[j]);
        }
      }
      svtkIdType baseFacePoint0 = cellPoints->GetId(0);
      svtkIdType baseFacePoint2 = cellPoints->GetId(2);
      svtkTypeInt64 cellOppositeFaceI = -1, pivotPoint = -1;
      svtkTypeInt64 dupPoint = -1;
      svtkFoamLabelVectorVector::CellType faceIPoints;
      for (int faceI = 1; faceI < 5; faceI++) // skip face 0 and 5
      {
        svtkTypeInt64 cellFaceI = cellFaces[faceI];
        facePoints.GetCell(cellFaceI, faceIPoints);
        int foundDup = -1, pointI = 0;
        for (; pointI < 4; pointI++) // each point
        {
          svtkTypeInt64 faceIPointI = faceIPoints[pointI];
          // matching two points in base face is enough to find a
          // duplicated point since neighboring faces share two
          // neighboring points (i. e. an edge)
          if (baseFacePoint0 == faceIPointI)
          {
            foundDup = 0;
            break;
          }
          else if (baseFacePoint2 == faceIPointI)
          {
            foundDup = 2;
            break;
          }
        }
        if (foundDup >= 0)
        {
          // find the pivot point if still haven't
          if (pivotPoint == -1)
          {
            dupPoint = foundDup;

            svtkTypeInt64 faceINextPoint = faceIPoints[(pointI + 1) % 4];

            // if the next point of the faceI-th face matches the
            // previous point of the base face use the previous point
            // of the faceI-th face as the pivot point; or use the
            // next point otherwise
            if (faceINextPoint ==
              (GetLabelValue(this->FaceOwner, cellFaceI, use64BitLabels) == cellId
                  ? cellPoints->GetId(1 + foundDup)
                  : cellPoints->GetId(3 - foundDup)))
            {
              pivotPoint = faceIPoints[(3 + pointI) % 4];
            }
            else
            {
              pivotPoint = faceINextPoint;
            }

            if (cellOppositeFaceI >= 0)
            {
              break;
            }
          }
        }
        else
        {
          // if no duplicated point found, faceI is the opposite face
          cellOppositeFaceI = cellFaceI;

          if (pivotPoint >= 0)
          {
            break;
          }
        }
      }

      // if the opposite face is not found until face 4, face 5 is
      // always the opposite face
      if (cellOppositeFaceI == -1)
      {
        cellOppositeFaceI = cellFaces[5];
      }

      // find the pivot point in opposite face
      svtkFoamLabelVectorVector::CellType oppositeFacePoints;
      facePoints.GetCell(cellOppositeFaceI, oppositeFacePoints);
      int pivotPointI = 0;
      for (; pivotPointI < 4; pivotPointI++)
      {
        if (oppositeFacePoints[pivotPointI] == pivotPoint)
        {
          break;
        }
      }

      // shift the pivot point if the point corresponds to point 2
      // of the base face
      if (dupPoint == 2)
      {
        pivotPointI = (pivotPointI + 2) % 4;
      }
      // copy the face-point list of the opposite face to cell-point list
      int basePointI = 4;
      if (GetLabelValue(this->FaceOwner, cellOppositeFaceI, use64BitLabels) == cellId)
      {
        for (int pointI = pivotPointI; pointI < 4; pointI++)
        {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
        }
        for (int pointI = 0; pointI < pivotPointI; pointI++)
        {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
        }
      }
      else
      {
        for (int pointI = pivotPointI; pointI >= 0; pointI--)
        {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
        }
        for (int pointI = 3; pointI > pivotPointI; pointI--)
        {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
        }
      }

      // create the hex cell and insert it into the mesh
      internalMesh->InsertNextCell(cellType, 8, cellPoints->GetPointer(0));
    }

    // the cell construction is about the same as that of a hex, but
    // the point ordering have to be reversed!!
    else if (cellType == SVTK_WEDGE)
    {
      // find the base face number
      int baseFaceId = 0;
      for (int j = 0; j < 5; j++)
      {
        if (facePoints.GetSize(cellFaces[j]) == 3)
        {
          baseFaceId = j;
          break;
        }
      }

      // get first face in correct order
      svtkTypeInt64 cellBaseFaceId = cellFaces[baseFaceId];
      svtkFoamLabelVectorVector::CellType face0Points;
      facePoints.GetCell(cellBaseFaceId, face0Points);

      if (GetLabelValue(this->FaceOwner, cellBaseFaceId, use64BitLabels) == cellId)
      {
        for (int j = 0; j < 3; j++)
        {
          cellPoints->SetId(j, face0Points[j]);
        }
      }
      else
      {
        // if it is a neighbor face flip the points
        for (int j = 0; j < 3; j++)
        {
          // add base face to cell points
          cellPoints->SetId(j, face0Points[2 - j]);
        }
      }
      svtkIdType baseFacePoint0 = cellPoints->GetId(0);
      svtkIdType baseFacePoint2 = cellPoints->GetId(2);
      svtkTypeInt64 cellOppositeFaceI = -1, pivotPoint = -1;
      bool dupPoint2 = false;
      svtkFoamLabelVectorVector::CellType faceIPoints;
      for (int faceI = 0; faceI < 5; faceI++)
      {
        if (faceI == baseFaceId)
        {
          continue;
        }
        svtkTypeInt64 cellFaceI = cellFaces[faceI];
        if (facePoints.GetSize(cellFaceI) == 3)
        {
          cellOppositeFaceI = cellFaceI;
        }
        // find the pivot point if still haven't
        else if (pivotPoint == -1)
        {
          facePoints.GetCell(cellFaceI, faceIPoints);
          bool found0Dup = false;
          int pointI = 0;
          for (; pointI < 4; pointI++) // each point
          {
            svtkTypeInt64 faceIPointI = faceIPoints[pointI];
            // matching two points in base face is enough to find a
            // duplicated point since neighboring faces share two
            // neighboring points (i. e. an edge)
            if (baseFacePoint0 == faceIPointI)
            {
              found0Dup = true;
              break;
            }
            else if (baseFacePoint2 == faceIPointI)
            {
              break;
            }
          }
          // the matching point must always be found so omit the check
          svtkIdType baseFacePrevPoint, baseFaceNextPoint;
          if (found0Dup)
          {
            baseFacePrevPoint = cellPoints->GetId(2);
            baseFaceNextPoint = cellPoints->GetId(1);
          }
          else
          {
            baseFacePrevPoint = cellPoints->GetId(1);
            baseFaceNextPoint = cellPoints->GetId(0);
            dupPoint2 = true;
          }

          svtkTypeInt64 faceINextPoint = faceIPoints[(pointI + 1) % 4];
          svtkTypeInt64 faceIPrevPoint = faceIPoints[(3 + pointI) % 4];

          // if the next point of the faceI-th face matches the
          // previous point of the base face use the previous point of
          // the faceI-th face as the pivot point; or use the next
          // point otherwise
          svtkTypeInt64 faceOwnerVal = GetLabelValue(this->FaceOwner, cellFaceI, use64BitLabels);
          if (faceINextPoint == (faceOwnerVal == cellId ? baseFacePrevPoint : baseFaceNextPoint))
          {
            pivotPoint = faceIPrevPoint;
          }
          else
          {
            pivotPoint = faceINextPoint;
          }
        }

        // break when both of opposite face and pivot point are found
        if (cellOppositeFaceI >= 0 && pivotPoint >= 0)
        {
          break;
        }
      }

      // find the pivot point in opposite face
      svtkFoamLabelVectorVector::CellType oppositeFacePoints;
      facePoints.GetCell(cellOppositeFaceI, oppositeFacePoints);
      int pivotPointI = 0;
      for (; pivotPointI < 3; pivotPointI++)
      {
        if (oppositeFacePoints[pivotPointI] == pivotPoint)
        {
          break;
        }
      }
      if (pivotPointI != 3)
      {
        // We have found a pivot. We can process cell as a wedge
        svtkTypeInt64 faceOwnerVal =
          GetLabelValue(this->FaceOwner, static_cast<svtkIdType>(cellOppositeFaceI), use64BitLabels);
        if (faceOwnerVal == cellId)
        {
          if (dupPoint2)
          {
            pivotPointI = (pivotPointI + 2) % 3;
          }
          int basePointI = 3;
          for (int pointI = pivotPointI; pointI >= 0; pointI--)
          {
            cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
          for (int pointI = 2; pointI > pivotPointI; pointI--)
          {
            cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
        }
        else
        {
          // shift the pivot point if the point corresponds to point 2
          // of the base face
          if (dupPoint2)
          {
            pivotPointI = (1 + pivotPointI) % 3;
          }
          // copy the face-point list of the opposite face to cell-point list
          int basePointI = 3;
          for (int pointI = pivotPointI; pointI < 3; pointI++)
          {
            cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
          for (int pointI = 0; pointI < pivotPointI; pointI++)
          {
            cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
        }

        // create the wedge cell and insert it into the mesh
        internalMesh->InsertNextCell(cellType, 6, cellPoints->GetPointer(0));
      }
      else
      {
        // We did not find a pivot: this cell was suspected to be a wedge but it
        // is not. Let's process it like a polyhedron instead.
        cellType = SVTK_POLYHEDRON;
      }
    }

    // OFpyramid | svtkPyramid || OFtet | svtkTetrahedron
    else if (cellType == SVTK_PYRAMID || cellType == SVTK_TETRA)
    {
      const svtkIdType nPoints = (cellType == SVTK_PYRAMID ? 5 : 4);
      size_t baseFaceId = 0;
      if (cellType == SVTK_PYRAMID)
      {
        // Find the pyramid base
        for (size_t j = 0; j < cellFaces.size(); j++)
        {
          if (facePoints.GetSize(cellFaces[j]) == 4)
          {
            baseFaceId = j;
            break;
          }
        }
      }

      const svtkTypeInt64 cellBaseFaceId = cellFaces[baseFaceId];
      svtkFoamLabelVectorVector::CellType baseFacePoints;
      facePoints.GetCell(cellBaseFaceId, baseFacePoints);

      // Take any adjacent (non-base) face
      const size_t adjacentFaceId = (baseFaceId ? 0 : 1);
      const svtkTypeInt64 cellAdjacentFaceId = cellFaces[adjacentFaceId];

      svtkFoamLabelVectorVector::CellType adjacentFacePoints;
      facePoints.GetCell(cellAdjacentFaceId, adjacentFacePoints);

      // Find the apex point (non-common to the base)
      // initialize with anything
      // - if the search really fails, we have much bigger problems anyhow
      svtkIdType apexPointI = adjacentFacePoints[0];
      for (size_t ptI = 0; ptI < adjacentFacePoints.size(); ++ptI)
      {
        apexPointI = adjacentFacePoints[ptI];
        bool foundDup = false;
        for (size_t baseI = 0; baseI < baseFacePoints.size(); ++baseI)
        {
          foundDup = (apexPointI == baseFacePoints[baseI]);
          if (foundDup)
          {
            break;
          }
        }
        if (!foundDup)
        {
          break;
        }
      }

      // Add base-face points (in order) to cell points
      if (GetLabelValue(this->FaceOwner, cellBaseFaceId, use64BitLabels) == cellId)
      {
        // if it is an owner face, flip the points (to point inwards)
        for (svtkIdType j = 0; j < static_cast<svtkIdType>(baseFacePoints.size()); ++j)
        {
          cellPoints->SetId(j, baseFacePoints[baseFacePoints.size() - 1 - j]);
        }
      }
      else
      {
        for (svtkIdType j = 0; j < static_cast<svtkIdType>(baseFacePoints.size()); ++j)
        {
          cellPoints->SetId(j, baseFacePoints[j]);
        }
      }

      // ... and add the apex-point
      cellPoints->SetId(nPoints - 1, apexPointI);

      // create the tetra or pyramid cell and insert it into the mesh
      internalMesh->InsertNextCell(cellType, nPoints, cellPoints->GetPointer(0));
    }

    // erroneous cells
    else if (cellType == SVTK_EMPTY_CELL)
    {
      svtkWarningMacro("Warning: No points in cellId " << cellId);
      internalMesh->InsertNextCell(SVTK_EMPTY_CELL, 0, cellPoints->GetPointer(0));
    }

    // OFpolyhedron || svtkConvexPointSet
    if (cellType == SVTK_POLYHEDRON)
    {
      if (additionalCells != nullptr) // decompose into tets and pyramids
      {
        // calculate cell centroid and insert it to point list
        svtkDataArray* polyCellPoints;
        if (use64BitLabels)
        {
          polyCellPoints = svtkTypeInt64Array::New();
        }
        else
        {
          polyCellPoints = svtkTypeInt32Array::New();
        }
        this->AdditionalCellPoints->push_back(polyCellPoints);
        float centroid[3];
        centroid[0] = centroid[1] = centroid[2] = 0.0F;
        for (size_t j = 0; j < cellFaces.size(); j++)
        {
          // remove duplicate points from faces
          svtkTypeInt64 cellFacesJ = cellFaces[j];
          svtkFoamLabelVectorVector::CellType faceJPoints;
          facePoints.GetCell(cellFacesJ, faceJPoints);
          for (size_t k = 0; k < faceJPoints.size(); k++)
          {
            svtkTypeInt64 faceJPointK = faceJPoints[k];
            bool foundDup = false;
            for (svtkIdType l = 0; l < polyCellPoints->GetDataSize(); l++)
            {
              svtkTypeInt64 polyCellPoint = GetLabelValue(polyCellPoints, l, use64BitLabels);
              if (polyCellPoint == faceJPointK)
              {
                foundDup = true;
                break; // look no more
              }
            }
            if (!foundDup)
            {
              AppendLabelValue(polyCellPoints, faceJPointK, use64BitLabels);
              float* pointK = pointArray->GetPointer(3 * faceJPointK);
              centroid[0] += pointK[0];
              centroid[1] += pointK[1];
              centroid[2] += pointK[2];
            }
          }
        }
        polyCellPoints->Squeeze();
        const float weight = 1.0F / static_cast<float>(polyCellPoints->GetDataSize());
        centroid[0] *= weight;
        centroid[1] *= weight;
        centroid[2] *= weight;
        pointArray->InsertNextTuple(centroid);

        // polyhedron decomposition.
        // a tweaked algorithm based on applications/utilities/postProcessing/
        // graphics/PVFoamReader/svtkFoam/svtkFoamAddInternalMesh.C
        bool insertDecomposedCell = true;
        int nAdditionalCells = 0;
        for (size_t j = 0; j < cellFaces.size(); j++)
        {
          svtkTypeInt64 cellFacesJ = cellFaces[j];
          svtkFoamLabelVectorVector::CellType faceJPoints;
          facePoints.GetCell(cellFacesJ, faceJPoints);
          svtkTypeInt64 faceOwnerValue = GetLabelValue(this->FaceOwner, cellFacesJ, use64BitLabels);
          int flipNeighbor = (faceOwnerValue == cellId ? -1 : 1);
          size_t nTris = faceJPoints.size() % 2;

          size_t vertI = 2;

          // shift the start and end of the vertex loop if the
          // triangle of a decomposed face is going to be flat. Far
          // from perfect but better than nothing to avoid flat cells
          // which stops time integration of Stream Tracer especially
          // for split-hex unstructured meshes created by
          // e. g. autoRefineMesh
          if (faceJPoints.size() >= 5 && nTris)
          {
            float *point0, *point1, *point2;
            point0 = pointArray->GetPointer(3 * faceJPoints[faceJPoints.size() - 1]);
            point1 = pointArray->GetPointer(3 * faceJPoints[0]);
            point2 = pointArray->GetPointer(3 * faceJPoints[faceJPoints.size() - 2]);
            float vsizeSqr1 = 0.0F, vsizeSqr2 = 0.0F, dotProduct = 0.0F;
            for (int i = 0; i < 3; i++)
            {
              const float v1 = point1[i] - point0[i], v2 = point2[i] - point0[i];
              vsizeSqr1 += v1 * v1;
              vsizeSqr2 += v2 * v2;
              dotProduct += v1 * v2;
            }
            // compare in squared representation to avoid using sqrt()
            if (dotProduct * (float)fabs(dotProduct) / (vsizeSqr1 * vsizeSqr2) < -1.0F + 1.0e-3F)
            {
              vertI = 1;
            }
          }

          cellPoints->SetId(0,
            faceJPoints[(vertI == 2) ? static_cast<svtkIdType>(0)
                                     : static_cast<svtkIdType>(faceJPoints.size() - 1)]);
          cellPoints->SetId(4, static_cast<svtkIdType>(this->NumPoints + nAdditionalPoints));

          // decompose a face into quads in order (flipping the
          // decomposed face if owner)
          size_t nQuadVerts = faceJPoints.size() - 1 - nTris;
          for (; vertI < nQuadVerts; vertI += 2)
          {
            cellPoints->SetId(1, faceJPoints[vertI - flipNeighbor]);
            cellPoints->SetId(2, faceJPoints[vertI]);
            cellPoints->SetId(3, faceJPoints[vertI + flipNeighbor]);

            // if the decomposed cell is the first one insert it to
            // the original position; or append to the decomposed cell
            // list otherwise
            if (insertDecomposedCell)
            {
              internalMesh->InsertNextCell(SVTK_PYRAMID, 5, cellPoints->GetPointer(0));
              insertDecomposedCell = false;
            }
            else
            {
              nAdditionalCells++;
              additionalCells->InsertNextTypedTuple(cellPoints->GetPointer(0));
            }
          }

          // if the number of vertices is odd there's a triangle
          if (nTris)
          {
            if (flipNeighbor == -1)
            {
              cellPoints->SetId(1, faceJPoints[vertI]);
              cellPoints->SetId(2, faceJPoints[vertI - 1]);
            }
            else
            {
              cellPoints->SetId(1, faceJPoints[vertI - 1]);
              cellPoints->SetId(2, faceJPoints[vertI]);
            }
            cellPoints->SetId(3, static_cast<svtkIdType>(this->NumPoints + nAdditionalPoints));

            if (insertDecomposedCell)
            {
              internalMesh->InsertNextCell(SVTK_TETRA, 4, cellPoints->GetPointer(0));
              insertDecomposedCell = false;
            }
            else
            {
              // set the 5th vertex number to -1 to distinguish a tetra cell
              cellPoints->SetId(4, -1);
              nAdditionalCells++;
              additionalCells->InsertNextTypedTuple(cellPoints->GetPointer(0));
            }
          }
        }

        nAdditionalPoints++;
        this->AdditionalCellIds->InsertNextValue(cellId);
        this->NumAdditionalCells->InsertNextValue(nAdditionalCells);
        this->NumTotalAdditionalCells += nAdditionalCells;
      }
      else // don't decompose; use SVTK_POLYHEDRON
      {
        // get first face
        svtkTypeInt64 cellFaces0 = cellFaces[0];
        svtkFoamLabelVectorVector::CellType baseFacePoints;
        facePoints.GetCell(cellFaces0, baseFacePoints);
        size_t nPoints = baseFacePoints.size();
        size_t nPolyPoints = baseFacePoints.size() + 1;
        if (nPoints > static_cast<size_t>(maxNPoints) ||
          nPolyPoints > static_cast<size_t>(maxNPolyPoints))
        {
          svtkErrorMacro(<< "Too large polyhedron at cellId = " << cellId);
          cellPoints->Delete();
          polyPoints->Delete();
          return;
        }
        polyPoints->SetId(0, static_cast<svtkIdType>(baseFacePoints.size()));
        svtkTypeInt64 faceOwnerValue = GetLabelValue(this->FaceOwner, cellFaces0, use64BitLabels);
        if (faceOwnerValue == cellId)
        {
          // add first face to cell points
          for (size_t j = 0; j < baseFacePoints.size(); j++)
          {
            svtkTypeInt64 pointJ = baseFacePoints[j];
            cellPoints->SetId(static_cast<svtkIdType>(j), static_cast<svtkIdType>(pointJ));
            polyPoints->SetId(static_cast<svtkIdType>(j + 1), static_cast<svtkIdType>(pointJ));
          }
        }
        else
        {
          // if it is a _neighbor_ face flip the points
          for (size_t j = 0; j < baseFacePoints.size(); j++)
          {
            svtkTypeInt64 pointJ = baseFacePoints[baseFacePoints.size() - 1 - j];
            cellPoints->SetId(static_cast<svtkIdType>(j), static_cast<svtkIdType>(pointJ));
            polyPoints->SetId(static_cast<svtkIdType>(j + 1), static_cast<svtkIdType>(pointJ));
          }
        }

        // loop through faces and create a list of all points
        // j = 1 skip baseFace
        for (size_t j = 1; j < cellFaces.size(); j++)
        {
          // remove duplicate points from faces
          svtkTypeInt64 cellFacesJ = cellFaces[j];
          svtkFoamLabelVectorVector::CellType faceJPoints;
          facePoints.GetCell(cellFacesJ, faceJPoints);
          if (nPolyPoints >= static_cast<size_t>(maxNPolyPoints))
          {
            svtkErrorMacro(<< "Too large polyhedron at cellId = " << cellId);
            cellPoints->Delete();
            polyPoints->Delete();
            return;
          }
          polyPoints->SetId(
            static_cast<svtkIdType>(nPolyPoints++), static_cast<svtkIdType>(faceJPoints.size()));
          int pointI, delta; // must be signed
          faceOwnerValue = GetLabelValue(this->FaceOwner, cellFacesJ, use64BitLabels);
          if (faceOwnerValue == cellId)
          {
            pointI = 0;
            delta = 1;
          }
          else
          {
            // if it is a _neighbor_ face flip the points
            pointI = static_cast<int>(faceJPoints.size()) - 1;
            delta = -1;
          }
          for (size_t k = 0; k < faceJPoints.size(); k++, pointI += delta)
          {
            svtkTypeInt64 faceJPointK = faceJPoints[pointI];
            bool foundDup = false;
            for (svtkIdType l = 0; l < static_cast<svtkIdType>(nPoints); l++)
            {
              if (cellPoints->GetId(l) == faceJPointK)
              {
                foundDup = true;
                break; // look no more
              }
            }
            if (!foundDup)
            {
              if (nPoints >= static_cast<size_t>(maxNPoints))
              {
                svtkErrorMacro(<< "Too large polyhedron at cellId = " << cellId);
                cellPoints->Delete();
                polyPoints->Delete();
                return;
              }
              cellPoints->SetId(
                static_cast<svtkIdType>(nPoints++), static_cast<svtkIdType>(faceJPointK));
            }
            if (nPolyPoints >= static_cast<size_t>(maxNPolyPoints))
            {
              svtkErrorMacro(<< "Too large polyhedron at cellId = " << cellId);
              cellPoints->Delete();
              polyPoints->Delete();
              return;
            }
            polyPoints->SetId(
              static_cast<svtkIdType>(nPolyPoints++), static_cast<svtkIdType>(faceJPointK));
          }
        }

        // create the poly cell and insert it into the mesh
        internalMesh->InsertNextCell(SVTK_POLYHEDRON, static_cast<svtkIdType>(nPoints),
          cellPoints->GetPointer(0), static_cast<svtkIdType>(cellFaces.size()),
          polyPoints->GetPointer(0));
      }
    }
  }
  cellPoints->Delete();
  polyPoints->Delete();
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReaderPrivate::SetBlockName(
  svtkMultiBlockDataSet* blocks, unsigned int blockI, const char* name)
{
  blocks->GetMetaData(blockI)->Set(svtkCompositeDataSet::NAME(), name);
}

//-----------------------------------------------------------------------------
// derive cell types and create the internal mesh
svtkUnstructuredGrid* svtkOpenFOAMReaderPrivate::MakeInternalMesh(
  const svtkFoamLabelVectorVector* cellsFaces, const svtkFoamLabelVectorVector* facesPoints,
  svtkFloatArray* pointArray)
{
  // Create Mesh
  svtkUnstructuredGrid* internalMesh = svtkUnstructuredGrid::New();
  internalMesh->Allocate(this->NumCells);

  if (this->Parent->GetDecomposePolyhedra())
  {
    // for polyhedral decomposition
    this->AdditionalCellIds = svtkIdTypeArray::New();
    this->NumAdditionalCells = svtkIntArray::New();
    this->AdditionalCellPoints = new svtkFoamLabelArrayVector;

    svtkIdTypeArray* additionalCells = svtkIdTypeArray::New();
    additionalCells->SetNumberOfComponents(5); // accommodates tetra or pyramid

    this->InsertCellsToGrid(
      internalMesh, cellsFaces, facesPoints, pointArray, additionalCells, nullptr);

    // for polyhedral decomposition
    pointArray->Squeeze();
    this->AdditionalCellIds->Squeeze();
    this->NumAdditionalCells->Squeeze();
    additionalCells->Squeeze();

    // insert decomposed cells into mesh
    const int nComponents = additionalCells->GetNumberOfComponents();
    svtkIdType nAdditionalCells = additionalCells->GetNumberOfTuples();
    for (svtkIdType i = 0; i < nAdditionalCells; i++)
    {
      if (additionalCells->GetComponent(i, 4) == -1)
      {
        internalMesh->InsertNextCell(SVTK_TETRA, 4, additionalCells->GetPointer(i * nComponents));
      }
      else
      {
        internalMesh->InsertNextCell(SVTK_PYRAMID, 5, additionalCells->GetPointer(i * nComponents));
      }
    }
    internalMesh->Squeeze();
    additionalCells->Delete();
  }
  else
  {
    this->InsertCellsToGrid(internalMesh, cellsFaces, facesPoints, pointArray, nullptr, nullptr);
  }

  // set the internal mesh points
  svtkPoints* points = svtkPoints::New();
  points->SetData(pointArray);
  internalMesh->SetPoints(points);
  points->Delete();

  return internalMesh;
}

//-----------------------------------------------------------------------------
// insert faces to grid
void svtkOpenFOAMReaderPrivate::InsertFacesToGrid(svtkPolyData* boundaryMesh,
  const svtkFoamLabelVectorVector* facesPoints, svtkIdType startFace, svtkIdType endFace,
  svtkDataArray* boundaryPointMap, svtkIdList* facePointsVtkId, svtkDataArray* labels,
  bool isLookupValue)
{
  svtkPolyData& bm = *boundaryMesh;
  bool use64BitLabels = this->Parent->GetUse64BitLabels();

  for (svtkIdType j = startFace; j < endFace; j++)
  {
    svtkIdType faceId;
    if (labels == nullptr)
    {
      faceId = j;
    }
    else
    {
      faceId = GetLabelValue(labels, j, use64BitLabels);
      if (faceId >= this->FaceOwner->GetNumberOfTuples())
      {
        svtkWarningMacro(<< "faceLabels id " << faceId << " exceeds the number of faces "
                        << this->FaceOwner->GetNumberOfTuples());
        bm.InsertNextCell(SVTK_EMPTY_CELL, 0, facePointsVtkId->GetPointer(0));
        continue;
      }
    }

    const void* facePoints = facesPoints->operator[](faceId);
    svtkIdType nFacePoints = facesPoints->GetSize(faceId);

    if (isLookupValue)
    {
      for (svtkIdType k = 0; k < nFacePoints; k++)
      {
        facePointsVtkId->SetId(k,
          boundaryPointMap->LookupValue(
            static_cast<svtkIdType>(GetLabelValue(facePoints, k, use64BitLabels))));
      }
    }
    else
    {
      if (boundaryPointMap)
      {
        for (svtkIdType k = 0; k < nFacePoints; k++)
        {
          facePointsVtkId->SetId(k,
            GetLabelValue(boundaryPointMap,
              static_cast<svtkIdType>(GetLabelValue(facePoints, k, use64BitLabels)),
              use64BitLabels));
        }
      }
      else
      {
        for (svtkIdType k = 0; k < nFacePoints; k++)
        {
          facePointsVtkId->SetId(k, GetLabelValue(facePoints, k, use64BitLabels));
        }
      }
    }

    // triangle
    if (nFacePoints == 3)
    {
      bm.InsertNextCell(SVTK_TRIANGLE, 3, facePointsVtkId->GetPointer(0));
    }
    // quad
    else if (nFacePoints == 4)
    {
      bm.InsertNextCell(SVTK_QUAD, 4, facePointsVtkId->GetPointer(0));
    }
    // polygon
    else
    {
      bm.InsertNextCell(SVTK_POLYGON, static_cast<int>(nFacePoints), facePointsVtkId->GetPointer(0));
    }
  }
}

//-----------------------------------------------------------------------------
// returns requested boundary meshes
svtkMultiBlockDataSet* svtkOpenFOAMReaderPrivate::MakeBoundaryMesh(
  const svtkFoamLabelVectorVector* facesPoints, svtkFloatArray* pointArray)
{
  const svtkIdType nBoundaries = static_cast<svtkIdType>(this->BoundaryDict.size());
  bool use64BitLabels = this->Parent->GetUse64BitLabels();

  // do a consistency check of BoundaryDict
  svtkIdType previousEndFace = -1;
  for (int boundaryI = 0; boundaryI < nBoundaries; boundaryI++)
  {
    const svtkFoamBoundaryEntry& beI = this->BoundaryDict[boundaryI];
    svtkIdType startFace = beI.StartFace;
    svtkIdType nFaces = beI.NFaces;
    if (startFace < 0 || nFaces < 0)
    {
      svtkErrorMacro(<< "Neither of startFace " << startFace << " nor nFaces " << nFaces
                    << " can be negative for patch " << beI.BoundaryName.c_str());
      return nullptr;
    }
    if (previousEndFace >= 0 && previousEndFace != startFace)
    {
      svtkErrorMacro(<< "The end face number " << previousEndFace - 1 << " of patch "
                    << this->BoundaryDict[boundaryI - 1].BoundaryName.c_str()
                    << " is not consistent with the start face number " << startFace << " of patch "
                    << beI.BoundaryName.c_str());
      return nullptr;
    }
    previousEndFace = startFace + nFaces;
  }
  if (previousEndFace > facesPoints->GetNumberOfElements())
  {
    svtkErrorMacro(<< "The end face number " << previousEndFace - 1 << " of the last patch "
                  << this->BoundaryDict[nBoundaries - 1].BoundaryName.c_str()
                  << " exceeds the number of faces " << facesPoints->GetNumberOfElements());
    return nullptr;
  }

  svtkMultiBlockDataSet* boundaryMesh = svtkMultiBlockDataSet::New();

  if (this->Parent->GetCreateCellToPoint())
  {
    svtkIdType boundaryStartFace =
      (!this->BoundaryDict.empty() ? this->BoundaryDict[0].StartFace : 0);
    this->AllBoundaries = svtkPolyData::New();
    this->AllBoundaries->AllocateEstimate(
      facesPoints->GetNumberOfElements() - boundaryStartFace, 1);
  }
  this->BoundaryPointMap = new svtkFoamLabelArrayVector;

  svtkIdTypeArray* nBoundaryPointsList = svtkIdTypeArray::New();
  nBoundaryPointsList->SetNumberOfValues(nBoundaries);

  // count the max number of points per face and the number of points
  // (with duplicates) in mesh
  svtkIdType maxNFacePoints = 0;
  for (int boundaryI = 0; boundaryI < nBoundaries; boundaryI++)
  {
    svtkIdType startFace = this->BoundaryDict[boundaryI].StartFace;
    svtkIdType endFace = startFace + this->BoundaryDict[boundaryI].NFaces;
    svtkIdType nPoints = 0;
    for (svtkIdType j = startFace; j < endFace; j++)
    {
      svtkIdType nFacePoints = facesPoints->GetSize(j);
      nPoints += nFacePoints;
      if (nFacePoints > maxNFacePoints)
      {
        maxNFacePoints = nFacePoints;
      }
    }
    nBoundaryPointsList->SetValue(boundaryI, nPoints);
  }

  // aloocate array for converting int vector to svtkIdType List:
  // workaround for 64bit machines
  svtkIdList* facePointsVtkId = svtkIdList::New();
  facePointsVtkId->SetNumberOfIds(maxNFacePoints);

  // create initial internal point list: set all points to -1
  if (this->Parent->GetCreateCellToPoint())
  {
    if (!use64BitLabels)
    {
      this->InternalPoints = svtkTypeInt32Array::New();
    }
    else
    {
      this->InternalPoints = svtkTypeInt64Array::New();
    }
    this->InternalPoints->SetNumberOfValues(this->NumPoints);
    this->InternalPoints->FillComponent(0, -1);

    // mark boundary points as 0
    for (int boundaryI = 0; boundaryI < nBoundaries; boundaryI++)
    {
      const svtkFoamBoundaryEntry& beI = this->BoundaryDict[boundaryI];
      if (beI.BoundaryType == svtkFoamBoundaryEntry::PHYSICAL ||
        beI.BoundaryType == svtkFoamBoundaryEntry::PROCESSOR)
      {
        svtkIdType startFace = beI.StartFace;
        svtkIdType endFace = startFace + beI.NFaces;

        for (svtkIdType j = startFace; j < endFace; j++)
        {
          const void* facePoints = facesPoints->operator[](j);
          svtkIdType nFacePoints = facesPoints->GetSize(j);
          for (svtkIdType k = 0; k < nFacePoints; k++)
          {
            SetLabelValue(this->InternalPoints, GetLabelValue(facePoints, k, use64BitLabels), 0,
              use64BitLabels);
          }
        }
      }
    }
  }

  svtkTypeInt64 nAllBoundaryPoints = 0;
  std::vector<std::vector<svtkIdType> > procCellList;
  svtkIntArray* pointTypes = nullptr;

  if (this->Parent->GetCreateCellToPoint())
  {
    // create global to AllBounaries point map
    for (int pointI = 0; pointI < this->NumPoints; pointI++)
    {
      if (GetLabelValue(this->InternalPoints, pointI, use64BitLabels) == 0)
      {
        SetLabelValue(this->InternalPoints, pointI, nAllBoundaryPoints, use64BitLabels);
        nAllBoundaryPoints++;
      }
    }

    if (!this->ProcessorName.empty())
    {
      // initialize physical-processor boundary shared point list
      procCellList.resize(static_cast<size_t>(nAllBoundaryPoints));
      pointTypes = svtkIntArray::New();
      pointTypes->SetNumberOfTuples(nAllBoundaryPoints);
      for (svtkIdType pointI = 0; pointI < nAllBoundaryPoints; pointI++)
      {
        pointTypes->SetValue(pointI, 0);
      }
    }
  }

  for (int boundaryI = 0; boundaryI < nBoundaries; boundaryI++)
  {
    const svtkFoamBoundaryEntry& beI = this->BoundaryDict[boundaryI];
    svtkIdType nFaces = beI.NFaces;
    svtkIdType startFace = beI.StartFace;
    svtkIdType endFace = startFace + nFaces;

    if (this->Parent->GetCreateCellToPoint() &&
      (beI.BoundaryType == svtkFoamBoundaryEntry::PHYSICAL ||
        beI.BoundaryType == svtkFoamBoundaryEntry::PROCESSOR))
    {
      // add faces to AllBoundaries
      this->InsertFacesToGrid(this->AllBoundaries, facesPoints, startFace, endFace,
        this->InternalPoints, facePointsVtkId, nullptr, false);

      if (!this->ProcessorName.empty())
      {
        // mark belonging boundary types and, if PROCESSOR, cell numbers
        svtkIdType abStartFace = beI.AllBoundariesStartFace;
        svtkIdType abEndFace = abStartFace + beI.NFaces;
        for (svtkIdType faceI = abStartFace; faceI < abEndFace; faceI++)
        {
          svtkIdType nPoints;
          const svtkIdType* points;
          this->AllBoundaries->GetCellPoints(faceI, nPoints, points);
          if (beI.BoundaryType == svtkFoamBoundaryEntry::PHYSICAL)
          {
            for (svtkIdType pointI = 0; pointI < nPoints; pointI++)
            {
              *pointTypes->GetPointer(points[pointI]) |= svtkFoamBoundaryEntry::PHYSICAL;
            }
          }
          else // PROCESSOR
          {
            for (svtkIdType pointI = 0; pointI < nPoints; pointI++)
            {
              svtkIdType pointJ = points[pointI];
              *pointTypes->GetPointer(pointJ) |= svtkFoamBoundaryEntry::PROCESSOR;
              procCellList[pointJ].push_back(faceI);
            }
          }
        }
      }
    }

    // skip below if inactive
    if (!beI.IsActive)
    {
      continue;
    }

    // create the mesh
    const unsigned int activeBoundaryI = boundaryMesh->GetNumberOfBlocks();
    svtkPolyData* bm = svtkPolyData::New();
    boundaryMesh->SetBlock(activeBoundaryI, bm);

    // set the name of boundary
    this->SetBlockName(boundaryMesh, activeBoundaryI, beI.BoundaryName.c_str());

    bm->AllocateEstimate(nFaces, 1);
    svtkIdType nBoundaryPoints = nBoundaryPointsList->GetValue(boundaryI);

    // create global to boundary-local point map and boundary points
    svtkDataArray* boundaryPointList;
    if (use64BitLabels)
    {
      boundaryPointList = svtkTypeInt64Array::New();
    }
    else
    {
      boundaryPointList = svtkTypeInt32Array::New();
    }
    boundaryPointList->SetNumberOfValues(nBoundaryPoints);
    svtkIdType pointI = 0;
    for (svtkIdType j = startFace; j < endFace; j++)
    {
      const void* facePoints = facesPoints->operator[](j);
      svtkIdType nFacePoints = facesPoints->GetSize(j);
      for (int k = 0; k < nFacePoints; k++)
      {
        SetLabelValue(
          boundaryPointList, pointI, GetLabelValue(facePoints, k, use64BitLabels), use64BitLabels);
        pointI++;
      }
    }
    svtkSortDataArray::Sort(boundaryPointList);

    svtkDataArray* bpMap;
    if (use64BitLabels)
    {
      bpMap = svtkTypeInt64Array::New();
    }
    else
    {
      bpMap = svtkTypeInt32Array::New();
    }
    this->BoundaryPointMap->push_back(bpMap);
    svtkFloatArray* boundaryPointArray = svtkFloatArray::New();
    boundaryPointArray->SetNumberOfComponents(3);
    svtkIdType oldPointJ = -1;
    for (int j = 0; j < nBoundaryPoints; j++)
    {
      svtkTypeInt64 pointJ = GetLabelValue(boundaryPointList, j, use64BitLabels);
      if (pointJ != oldPointJ)
      {
        oldPointJ = pointJ;
        boundaryPointArray->InsertNextTuple(pointArray->GetPointer(3 * pointJ));
        AppendLabelValue(bpMap, pointJ, use64BitLabels);
      }
    }
    boundaryPointArray->Squeeze();
    bpMap->Squeeze();
    boundaryPointList->Delete();
    svtkPoints* boundaryPoints = svtkPoints::New();
    boundaryPoints->SetData(boundaryPointArray);
    boundaryPointArray->Delete();

    // set points for boundary
    bm->SetPoints(boundaryPoints);
    boundaryPoints->Delete();

    // insert faces to boundary mesh
    this->InsertFacesToGrid(
      bm, facesPoints, startFace, endFace, bpMap, facePointsVtkId, nullptr, true);
    bm->Delete();
    bpMap->ClearLookup();
  }

  nBoundaryPointsList->Delete();
  facePointsVtkId->Delete();

  if (this->Parent->GetCreateCellToPoint())
  {
    this->AllBoundaries->Squeeze();
    if (!use64BitLabels)
    {
      this->AllBoundariesPointMap = svtkTypeInt32Array::New();
    }
    else
    {
      this->AllBoundariesPointMap = svtkTypeInt64Array::New();
    }
    svtkDataArray& abpMap = *this->AllBoundariesPointMap;
    abpMap.SetNumberOfValues(nAllBoundaryPoints);

    // create lists of internal points and AllBoundaries points
    svtkIdType nInternalPoints = 0;
    for (svtkIdType pointI = 0, allBoundaryPointI = 0; pointI < this->NumPoints; pointI++)
    {
      svtkIdType globalPointId = GetLabelValue(this->InternalPoints, pointI, use64BitLabels);
      if (globalPointId == -1)
      {
        SetLabelValue(this->InternalPoints, nInternalPoints, pointI, use64BitLabels);
        nInternalPoints++;
      }
      else
      {
        SetLabelValue(&abpMap, allBoundaryPointI, pointI, use64BitLabels);
        allBoundaryPointI++;
      }
    }
    // shrink to the number of internal points
    if (nInternalPoints > 0)
    {
      this->InternalPoints->Resize(nInternalPoints);
    }
    else
    {
      this->InternalPoints->Delete();
      this->InternalPoints = nullptr;
    }

    // set dummy svtkPoints to tell the grid the number of points
    // (otherwise GetPointCells will crash)
    svtkPoints* allBoundaryPoints = svtkPoints::New();
    allBoundaryPoints->SetNumberOfPoints(abpMap.GetNumberOfTuples());
    this->AllBoundaries->SetPoints(allBoundaryPoints);
    allBoundaryPoints->Delete();

    if (!this->ProcessorName.empty())
    {
      // remove links to processor boundary faces from point-to-cell
      // links of physical-processor shared points to avoid cracky seams
      // on fixedValue-type boundaries which are noticeable when all the
      // decomposed meshes are appended
      this->AllBoundaries->BuildLinks();
      for (int pointI = 0; pointI < nAllBoundaryPoints; pointI++)
      {
        if (pointTypes->GetValue(pointI) ==
          (svtkFoamBoundaryEntry::PHYSICAL | svtkFoamBoundaryEntry::PROCESSOR))
        {
          const std::vector<svtkIdType>& procCells = procCellList[pointI];
          for (size_t cellI = 0; cellI < procCellList[pointI].size(); cellI++)
          {
            this->AllBoundaries->RemoveReferenceToCell(pointI, procCells[cellI]);
          }
          // omit reclaiming memory as the possibly recovered size should
          // not typically be so large
        }
      }
      pointTypes->Delete();
    }
  }

  return boundaryMesh;
}

//-----------------------------------------------------------------------------
// truncate face owner to have only boundary face info
void svtkOpenFOAMReaderPrivate::TruncateFaceOwner()
{
  svtkIdType boundaryStartFace = !this->BoundaryDict.empty() ? this->BoundaryDict[0].StartFace
                                                            : this->FaceOwner->GetNumberOfTuples();
  // all the boundary faces
  svtkIdType nBoundaryFaces = this->FaceOwner->GetNumberOfTuples() - boundaryStartFace;
  memmove(this->FaceOwner->GetVoidPointer(0), this->FaceOwner->GetVoidPointer(boundaryStartFace),
    static_cast<size_t>(this->FaceOwner->GetDataTypeSize() * nBoundaryFaces));
  this->FaceOwner->Resize(nBoundaryFaces);
}

//-----------------------------------------------------------------------------
// this is necessary due to the strange svtkDataArrayTemplate::Resize()
// implementation when the array size is to be extended
template <typename T1, typename T2>
bool svtkOpenFOAMReaderPrivate::ExtendArray(T1* array, svtkIdType nTuples)
{
  svtkIdType newSize = nTuples * array->GetNumberOfComponents();
  void* ptr = malloc(static_cast<size_t>(newSize * array->GetDataTypeSize()));
  if (ptr == nullptr)
  {
    return false;
  }
  memmove(ptr, array->GetVoidPointer(0), array->GetDataSize() * array->GetDataTypeSize());
  array->SetArray(static_cast<T2*>(ptr), newSize, 0);
  return true;
}

//-----------------------------------------------------------------------------
// move polyhedral cell centroids
svtkPoints* svtkOpenFOAMReaderPrivate::MoveInternalMesh(
  svtkUnstructuredGrid* internalMesh, svtkFloatArray* pointArray)
{
  bool use64BitLabels = this->Parent->GetUse64BitLabels();
  if (this->Parent->GetDecomposePolyhedra())
  {
    const svtkIdType nAdditionalCells = static_cast<svtkIdType>(this->AdditionalCellPoints->size());
    this->ExtendArray<svtkFloatArray, float>(pointArray, this->NumPoints + nAdditionalCells);
    for (int i = 0; i < nAdditionalCells; i++)
    {
      svtkDataArray* polyCellPoints = this->AdditionalCellPoints->operator[](i);
      float centroid[3];
      centroid[0] = centroid[1] = centroid[2] = 0.0F;
      svtkIdType nCellPoints = polyCellPoints->GetDataSize();
      for (svtkIdType j = 0; j < nCellPoints; j++)
      {
        float* pointK =
          pointArray->GetPointer(3 * GetLabelValue(polyCellPoints, j, use64BitLabels));
        centroid[0] += pointK[0];
        centroid[1] += pointK[1];
        centroid[2] += pointK[2];
      }
      const float weight = (nCellPoints ? 1.0F / static_cast<float>(nCellPoints) : 0.0F);
      centroid[0] *= weight;
      centroid[1] *= weight;
      centroid[2] *= weight;
      pointArray->InsertTuple(this->NumPoints + i, centroid);
    }
  }
  if (internalMesh->GetPoints()->GetNumberOfPoints() != pointArray->GetNumberOfTuples())
  {
    svtkErrorMacro(<< "The numbers of points for old points "
                  << internalMesh->GetPoints()->GetNumberOfPoints() << " and new points"
                  << pointArray->GetNumberOfTuples() << " don't match");
    return nullptr;
  }

  // instantiate the points class
  svtkPoints* points = svtkPoints::New();
  points->SetData(pointArray);
  internalMesh->SetPoints(points);
  return points;
}

//-----------------------------------------------------------------------------
// move boundary points
void svtkOpenFOAMReaderPrivate::MoveBoundaryMesh(
  svtkMultiBlockDataSet* boundaryMesh, svtkFloatArray* pointArray)
{
  bool use64BitLabels = this->Parent->GetUse64BitLabels();
  for (svtkIdType boundaryI = 0, activeBoundaryI = 0;
       boundaryI < static_cast<svtkIdType>(this->BoundaryDict.size()); boundaryI++)
  {
    if (this->BoundaryDict[boundaryI].IsActive)
    {
      svtkDataArray* bpMap = this->BoundaryPointMap->operator[](activeBoundaryI);
      svtkIdType nBoundaryPoints = bpMap->GetNumberOfTuples();
      svtkFloatArray* boundaryPointArray = svtkFloatArray::New();
      boundaryPointArray->SetNumberOfComponents(3);
      boundaryPointArray->SetNumberOfTuples(nBoundaryPoints);
      for (int pointI = 0; pointI < nBoundaryPoints; pointI++)
      {
        boundaryPointArray->SetTuple(
          pointI, GetLabelValue(bpMap, pointI, use64BitLabels), pointArray);
      }
      svtkPoints* boundaryPoints = svtkPoints::New();
      boundaryPoints->SetData(boundaryPointArray);
      boundaryPointArray->Delete();
      svtkPolyData::SafeDownCast(boundaryMesh->GetBlock(static_cast<unsigned int>(activeBoundaryI)))
        ->SetPoints(boundaryPoints);
      boundaryPoints->Delete();
      activeBoundaryI++;
    }
  }
}

//-----------------------------------------------------------------------------
// as of now the function does not do interpolation, but do just averaging.
void svtkOpenFOAMReaderPrivate::InterpolateCellToPoint(svtkFloatArray* pData, svtkFloatArray* iData,
  svtkPointSet* mesh, svtkDataArray* pointList, svtkTypeInt64 nPoints)
{
  if (nPoints == 0)
  {
    return;
  }

  bool use64BitLabels = this->Parent->GetUse64BitLabels();

  // a dummy call to let GetPointCells() build the cell links if still not built
  // (not using BuildLinks() since it always rebuild links)
  svtkIdList* pointCells = svtkIdList::New();
  mesh->GetPointCells(0, pointCells);
  pointCells->Delete();

  // Set up to grab point cells
  svtkUnstructuredGrid* ug = svtkUnstructuredGrid::SafeDownCast(mesh);
  svtkPolyData* pd = svtkPolyData::SafeDownCast(mesh);
  svtkIdType nCells;
  svtkIdType* cells;

  const int nComponents = iData->GetNumberOfComponents();

  if (nComponents == 1)
  {
    // a special case with the innermost componentI loop unrolled
    float* tuples = iData->GetPointer(0);
    for (svtkTypeInt64 pointI = 0; pointI < nPoints; pointI++)
    {
      svtkTypeInt64 pI = pointList ? GetLabelValue(pointList, pointI, use64BitLabels) : pointI;
      if (ug)
      {
        ug->GetPointCells(pI, nCells, cells);
      }
      else
      {
        pd->GetPointCells(pI, nCells, cells);
      }

      // use double intermediate variable for precision
      double interpolatedValue = 0.0;
      for (int cellI = 0; cellI < nCells; cellI++)
      {
        interpolatedValue += tuples[cells[cellI]];
      }
      interpolatedValue = (nCells ? interpolatedValue / static_cast<double>(nCells) : 0.0);
      pData->SetValue(pI, static_cast<float>(interpolatedValue));
    }
  }
  else if (nComponents == 3)
  {
    // a special case with the innermost componentI loop unrolled
    float* pDataPtr = pData->GetPointer(0);
    for (svtkTypeInt64 pointI = 0; pointI < nPoints; pointI++)
    {
      svtkTypeInt64 pI = pointList ? GetLabelValue(pointList, pointI, use64BitLabels) : pointI;
      if (ug)
      {
        ug->GetPointCells(pI, nCells, cells);
      }
      else
      {
        pd->GetPointCells(pI, nCells, cells);
      }

      // use double intermediate variables for precision
      const double weight = (nCells ? 1.0 / static_cast<double>(nCells) : 0.0);
      double summedValue0 = 0.0, summedValue1 = 0.0, summedValue2 = 0.0;

      // hand unrolling
      for (int cellI = 0; cellI < nCells; cellI++)
      {
        const float* tuple = iData->GetPointer(3 * cells[cellI]);
        summedValue0 += tuple[0];
        summedValue1 += tuple[1];
        summedValue2 += tuple[2];
      }

      float* interpolatedValue = &pDataPtr[3 * pI];
      interpolatedValue[0] = static_cast<float>(weight * summedValue0);
      interpolatedValue[1] = static_cast<float>(weight * summedValue1);
      interpolatedValue[2] = static_cast<float>(weight * summedValue2);
    }
  }
  else
  {
    float* pDataPtr = pData->GetPointer(0);
    for (svtkTypeInt64 pointI = 0; pointI < nPoints; pointI++)
    {
      svtkTypeInt64 pI = pointList ? GetLabelValue(pointList, pointI, use64BitLabels) : pointI;
      if (ug)
      {
        ug->GetPointCells(pI, nCells, cells);
      }
      else
      {
        pd->GetPointCells(pI, nCells, cells);
      }

      // use double intermediate variables for precision
      const double weight = (nCells ? 1.0 / static_cast<double>(nCells) : 0.0);
      float* interpolatedValue = &pDataPtr[nComponents * pI];
      // a bit strange loop order but this works fastest
      for (int componentI = 0; componentI < nComponents; componentI++)
      {
        const float* tuple = iData->GetPointer(componentI);
        double summedValue = 0.0;
        for (int cellI = 0; cellI < nCells; cellI++)
        {
          summedValue += tuple[nComponents * cells[cellI]];
        }
        interpolatedValue[componentI] = static_cast<float>(weight * summedValue);
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool svtkOpenFOAMReaderPrivate::ReadFieldFile(svtkFoamIOobject* ioPtr, svtkFoamDict* dictPtr,
  const svtkStdString& varName, svtkDataArraySelection* selection)
{
  const svtkStdString varPath(this->CurrentTimeRegionPath() + "/" + varName);

  // open the file
  svtkFoamIOobject& io = *ioPtr;
  if (!io.Open(varPath))
  {
    svtkErrorMacro(<< "Error opening " << io.GetFileName().c_str() << ": " << io.GetError().c_str());
    return false;
  }

  // if the variable is disabled on selection panel then skip it
  if (selection->ArrayExists(io.GetObjectName().c_str()) &&
    !selection->ArrayIsEnabled(io.GetObjectName().c_str()))
  {
    return false;
  }

  // read the field file into dictionary
  svtkFoamDict& dict = *dictPtr;
  if (!dict.Read(io))
  {
    svtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of "
                  << io.GetFileName().c_str() << ": " << io.GetError().c_str());
    return false;
  }

  if (dict.GetType() != svtkFoamToken::DICTIONARY)
  {
    svtkErrorMacro(<< "File " << io.GetFileName().c_str() << "is not valid as a field file");
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
svtkFloatArray* svtkOpenFOAMReaderPrivate::FillField(svtkFoamEntry* entryPtr, svtkIdType nElements,
  svtkFoamIOobject* ioPtr, const svtkStdString& fieldType)
{
  svtkFloatArray* data;
  svtkFoamEntry& entry = *entryPtr;
  const svtkStdString& className = ioPtr->GetClassName();

  // "uniformValue" keyword is for uniformFixedValue B.C.
  if (entry.FirstValue().GetIsUniform() || entry.GetKeyword() == "uniformValue")
  {
    if (entry.FirstValue().GetType() == svtkFoamToken::SCALAR ||
      entry.FirstValue().GetType() == svtkFoamToken::LABEL)
    {
      const float num = entry.ToFloat();
      data = svtkFloatArray::New();
      data->SetNumberOfValues(nElements);
      for (svtkIdType i = 0; i < nElements; i++)
      {
        data->SetValue(i, num);
      }
    }
    else
    {
      float tupleBuffer[9], *tuple;
      int nComponents;
      // have to determine the type of vector
      if (entry.FirstValue().GetType() == svtkFoamToken::LABELLIST)
      {
        svtkDataArray& ll = entry.LabelList();
        nComponents = static_cast<int>(ll.GetNumberOfTuples());
        for (int componentI = 0; componentI < nComponents; componentI++)
        {
          tupleBuffer[componentI] = static_cast<float>(ll.GetTuple1(componentI));
        }
        tuple = tupleBuffer;
      }
      else if (entry.FirstValue().GetType() == svtkFoamToken::SCALARLIST)
      {
        svtkFloatArray& sl = entry.ScalarList();
        nComponents = static_cast<int>(sl.GetSize());
        tuple = sl.GetPointer(0);
      }
      else
      {
        svtkErrorMacro(<< "Wrong list type for uniform field");
        return nullptr;
      }

      if ((fieldType == "SphericalTensorField" && nComponents == 1) ||
        (fieldType == "VectorField" && nComponents == 3) ||
        (fieldType == "SymmTensorField" && nComponents == 6) ||
        (fieldType == "TensorField" && nComponents == 9))
      {
        data = svtkFloatArray::New();
        data->SetNumberOfComponents(nComponents);
        data->SetNumberOfTuples(nElements);
        // swap the components of symmTensor to match the component
        // names in paraview
        if (nComponents == 6)
        {
          const float symxy = tuple[1], symxz = tuple[2], symyy = tuple[3];
          const float symyz = tuple[4], symzz = tuple[5];
          tuple[1] = symyy;
          tuple[2] = symzz;
          tuple[3] = symxy;
          tuple[4] = symyz;
          tuple[5] = symxz;
        }
        for (svtkIdType i = 0; i < nElements; i++)
        {
          data->SetTuple(i, tuple);
        }
      }
      else
      {
        svtkErrorMacro(<< "Number of components and field class doesn't match "
                      << "for " << ioPtr->GetFileName().c_str() << ". class = " << className.c_str()
                      << ", nComponents = " << nComponents);
        return nullptr;
      }
    }
  }
  else // nonuniform
  {
    if ((fieldType == "ScalarField" && entry.FirstValue().GetType() == svtkFoamToken::SCALARLIST) ||
      ((fieldType == "VectorField" || fieldType == "SphericalTensorField" ||
         fieldType == "SymmTensorField" || fieldType == "TensorField") &&
        entry.FirstValue().GetType() == svtkFoamToken::VECTORLIST))
    {
      svtkIdType nTuples = entry.ScalarList().GetNumberOfTuples();
      if (nTuples != nElements)
      {
        svtkErrorMacro(<< "Number of cells/points in mesh and field don't match: "
                      << "mesh = " << nElements << ", field = " << nTuples);
        return nullptr;
      }
      data = static_cast<svtkFloatArray*>(entry.Ptr());
      // swap the components of symmTensor to match the component
      // names in paraview
      const int nComponents = data->GetNumberOfComponents();
      if (nComponents == 6)
      {
        for (int tupleI = 0; tupleI < nTuples; tupleI++)
        {
          float* tuple = data->GetPointer(nComponents * tupleI);
          const float symxy = tuple[1], symxz = tuple[2], symyy = tuple[3];
          const float symyz = tuple[4], symzz = tuple[5];
          tuple[1] = symyy;
          tuple[2] = symzz;
          tuple[3] = symxy;
          tuple[4] = symyz;
          tuple[5] = symxz;
        }
      }
    }
    else if (entry.FirstValue().GetType() == svtkFoamToken::EMPTYLIST && nElements <= 0)
    {
      data = svtkFloatArray::New();
      // set the number of components as appropriate if the list is empty
      if (fieldType == "ScalarField" || fieldType == "SphericalTensorField")
      {
        data->SetNumberOfComponents(1);
      }
      else if (fieldType == "VectorField")
      {
        data->SetNumberOfComponents(3);
      }
      else if (fieldType == "SymmTensorField")
      {
        data->SetNumberOfComponents(6);
      }
      else if (fieldType == "TensorField")
      {
        data->SetNumberOfComponents(9);
      }
    }
    else
    {
      svtkErrorMacro(<< ioPtr->GetFileName().c_str() << " is not a valid "
                    << ioPtr->GetClassName().c_str());
      return nullptr;
    }
  }
  return data;
}

//-----------------------------------------------------------------------------
// convert OpenFOAM's dimension array representation to string
void svtkOpenFOAMReaderPrivate::ConstructDimensions(svtkStdString* dimString, svtkFoamDict* dictPtr)
{
  if (!this->Parent->GetAddDimensionsToArrayNames())
  {
    return;
  }
  bool use64BitLabels = this->Parent->GetUse64BitLabels();
  svtkFoamEntry* dimEntry = dictPtr->Lookup("dimensions");
  if (dimEntry != nullptr && dimEntry->FirstValue().GetType() == svtkFoamToken::LABELLIST)
  {
    svtkDataArray& dims = dimEntry->LabelList();
    if (dims.GetNumberOfTuples() == 7)
    {
      svtkTypeInt64 dimSet[7];
      for (svtkIdType dimI = 0; dimI < 7; dimI++)
      {
        dimSet[dimI] = GetLabelValue(&dims, dimI, use64BitLabels);
      }
      static const char* units[7] = { "kg", "m", "s", "K", "mol", "A", "cd" };
      std::ostringstream posDim, negDim;
      int posSpc = 0, negSpc = 0;
      if (dimSet[0] == 1 && dimSet[1] == -1 && dimSet[2] == -2)
      {
        posDim << "Pa";
        dimSet[0] = dimSet[1] = dimSet[2] = 0;
        posSpc = 1;
      }
      for (int dimI = 0; dimI < 7; dimI++)
      {
        svtkTypeInt64 dimDim = dimSet[dimI];
        if (dimDim > 0)
        {
          if (posSpc)
          {
            posDim << " ";
          }
          posDim << units[dimI];
          if (dimDim > 1)
          {
            posDim << dimDim;
          }
          posSpc++;
        }
        else if (dimDim < 0)
        {
          if (negSpc)
          {
            negDim << " ";
          }
          negDim << units[dimI];
          if (dimDim < -1)
          {
            negDim << -dimDim;
          }
          negSpc++;
        }
      }
      *dimString += " [" + posDim.str();
      if (negSpc > 0)
      {
        if (posSpc == 0)
        {
          *dimString += "1";
        }
        if (negSpc > 1)
        {
          *dimString += "/(" + negDim.str() + ")";
        }
        else
        {
          *dimString += "/" + negDim.str();
        }
      }
      else if (posSpc == 0)
      {
        *dimString += "-";
      }
      *dimString += "]";
    }
  }
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReaderPrivate::GetVolFieldAtTimeStep(svtkUnstructuredGrid* internalMesh,
  svtkMultiBlockDataSet* boundaryMesh, const svtkStdString& varName)
{
  bool use64BitLabels = this->Parent->GetUse64BitLabels();
  svtkFoamIOobject io(this->CasePath, this->Parent);
  svtkFoamDict dict;
  if (!this->ReadFieldFile(&io, &dict, varName, this->Parent->CellDataArraySelection))
  {
    return;
  }

  if (io.GetClassName().substr(0, 3) != "vol")
  {
    svtkErrorMacro(<< io.GetFileName().c_str() << " is not a volField");
    return;
  }

  svtkFoamEntry* iEntry = dict.Lookup("internalField");
  if (iEntry == nullptr)
  {
    svtkErrorMacro(<< "internalField not found in " << io.GetFileName().c_str());
    return;
  }

  if (iEntry->FirstValue().GetType() == svtkFoamToken::EMPTYLIST)
  {
    // if there's no cell there shouldn't be any boundary faces either
    if (this->NumCells > 0)
    {
      svtkErrorMacro(<< "internalField of " << io.GetFileName().c_str() << " is empty");
    }
    return;
  }

  svtkStdString fieldType = io.GetClassName().substr(3, svtkStdString::npos);
  svtkFloatArray* iData = this->FillField(iEntry, this->NumCells, &io, fieldType);
  if (iData == nullptr)
  {
    return;
  }

  svtkStdString dimString;
  this->ConstructDimensions(&dimString, &dict);

  svtkFloatArray *acData = nullptr, *ctpData = nullptr;

  if (this->Parent->GetCreateCellToPoint())
  {
    acData = svtkFloatArray::New();
    acData->SetNumberOfComponents(iData->GetNumberOfComponents());
    acData->SetNumberOfTuples(this->AllBoundaries->GetNumberOfCells());
  }

  if (iData->GetSize() > 0)
  {
    // Add field only if internal Mesh exists (skip if not selected).
    // Note we still need to read internalField even if internal mesh is
    // not selected, since boundaries without value entries may refer to
    // the internalField.
    if (internalMesh != nullptr)
    {
      if (this->Parent->GetDecomposePolyhedra())
      {
        // add values for decomposed cells
        this->ExtendArray<svtkFloatArray, float>(
          iData, this->NumCells + this->NumTotalAdditionalCells);
        svtkIdType nTuples = this->AdditionalCellIds->GetNumberOfTuples();
        svtkIdType additionalCellI = this->NumCells;
        for (svtkIdType tupleI = 0; tupleI < nTuples; tupleI++)
        {
          const int nCells = this->NumAdditionalCells->GetValue(tupleI);
          const svtkIdType cellId = this->AdditionalCellIds->GetValue(tupleI);
          for (int cellI = 0; cellI < nCells; cellI++)
          {
            iData->InsertTuple(additionalCellI++, cellId, iData);
          }
        }
      }

      // set data to internal mesh
      this->AddArrayToFieldData(internalMesh->GetCellData(), iData, io.GetObjectName() + dimString);

      if (this->Parent->GetCreateCellToPoint())
      {
        // Create cell-to-point interpolated data
        ctpData = svtkFloatArray::New();
        ctpData->SetNumberOfComponents(iData->GetNumberOfComponents());
        ctpData->SetNumberOfTuples(internalMesh->GetPoints()->GetNumberOfPoints());
        if (this->InternalPoints != nullptr)
        {
          this->InterpolateCellToPoint(ctpData, iData, internalMesh, this->InternalPoints,
            this->InternalPoints->GetNumberOfTuples());
        }

        if (this->Parent->GetDecomposePolyhedra())
        {
          // assign cell values to additional points
          svtkIdType nPoints = this->AdditionalCellIds->GetNumberOfTuples();
          for (svtkIdType pointI = 0; pointI < nPoints; pointI++)
          {
            ctpData->SetTuple(
              this->NumPoints + pointI, this->AdditionalCellIds->GetValue(pointI), iData);
          }
        }
      }
    }
  }
  else
  {
    // determine as there's no cells
    iData->Delete();
    if (acData != nullptr)
    {
      acData->Delete();
    }
    return;
  }

  // set boundary values
  const svtkFoamEntry* bEntry = dict.Lookup("boundaryField");
  if (bEntry == nullptr)
  {
    svtkWarningMacro(<< "boundaryField not found in object " << varName.c_str()
                    << " at time = " << this->TimeNames->GetValue(this->TimeStep).c_str());
    iData->Delete();
    if (acData != nullptr)
    {
      acData->Delete();
    }
    if (ctpData != nullptr)
    {
      ctpData->Delete();
    }
    return;
  }

  for (int boundaryI = 0, activeBoundaryI = 0;
       boundaryI < static_cast<int>(this->BoundaryDict.size()); boundaryI++)
  {
    const svtkFoamBoundaryEntry& beI = this->BoundaryDict[boundaryI];
    const svtkStdString& boundaryNameI = beI.BoundaryName;

    const svtkFoamEntry* bEntryI = bEntry->Dictionary().Lookup(boundaryNameI, true);
    if (bEntryI == nullptr)
    {
      svtkWarningMacro(<< "boundaryField " << boundaryNameI.c_str() << " not found in object "
                      << varName.c_str()
                      << " at time = " << this->TimeNames->GetValue(this->TimeStep).c_str());
      iData->Delete();
      if (acData != nullptr)
      {
        acData->Delete();
      }
      if (ctpData != nullptr)
      {
        ctpData->Delete();
      }
      return;
    }

    if (bEntryI->FirstValue().GetType() != svtkFoamToken::DICTIONARY)
    {
      svtkWarningMacro(<< "Type of boundaryField " << boundaryNameI.c_str()
                      << " is not a subdictionary in object " << varName.c_str()
                      << " at time = " << this->TimeNames->GetValue(this->TimeStep).c_str());
      iData->Delete();
      if (acData != nullptr)
      {
        acData->Delete();
      }
      if (ctpData != nullptr)
      {
        ctpData->Delete();
      }
      return;
    }

    svtkIdType nFaces = beI.NFaces;

    svtkFloatArray* vData = nullptr;
    bool valueFound = false;
    svtkFoamEntry* vEntry = bEntryI->Dictionary().Lookup("value");
    if (vEntry != nullptr) // the boundary has a value entry
    {
      vData = this->FillField(vEntry, nFaces, &io, fieldType);
      if (vData == nullptr)
      {
        iData->Delete();
        if (acData != nullptr)
        {
          acData->Delete();
        }
        if (ctpData != nullptr)
        {
          ctpData->Delete();
        }
        return;
      }
      valueFound = true;
    }
    else
    {
      // uniformFixedValue B.C.
      const svtkFoamEntry* ufvEntry = bEntryI->Dictionary().Lookup("type");
      if (ufvEntry != nullptr)
      {
        if (ufvEntry->ToString() == "uniformFixedValue")
        {
          // the boundary is of uniformFixedValue type
          svtkFoamEntry* uvEntry = bEntryI->Dictionary().Lookup("uniformValue");
          if (uvEntry != nullptr) // and has a uniformValue entry
          {
            vData = this->FillField(uvEntry, nFaces, &io, fieldType);
            if (vData == nullptr)
            {
              iData->Delete();
              if (acData != nullptr)
              {
                acData->Delete();
              }
              if (ctpData != nullptr)
              {
                ctpData->Delete();
              }
              return;
            }
            valueFound = true;
          }
        }
      }
    }

    svtkIdType boundaryStartFace = beI.StartFace - this->BoundaryDict[0].StartFace;

    if (!valueFound) // doesn't have a value nor uniformValue entry
    {
      // use patch-internal values as boundary values
      vData = svtkFloatArray::New();
      vData->SetNumberOfComponents(iData->GetNumberOfComponents());
      vData->SetNumberOfTuples(nFaces);
      for (int j = 0; j < nFaces; j++)
      {
        svtkTypeInt64 cellId = GetLabelValue(this->FaceOwner, boundaryStartFace + j, use64BitLabels);
        vData->SetTuple(j, cellId, iData);
      }
    }

    if (this->Parent->GetCreateCellToPoint())
    {
      svtkIdType startFace = beI.AllBoundariesStartFace;
      // if reading a processor sub-case of a decomposed case as is,
      // use the patch values of the processor patch as is
      if (beI.BoundaryType == svtkFoamBoundaryEntry::PHYSICAL ||
        (this->ProcessorName.empty() && beI.BoundaryType == svtkFoamBoundaryEntry::PROCESSOR))
      {
        // set the same value to AllBoundaries
        for (svtkIdType faceI = 0; faceI < nFaces; faceI++)
        {
          acData->SetTuple(faceI + startFace, faceI, vData);
        }
      }
      // implies && this->ProcessorName != ""
      else if (beI.BoundaryType == svtkFoamBoundaryEntry::PROCESSOR)
      {
        // average patch internal value and patch value assuming the
        // patch value to be the patchInternalField of the neighbor
        // decomposed mesh. Using double precision to avoid degrade in
        // accuracy.
        const int nComponents = vData->GetNumberOfComponents();
        for (svtkIdType faceI = 0; faceI < nFaces; faceI++)
        {
          const float* vTuple = vData->GetPointer(nComponents * faceI);
          const float* iTuple = iData->GetPointer(nComponents *
            GetLabelValue(this->FaceOwner, boundaryStartFace + faceI, use64BitLabels));
          float* acTuple = acData->GetPointer(nComponents * (startFace + faceI));
          for (int componentI = 0; componentI < nComponents; componentI++)
          {
            acTuple[componentI] = static_cast<float>(
              (static_cast<double>(vTuple[componentI]) + static_cast<double>(iTuple[componentI])) *
              0.5);
          }
        }
      }
    }

    if (beI.IsActive)
    {
      svtkPolyData* bm = svtkPolyData::SafeDownCast(boundaryMesh->GetBlock(activeBoundaryI));
      this->AddArrayToFieldData(bm->GetCellData(), vData, io.GetObjectName() + dimString);

      if (this->Parent->GetCreateCellToPoint())
      {
        // construct cell-to-point interpolated boundary values. This
        // is done independently from allBoundary interpolation so
        // that the interpolated values are not affected by
        // neighboring patches especially at patch edges and for
        // baffle patches
        svtkFloatArray* pData = svtkFloatArray::New();
        pData->SetNumberOfComponents(vData->GetNumberOfComponents());
        svtkIdType nPoints = bm->GetPoints()->GetNumberOfPoints();
        pData->SetNumberOfTuples(nPoints);
        this->InterpolateCellToPoint(pData, vData, bm, nullptr, nPoints);
        this->AddArrayToFieldData(bm->GetPointData(), pData, io.GetObjectName() + dimString);
        pData->Delete();
      }

      activeBoundaryI++;
    }
    vData->Delete();
  }
  iData->Delete();

  if (this->Parent->GetCreateCellToPoint())
  {
    // Create cell-to-point interpolated data for all boundaries and
    // override internal values
    svtkFloatArray* bpData = svtkFloatArray::New();
    bpData->SetNumberOfComponents(acData->GetNumberOfComponents());
    svtkIdType nPoints = this->AllBoundariesPointMap->GetNumberOfTuples();
    bpData->SetNumberOfTuples(nPoints);
    this->InterpolateCellToPoint(bpData, acData, this->AllBoundaries, nullptr, nPoints);
    acData->Delete();

    if (ctpData != nullptr)
    {
      // set cell-to-pint data for internal mesh
      for (svtkIdType pointI = 0; pointI < nPoints; pointI++)
      {
        ctpData->SetTuple(
          GetLabelValue(this->AllBoundariesPointMap, pointI, use64BitLabels), pointI, bpData);
      }
      this->AddArrayToFieldData(
        internalMesh->GetPointData(), ctpData, io.GetObjectName() + dimString);
      ctpData->Delete();
    }

    bpData->Delete();
  }
}

//-----------------------------------------------------------------------------
// read point field at a timestep
void svtkOpenFOAMReaderPrivate::GetPointFieldAtTimeStep(svtkUnstructuredGrid* internalMesh,
  svtkMultiBlockDataSet* boundaryMesh, const svtkStdString& varName)
{
  bool use64BitLabels = this->Parent->GetUse64BitLabels();
  svtkFoamIOobject io(this->CasePath, this->Parent);
  svtkFoamDict dict;
  if (!this->ReadFieldFile(&io, &dict, varName, this->Parent->PointDataArraySelection))
  {
    return;
  }

  if (io.GetClassName().substr(0, 5) != "point")
  {
    svtkErrorMacro(<< io.GetFileName().c_str() << " is not a pointField");
    return;
  }

  svtkFoamEntry* iEntry = dict.Lookup("internalField");
  if (iEntry == nullptr)
  {
    svtkErrorMacro(<< "internalField not found in " << io.GetFileName().c_str());
    return;
  }

  if (iEntry->FirstValue().GetType() == svtkFoamToken::EMPTYLIST)
  {
    // if there's no cell there shouldn't be any boundary faces either
    if (this->NumPoints > 0)
    {
      svtkErrorMacro(<< "internalField of " << io.GetFileName().c_str() << " is empty");
    }
    return;
  }

  svtkStdString fieldType = io.GetClassName().substr(5, svtkStdString::npos);
  svtkFloatArray* iData = this->FillField(iEntry, this->NumPoints, &io, fieldType);
  if (iData == nullptr)
  {
    return;
  }

  svtkStdString dimString;
  this->ConstructDimensions(&dimString, &dict);

  // AdditionalCellPoints is nullptr if creation of InternalMesh had been skipped
  if (this->AdditionalCellPoints != nullptr)
  {
    // point-to-cell interpolation to additional cell centroidal points
    // for decomposed cells
    const int nAdditionalPoints = static_cast<int>(this->AdditionalCellPoints->size());
    const int nComponents = iData->GetNumberOfComponents();
    this->ExtendArray<svtkFloatArray, float>(iData, this->NumPoints + nAdditionalPoints);
    for (int i = 0; i < nAdditionalPoints; i++)
    {
      svtkDataArray* acp = this->AdditionalCellPoints->operator[](i);
      svtkIdType nPoints = acp->GetDataSize();
      double interpolatedValue[9];
      for (int k = 0; k < nComponents; k++)
      {
        interpolatedValue[k] = 0.0;
      }
      for (svtkIdType j = 0; j < nPoints; j++)
      {
        const float* tuple = iData->GetPointer(nComponents * GetLabelValue(acp, j, use64BitLabels));
        for (int k = 0; k < nComponents; k++)
        {
          interpolatedValue[k] += tuple[k];
        }
      }
      const double weight = 1.0 / static_cast<double>(nPoints);
      for (int k = 0; k < nComponents; k++)
      {
        interpolatedValue[k] *= weight;
      }
      // will automatically be converted to float
      iData->InsertTuple(this->NumPoints + i, interpolatedValue);
    }
  }

  if (iData->GetSize() > 0)
  {
    // Add field only if internal Mesh exists (skip if not selected).
    // Note we still need to read internalField even if internal mesh is
    // not selected, since boundaries without value entries may refer to
    // the internalField.
    if (internalMesh != nullptr)
    {
      // set data to internal mesh
      this->AddArrayToFieldData(
        internalMesh->GetPointData(), iData, io.GetObjectName() + dimString);
    }
  }
  else
  {
    // determine as there's no points
    iData->Delete();
    return;
  }

  // use patch-internal values as boundary values
  for (svtkIdType boundaryI = 0, activeBoundaryI = 0;
       boundaryI < static_cast<svtkIdType>(this->BoundaryDict.size()); boundaryI++)
  {
    if (this->BoundaryDict[boundaryI].IsActive)
    {
      svtkFloatArray* vData = svtkFloatArray::New();
      svtkDataArray* bpMap = this->BoundaryPointMap->operator[](activeBoundaryI);
      const svtkIdType nPoints = bpMap->GetNumberOfTuples();
      vData->SetNumberOfComponents(iData->GetNumberOfComponents());
      vData->SetNumberOfTuples(nPoints);
      for (svtkIdType j = 0; j < nPoints; j++)
      {
        vData->SetTuple(j, GetLabelValue(bpMap, j, use64BitLabels), iData);
      }
      this->AddArrayToFieldData(
        svtkPolyData::SafeDownCast(
          boundaryMesh->GetBlock(static_cast<unsigned int>(activeBoundaryI)))
          ->GetPointData(),
        vData, io.GetObjectName() + dimString);
      vData->Delete();
      activeBoundaryI++;
    }
  }
  iData->Delete();
}

//-----------------------------------------------------------------------------
svtkMultiBlockDataSet* svtkOpenFOAMReaderPrivate::MakeLagrangianMesh()
{
  svtkMultiBlockDataSet* lagrangianMesh = svtkMultiBlockDataSet::New();

  for (int cloudI = 0; cloudI < this->Parent->LagrangianPaths->GetNumberOfTuples(); cloudI++)
  {
    const svtkStdString& pathI = this->Parent->LagrangianPaths->GetValue(cloudI);

    // still can't distinguish on patch selection panel, but can
    // distinguish the "lagrangian" reserved path component and a mesh
    // region with the same name
    svtkStdString subCloudName;
    if (pathI[0] == '/')
    {
      subCloudName = pathI.substr(1, svtkStdString::npos);
    }
    else
    {
      subCloudName = pathI;
    }
    if (this->RegionName != pathI.substr(0, pathI.find('/')) ||
      !this->Parent->GetPatchArrayStatus(subCloudName.c_str()))
    {
      continue;
    }

    const svtkStdString cloudPath(this->CurrentTimePath() + "/" + subCloudName + "/");
    const svtkStdString positionsPath(cloudPath + "positions");

    // create an empty mesh to keep node/leaf structure of the
    // multi-block consistent even if mesh doesn't exist
    svtkPolyData* meshI = svtkPolyData::New();
    const int blockI = lagrangianMesh->GetNumberOfBlocks();
    lagrangianMesh->SetBlock(blockI, meshI);
    // extract the cloud name
    this->SetBlockName(lagrangianMesh, blockI, pathI.substr(pathI.rfind('/') + 1).c_str());

    svtkFoamIOobject io(this->CasePath, this->Parent);
    if (!(io.Open(positionsPath) || io.Open(positionsPath + ".gz")))
    {
      meshI->Delete();
      continue;
    }

    svtkFoamEntryValue dict(nullptr);
    try
    {
      if (io.GetUse64BitFloats())
      {
        dict.ReadNonuniformList<svtkFoamToken::VECTORLIST,
          svtkFoamEntryValue::vectorListTraits<svtkFloatArray, double, 3, true> >(io);
      }
      else
      {
        dict.ReadNonuniformList<svtkFoamToken::VECTORLIST,
          svtkFoamEntryValue::vectorListTraits<svtkFloatArray, float, 3, true> >(io);
      }
    }
    catch (svtkFoamError& e)
    {
      svtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of "
                    << io.GetFileName().c_str() << ": " << e.c_str());
      meshI->Delete();
      continue;
    }
    io.Close();

    svtkFloatArray* pointArray = static_cast<svtkFloatArray*>(dict.Ptr());
    const svtkIdType nParticles = pointArray->GetNumberOfTuples();

    // instantiate the points class
    svtkPoints* points = svtkPoints::New();
    points->SetData(pointArray);
    pointArray->Delete();

    // create lagrangian mesh
    meshI->AllocateEstimate(nParticles, 1);
    for (svtkIdType i = 0; i < nParticles; i++)
    {
      meshI->InsertNextCell(SVTK_VERTEX, 1, &i);
    }
    meshI->SetPoints(points);
    points->Delete();

    // read lagrangian fields
    for (int fieldI = 0; fieldI < this->LagrangianFieldFiles->GetNumberOfValues(); fieldI++)
    {
      const svtkStdString varPath(cloudPath + this->LagrangianFieldFiles->GetValue(fieldI));

      svtkFoamIOobject io2(this->CasePath, this->Parent);
      if (!io2.Open(varPath))
      {
        // if the field file doesn't exist we simply return without
        // issuing an error as a simple way of supporting multi-region
        // lagrangians
        continue;
      }

      // if the variable is disabled on selection panel then skip it
      const svtkStdString selectionName(io2.GetObjectName());
      if (this->Parent->LagrangianDataArraySelection->ArrayExists(selectionName.c_str()) &&
        !this->Parent->GetLagrangianArrayStatus(selectionName.c_str()))
      {
        continue;
      }

      // read the field file into dictionary
      svtkFoamEntryValue dict2(nullptr);
      if (!dict2.ReadField(io2))
      {
        svtkErrorMacro(<< "Error reading line " << io2.GetLineNumber() << " of "
                      << io2.GetFileName().c_str() << ": " << io2.GetError().c_str());
        continue;
      }

      // set lagrangian values
      if (dict2.GetType() != svtkFoamToken::SCALARLIST &&
        dict2.GetType() != svtkFoamToken::VECTORLIST && dict2.GetType() != svtkFoamToken::LABELLIST)
      {
        svtkErrorMacro(<< io2.GetFileName().c_str() << ": Unsupported lagrangian field type "
                      << io2.GetClassName().c_str());
        continue;
      }

      svtkDataArray* lData = static_cast<svtkDataArray*>(dict2.Ptr());

      // GetNumberOfTuples() works for both scalar and vector
      const svtkIdType nParticles2 = lData->GetNumberOfTuples();
      if (nParticles2 != meshI->GetNumberOfCells())
      {
        svtkErrorMacro(<< io2.GetFileName().c_str()
                      << ": Sizes of lagrangian mesh and field don't match: mesh = "
                      << meshI->GetNumberOfCells() << ", field = " << nParticles2);
        lData->Delete();
        continue;
      }

      this->AddArrayToFieldData(meshI->GetCellData(), lData, selectionName);
      if (this->Parent->GetCreateCellToPoint())
      {
        // questionable if this is worth bothering with:
        this->AddArrayToFieldData(meshI->GetPointData(), lData, selectionName);
      }
      lData->Delete();
    }
    meshI->Delete();
  }
  return lagrangianMesh;
}

//-----------------------------------------------------------------------------
// returns a dictionary of block names for a specified domain
svtkFoamDict* svtkOpenFOAMReaderPrivate::GatherBlocks(const char* typeIn, bool mustRead)
{
  svtkStdString type(typeIn);
  svtkStdString blockPath = this->CurrentTimeRegionMeshPath(this->PolyMeshFacesDir) + type;

  svtkFoamIOobject io(this->CasePath, this->Parent);
  if (!(io.Open(blockPath) || io.Open(blockPath + ".gz")))
  {
    if (mustRead)
    {
      svtkErrorMacro(<< "Error opening " << io.GetFileName().c_str() << ": "
                    << io.GetError().c_str());
    }
    return nullptr;
  }

  svtkFoamDict* dictPtr = new svtkFoamDict;
  svtkFoamDict& dict = *dictPtr;
  if (!dict.Read(io))
  {
    svtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of "
                  << io.GetFileName().c_str() << ": " << io.GetError().c_str());
    delete dictPtr;
    return nullptr;
  }
  if (dict.GetType() != svtkFoamToken::DICTIONARY)
  {
    svtkErrorMacro(<< "The file type of " << io.GetFileName().c_str() << " is not a dictionary");
    delete dictPtr;
    return nullptr;
  }
  return dictPtr;
}

//-----------------------------------------------------------------------------
// returns a requested point zone mesh
bool svtkOpenFOAMReaderPrivate::GetPointZoneMesh(
  svtkMultiBlockDataSet* pointZoneMesh, svtkPoints* points)
{
  bool use64BitLabels = this->Parent->GetUse64BitLabels();
  svtkFoamDict* pointZoneDictPtr = this->GatherBlocks("pointZones", false);

  if (pointZoneDictPtr == nullptr)
  {
    // not an error
    return true;
  }

  svtkFoamDict& pointZoneDict = *pointZoneDictPtr;
  int nPointZones = static_cast<int>(pointZoneDict.size());

  for (int i = 0; i < nPointZones; i++)
  {
    // look up point labels
    svtkFoamDict& dict = pointZoneDict[i]->Dictionary();
    svtkFoamEntry* pointLabelsEntry = dict.Lookup("pointLabels");
    if (pointLabelsEntry == nullptr)
    {
      delete pointZoneDictPtr;
      svtkErrorMacro(<< "pointLabels not found in pointZones");
      return false;
    }

    // allocate an empty mesh if the list is empty
    if (pointLabelsEntry->FirstValue().GetType() == svtkFoamToken::EMPTYLIST)
    {
      svtkPolyData* pzm = svtkPolyData::New();
      pointZoneMesh->SetBlock(i, pzm);
      pzm->Delete();
      // set name
      this->SetBlockName(pointZoneMesh, i, pointZoneDict[i]->GetKeyword().c_str());
      continue;
    }

    if (pointLabelsEntry->FirstValue().GetType() != svtkFoamToken::LABELLIST)
    {
      delete pointZoneDictPtr;
      svtkErrorMacro(<< "pointLabels not of type labelList: type = "
                    << pointLabelsEntry->FirstValue().GetType());
      return false;
    }

    svtkDataArray& labels = pointLabelsEntry->LabelList();

    svtkIdType nPoints = labels.GetNumberOfTuples();
    if (nPoints > this->NumPoints)
    {
      svtkErrorMacro(<< "The length of pointLabels " << nPoints << " for pointZone "
                    << pointZoneDict[i]->GetKeyword().c_str() << " exceeds the number of points "
                    << this->NumPoints);
      delete pointZoneDictPtr;
      return false;
    }

    // allocate new grid: we do not use resize() beforehand since it
    // could lead to undefined pointer if we return by error
    svtkPolyData* pzm = svtkPolyData::New();

    // set pointZone size
    pzm->AllocateEstimate(nPoints, 1);

    // insert points
    for (svtkIdType j = 0; j < nPoints; j++)
    {
      svtkIdType pointLabel =
        static_cast<svtkIdType>(GetLabelValue(&labels, j, use64BitLabels)); // must be svtkIdType
      if (pointLabel >= this->NumPoints)
      {
        svtkWarningMacro(<< "pointLabels id " << pointLabel << " exceeds the number of points "
                        << this->NumPoints);
        pzm->InsertNextCell(SVTK_EMPTY_CELL, 0, &pointLabel);
        continue;
      }
      pzm->InsertNextCell(SVTK_VERTEX, 1, &pointLabel);
    }
    pzm->SetPoints(points);

    pointZoneMesh->SetBlock(i, pzm);
    pzm->Delete();
    // set name
    this->SetBlockName(pointZoneMesh, i, pointZoneDict[i]->GetKeyword().c_str());
  }

  delete pointZoneDictPtr;

  return true;
}

//-----------------------------------------------------------------------------
// returns a requested face zone mesh
bool svtkOpenFOAMReaderPrivate::GetFaceZoneMesh(svtkMultiBlockDataSet* faceZoneMesh,
  const svtkFoamLabelVectorVector* facesPoints, svtkPoints* points)
{
  bool use64BitLabels = this->Parent->GetUse64BitLabels();
  svtkFoamDict* faceZoneDictPtr = this->GatherBlocks("faceZones", false);

  if (faceZoneDictPtr == nullptr)
  {
    // not an error
    return true;
  }

  svtkFoamDict& faceZoneDict = *faceZoneDictPtr;
  int nFaceZones = static_cast<int>(faceZoneDict.size());

  for (int i = 0; i < nFaceZones; i++)
  {
    // look up face labels
    svtkFoamDict& dict = faceZoneDict[i]->Dictionary();
    svtkFoamEntry* faceLabelsEntry = dict.Lookup("faceLabels");
    if (faceLabelsEntry == nullptr)
    {
      delete faceZoneDictPtr;
      svtkErrorMacro(<< "faceLabels not found in faceZones");
      return false;
    }

    // allocate an empty mesh if the list is empty
    if (faceLabelsEntry->FirstValue().GetType() == svtkFoamToken::EMPTYLIST)
    {
      svtkPolyData* fzm = svtkPolyData::New();
      faceZoneMesh->SetBlock(i, fzm);
      fzm->Delete();
      // set name
      this->SetBlockName(faceZoneMesh, i, faceZoneDict[i]->GetKeyword().c_str());
      continue;
    }

    if (faceLabelsEntry->FirstValue().GetType() != svtkFoamToken::LABELLIST)
    {
      delete faceZoneDictPtr;
      svtkErrorMacro(<< "faceLabels not of type labelList");
      return false;
    }

    svtkDataArray& labels = faceLabelsEntry->LabelList();

    svtkIdType nFaces = labels.GetNumberOfTuples();
    if (nFaces > this->FaceOwner->GetNumberOfTuples())
    {
      svtkErrorMacro(<< "The length of faceLabels " << nFaces << " for faceZone "
                    << faceZoneDict[i]->GetKeyword().c_str() << " exceeds the number of faces "
                    << this->FaceOwner->GetNumberOfTuples());
      delete faceZoneDictPtr;
      return false;
    }

    // allocate new grid: we do not use resize() beforehand since it
    // could lead to undefined pointer if we return by error
    svtkPolyData* fzm = svtkPolyData::New();

    // set faceZone size
    fzm->AllocateEstimate(nFaces, 1);

    // allocate array for converting int vector to svtkIdType vector:
    // workaround for 64bit machines
    svtkIdType maxNFacePoints = 0;
    for (svtkIdType j = 0; j < nFaces; j++)
    {
      svtkIdType nFacePoints = facesPoints->GetSize(GetLabelValue(&labels, j, use64BitLabels));
      if (nFacePoints > maxNFacePoints)
      {
        maxNFacePoints = nFacePoints;
      }
    }
    svtkIdList* facePointsVtkId = svtkIdList::New();
    facePointsVtkId->SetNumberOfIds(maxNFacePoints);

    // insert faces
    this->InsertFacesToGrid(fzm, facesPoints, 0, nFaces, nullptr, facePointsVtkId, &labels, false);

    facePointsVtkId->Delete();
    fzm->SetPoints(points);
    faceZoneMesh->SetBlock(i, fzm);
    fzm->Delete();
    // set name
    this->SetBlockName(faceZoneMesh, i, faceZoneDict[i]->GetKeyword().c_str());
  }

  delete faceZoneDictPtr;

  return true;
}

//-----------------------------------------------------------------------------
// returns a requested cell zone mesh
bool svtkOpenFOAMReaderPrivate::GetCellZoneMesh(svtkMultiBlockDataSet* cellZoneMesh,
  const svtkFoamLabelVectorVector* cellsFaces, const svtkFoamLabelVectorVector* facesPoints,
  svtkPoints* points)
{
  svtkFoamDict* cellZoneDictPtr = this->GatherBlocks("cellZones", false);

  if (cellZoneDictPtr == nullptr)
  {
    // not an error
    return true;
  }

  svtkFoamDict& cellZoneDict = *cellZoneDictPtr;
  int nCellZones = static_cast<int>(cellZoneDict.size());

  for (int i = 0; i < nCellZones; i++)
  {
    // look up cell labels
    svtkFoamDict& dict = cellZoneDict[i]->Dictionary();
    svtkFoamEntry* cellLabelsEntry = dict.Lookup("cellLabels");
    if (cellLabelsEntry == nullptr)
    {
      delete cellZoneDictPtr;
      svtkErrorMacro(<< "cellLabels not found in cellZones");
      return false;
    }

    // allocate an empty mesh if the list is empty
    if (cellLabelsEntry->FirstValue().GetType() == svtkFoamToken::EMPTYLIST)
    {
      svtkUnstructuredGrid* czm = svtkUnstructuredGrid::New();
      cellZoneMesh->SetBlock(i, czm);
      // set name
      this->SetBlockName(cellZoneMesh, i, cellZoneDict[i]->GetKeyword().c_str());
      continue;
    }

    if (cellLabelsEntry->FirstValue().GetType() != svtkFoamToken::LABELLIST)
    {
      delete cellZoneDictPtr;
      svtkErrorMacro(<< "cellLabels not of type labelList");
      return false;
    }

    svtkDataArray& labels = cellLabelsEntry->LabelList();

    svtkIdType nCells = labels.GetNumberOfTuples();
    if (nCells > this->NumCells)
    {
      svtkErrorMacro(<< "The length of cellLabels " << nCells << " for cellZone "
                    << cellZoneDict[i]->GetKeyword().c_str() << " exceeds the number of cells "
                    << this->NumCells);
      delete cellZoneDictPtr;
      return false;
    }

    // allocate new grid: we do not use resize() beforehand since it
    // could lead to undefined pointers if we return by error
    svtkUnstructuredGrid* czm = svtkUnstructuredGrid::New();

    // set cellZone size
    czm->Allocate(nCells);

    // insert cells
    this->InsertCellsToGrid(czm, cellsFaces, facesPoints, nullptr, nullptr, &labels);

    // set cell zone points
    czm->SetPoints(points);

    cellZoneMesh->SetBlock(i, czm);
    czm->Delete();

    // set name
    this->SetBlockName(cellZoneMesh, i, cellZoneDict[i]->GetKeyword().c_str());
  }

  delete cellZoneDictPtr;
  return true;
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReaderPrivate::AddArrayToFieldData(
  svtkDataSetAttributes* fieldData, svtkDataArray* array, const svtkStdString& arrayName)
{
  // exclude dimensional unit string if any
  const svtkStdString arrayNameString(arrayName.substr(0, arrayName.find(' ')));
  array->SetName(arrayName.c_str());

  if (array->GetNumberOfComponents() == 1 && arrayNameString == "p")
  {
    fieldData->SetScalars(array);
  }
  else if (array->GetNumberOfComponents() == 3 && arrayNameString == "U")
  {
    fieldData->SetVectors(array);
  }
  else
  {
    fieldData->AddArray(array);
  }
}

//-----------------------------------------------------------------------------
// return 0 if there's any error, 1 if success
int svtkOpenFOAMReaderPrivate::RequestData(svtkMultiBlockDataSet* output, bool recreateInternalMesh,
  bool recreateBoundaryMesh, bool updateVariables)
{
  recreateInternalMesh |= this->TimeStepOld == -1 ||
    this->InternalMeshSelectionStatus != this->InternalMeshSelectionStatusOld ||
    this->PolyMeshFacesDir->GetValue(this->TimeStep) !=
      this->PolyMeshFacesDir->GetValue(this->TimeStepOld) ||
    this->FaceOwner == nullptr;
  recreateBoundaryMesh |= recreateInternalMesh;
  updateVariables |= recreateBoundaryMesh || this->TimeStep != this->TimeStepOld;
  const bool pointsMoved = this->TimeStepOld == -1 ||
    this->PolyMeshPointsDir->GetValue(this->TimeStep) !=
      this->PolyMeshPointsDir->GetValue(this->TimeStepOld);
  const bool moveInternalPoints = !recreateInternalMesh && pointsMoved;
  const bool moveBoundaryPoints = !recreateBoundaryMesh && pointsMoved;

  // RegionName check is added since subregions have region name prefixes
  const bool createEulerians =
    this->Parent->PatchDataArraySelection->ArrayExists("internalMesh") || !this->RegionName.empty();

  // determine if we need to reconstruct meshes
  if (recreateInternalMesh)
  {
    this->ClearInternalMeshes();
  }
  if (recreateBoundaryMesh)
  {
    this->ClearBoundaryMeshes();
  }

  svtkFoamLabelVectorVector* facePoints = nullptr;
  svtkStdString meshDir;
  if (createEulerians && (recreateInternalMesh || recreateBoundaryMesh))
  {
    // create paths to polyMesh files
    meshDir = this->CurrentTimeRegionMeshPath(this->PolyMeshFacesDir);

    // create the faces vector
    facePoints = this->ReadFacesFile(meshDir);
    if (facePoints == nullptr)
    {
      return 0;
    }
    this->Parent->UpdateProgress(0.2);
  }

  svtkFoamLabelVectorVector* cellFaces = nullptr;
  if (createEulerians && recreateInternalMesh)
  {
    // read owner/neighbor and create the FaceOwner and cellFaces vectors
    cellFaces = this->ReadOwnerNeighborFiles(meshDir, facePoints);
    if (cellFaces == nullptr)
    {
      delete facePoints;
      return 0;
    }
    this->Parent->UpdateProgress(0.3);
  }

  svtkFloatArray* pointArray = nullptr;
  if (createEulerians &&
    (recreateInternalMesh ||
      (recreateBoundaryMesh && !recreateInternalMesh && this->InternalMesh == nullptr) ||
      moveInternalPoints || moveBoundaryPoints))
  {
    // get the points
    pointArray = this->ReadPointsFile();
    if ((pointArray == nullptr && recreateInternalMesh) ||
      (facePoints != nullptr && !this->CheckFacePoints(facePoints)))
    {
      delete cellFaces;
      delete facePoints;
      return 0;
    }
    this->Parent->UpdateProgress(0.4);
  }

  // make internal mesh
  // Create Internal Mesh only if required for display
  if (createEulerians && recreateInternalMesh)
  {
    if (this->Parent->GetPatchArrayStatus((this->RegionPrefix() + "internalMesh").c_str()))
    {
      this->InternalMesh = this->MakeInternalMesh(cellFaces, facePoints, pointArray);
    }
    // read and construct zones
    if (this->Parent->GetReadZones())
    {
      svtkPoints* points;
      if (this->InternalMesh != nullptr)
      {
        points = this->InternalMesh->GetPoints();
      }
      else
      {
        points = svtkPoints::New();
        points->SetData(pointArray);
      }

      this->PointZoneMesh = svtkMultiBlockDataSet::New();
      if (!this->GetPointZoneMesh(this->PointZoneMesh, points))
      {
        this->PointZoneMesh->Delete();
        this->PointZoneMesh = nullptr;
        delete cellFaces;
        delete facePoints;
        if (this->InternalMesh == nullptr)
        {
          points->Delete();
        }
        pointArray->Delete();
        return 0;
      }
      if (this->PointZoneMesh->GetNumberOfBlocks() == 0)
      {
        this->PointZoneMesh->Delete();
        this->PointZoneMesh = nullptr;
      }

      this->FaceZoneMesh = svtkMultiBlockDataSet::New();
      if (!this->GetFaceZoneMesh(this->FaceZoneMesh, facePoints, points))
      {
        this->FaceZoneMesh->Delete();
        this->FaceZoneMesh = nullptr;
        if (this->PointZoneMesh != nullptr)
        {
          this->PointZoneMesh->Delete();
          this->PointZoneMesh = nullptr;
        }
        delete cellFaces;
        delete facePoints;
        if (this->InternalMesh == nullptr)
        {
          points->Delete();
        }
        pointArray->Delete();
        return 0;
      }
      if (this->FaceZoneMesh->GetNumberOfBlocks() == 0)
      {
        this->FaceZoneMesh->Delete();
        this->FaceZoneMesh = nullptr;
      }

      this->CellZoneMesh = svtkMultiBlockDataSet::New();
      if (!this->GetCellZoneMesh(this->CellZoneMesh, cellFaces, facePoints, points))
      {
        this->CellZoneMesh->Delete();
        this->CellZoneMesh = nullptr;
        if (this->FaceZoneMesh != nullptr)
        {
          this->FaceZoneMesh->Delete();
          this->FaceZoneMesh = nullptr;
        }
        if (this->PointZoneMesh != nullptr)
        {
          this->PointZoneMesh->Delete();
          this->PointZoneMesh = nullptr;
        }
        delete cellFaces;
        delete facePoints;
        if (this->InternalMesh == nullptr)
        {
          points->Delete();
        }
        pointArray->Delete();
        return 0;
      }
      if (this->CellZoneMesh->GetNumberOfBlocks() == 0)
      {
        this->CellZoneMesh->Delete();
        this->CellZoneMesh = nullptr;
      }
      if (this->InternalMesh == nullptr)
      {
        points->Delete();
      }
    }
    delete cellFaces;
    this->TruncateFaceOwner();
  }

  if (createEulerians && recreateBoundaryMesh)
  {
    svtkFloatArray* boundaryPointArray;
    if (pointArray != nullptr)
    {
      boundaryPointArray = pointArray;
    }
    else
    {
      boundaryPointArray = static_cast<svtkFloatArray*>(this->InternalMesh->GetPoints()->GetData());
    }
    // create boundary mesh
    this->BoundaryMesh = this->MakeBoundaryMesh(facePoints, boundaryPointArray);
    if (this->BoundaryMesh == nullptr)
    {
      delete facePoints;
      if (pointArray != nullptr)
      {
        pointArray->Delete();
      }
      return 0;
    }
  }

  delete facePoints;

  // if only point coordinates change refresh point vector
  if (createEulerians && moveInternalPoints)
  {
    // refresh the points in each mesh
    svtkPoints* points;
    // Check if Internal Mesh exists first....
    if (this->InternalMesh != nullptr)
    {
      points = this->MoveInternalMesh(this->InternalMesh, pointArray);
      if (points == nullptr)
      {
        pointArray->Delete();
        return 0;
      }
    }
    else
    {
      points = svtkPoints::New();
      points->SetData(pointArray);
    }

    if (this->PointZoneMesh != nullptr)
    {
      for (unsigned int i = 0; i < this->PointZoneMesh->GetNumberOfBlocks(); i++)
      {
        svtkPolyData::SafeDownCast(this->PointZoneMesh->GetBlock(i))->SetPoints(points);
      }
    }
    if (this->FaceZoneMesh != nullptr)
    {
      for (unsigned int i = 0; i < this->FaceZoneMesh->GetNumberOfBlocks(); i++)
      {
        svtkPolyData::SafeDownCast(this->FaceZoneMesh->GetBlock(i))->SetPoints(points);
      }
    }
    if (this->CellZoneMesh != nullptr)
    {
      for (unsigned int i = 0; i < this->CellZoneMesh->GetNumberOfBlocks(); i++)
      {
        svtkUnstructuredGrid::SafeDownCast(this->CellZoneMesh->GetBlock(i))->SetPoints(points);
      }
    }
    points->Delete();
  }

  if (createEulerians && moveBoundaryPoints)
  {
    // Check if Boundary Mesh exists first....
    if (this->BoundaryMesh != nullptr)
    {
      this->MoveBoundaryMesh(this->BoundaryMesh, pointArray);
    }
  }

  if (pointArray != nullptr)
  {
    pointArray->Delete();
  }
  this->Parent->UpdateProgress(0.5);

  svtkMultiBlockDataSet* lagrangianMesh = nullptr;
  if (updateVariables)
  {
    if (createEulerians)
    {
      if (!recreateInternalMesh && this->InternalMesh != nullptr)
      {
        // clean up arrays of the previous timestep
        // Check if Internal Mesh Exists first...
        this->InternalMesh->GetCellData()->Initialize();
        this->InternalMesh->GetPointData()->Initialize();
      }
      // Check if Boundary Mesh Exists first...
      if (!recreateBoundaryMesh && this->BoundaryMesh != nullptr)
      {
        for (unsigned int i = 0; i < this->BoundaryMesh->GetNumberOfBlocks(); i++)
        {
          svtkPolyData* bm = svtkPolyData::SafeDownCast(this->BoundaryMesh->GetBlock(i));
          bm->GetCellData()->Initialize();
          bm->GetPointData()->Initialize();
        }
      }
      // read field data variables into Internal/Boundary meshes
      for (int i = 0; i < (int)this->VolFieldFiles->GetNumberOfValues(); i++)
      {
        this->GetVolFieldAtTimeStep(
          this->InternalMesh, this->BoundaryMesh, this->VolFieldFiles->GetValue(i));
        this->Parent->UpdateProgress(0.5 +
          0.25 * ((float)(i + 1) / ((float)this->VolFieldFiles->GetNumberOfValues() + 0.0001)));
      }
      for (int i = 0; i < (int)this->PointFieldFiles->GetNumberOfValues(); i++)
      {
        this->GetPointFieldAtTimeStep(
          this->InternalMesh, this->BoundaryMesh, this->PointFieldFiles->GetValue(i));
        this->Parent->UpdateProgress(0.75 +
          0.125 * ((float)(i + 1) / ((float)this->PointFieldFiles->GetNumberOfValues() + 0.0001)));
      }
    }
    // read lagrangian mesh and fields
    lagrangianMesh = this->MakeLagrangianMesh();
  }

  if (this->InternalMesh && this->Parent->CopyDataToCellZones && this->CellZoneMesh)
  {
    for (unsigned int i = 0; i < this->CellZoneMesh->GetNumberOfBlocks(); i++)
    {
      svtkUnstructuredGrid* ug = svtkUnstructuredGrid::SafeDownCast(this->CellZoneMesh->GetBlock(i));
      svtkIdTypeArray* idArray = svtkIdTypeArray::SafeDownCast(ug->GetCellData()->GetArray("CellId"));

      // allocate arrays, cellId array will be removed
      ug->GetCellData()->CopyAllocate(this->InternalMesh->GetCellData(), ug->GetNumberOfCells());

      // copy tuples
      for (svtkIdType j = 0; j < ug->GetNumberOfCells(); j++)
      {
        ug->GetCellData()->CopyData(this->InternalMesh->GetCellData(), idArray->GetValue(j), j);
      }

      // we need to add the id array because it has been previously removed
      ug->GetCellData()->AddArray(idArray);

      // copy points data
      ug->GetPointData()->ShallowCopy(this->InternalMesh->GetPointData());
    }
  }

  // Add Internal Mesh to final output only if selected for display
  if (this->InternalMesh != nullptr)
  {
    output->SetBlock(0, this->InternalMesh);
    this->SetBlockName(output, 0, "internalMesh");
  }

  // set boundary meshes/data as output
  if (this->BoundaryMesh != nullptr && this->BoundaryMesh->GetNumberOfBlocks() > 0)
  {
    const unsigned int groupTypeI = output->GetNumberOfBlocks();
    output->SetBlock(groupTypeI, this->BoundaryMesh);
    this->SetBlockName(output, groupTypeI, "Patches");
  }

  // set lagrangian mesh as output
  if (lagrangianMesh != nullptr)
  {
    if (lagrangianMesh->GetNumberOfBlocks() > 0)
    {
      const unsigned int groupTypeI = output->GetNumberOfBlocks();
      output->SetBlock(groupTypeI, lagrangianMesh);
      this->SetBlockName(output, groupTypeI, "Lagrangian Particles");
    }
    lagrangianMesh->Delete();
  }

  if (this->Parent->GetReadZones())
  {
    svtkMultiBlockDataSet* zones = nullptr;
    // set Zone Meshes as output
    if (this->PointZoneMesh != nullptr)
    {
      zones = svtkMultiBlockDataSet::New();
      const unsigned int zoneTypeI = zones->GetNumberOfBlocks();
      zones->SetBlock(zoneTypeI, this->PointZoneMesh);
      this->SetBlockName(zones, zoneTypeI, "pointZones");
    }

    if (this->FaceZoneMesh != nullptr)
    {
      if (zones == nullptr)
      {
        zones = svtkMultiBlockDataSet::New();
      }
      const unsigned int zoneTypeI = zones->GetNumberOfBlocks();
      zones->SetBlock(zoneTypeI, this->FaceZoneMesh);
      this->SetBlockName(zones, zoneTypeI, "faceZones");
    }

    if (this->CellZoneMesh != nullptr)
    {
      if (zones == nullptr)
      {
        zones = svtkMultiBlockDataSet::New();
      }
      const unsigned int zoneTypeI = zones->GetNumberOfBlocks();
      zones->SetBlock(zoneTypeI, this->CellZoneMesh);
      this->SetBlockName(zones, zoneTypeI, "cellZones");
    }
    if (zones != nullptr)
    {
      const unsigned int groupTypeI = output->GetNumberOfBlocks();
      output->SetBlock(groupTypeI, zones);
      this->SetBlockName(output, groupTypeI, "Zones");
    }
  }

  if (this->Parent->GetCacheMesh())
  {
    this->TimeStepOld = this->TimeStep;
  }
  else
  {
    this->ClearMeshes();
    this->TimeStepOld = -1;
  }
  this->InternalMeshSelectionStatusOld = this->InternalMeshSelectionStatus;

  this->Parent->UpdateProgress(1.0);
  return 1;
}

//-----------------------------------------------------------------------------
// constructor
svtkOpenFOAMReader::svtkOpenFOAMReader()
{
  this->SetNumberOfInputPorts(0);

  this->Parent = this;
  // must be false to avoid reloading by svtkAppendCompositeDataLeaves::Update()
  this->Refresh = false;

  // initialize file name
  this->FileName = nullptr;
  this->FileNameOld = new svtkStdString;

  // Case path
  this->CasePath = svtkCharArray::New();

  // Child readers
  this->Readers = svtkCollection::New();

  // SVTK CLASSES
  this->PatchDataArraySelection = svtkDataArraySelection::New();
  this->CellDataArraySelection = svtkDataArraySelection::New();
  this->PointDataArraySelection = svtkDataArraySelection::New();
  this->LagrangianDataArraySelection = svtkDataArraySelection::New();

  this->PatchSelectionMTimeOld = 0;
  this->CellSelectionMTimeOld = 0;
  this->PointSelectionMTimeOld = 0;
  this->LagrangianSelectionMTimeOld = 0;

  // for creating cell-to-point translated data
  this->CreateCellToPoint = 1;
  this->CreateCellToPointOld = 1;

  // for caching mesh
  this->CacheMesh = 1;

  // for decomposing polyhedra
  this->DecomposePolyhedra = 0;
  this->DecomposePolyhedraOld = 0;

  // for lagrangian/positions format without the additional data that existed
  // in OpenFOAM 1.4-2.4
  this->PositionsIsIn13Format = 1;
  this->PositionsIsIn13FormatOld = 1;

  // for reading zones
  this->ReadZones = 0; // turned off by default
  this->ReadZonesOld = 0;

  // Ignore 0/ time directory, which is normally missing Lagrangian fields
  this->SkipZeroTime = false;
  this->SkipZeroTimeOld = false;

  // determine if time directories are to be listed according to controlDict
  this->ListTimeStepsByControlDict = 0;
  this->ListTimeStepsByControlDictOld = 0;

  // add dimensions to array names
  this->AddDimensionsToArrayNames = 0;
  this->AddDimensionsToArrayNamesOld = 0;

  // Lagrangian paths
  this->LagrangianPaths = svtkStringArray::New();

  this->CurrentReaderIndex = 0;
  this->NumberOfReaders = 0;
  this->Use64BitLabels = false;
  this->Use64BitFloats = true;
  this->Use64BitLabelsOld = false;
  this->Use64BitFloatsOld = true;
  this->CopyDataToCellZones = false;
}

//-----------------------------------------------------------------------------
// destructor
svtkOpenFOAMReader::~svtkOpenFOAMReader()
{
  this->LagrangianPaths->Delete();

  this->PatchDataArraySelection->Delete();
  this->CellDataArraySelection->Delete();
  this->PointDataArraySelection->Delete();
  this->LagrangianDataArraySelection->Delete();

  this->Readers->Delete();
  this->CasePath->Delete();

  this->SetFileName(nullptr);
  delete this->FileNameOld;
}

//-----------------------------------------------------------------------------
// CanReadFile
int svtkOpenFOAMReader::CanReadFile(const char* svtkNotUsed(fileName))
{
  return 1; // so far CanReadFile does nothing.
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReader::SetUse64BitLabels(bool val)
{
  if (this->Use64BitLabels != val)
  {
    this->Use64BitLabels = val;
    this->Refresh = true; // Need to reread everything
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReader::SetUse64BitFloats(bool val)
{
  if (this->Use64BitFloats != val)
  {
    this->Use64BitFloats = val;
    this->Refresh = true; // Need to reread everything
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
// PrintSelf
void svtkOpenFOAMReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Refresh: " << this->Refresh << endl;
  os << indent << "CreateCellToPoint: " << this->CreateCellToPoint << endl;
  os << indent << "CacheMesh: " << this->CacheMesh << endl;
  os << indent << "DecomposePolyhedra: " << this->DecomposePolyhedra << endl;
  os << indent << "PositionsIsIn13Format: " << this->PositionsIsIn13Format << endl;
  os << indent << "ReadZones: " << this->ReadZones << endl;
  os << indent << "SkipZeroTime: " << this->SkipZeroTime << endl;
  os << indent << "ListTimeStepsByControlDict: " << this->ListTimeStepsByControlDict << endl;
  os << indent << "AddDimensionsToArrayNames: " << this->AddDimensionsToArrayNames << endl;

  this->Readers->InitTraversal();
  svtkObject* reader;
  while ((reader = this->Readers->GetNextItemAsObject()) != nullptr)
  {
    os << indent << "Reader instance " << static_cast<void*>(reader) << ": \n";
    reader->PrintSelf(os, indent.GetNextIndent());
  }
}

//-----------------------------------------------------------------------------
// selection list handlers

int svtkOpenFOAMReader::GetNumberOfSelectionArrays(svtkDataArraySelection* s)
{
  return s->GetNumberOfArrays();
}

int svtkOpenFOAMReader::GetSelectionArrayStatus(svtkDataArraySelection* s, const char* name)
{
  return s->ArrayIsEnabled(name);
}

void svtkOpenFOAMReader::SetSelectionArrayStatus(
  svtkDataArraySelection* s, const char* name, int status)
{
  svtkMTimeType mTime = s->GetMTime();
  if (status)
  {
    s->EnableArray(name);
  }
  else
  {
    s->DisableArray(name);
  }
  if (mTime != s->GetMTime()) // indicate that the pipeline needs to be updated
  {
    this->Modified();
  }
}

const char* svtkOpenFOAMReader::GetSelectionArrayName(svtkDataArraySelection* s, int index)
{
  return s->GetArrayName(index);
}

void svtkOpenFOAMReader::DisableAllSelectionArrays(svtkDataArraySelection* s)
{
  svtkMTimeType mTime = s->GetMTime();
  s->DisableAllArrays();
  if (mTime != s->GetMTime())
  {
    this->Modified();
  }
}

void svtkOpenFOAMReader::EnableAllSelectionArrays(svtkDataArraySelection* s)
{
  svtkMTimeType mTime = s->GetMTime();
  s->EnableAllArrays();
  if (mTime != s->GetMTime())
  {
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
// RequestInformation
int svtkOpenFOAMReader::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  if (!this->FileName || !*(this->FileName))
  {
    svtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  if (this->Parent == this &&
    (*this->FileNameOld != this->FileName ||
      this->ListTimeStepsByControlDict != this->ListTimeStepsByControlDictOld ||
      this->SkipZeroTime != this->SkipZeroTimeOld || this->Refresh))
  {
    // retain selection status when just refreshing a case
    if (!this->FileNameOld->empty() && *this->FileNameOld != this->FileName)
    {
      // clear selections
      this->CellDataArraySelection->RemoveAllArrays();
      this->PointDataArraySelection->RemoveAllArrays();
      this->LagrangianDataArraySelection->RemoveAllArrays();
      this->PatchDataArraySelection->RemoveAllArrays();
    }

    // Reset NumberOfReaders here so that the variable will not be
    // reset unwantedly when MakeInformationVector() is called from
    // svtkPOpenFOAMReader
    this->NumberOfReaders = 0;

    if (!this->MakeInformationVector(outputVector, svtkStdString("")) ||
      !this->MakeMetaDataAtTimeStep(true))
    {
      return 0;
    }
    this->Refresh = false;
  }
  return 1;
}

//-----------------------------------------------------------------------------
// RequestData
int svtkOpenFOAMReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int nSteps = 0;
  double requestedTimeValue(0);
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    nSteps = outInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());

    requestedTimeValue = (1 == nSteps
        // Only one time-step available, UPDATE_TIME_STEP is unreliable
        ? outInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), 0)
        : outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));
  }

  if (nSteps > 0)
  {
    outInfo->Set(svtkDataObject::DATA_TIME_STEP(), requestedTimeValue);
    this->SetTimeValue(requestedTimeValue);
  }

  if (this->Parent == this)
  {
    output->GetFieldData()->AddArray(this->CasePath);
    if (!this->MakeMetaDataAtTimeStep(false))
    {
      return 0;
    }
    this->CurrentReaderIndex = 0;
  }

  // compute flags
  // internal mesh selection change is detected within each reader
  const bool recreateInternalMesh = (!this->Parent->CacheMesh) ||
    (this->Parent->DecomposePolyhedra != this->Parent->DecomposePolyhedraOld) ||
    (this->Parent->ReadZones != this->Parent->ReadZonesOld) ||
    (this->Parent->SkipZeroTime != this->Parent->SkipZeroTimeOld) ||
    (this->Parent->ListTimeStepsByControlDict != this->Parent->ListTimeStepsByControlDictOld) ||
    (this->Parent->Use64BitLabels != this->Parent->Use64BitLabelsOld) ||
    (this->Parent->Use64BitFloats != this->Parent->Use64BitFloatsOld);

  const bool recreateBoundaryMesh =
    (this->Parent->PatchDataArraySelection->GetMTime() != this->Parent->PatchSelectionMTimeOld) ||
    (this->Parent->CreateCellToPoint != this->Parent->CreateCellToPointOld) ||
    (this->Parent->Use64BitLabels != this->Parent->Use64BitLabelsOld) ||
    (this->Parent->Use64BitFloats != this->Parent->Use64BitFloatsOld);

  const bool updateVariables =
    (this->Parent->CellDataArraySelection->GetMTime() != this->Parent->CellSelectionMTimeOld) ||
    (this->Parent->PointDataArraySelection->GetMTime() != this->Parent->PointSelectionMTimeOld) ||
    (this->Parent->LagrangianDataArraySelection->GetMTime() !=
      this->Parent->LagrangianSelectionMTimeOld) ||
    (this->Parent->PositionsIsIn13Format != this->Parent->PositionsIsIn13FormatOld) ||
    (this->Parent->AddDimensionsToArrayNames != this->Parent->AddDimensionsToArrayNamesOld) ||
    (this->Parent->Use64BitLabels != this->Parent->Use64BitLabelsOld) ||
    (this->Parent->Use64BitFloats != this->Parent->Use64BitFloatsOld);

  // create dataset
  int ret = 1;
  svtkOpenFOAMReaderPrivate* reader;
  // if the only region is not a subregion, omit being wrapped by a
  // multiblock dataset
  if (this->Readers->GetNumberOfItems() == 1 &&
    (reader = svtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetItemAsObject(0)))
      ->GetRegionName()
      .empty())
  {
    ret = reader->RequestData(output, recreateInternalMesh, recreateBoundaryMesh, updateVariables);
    this->Parent->CurrentReaderIndex++;
  }
  else
  {
    this->Readers->InitTraversal();
    while ((reader = svtkOpenFOAMReaderPrivate::SafeDownCast(
              this->Readers->GetNextItemAsObject())) != nullptr)
    {
      svtkMultiBlockDataSet* subOutput = svtkMultiBlockDataSet::New();
      if (reader->RequestData(
            subOutput, recreateInternalMesh, recreateBoundaryMesh, updateVariables))
      {
        svtkStdString regionName(reader->GetRegionName());
        if (regionName.empty())
        {
          regionName = "defaultRegion";
        }
        const int blockI = output->GetNumberOfBlocks();
        output->SetBlock(blockI, subOutput);
        output->GetMetaData(blockI)->Set(svtkCompositeDataSet::NAME(), regionName.c_str());
      }
      else
      {
        ret = 0;
      }
      subOutput->Delete();
      this->Parent->CurrentReaderIndex++;
    }
  }

  if (this->Parent == this) // update only if this is the top-level reader
  {
    this->UpdateStatus();
  }

  return ret;
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReader::SetTimeInformation(
  svtkInformationVector* outputVector, svtkDoubleArray* timeValues)
{
  double timeRange[2];
  if (timeValues->GetNumberOfTuples() > 0)
  {
    outputVector->GetInformationObject(0)->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(),
      timeValues->GetPointer(0), static_cast<int>(timeValues->GetNumberOfTuples()));

    timeRange[0] = timeValues->GetValue(0);
    timeRange[1] = timeValues->GetValue(timeValues->GetNumberOfTuples() - 1);
  }
  else
  {
    timeRange[0] = timeRange[1] = 0.0;
    outputVector->GetInformationObject(0)->Set(
      svtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeRange, 0);
  }
  outputVector->GetInformationObject(0)->Set(
    svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
}

//-----------------------------------------------------------------------------
int svtkOpenFOAMReader::MakeInformationVector(
  svtkInformationVector* outputVector, const svtkStdString& procName)
{
  *this->FileNameOld = svtkStdString(this->FileName);

  // clear prior case information
  this->Readers->RemoveAllItems();

  // recreate case information
  svtkStdString casePath, controlDictPath;
  this->CreateCasePath(casePath, controlDictPath);
  casePath += procName + (procName.empty() ? "" : "/");
  svtkOpenFOAMReaderPrivate* masterReader = svtkOpenFOAMReaderPrivate::New();
  if (!masterReader->MakeInformationVector(casePath, controlDictPath, procName, this->Parent))
  {
    masterReader->Delete();
    return 0;
  }

  if (masterReader->GetTimeValues()->GetNumberOfTuples() == 0)
  {
    svtkErrorMacro(<< this->FileName << " contains no timestep data.");
    masterReader->Delete();
    return 0;
  }

  this->Readers->AddItem(masterReader);

  if (outputVector != nullptr)
  {
    this->SetTimeInformation(outputVector, masterReader->GetTimeValues());
  }

  // search subregions under constant subdirectory
  svtkStdString constantPath(casePath + "constant/");
  svtkDirectory* dir = svtkDirectory::New();
  if (!dir->Open(constantPath.c_str()))
  {
    svtkErrorMacro(<< "Can't open " << constantPath.c_str());
    return 0;
  }
  for (int fileI = 0; fileI < dir->GetNumberOfFiles(); fileI++)
  {
    svtkStdString subDir(dir->GetFile(fileI));
    if (subDir != "." && subDir != ".." && dir->FileIsDirectory(subDir.c_str()))
    {
      svtkStdString boundaryPath(constantPath + subDir + "/polyMesh/boundary");
      if (svtksys::SystemTools::FileExists(boundaryPath.c_str(), true) ||
        svtksys::SystemTools::FileExists((boundaryPath + ".gz").c_str(), true))
      {
        svtkOpenFOAMReaderPrivate* subReader = svtkOpenFOAMReaderPrivate::New();
        subReader->SetupInformation(casePath, subDir, procName, masterReader);
        this->Readers->AddItem(subReader);
        subReader->Delete();
      }
    }
  }
  dir->Delete();
  masterReader->Delete();
  this->Parent->NumberOfReaders += this->Readers->GetNumberOfItems();

  if (this->Parent == this)
  {
    this->CreateCharArrayFromString(this->CasePath, "CasePath", casePath);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReader::CreateCasePath(svtkStdString& casePath, svtkStdString& controlDictPath)
{
#if defined(_WIN32)
  const svtkStdString pathFindSeparator = "/\\", pathSeparator = "\\";
#else
  const svtkStdString pathFindSeparator = "/", pathSeparator = "/";
#endif
  controlDictPath = this->FileName;

  // determine the case directory and path to controlDict
  svtkStdString::size_type pos = controlDictPath.find_last_of(pathFindSeparator);
  if (pos == svtkStdString::npos)
  {
    // if there's no prepending path, prefix with the current directory
    controlDictPath = "." + pathSeparator + controlDictPath;
    pos = 1;
  }
  if (controlDictPath.substr(pos + 1, 11) == "controlDict")
  {
    // remove trailing "/controlDict*"
    casePath = controlDictPath.substr(0, pos - 1);
    if (casePath == ".")
    {
      casePath = ".." + pathSeparator;
    }
    else
    {
      pos = casePath.find_last_of(pathFindSeparator);
      if (pos == svtkStdString::npos)
      {
        casePath = "." + pathSeparator;
      }
      else
      {
        // remove trailing "system" (or any other directory name)
        casePath.erase(pos + 1); // preserve the last "/"
      }
    }
  }
  else
  {
    // if the file is named other than controlDict*, use the directory
    // containing the file as case directory
    casePath = controlDictPath.substr(0, pos + 1);
    controlDictPath = casePath + "system" + pathSeparator + "controlDict";
  }
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReader::AddSelectionNames(
  svtkDataArraySelection* selections, svtkStringArray* objects)
{
  objects->Squeeze();
  svtkSortDataArray::Sort(objects);
  for (int nameI = 0; nameI < objects->GetNumberOfValues(); nameI++)
  {
    selections->AddArray(objects->GetValue(nameI).c_str());
  }
  objects->Delete();
}

//-----------------------------------------------------------------------------
bool svtkOpenFOAMReader::SetTimeValue(const double timeValue)
{
  bool modified = false;
  svtkOpenFOAMReaderPrivate* reader;
  this->Readers->InitTraversal();
  while ((reader = svtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetNextItemAsObject())) !=
    nullptr)
  {
    svtkMTimeType mTime = reader->GetMTime();
    reader->SetTimeValue(timeValue);
    if (reader->GetMTime() != mTime)
    {
      modified = true;
    }
  }
  return modified;
}

//-----------------------------------------------------------------------------
svtkDoubleArray* svtkOpenFOAMReader::GetTimeValues()
{
  if (this->Readers->GetNumberOfItems() <= 0)
  {
    return nullptr;
  }
  svtkOpenFOAMReaderPrivate* reader =
    svtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetItemAsObject(0));
  return reader != nullptr ? reader->GetTimeValues() : nullptr;
}

//-----------------------------------------------------------------------------
int svtkOpenFOAMReader::MakeMetaDataAtTimeStep(const bool listNextTimeStep)
{
  svtkStringArray* cellSelectionNames = svtkStringArray::New();
  svtkStringArray* pointSelectionNames = svtkStringArray::New();
  svtkStringArray* lagrangianSelectionNames = svtkStringArray::New();
  int ret = 1;
  svtkOpenFOAMReaderPrivate* reader;
  this->Readers->InitTraversal();
  while ((reader = svtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetNextItemAsObject())) !=
    nullptr)
  {
    ret *= reader->MakeMetaDataAtTimeStep(
      cellSelectionNames, pointSelectionNames, lagrangianSelectionNames, listNextTimeStep);
  }
  this->AddSelectionNames(this->Parent->CellDataArraySelection, cellSelectionNames);
  this->AddSelectionNames(this->Parent->PointDataArraySelection, pointSelectionNames);
  this->AddSelectionNames(this->Parent->LagrangianDataArraySelection, lagrangianSelectionNames);

  return ret;
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReader::CreateCharArrayFromString(
  svtkCharArray* array, const char* name, svtkStdString& string)
{
  array->Initialize();
  array->SetName(name);
  const size_t len = string.length();
  char* ptr = array->WritePointer(0, static_cast<svtkIdType>(len + 1));
  memcpy(ptr, string.c_str(), len);
  ptr[len] = '\0';
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReader::UpdateStatus()
{
  // update selection MTimes
  this->PatchSelectionMTimeOld = this->PatchDataArraySelection->GetMTime();
  this->CellSelectionMTimeOld = this->CellDataArraySelection->GetMTime();
  this->PointSelectionMTimeOld = this->PointDataArraySelection->GetMTime();
  this->LagrangianSelectionMTimeOld = this->LagrangianDataArraySelection->GetMTime();
  this->CreateCellToPointOld = this->CreateCellToPoint;
  this->DecomposePolyhedraOld = this->DecomposePolyhedra;
  this->PositionsIsIn13FormatOld = this->PositionsIsIn13Format;
  this->ReadZonesOld = this->ReadZones;
  this->SkipZeroTimeOld = this->SkipZeroTime;
  this->ListTimeStepsByControlDictOld = this->ListTimeStepsByControlDict;
  this->AddDimensionsToArrayNamesOld = this->AddDimensionsToArrayNames;
  this->Use64BitLabelsOld = this->Use64BitLabels;
  this->Use64BitFloatsOld = this->Use64BitFloats;
}

//-----------------------------------------------------------------------------
void svtkOpenFOAMReader::UpdateProgress(double amount)
{
  this->svtkAlgorithm::UpdateProgress(
    (static_cast<double>(this->Parent->CurrentReaderIndex) + amount) /
    static_cast<double>(this->Parent->NumberOfReaders));
}
