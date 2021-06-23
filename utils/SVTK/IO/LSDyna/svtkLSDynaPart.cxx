/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLSDynaPart.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "svtkLSDynaPart.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkStringArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include <algorithm>
#include <map>
#include <vector>

namespace
{

static const char* TypeNames[] = { "PARTICLE", "BEAM", "SHELL", "THICK_SHELL", "SOLID",
  "RIGID_BODY", "ROAD_SURFACE", nullptr };

typedef std::vector<bool> BitVector;

}

//-----------------------------------------------------------------------------
// lightweight class that holds the cell properties
class svtkLSDynaPart::InternalCellProperties
{
protected:
  class CellProperty
  {
  public:
    template <typename T>
    CellProperty(T, const int& sp, const svtkIdType& numTuples, const svtkIdType& nc)
      : startPos(sp)
      , numComps(nc)
    {
      Data = new unsigned char[numTuples * nc * sizeof(T)];
      loc = Data;
      len = numComps * sizeof(T);
    }
    ~CellProperty() { delete[] Data; }
    template <typename T>
    void insertNextTuple(T* values)
    {
      memcpy(loc, values + startPos, len);
      loc = ((T*)loc) + numComps;
    }
    void resetForNextTimeStep() { loc = Data; }

    unsigned char* Data;

  protected:
    int startPos;
    size_t len;
    svtkIdType numComps;
    void* loc;
  };

public:
  InternalCellProperties()
    : DeadCells(nullptr)
    , DeadIndex(0)
    , UserIds(nullptr)
    , UserIdIndex(0)
  {
  }

  ~InternalCellProperties()
  {
    std::vector<CellProperty*>::iterator it;
    for (it = Properties.begin(); it != Properties.end(); ++it)
    {
      delete (*it);
      (*it) = nullptr;
    }
    this->Properties.clear();

    delete[] this->DeadCells;
    delete[] this->UserIds;
  }

  bool NoDeadCells() const { return DeadCells == nullptr; }
  bool NoUserIds() const { return UserIds == nullptr; }

  template <typename T>
  void* AddProperty(const int& offset, const int& numTuples, const int& numComps)
  {
    CellProperty* prop = new CellProperty(T(), offset, numTuples, numComps);
    this->Properties.push_back(prop);

    // return the location to set the void pointer too
    return prop->Data;
  }

  template <typename T>
  void AddCellInfo(T* cellproperty)
  {
    std::vector<CellProperty*>::iterator it;
    for (it = Properties.begin(); it != Properties.end(); ++it)
    {
      (*it)->insertNextTuple(cellproperty);
    }
  }

  void SetDeadCells(unsigned char* dead, const svtkIdType& size)
  {
    memcpy(this->DeadCells + this->DeadIndex, dead, sizeof(unsigned char) * size);
    this->DeadIndex += size;
  }

  bool IsCellDead(const svtkIdType& index) const { return this->DeadCells[index] == 0; }

  void SetNextUserId(const svtkIdType& id) { this->UserIds[this->UserIdIndex++] = id; }

  void SetDeadCellArray(unsigned char* gc)
  {
    this->DeadCells = gc;
    this->DeadIndex = 0;
  }

  void SetMaterialIdArray(svtkIdType* ids)
  {
    this->UserIds = ids;
    this->UserIdIndex = 0;
  }

  void ResetForNextTimeStep()
  {
    this->DeadIndex = 0;
    this->UserIdIndex = 0;
    std::vector<CellProperty*>::iterator it;
    for (it = Properties.begin(); it != Properties.end(); ++it)
    {
      (*it)->resetForNextTimeStep();
    }
  }

  void* GetDeadVoidPtr() { return static_cast<void*>(this->DeadCells); }

protected:
  std::vector<CellProperty*> Properties;

  // the two cell data arrays that aren't packed with cell state info
  unsigned char* DeadCells;
  svtkIdType DeadIndex;

  svtkIdType* UserIds;
  svtkIdType UserIdIndex;
};

//-----------------------------------------------------------------------------
class svtkLSDynaPart::InternalCells
{
  // lightweight class that holds the cell topology.In buildTopology
  // we will set the unstructured grid pointers to look at these vectors
public:
  size_t size() const { return types.size(); }
  size_t dataSize() const { return data.size(); }

  void add(const int& cellType, const svtkIdType& npts, svtkIdType conn[8])
  {
    types.push_back(static_cast<unsigned char>(cellType));

    data.push_back(npts); // add in the num of points
    locations.push_back(static_cast<svtkIdType>(data.size() - 1));
    data.insert(data.end(), conn, conn + npts);
  }

  void reserve(const svtkIdType& numCells, const svtkIdType& dataLen)
  {
    types.reserve(numCells);
    locations.reserve(numCells);
    // data len only holds total number of points across the cells
    data.reserve(numCells + dataLen);
  }

  std::vector<unsigned char> types;
  std::vector<svtkIdType> locations;
  std::vector<svtkIdType> data;
};

//-----------------------------------------------------------------------------
class svtkLSDynaPart::InternalPointsUsed
{
  // Base class that tracks which points this part uses
public:
  // uses the relative index based on the minId
  InternalPointsUsed(const svtkIdType& min, const svtkIdType& max)
    : MinId(min)
    , MaxId(max + 1)
  {
  } // maxId is meant to be exclusive

  virtual ~InternalPointsUsed() = default;

  virtual bool isUsed(const svtkIdType& index) const = 0;

  // the min and max id allow the parts to be sorted in the collection
  // based on the points they need to allow for subsections of the global point
  // array to be sent to only parts that use it
  svtkIdType minId() const { return MinId; }
  svtkIdType maxId() const { return MaxId; }

protected:
  svtkIdType MinId;
  svtkIdType MaxId;
};

//-----------------------------------------------------------------------------
class svtkLSDynaPart::DensePointsUsed : public svtkLSDynaPart::InternalPointsUsed
{
  // uses a min and max id to bound the bit vector of points that this part
  // uses. If the points for the part are all bunched up in the global point
  // space this is used as it saves tons of space.
public:
  DensePointsUsed(BitVector* pointsUsed, const svtkIdType& min, const svtkIdType& max)
    : InternalPointsUsed(min, max)
    , UsedPoints(pointsUsed->begin() + min, pointsUsed->begin() + (max + 1))
  {
  }

  bool isUsed(const svtkIdType& index) const override { return UsedPoints[index]; }

protected:
  BitVector UsedPoints;
};

//-----------------------------------------------------------------------------
class svtkLSDynaPart::SparsePointsUsed : public svtkLSDynaPart::InternalPointsUsed
{
  // uses a set to store highly unrelated points. I doubt this is used by
  // many parts as the part would need to use a few points whose indices was
  // at the extremes of the global point set
public:
  SparsePointsUsed(BitVector* pointsUsed, const svtkIdType& min, const svtkIdType& max)
    : InternalPointsUsed(min, max)
  {
    for (svtkIdType i = this->MinId; i < this->MaxId; ++i)
    {
      // we need relative ids
      if ((*pointsUsed)[i])
      {
        this->UsedPoints.insert(i - this->MinId);
      }
    }
  }
  bool isUsed(const svtkIdType& index) const override
  {
    return this->UsedPoints.find(index) != this->UsedPoints.end();
  }

protected:
  std::set<svtkIdType> UsedPoints;
};

//-----------------------------------------------------------------------------
class svtkLSDynaPart::InternalCurrentPointInfo
{
public:
  InternalCurrentPointInfo()
    : ptr(nullptr)
    , index(0)
  {
  }
  void* ptr;
  svtkIdType index;
};

svtkStandardNewMacro(svtkLSDynaPart);

//-----------------------------------------------------------------------------
svtkLSDynaPart::svtkLSDynaPart()
{
  this->Cells = new svtkLSDynaPart::InternalCells();
  this->CellProperties = new svtkLSDynaPart::InternalCellProperties();
  this->CurrentPointPropInfo = new svtkLSDynaPart::InternalCurrentPointInfo();
  this->GlobalPointsUsed = nullptr;

  this->Type = LSDynaMetaData::NUM_CELL_TYPES;
  this->Name = svtkStdString();
  this->UserMaterialId = -1;
  this->PartId = -1;

  this->NumberOfCells = -1;
  this->NumberOfPoints = -1;

  this->DeadCellsAsGhostArray = false;
  this->HasDeadCells = false;
  this->TopologyBuilt = false;
  this->DoubleBased = true;

  this->Grid = nullptr;
  this->ThresholdGrid = nullptr;
  this->Points = nullptr;
}

//-----------------------------------------------------------------------------
svtkLSDynaPart::~svtkLSDynaPart()
{
  delete this->Cells;
  delete this->CellProperties;
  delete this->CurrentPointPropInfo;

  if (Grid)
  {
    Grid->Delete();
    Grid = nullptr;
  }
  if (Points)
  {
    Points->Delete();
    Points = nullptr;
  }
  delete this->GlobalPointsUsed;
  if (this->ThresholdGrid)
  {
    this->ThresholdGrid->Delete();
  }
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "Type " << this->Type << "(" << TypeNames[this->Type] << ")" << endl;
  os << indent << "Name " << this->Name << endl;
  os << indent << "UserMaterialId " << this->UserMaterialId << endl;
  os << indent << "Number of Cells " << this->NumberOfCells << endl;
  os << indent << "Number of Points " << this->NumberOfPoints << endl;
  os << indent << "TopologyBuilt" << this->TopologyBuilt << endl;
}

//-----------------------------------------------------------------------------
bool svtkLSDynaPart::HasCells() const
{
  return this->Cells->size() > 0;
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::InitPart(svtkStdString name, const svtkIdType& partId, const svtkIdType& userMatId,
  const svtkIdType& numGlobalPoints, const int& sizeOfWord)
{
  // we don't know intill we read the material section
  // which type of a part we are. This is because
  // when using user material ids they are in Id sorted order
  // not in order based on the part type
  this->Name = name;
  this->PartId = partId;
  this->UserMaterialId = userMatId;
  this->DoubleBased = (sizeOfWord == 8);
  this->NumberOfGlobalPoints = numGlobalPoints;

  this->GlobalPointsUsed = nullptr;

  this->Grid = svtkUnstructuredGrid::New();
  this->Points = svtkPoints::New();

  this->Grid->SetPoints(this->Points);

  // now add in the field data to the grid.
  // Data is the name, type, and material id
  svtkFieldData* fd = this->Grid->GetFieldData();

  svtkStringArray* partName = svtkStringArray::New();
  partName->SetName("Name");
  partName->SetNumberOfValues(1);
  partName->SetValue(0, this->Name);
  fd->AddArray(partName);
  partName->FastDelete();

  svtkStringArray* partType = svtkStringArray::New();
  partType->SetName("Type");
  partType->SetNumberOfValues(1);
  partType->SetValue(0, TypeNames[this->Type]);
  fd->AddArray(partType);
  partType->FastDelete();

  svtkIntArray* materialId = svtkIntArray::New();
  materialId->SetName("Material Id");
  materialId->SetNumberOfValues(1);
  materialId->SetValue(0, this->UserMaterialId);
  fd->AddArray(materialId);
  materialId->FastDelete();
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::SetPartType(int type)
{
  switch (type)
  {
    case 0:
      this->Type = LSDynaMetaData::PARTICLE;
      break;
    case 1:
      this->Type = LSDynaMetaData::BEAM;
      break;
    case 2:
      this->Type = LSDynaMetaData::SHELL;
      break;
    case 3:
      this->Type = LSDynaMetaData::THICK_SHELL;
      break;
    case 4:
      this->Type = LSDynaMetaData::SOLID;
      break;
    case 5:
      this->Type = LSDynaMetaData::RIGID_BODY;
      break;
    case 6:
      this->Type = LSDynaMetaData::ROAD_SURFACE;
      break;
    default:
      svtkErrorMacro("Invalid Part Type set");
      break;
  }
}

//-----------------------------------------------------------------------------
bool svtkLSDynaPart::hasValidType() const
{
  return (this->Type >= LSDynaMetaData::PARTICLE && this->Type <= LSDynaMetaData::ROAD_SURFACE);
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::AllocateCellMemory(const svtkIdType& numCells, const svtkIdType& cellLen)
{
  this->Cells->reserve(numCells, cellLen);
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::AddCell(const int& cellType, const svtkIdType& npts, svtkIdType conn[8])
{
  this->Cells->add(cellType, npts, conn);
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::BuildToplogy()
{
  // determine the number of points that this part has
  // and what points those are in the global point map
  // fixup the cell topology to use the local parts point ids
  // This must come before BuildCells since it remaps the point ids in the
  // connectivity structures.
  this->BuildUniquePoints();

  // make the unstrucuted grid data point to the Cells memory
  this->BuildCells();

  this->TopologyBuilt = true;
}

//-----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkLSDynaPart::GenerateGrid()
{
  this->CellProperties->ResetForNextTimeStep();

  // we have to mark all the properties as modified so the information
  // tab will be at the correct values
  svtkCellData* cd = this->Grid->GetCellData();
  int numArrays = cd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; ++i)
  {
    cd->GetArray(i)->Modified();
  }

  this->Points->Modified();
  svtkPointData* pd = this->Grid->GetPointData();
  numArrays = pd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; ++i)
  {
    pd->GetArray(i)->Modified();
  }

  if (!this->HasDeadCells || this->DeadCellsAsGhostArray)
  {
    return this->Grid;
  }
  else
  {
    // we threshold the datset on the ghost cells and return
    // the new dataset
    return this->RemoveDeletedCells();
  }
}

//-----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkLSDynaPart::RemoveDeletedCells()
{
  if (this->ThresholdGrid)
  {
    this->ThresholdGrid->Delete();
  }
  this->ThresholdGrid = svtkUnstructuredGrid::New();
  this->ThresholdGrid->Allocate(this->NumberOfCells);

  // copy field data
  this->ThresholdGrid->SetFieldData(this->Grid->GetFieldData());

  svtkPointData* oldPd = this->Grid->GetPointData();
  svtkPointData* pd = this->ThresholdGrid->GetPointData();
  pd->CopyGlobalIdsOn();
  pd->CopyAllocate(oldPd);

  svtkCellData* oldCd = this->Grid->GetCellData();
  svtkCellData* cd = this->ThresholdGrid->GetCellData();
  cd->CopyGlobalIdsOn();
  cd->CopyAllocate(oldCd);

  svtkPoints* newPoints = svtkPoints::New();
  if (this->DoubleBased)
  {
    newPoints->SetDataTypeToDouble();
  }
  else
  {
    newPoints->SetDataTypeToFloat();
  }
  newPoints->Allocate(this->NumberOfPoints);

  svtkIdList* pointMap = svtkIdList::New();
  pointMap->SetNumberOfIds(this->NumberOfPoints);
  for (svtkIdType i = 0; i < this->NumberOfPoints; ++i)
  {
    pointMap->SetId(i, -1);
  }

  double pt[3];
  svtkIdType numCellPts = 0, ptId = 0, newId = 0, newCellId = 0;
  svtkIdList* newCellPts = svtkIdList::New();
  svtkIdList* cellPts = nullptr;
  for (svtkIdType cellId = 0; cellId < this->NumberOfCells; ++cellId)
  {
    svtkCell* cell = this->Grid->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    if (this->CellProperties->IsCellDead(cellId) && numCellPts > 0)
    {
      for (svtkIdType i = 0; i < numCellPts; i++)
      {
        ptId = cellPts->GetId(i);
        if ((newId = pointMap->GetId(ptId)) < 0)
        {
          this->Grid->GetPoint(ptId, pt);
          newId = newPoints->InsertNextPoint(pt);
          pointMap->SetId(ptId, newId);
          pd->CopyData(oldPd, ptId, newId);
        }
        newCellPts->InsertId(i, newId);
      }
      newCellId = this->ThresholdGrid->InsertNextCell(cell->GetCellType(), newCellPts);
      cd->CopyData(oldCd, cellId, newCellId);
      newCellPts->Reset();
    }
  }

  pointMap->Delete();
  newCellPts->Delete();

  this->ThresholdGrid->SetPoints(newPoints);
  newPoints->FastDelete();

  this->ThresholdGrid->Squeeze();
  cd->RemoveArray(svtkDataSetAttributes::GhostArrayName());

  return this->ThresholdGrid;
}
//-----------------------------------------------------------------------------
void svtkLSDynaPart::EnableDeadCells(const int& deadCellsAsGhostArray)
{
  this->HasDeadCells = true;
  this->DeadCellsAsGhostArray = deadCellsAsGhostArray == 1;
  if (this->CellProperties->NoDeadCells())
  {
    // we are using the ghost levels to hide cells that have been
    // classified as dead, rather than the intended purpose
    unsigned char* dead = new unsigned char[this->NumberOfCells];

    // the cell properties will delete the ghost array when needed
    this->CellProperties->SetDeadCellArray(dead);
  }

  if (!this->Grid->GetCellData()->HasArray(svtkDataSetAttributes::GhostArrayName()))
  {
    svtkUnsignedCharArray* deadCells = svtkUnsignedCharArray::New();
    deadCells->SetName(svtkDataSetAttributes::GhostArrayName());
    deadCells->SetVoidArray(this->CellProperties->GetDeadVoidPtr(), this->NumberOfCells, 1);

    this->Grid->GetCellData()->AddArray(deadCells);
    deadCells->FastDelete();
  }
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::DisableDeadCells()
{
  this->HasDeadCells = false;
  if (this->Grid->GetCellData()->HasArray(svtkDataSetAttributes::GhostArrayName()))
  {
    this->Grid->GetCellData()->RemoveArray(svtkDataSetAttributes::GhostArrayName());
  }
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::SetCellsDeadState(unsigned char* dead, const svtkIdType& size)
{
  // presumes the HideDeletedCells is true, doesn't check for speed
  this->CellProperties->SetDeadCells(dead, size);
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::EnableCellUserIds()
{
  if (this->CellProperties->NoUserIds())
  {
    svtkIdType* ids = new svtkIdType[this->NumberOfCells];

    // the cell properties will delete the ghost array when needed
    this->CellProperties->SetMaterialIdArray(ids);

    svtkIdTypeArray* userIds = svtkIdTypeArray::New();
    userIds->SetName("UserIds");
    userIds->SetVoidArray(ids, this->NumberOfCells, 1);
    this->Grid->GetCellData()->SetGlobalIds(userIds);
    userIds->FastDelete();
  }
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::SetNextCellUserIds(const svtkIdType& value)
{
  this->CellProperties->SetNextUserId(value);
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::AddPointProperty(const char* name, const svtkIdType& numComps,
  const bool& isIdTypeProperty, const bool& isProperty, const bool& isGeometryPoints)
{
  // adding a point property means that this is the next property
  // we are going to be reading from file

  // first step is getting the ptr to the start of the right property
  this->GetPropertyData(name, numComps, isIdTypeProperty, isProperty, isGeometryPoints);
  this->CurrentPointPropInfo->index = 0;
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::ReadPointBasedProperty(float* data, const svtkIdType& numTuples,
  const svtkIdType& numComps, const svtkIdType& currentGlobalPointIndex)
{
  float* ptr = static_cast<float*>(this->CurrentPointPropInfo->ptr);
  this->AddPointInformation(data, ptr, numTuples, numComps, currentGlobalPointIndex);
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::ReadPointBasedProperty(double* data, const svtkIdType& numTuples,
  const svtkIdType& numComps, const svtkIdType& currentGlobalPointIndex)
{
  double* ptr = static_cast<double*>(this->CurrentPointPropInfo->ptr);
  this->AddPointInformation(data, ptr, numTuples, numComps, currentGlobalPointIndex);
}

//-----------------------------------------------------------------------------
template <typename T>
void svtkLSDynaPart::AddPointInformation(T* buffer, T* pointData, const svtkIdType& numTuples,
  const svtkIdType& numComps, const svtkIdType& currentGlobalIndex)
{
  // only read the subset of points of this part that fall
  // inside the src buffer
  svtkIdType start(std::max(this->GlobalPointsUsed->minId(), currentGlobalIndex));
  svtkIdType end(std::min(this->GlobalPointsUsed->maxId(), currentGlobalIndex + numTuples));

  // if the part has no place in this section of the points buffer
  // end will be larger than start
  if (start >= end)
  {
    return;
  }

  // offset all the pointers to the correct place
  T* src = buffer + ((start - currentGlobalIndex) * numComps);
  T* dest = pointData + (this->CurrentPointPropInfo->index * numComps);
  const size_t msize = sizeof(T) * numComps;

  // fix the start and end to be relative to the min id
  // this is because the global point used class is relative index based
  start -= this->GlobalPointsUsed->minId();
  end -= this->GlobalPointsUsed->minId();
  svtkIdType numPointsRead = 0;
  for (; start < end; ++start, src += numComps)
  {

    if (this->GlobalPointsUsed->isUsed(start))
    {
      memcpy(dest, src, msize);
      dest += numComps;
      ++numPointsRead;
    }
  }

  this->CurrentPointPropInfo->index += numPointsRead;
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::GetPropertyData(const char* name, const svtkIdType& numComps,
  const bool& isIdTypeProperty, const bool& isProperty, const bool& isGeometry)
{
  this->CurrentPointPropInfo->ptr = nullptr;
  svtkDataArray* data = nullptr;
  if (isProperty)
  {
    data = this->Grid->GetPointData()->GetArray(name);
    if (!data)
    {
      // we have to construct the data array first
      if (!isIdTypeProperty)
      {
        data = (this->DoubleBased) ? (svtkDataArray*)svtkDoubleArray::New()
                                   : (svtkDataArray*)svtkFloatArray::New();
        this->Grid->GetPointData()->AddArray(data);
      }
      else
      {
        // the exception of the point arrays is the idType array which is
        data = svtkIdTypeArray::New();
        this->Grid->GetPointData()->SetGlobalIds(data);
      }
      data->SetName(name);
      data->SetNumberOfComponents(numComps);
      data->SetNumberOfTuples(this->NumberOfPoints);
      data->FastDelete();
    }
  }
  if (isGeometry)
  {
    if (this->DoubleBased)
    {
      this->Points->SetDataTypeToDouble();
    }
    else
    {
      this->Points->SetDataTypeToFloat();
    }

    if (data)
    {
      // this is the deflection array and needs to be set as the points
      // array
      this->Points->SetData(data);
    }
    else
    {
      // this is a pure geometry array and nothing else
      this->Points->SetNumberOfPoints(this->NumberOfPoints);
      data = this->Points->GetData();
    }
  }
  this->CurrentPointPropInfo->ptr = data->GetVoidPointer(0);
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::AddCellProperty(const char* name, const int& offset, const int& numComps)
{
  if (this->Grid->GetCellData()->HasArray(name))
  {
    // we only have to fill the cell properties class the first
    // time step after creating the part, the reset of the time
    // we are just changing the value in the data arrays
    return;
  }

  svtkDataArray* data = nullptr;
  void* ptr = nullptr;
  if (this->DoubleBased)
  {
    ptr = this->CellProperties->AddProperty<double>(offset, this->NumberOfCells, numComps);
  }
  else
  {
    ptr = this->CellProperties->AddProperty<float>(offset, this->NumberOfCells, numComps);
  }

  if (ptr)
  {
    data = (this->DoubleBased) ? (svtkDataArray*)svtkDoubleArray::New()
                               : (svtkDataArray*)svtkFloatArray::New();

    // we will manage the memory that the cell property points too
    data->SetNumberOfComponents(numComps);
    data->SetVoidArray(ptr, this->NumberOfCells * numComps, 1);
    data->SetName(name);
    this->Grid->GetCellData()->AddArray(data);
    data->FastDelete();
  }
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::ReadCellProperties(
  float* cellProperties, const svtkIdType& numCells, const svtkIdType& numPropertiesInCell)
{
  float* cell = cellProperties;
  for (svtkIdType i = 0; i < numCells; ++i)
  {
    this->CellProperties->AddCellInfo(cell);
    cell += numPropertiesInCell;
  }
}
//-----------------------------------------------------------------------------
void svtkLSDynaPart::ReadCellProperties(
  double* cellProperties, const svtkIdType& numCells, const svtkIdType& numPropertiesInCell)
{
  double* cell = cellProperties;
  for (svtkIdType i = 0; i < numCells; ++i)
  {
    this->CellProperties->AddCellInfo(cell);
    cell += numPropertiesInCell;
  }
}

//-----------------------------------------------------------------------------
svtkIdType svtkLSDynaPart::GetMinGlobalPointId() const
{
  // presumes topology has been built already
  return this->GlobalPointsUsed->minId();
}

//-----------------------------------------------------------------------------
svtkIdType svtkLSDynaPart::GetMaxGlobalPointId() const
{
  // presumes topology has been built already
  return this->GlobalPointsUsed->maxId();
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::BuildCells()
{
  this->NumberOfCells = static_cast<svtkIdType>(this->Cells->size());

  // make the unstrucuted grid data structures point to the
  // Cells vectors underlying memory
  svtkIdType cellDataSize = static_cast<svtkIdType>(this->Cells->dataSize());

  // copy the contents from the part into a cell array.
  svtkIdTypeArray* cellArray = svtkIdTypeArray::New();
  cellArray->SetVoidArray(&this->Cells->data[0], cellDataSize, 1);

  // set the idtype array as the cellarray
  svtkCellArray* cells = svtkCellArray::New();
  cells->ImportLegacyFormat(cellArray);
  cellArray->FastDelete();

  // now copy the cell types from the vector to
  svtkUnsignedCharArray* cellTypes = svtkUnsignedCharArray::New();
  cellTypes->SetVoidArray(&this->Cells->types[0], this->NumberOfCells, 1);

  // actually set up the grid
  this->Grid->SetCells(cellTypes, cells, nullptr, nullptr);

  // remove references
  cellTypes->FastDelete();
  cells->FastDelete();
}

//-----------------------------------------------------------------------------
void svtkLSDynaPart::BuildUniquePoints()
{

  // we need to determine the number of unique points in this part
  // walk the cell structure to find all the unique points

  std::vector<svtkIdType>::const_iterator cellIt;
  std::vector<svtkIdType>::iterator cIt;

  BitVector pointUsage(this->NumberOfGlobalPoints, false);
  this->NumberOfPoints = 0;
  for (cellIt = this->Cells->data.begin(); cellIt != this->Cells->data.end();)
  {
    const svtkIdType npts(*cellIt);
    ++cellIt;
    for (svtkIdType i = 0; i < npts; ++i, ++cellIt)
    {
      const svtkIdType id((*cellIt) - 1);
      if (!pointUsage[id])
      {
        pointUsage[id] = true;
        ++this->NumberOfPoints; // get the number of unique points
      }
    }
  }

  // find the min and max points used
  svtkIdType min = this->NumberOfGlobalPoints + 1;
  svtkIdType max = -1;
  svtkIdType pos = 0, numPointsFound = 0;
  for (BitVector::const_iterator constIt = pointUsage.begin(); constIt != pointUsage.end();
       ++constIt, ++pos)
  {
    if (*constIt)
    {
      ++numPointsFound;
    }
    if (numPointsFound == 1 && min > pos)
    {
      min = pos;
    }
    if (numPointsFound == this->NumberOfPoints)
    {
      max = pos;
      break; // we iterated long enough
    }
  }

  // we do a two phase because we can minimize memory usage
  // we should make this a class like DensePointsUsed since
  // we can use the ratio to determine if a vector or a map is more
  // space efficient
  std::vector<svtkIdType> uniquePoints;
  const svtkIdType size(1 + max - min);
  uniquePoints.resize(size, -1);

  svtkIdType idx = 0;
  pos = 0;
  for (svtkIdType i = min; i <= max; ++i, ++idx)
  {
    if (pointUsage[i])
    {
      uniquePoints[idx] = pos++;
    }
  }

  // now fixup the cellIds
  for (cIt = this->Cells->data.begin(); cIt != this->Cells->data.end();)
  {
    const svtkIdType npts(*cIt);
    ++cIt;
    for (svtkIdType i = 0; i < npts; ++i, ++cIt)
    {
      const svtkIdType oId((*cIt) - min - 1);
      *cIt = uniquePoints[oId];
    }
  }

  // determine the type of global point id storage is best
  svtkIdType ratio = (this->NumberOfPoints * sizeof(svtkIdType)) / (max - min);
  if (ratio > 0)
  {
    // the size of the bit array is less than the size of each number in memory
    // by it self
    this->GlobalPointsUsed = new svtkLSDynaPart::DensePointsUsed(&pointUsage, min, max);
  }
  else
  {
    this->GlobalPointsUsed = new svtkLSDynaPart::SparsePointsUsed(&pointUsage, min, max);
  }
}
