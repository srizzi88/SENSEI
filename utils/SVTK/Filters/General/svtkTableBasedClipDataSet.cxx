/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableBasedClipDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*****************************************************************************
 *
 * Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
 * Produced at the Lawrence Livermore National Laboratory
 * LLNL-CODE-400124
 * All rights reserved.
 *
 * This file was adapted from the VisIt clipper (svtkVisItClipper). For  details,
 * see https://visit.llnl.gov/.  The full copyright notice is contained in the
 * file COPYRIGHT located at the root of the VisIt distribution or at
 * http://www.llnl.gov/visit/copyright.html.
 *
 *****************************************************************************/

#include "svtkTableBasedClipDataSet.h"

#include "svtkCallbackCommand.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtkIncrementalPointLocator.h"
#include "svtkMergePoints.h"

#include "svtkClipDataSet.h"
#include "svtkImplicitFunction.h"
#include "svtkPlane.h"

#include "svtkAppendFilter.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkImageData.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include "svtkTableBasedClipCases.cxx"

svtkStandardNewMacro(svtkTableBasedClipDataSet);
svtkCxxSetObjectMacro(svtkTableBasedClipDataSet, ClipFunction, svtkImplicitFunction);

// ============================================================================
// ============== svtkTableBasedClipperDataSetFromVolume (begin) ===============
// ============================================================================

struct TableBasedClipperPointEntry
{
  svtkIdType ptIds[2];
  double percent;
};

// ---- svtkTableBasedClipperPointList (bein)
class svtkTableBasedClipperPointList
{
public:
  svtkTableBasedClipperPointList();
  virtual ~svtkTableBasedClipperPointList();

  svtkIdType AddPoint(svtkIdType, svtkIdType, double);
  svtkIdType GetTotalNumberOfPoints() const;
  int GetNumberOfLists() const;
  int GetList(svtkIdType, const TableBasedClipperPointEntry*&) const;

protected:
  svtkIdType currentList;
  svtkIdType currentPoint;
  int listSize;
  int pointsPerList;
  TableBasedClipperPointEntry** list;
};
// ---- svtkTableBasedClipperPointList (end)

// ---- svtkTableBasedClipperEdgeHashEntry (begin)
class svtkTableBasedClipperEdgeHashEntry
{
public:
  svtkTableBasedClipperEdgeHashEntry();
  virtual ~svtkTableBasedClipperEdgeHashEntry() = default;

  int GetPointId() { return ptId; }
  void SetInfo(int, int, int);
  void SetNext(svtkTableBasedClipperEdgeHashEntry* n) { next = n; }
  bool IsMatch(int i1, int i2) { return (i1 == id1 && i2 == id2 ? true : false); }

  svtkTableBasedClipperEdgeHashEntry* GetNext() { return next; }

protected:
  int id1, id2;
  int ptId;
  svtkTableBasedClipperEdgeHashEntry* next;
};
// ---- svtkTableBasedClipperEdgeHashEntry (end)

// ---- svtkTableBasedClipperEdgeHashEntryMemoryManager (begin)
#define FREE_ENTRY_LIST_SIZE 16384
#define POOL_SIZE 256
class svtkTableBasedClipperEdgeHashEntryMemoryManager
{
public:
  svtkTableBasedClipperEdgeHashEntryMemoryManager();
  virtual ~svtkTableBasedClipperEdgeHashEntryMemoryManager();

  inline svtkTableBasedClipperEdgeHashEntry* GetFreeEdgeHashEntry()
  {
    if (freeEntryindex <= 0)
    {
      AllocateEdgeHashEntryPool();
    }
    freeEntryindex--;
    return freeEntrylist[freeEntryindex];
  }

  inline void ReRegisterEdgeHashEntry(svtkTableBasedClipperEdgeHashEntry* q)
  {
    if (freeEntryindex >= FREE_ENTRY_LIST_SIZE - 1)
    {
      // We've got plenty, so ignore this one.
      return;
    }
    freeEntrylist[freeEntryindex] = q;
    freeEntryindex++;
  }

protected:
  int freeEntryindex;
  svtkTableBasedClipperEdgeHashEntry* freeEntrylist[FREE_ENTRY_LIST_SIZE];
  std::vector<svtkTableBasedClipperEdgeHashEntry*> edgeHashEntrypool;

  void AllocateEdgeHashEntryPool();
};
// ---- svtkTableBasedClipperEdgeHashEntryMemoryManager (end)

// ---- svtkTableBasedClipperEdgeHashTable (begin)
class svtkTableBasedClipperEdgeHashTable
{
public:
  svtkTableBasedClipperEdgeHashTable(int, svtkTableBasedClipperPointList&);
  virtual ~svtkTableBasedClipperEdgeHashTable();

  svtkIdType AddPoint(svtkIdType, svtkIdType, double);
  svtkTableBasedClipperPointList& GetPointList();

protected:
  int nHashes;
  svtkTableBasedClipperPointList& pointlist;
  svtkTableBasedClipperEdgeHashEntry** hashes;
  svtkTableBasedClipperEdgeHashEntryMemoryManager emm;

  int GetKey(int, int);

private:
  svtkTableBasedClipperEdgeHashTable(const svtkTableBasedClipperEdgeHashTable&) = delete;
  void operator=(const svtkTableBasedClipperEdgeHashTable&) = delete;
};
// ---- svtkTableBasedClipperEdgeHashTable (end)

class svtkTableBasedClipperDataSetFromVolume
{
public:
  svtkTableBasedClipperDataSetFromVolume(svtkIdType ptSizeGuess);
  svtkTableBasedClipperDataSetFromVolume(svtkIdType nPts, svtkIdType ptSizeGuess);
  virtual ~svtkTableBasedClipperDataSetFromVolume() = default;

  svtkIdType AddPoint(svtkIdType p1, svtkIdType p2, double percent)
  {
    return numPrevPts + edges.AddPoint(p1, p2, percent);
  }

protected:
  int numPrevPts;
  svtkTableBasedClipperPointList pt_list;
  svtkTableBasedClipperEdgeHashTable edges;

private:
  svtkTableBasedClipperDataSetFromVolume(const svtkTableBasedClipperDataSetFromVolume&) = delete;
  void operator=(const svtkTableBasedClipperDataSetFromVolume&) = delete;
};

svtkTableBasedClipperPointList::svtkTableBasedClipperPointList()
{
  listSize = 4096;
  pointsPerList = 1024;

  list = new TableBasedClipperPointEntry*[listSize];
  list[0] = new TableBasedClipperPointEntry[pointsPerList];
  for (int i = 1; i < listSize; i++)
  {
    list[i] = nullptr;
  }

  currentList = 0;
  currentPoint = 0;
}

svtkTableBasedClipperPointList::~svtkTableBasedClipperPointList()
{
  for (int i = 0; i < listSize; i++)
  {
    if (list[i] != nullptr)
    {
      delete[] list[i];
    }
    else
    {
      break;
    }
  }

  delete[] list;
}

int svtkTableBasedClipperPointList::GetList(
  svtkIdType listId, const TableBasedClipperPointEntry*& outlist) const
{
  if (listId < 0 || listId > currentList)
  {
    outlist = nullptr;
    return 0;
  }

  outlist = list[listId];
  return (listId == currentList ? currentPoint : pointsPerList);
}

int svtkTableBasedClipperPointList::GetNumberOfLists() const
{
  return currentList + 1;
}

svtkIdType svtkTableBasedClipperPointList::GetTotalNumberOfPoints() const
{
  svtkIdType numFullLists = currentList; // actually currentList-1+1
  svtkIdType numExtra = currentPoint;    // again, currentPoint-1+1

  return numFullLists * pointsPerList + numExtra;
}

svtkIdType svtkTableBasedClipperPointList::AddPoint(svtkIdType pt0, svtkIdType pt1, double percent)
{
  if (currentPoint >= pointsPerList)
  {
    if ((currentList + 1) >= listSize)
    {
      TableBasedClipperPointEntry** tmpList = new TableBasedClipperPointEntry*[2 * listSize];
      for (int i = 0; i < listSize; i++)
      {
        tmpList[i] = list[i];
      }

      for (int i = listSize; i < listSize * 2; i++)
      {
        tmpList[i] = nullptr;
      }

      listSize *= 2;
      delete[] list;
      list = tmpList;
    }

    currentList++;
    list[currentList] = new TableBasedClipperPointEntry[pointsPerList];
    currentPoint = 0;
  }

  list[currentList][currentPoint].ptIds[0] = pt0;
  list[currentList][currentPoint].ptIds[1] = pt1;
  list[currentList][currentPoint].percent = percent;
  currentPoint++;

  return (GetTotalNumberOfPoints() - 1);
}

svtkTableBasedClipperEdgeHashEntry::svtkTableBasedClipperEdgeHashEntry()
{
  id1 = -1;
  id2 = -1;
  ptId = -1;
  next = nullptr;
}

void svtkTableBasedClipperEdgeHashEntry::SetInfo(int i1, int i2, int pId)
{
  id1 = i1;
  id2 = i2;
  ptId = pId;
  next = nullptr;
}

svtkTableBasedClipperEdgeHashEntryMemoryManager::svtkTableBasedClipperEdgeHashEntryMemoryManager()
{
  freeEntryindex = 0;
}

svtkTableBasedClipperEdgeHashEntryMemoryManager::~svtkTableBasedClipperEdgeHashEntryMemoryManager()
{
  int npools = static_cast<int>(edgeHashEntrypool.size());
  for (int i = 0; i < npools; i++)
  {
    svtkTableBasedClipperEdgeHashEntry* pool = edgeHashEntrypool[i];
    delete[] pool;
  }
}

void svtkTableBasedClipperEdgeHashEntryMemoryManager::AllocateEdgeHashEntryPool()
{
  if (freeEntryindex == 0)
  {
    svtkTableBasedClipperEdgeHashEntry* newlist = new svtkTableBasedClipperEdgeHashEntry[POOL_SIZE];
    edgeHashEntrypool.push_back(newlist);

    for (int i = 0; i < POOL_SIZE; i++)
    {
      freeEntrylist[i] = &(newlist[i]);
    }

    freeEntryindex = POOL_SIZE;
  }
}

svtkTableBasedClipperEdgeHashTable::svtkTableBasedClipperEdgeHashTable(
  int nh, svtkTableBasedClipperPointList& p)
  : pointlist(p)
{
  nHashes = nh;
  hashes = new svtkTableBasedClipperEdgeHashEntry*[nHashes];
  for (int i = 0; i < nHashes; i++)
  {
    hashes[i] = nullptr;
  }
}

svtkTableBasedClipperEdgeHashTable::~svtkTableBasedClipperEdgeHashTable()
{
  delete[] hashes;
}

int svtkTableBasedClipperEdgeHashTable::GetKey(int p1, int p2)
{
  int rv = (int)((unsigned int)p1 * 18457U + (unsigned int)p2 * 234749U) % nHashes;

  // In case of overflows and modulo with negative numbers.
  if (rv < 0)
  {
    rv += nHashes;
  }

  return rv;
}

svtkIdType svtkTableBasedClipperEdgeHashTable::AddPoint(svtkIdType ap1, svtkIdType ap2, double apercent)
{
  svtkIdType p1, p2;
  double percent;

  if (ap2 < ap1)
  {
    p1 = ap2;
    p2 = ap1;
    percent = 1.0 - apercent;
  }
  else
  {
    p1 = ap1;
    p2 = ap2;
    percent = apercent;
  }

  int key = GetKey(p1, p2);

  //
  // See if we have any matches in the current hashes.
  //
  svtkTableBasedClipperEdgeHashEntry* cur = hashes[key];
  while (cur != nullptr)
  {
    if (cur->IsMatch(p1, p2))
    {
      //
      // We found a match.
      //
      return cur->GetPointId();
    }

    cur = cur->GetNext();
  }

  //
  // There was no match.  We will have to add a new entry.
  //
  svtkTableBasedClipperEdgeHashEntry* new_one = emm.GetFreeEdgeHashEntry();

  svtkIdType newPt = pointlist.AddPoint(p1, p2, percent);
  new_one->SetInfo(p1, p2, newPt);
  new_one->SetNext(hashes[key]);
  hashes[key] = new_one;

  return newPt;
}

svtkTableBasedClipperDataSetFromVolume::svtkTableBasedClipperDataSetFromVolume(svtkIdType ptSizeGuess)
  : numPrevPts(0)
  , pt_list()
  , edges(ptSizeGuess, pt_list)
{
}

svtkTableBasedClipperDataSetFromVolume::svtkTableBasedClipperDataSetFromVolume(
  svtkIdType nPts, svtkIdType ptSizeGuess)
  : numPrevPts(nPts)
  , pt_list()
  , edges(ptSizeGuess, pt_list)
{
}
// ============================================================================
// ============== svtkTableBasedClipperDataSetFromVolume ( end ) ===============
// ============================================================================

// ============================================================================
// =============== svtkTableBasedClipperVolumeFromVolume (begin) ===============
// ============================================================================

class svtkTableBasedClipperShapeList
{
public:
  svtkTableBasedClipperShapeList(int size);
  virtual ~svtkTableBasedClipperShapeList();
  virtual int GetSVTKType() const = 0;
  int GetShapeSize() const { return shapeSize; }
  int GetTotalNumberOfShapes() const;
  int GetNumberOfLists() const;
  int GetList(svtkIdType, const svtkIdType*&) const;

protected:
  svtkIdType** list;
  int currentList;
  int currentShape;
  int listSize;
  int shapesPerList;
  int shapeSize;
};

class svtkTableBasedClipperHexList : public svtkTableBasedClipperShapeList
{
public:
  svtkTableBasedClipperHexList();
  ~svtkTableBasedClipperHexList() override;
  int GetSVTKType() const override { return SVTK_HEXAHEDRON; }
  void AddHex(svtkIdType, svtkIdType, svtkIdType, svtkIdType, svtkIdType, svtkIdType, svtkIdType,
    svtkIdType, svtkIdType);
};

class svtkTableBasedClipperWedgeList : public svtkTableBasedClipperShapeList
{
public:
  svtkTableBasedClipperWedgeList();
  ~svtkTableBasedClipperWedgeList() override;
  int GetSVTKType() const override { return SVTK_WEDGE; }
  void AddWedge(svtkIdType, svtkIdType, svtkIdType, svtkIdType, svtkIdType, svtkIdType, svtkIdType);
};

class svtkTableBasedClipperPyramidList : public svtkTableBasedClipperShapeList
{
public:
  svtkTableBasedClipperPyramidList();
  ~svtkTableBasedClipperPyramidList() override;
  int GetSVTKType() const override { return SVTK_PYRAMID; }
  void AddPyramid(svtkIdType, svtkIdType, svtkIdType, svtkIdType, svtkIdType, svtkIdType);
};

class svtkTableBasedClipperTetList : public svtkTableBasedClipperShapeList
{
public:
  svtkTableBasedClipperTetList();
  ~svtkTableBasedClipperTetList() override;
  int GetSVTKType() const override { return SVTK_TETRA; }
  void AddTet(svtkIdType, svtkIdType, svtkIdType, svtkIdType, svtkIdType);
};

class svtkTableBasedClipperQuadList : public svtkTableBasedClipperShapeList
{
public:
  svtkTableBasedClipperQuadList();
  ~svtkTableBasedClipperQuadList() override;
  int GetSVTKType() const override { return SVTK_QUAD; }
  void AddQuad(svtkIdType, svtkIdType, svtkIdType, svtkIdType, svtkIdType);
};

class svtkTableBasedClipperTriList : public svtkTableBasedClipperShapeList
{
public:
  svtkTableBasedClipperTriList();
  ~svtkTableBasedClipperTriList() override;
  int GetSVTKType() const override { return SVTK_TRIANGLE; }
  void AddTri(svtkIdType, svtkIdType, svtkIdType, svtkIdType);
};

class svtkTableBasedClipperLineList : public svtkTableBasedClipperShapeList
{
public:
  svtkTableBasedClipperLineList();
  ~svtkTableBasedClipperLineList() override;
  int GetSVTKType() const override { return SVTK_LINE; }
  void AddLine(svtkIdType, svtkIdType, svtkIdType);
};

class svtkTableBasedClipperVertexList : public svtkTableBasedClipperShapeList
{
public:
  svtkTableBasedClipperVertexList();
  ~svtkTableBasedClipperVertexList() override;
  int GetSVTKType() const override { return SVTK_VERTEX; }
  void AddVertex(svtkIdType, svtkIdType);
};

struct TableBasedClipperCentroidPointEntry
{
  svtkIdType nPts;
  int ptIds[8];
};

class svtkTableBasedClipperCentroidPointList
{
public:
  svtkTableBasedClipperCentroidPointList();
  virtual ~svtkTableBasedClipperCentroidPointList();

  svtkIdType AddPoint(svtkIdType, svtkIdType*);

  svtkIdType GetTotalNumberOfPoints() const;
  int GetNumberOfLists() const;
  int GetList(svtkIdType, const TableBasedClipperCentroidPointEntry*&) const;

protected:
  TableBasedClipperCentroidPointEntry** list;
  int currentList;
  int currentPoint;
  int listSize;
  int pointsPerList;
};

struct TableBasedClipperCommonPointsStructure
{
  bool hasPtsList;
  double* pts_ptr;
  int* dims;
  double* X;
  double* Y;
  double* Z;
};

class svtkTableBasedClipperVolumeFromVolume : public svtkTableBasedClipperDataSetFromVolume
{
public:
  svtkTableBasedClipperVolumeFromVolume(int precision, svtkIdType nPts, svtkIdType ptSizeGuess);
  ~svtkTableBasedClipperVolumeFromVolume() override = default;

  void ConstructDataSet(svtkDataSet*, svtkUnstructuredGrid*, double*);
  void ConstructDataSet(svtkDataSet*, svtkUnstructuredGrid*, int*, double*, double*, double*);

  svtkIdType AddCentroidPoint(int n, svtkIdType* p)
  {
    return -1 - centroid_list.AddPoint(static_cast<svtkIdType>(n), p);
  }

  void AddHex(svtkIdType z, svtkIdType v0, svtkIdType v1, svtkIdType v2, svtkIdType v3, svtkIdType v4,
    svtkIdType v5, svtkIdType v6, svtkIdType v7)
  {
    this->hexes.AddHex(z, v0, v1, v2, v3, v4, v5, v6, v7);
  }

  void AddWedge(
    svtkIdType z, svtkIdType v0, svtkIdType v1, svtkIdType v2, svtkIdType v3, svtkIdType v4, svtkIdType v5)
  {
    this->wedges.AddWedge(z, v0, v1, v2, v3, v4, v5);
  }
  void AddPyramid(svtkIdType z, svtkIdType v0, svtkIdType v1, svtkIdType v2, svtkIdType v3, svtkIdType v4)
  {
    this->pyramids.AddPyramid(z, v0, v1, v2, v3, v4);
  }
  void AddTet(svtkIdType z, svtkIdType v0, svtkIdType v1, svtkIdType v2, svtkIdType v3)
  {
    this->tets.AddTet(z, v0, v1, v2, v3);
  }
  void AddQuad(svtkIdType z, svtkIdType v0, svtkIdType v1, svtkIdType v2, svtkIdType v3)
  {
    this->quads.AddQuad(z, v0, v1, v2, v3);
  }
  void AddTri(svtkIdType z, svtkIdType v0, svtkIdType v1, svtkIdType v2)
  {
    this->tris.AddTri(z, v0, v1, v2);
  }
  void AddLine(svtkIdType z, svtkIdType v0, svtkIdType v1) { this->lines.AddLine(z, v0, v1); }
  void AddVertex(svtkIdType z, svtkIdType v0) { this->vertices.AddVertex(z, v0); }

protected:
  svtkTableBasedClipperCentroidPointList centroid_list;
  svtkTableBasedClipperHexList hexes;
  svtkTableBasedClipperWedgeList wedges;
  svtkTableBasedClipperPyramidList pyramids;
  svtkTableBasedClipperTetList tets;
  svtkTableBasedClipperQuadList quads;
  svtkTableBasedClipperTriList tris;
  svtkTableBasedClipperLineList lines;
  svtkTableBasedClipperVertexList vertices;

  svtkTableBasedClipperShapeList* shapes[8];
  const int nshapes;
  int OutputPointsPrecision;

  void ConstructDataSet(svtkDataSet*, svtkUnstructuredGrid*, TableBasedClipperCommonPointsStructure&);
};

svtkTableBasedClipperVolumeFromVolume::svtkTableBasedClipperVolumeFromVolume(
  int precision, svtkIdType nPts, svtkIdType ptSizeGuess)
  : svtkTableBasedClipperDataSetFromVolume(nPts, ptSizeGuess)
  , nshapes(8)
  , OutputPointsPrecision(precision)
{
  shapes[0] = &tets;
  shapes[1] = &pyramids;
  shapes[2] = &wedges;
  shapes[3] = &hexes;
  shapes[4] = &quads;
  shapes[5] = &tris;
  shapes[6] = &lines;
  shapes[7] = &vertices;
}

svtkTableBasedClipperCentroidPointList::svtkTableBasedClipperCentroidPointList()
{
  listSize = 4096;
  pointsPerList = 1024;

  list = new TableBasedClipperCentroidPointEntry*[listSize];
  list[0] = new TableBasedClipperCentroidPointEntry[pointsPerList];
  for (int i = 1; i < listSize; i++)
  {
    list[i] = nullptr;
  }

  currentList = 0;
  currentPoint = 0;
}

svtkTableBasedClipperCentroidPointList::~svtkTableBasedClipperCentroidPointList()
{
  for (int i = 0; i < listSize; i++)
  {
    if (list[i] != nullptr)
    {
      delete[] list[i];
    }
    else
    {
      break;
    }
  }

  delete[] list;
}

int svtkTableBasedClipperCentroidPointList::GetList(
  svtkIdType listId, const TableBasedClipperCentroidPointEntry*& outlist) const
{
  if (listId < 0 || listId > currentList)
  {
    outlist = nullptr;
    return 0;
  }

  outlist = list[listId];
  return (listId == currentList ? currentPoint : pointsPerList);
}

int svtkTableBasedClipperCentroidPointList::GetNumberOfLists() const
{
  return currentList + 1;
}

svtkIdType svtkTableBasedClipperCentroidPointList::GetTotalNumberOfPoints() const
{
  svtkIdType numFullLists = static_cast<svtkIdType>(currentList); // actually currentList-1+1
  svtkIdType numExtra = static_cast<svtkIdType>(currentPoint);    // again, currentPoint-1+1

  return numFullLists * pointsPerList + numExtra;
}

svtkIdType svtkTableBasedClipperCentroidPointList::AddPoint(svtkIdType npts, svtkIdType* pts)
{
  if (currentPoint >= pointsPerList)
  {
    if ((currentList + 1) >= listSize)
    {
      TableBasedClipperCentroidPointEntry** tmpList =
        new TableBasedClipperCentroidPointEntry*[2 * listSize];

      for (int i = 0; i < listSize; i++)
      {
        tmpList[i] = list[i];
      }

      for (int i = listSize; i < listSize * 2; i++)
      {
        tmpList[i] = nullptr;
      }

      listSize *= 2;
      delete[] list;
      list = tmpList;
    }

    currentList++;
    list[currentList] = new TableBasedClipperCentroidPointEntry[pointsPerList];
    currentPoint = 0;
  }

  list[currentList][currentPoint].nPts = npts;
  for (int i = 0; i < npts; i++)
  {
    list[currentList][currentPoint].ptIds[i] = pts[i];
  }
  currentPoint++;

  return (GetTotalNumberOfPoints() - 1);
}

svtkTableBasedClipperShapeList::svtkTableBasedClipperShapeList(int size)
{
  shapeSize = size;
  listSize = 4096;
  shapesPerList = 1024;

  list = new svtkIdType*[listSize];
  list[0] = new svtkIdType[(shapeSize + 1) * shapesPerList];

  for (int i = 1; i < listSize; i++)
  {
    list[i] = nullptr;
  }

  currentList = 0;
  currentShape = 0;
}

svtkTableBasedClipperShapeList::~svtkTableBasedClipperShapeList()
{
  for (int i = 0; i < listSize; i++)
  {
    if (list[i] != nullptr)
    {
      delete[] list[i];
    }
    else
    {
      break;
    }
  }

  delete[] list;
}

int svtkTableBasedClipperShapeList::GetList(svtkIdType listId, const svtkIdType*& outlist) const
{
  if (listId < 0 || listId > currentList)
  {
    outlist = nullptr;
    return 0;
  }

  outlist = list[listId];
  return (listId == currentList ? currentShape : shapesPerList);
}

int svtkTableBasedClipperShapeList::GetNumberOfLists() const
{
  return currentList + 1;
}

int svtkTableBasedClipperShapeList::GetTotalNumberOfShapes() const
{
  int numFullLists = currentList; // actually currentList-1+1
  int numExtra = currentShape;    // again, currentShape-1+1

  return numFullLists * shapesPerList + numExtra;
}

svtkTableBasedClipperHexList::svtkTableBasedClipperHexList()
  : svtkTableBasedClipperShapeList(8)
{
}

svtkTableBasedClipperHexList::~svtkTableBasedClipperHexList() = default;

void svtkTableBasedClipperHexList::AddHex(svtkIdType cellId, svtkIdType v1, svtkIdType v2, svtkIdType v3,
  svtkIdType v4, svtkIdType v5, svtkIdType v6, svtkIdType v7, svtkIdType v8)
{
  if (currentShape >= shapesPerList)
  {
    if ((currentList + 1) >= listSize)
    {
      svtkIdType** tmpList = new svtkIdType*[2 * listSize];

      for (int i = 0; i < listSize; i++)
      {
        tmpList[i] = list[i];
      }

      for (int i = listSize; i < listSize * 2; i++)
      {
        tmpList[i] = nullptr;
      }

      listSize *= 2;
      delete[] list;
      list = tmpList;
    }

    currentList++;
    list[currentList] = new svtkIdType[(shapeSize + 1) * shapesPerList];
    currentShape = 0;
  }

  int idx = (shapeSize + 1) * currentShape;
  list[currentList][idx + 0] = cellId;
  list[currentList][idx + 1] = v1;
  list[currentList][idx + 2] = v2;
  list[currentList][idx + 3] = v3;
  list[currentList][idx + 4] = v4;
  list[currentList][idx + 5] = v5;
  list[currentList][idx + 6] = v6;
  list[currentList][idx + 7] = v7;
  list[currentList][idx + 8] = v8;
  currentShape++;
}

svtkTableBasedClipperWedgeList::svtkTableBasedClipperWedgeList()
  : svtkTableBasedClipperShapeList(6)
{
}

svtkTableBasedClipperWedgeList::~svtkTableBasedClipperWedgeList() = default;

void svtkTableBasedClipperWedgeList::AddWedge(svtkIdType cellId, svtkIdType v1, svtkIdType v2,
  svtkIdType v3, svtkIdType v4, svtkIdType v5, svtkIdType v6)
{
  if (currentShape >= shapesPerList)
  {
    if ((currentList + 1) >= listSize)
    {
      svtkIdType** tmpList = new svtkIdType*[2 * listSize];
      for (int i = 0; i < listSize; i++)
      {
        tmpList[i] = list[i];
      }

      for (int i = listSize; i < listSize * 2; i++)
      {
        tmpList[i] = nullptr;
      }

      listSize *= 2;
      delete[] list;
      list = tmpList;
    }

    currentList++;
    list[currentList] = new svtkIdType[(shapeSize + 1) * shapesPerList];
    currentShape = 0;
  }

  int idx = (shapeSize + 1) * currentShape;
  list[currentList][idx + 0] = cellId;
  list[currentList][idx + 1] = v1;
  list[currentList][idx + 2] = v2;
  list[currentList][idx + 3] = v3;
  list[currentList][idx + 4] = v4;
  list[currentList][idx + 5] = v5;
  list[currentList][idx + 6] = v6;
  currentShape++;
}

svtkTableBasedClipperPyramidList::svtkTableBasedClipperPyramidList()
  : svtkTableBasedClipperShapeList(5)
{
}

svtkTableBasedClipperPyramidList::~svtkTableBasedClipperPyramidList() = default;

void svtkTableBasedClipperPyramidList::AddPyramid(
  svtkIdType cellId, svtkIdType v1, svtkIdType v2, svtkIdType v3, svtkIdType v4, svtkIdType v5)
{
  if (currentShape >= shapesPerList)
  {
    if ((currentList + 1) >= listSize)
    {
      svtkIdType** tmpList = new svtkIdType*[2 * listSize];

      for (int i = 0; i < listSize; i++)
      {
        tmpList[i] = list[i];
      }

      for (int i = listSize; i < listSize * 2; i++)
      {
        tmpList[i] = nullptr;
      }

      listSize *= 2;
      delete[] list;
      list = tmpList;
    }

    currentList++;
    list[currentList] = new svtkIdType[(shapeSize + 1) * shapesPerList];
    currentShape = 0;
  }

  int idx = (shapeSize + 1) * currentShape;
  list[currentList][idx + 0] = cellId;
  list[currentList][idx + 1] = v1;
  list[currentList][idx + 2] = v2;
  list[currentList][idx + 3] = v3;
  list[currentList][idx + 4] = v4;
  list[currentList][idx + 5] = v5;
  currentShape++;
}

svtkTableBasedClipperTetList::svtkTableBasedClipperTetList()
  : svtkTableBasedClipperShapeList(4)
{
}

svtkTableBasedClipperTetList::~svtkTableBasedClipperTetList() = default;

void svtkTableBasedClipperTetList::AddTet(
  svtkIdType cellId, svtkIdType v1, svtkIdType v2, svtkIdType v3, svtkIdType v4)
{
  if (currentShape >= shapesPerList)
  {
    if ((currentList + 1) >= listSize)
    {
      svtkIdType** tmpList = new svtkIdType*[2 * listSize];

      for (int i = 0; i < listSize; i++)
      {
        tmpList[i] = list[i];
      }

      for (int i = listSize; i < listSize * 2; i++)
      {
        tmpList[i] = nullptr;
      }

      listSize *= 2;
      delete[] list;
      list = tmpList;
    }

    currentList++;
    list[currentList] = new svtkIdType[(shapeSize + 1) * shapesPerList];
    currentShape = 0;
  }

  int idx = (shapeSize + 1) * currentShape;
  list[currentList][idx + 0] = cellId;
  list[currentList][idx + 1] = v1;
  list[currentList][idx + 2] = v2;
  list[currentList][idx + 3] = v3;
  list[currentList][idx + 4] = v4;
  currentShape++;
}

svtkTableBasedClipperQuadList::svtkTableBasedClipperQuadList()
  : svtkTableBasedClipperShapeList(4)
{
}

svtkTableBasedClipperQuadList::~svtkTableBasedClipperQuadList() = default;

void svtkTableBasedClipperQuadList::AddQuad(
  svtkIdType cellId, svtkIdType v1, svtkIdType v2, svtkIdType v3, svtkIdType v4)
{
  if (currentShape >= shapesPerList)
  {
    if ((currentList + 1) >= listSize)
    {
      svtkIdType** tmpList = new svtkIdType*[2 * listSize];
      for (int i = 0; i < listSize; i++)
      {
        tmpList[i] = list[i];
      }

      for (int i = listSize; i < listSize * 2; i++)
      {
        tmpList[i] = nullptr;
      }

      listSize *= 2;
      delete[] list;
      list = tmpList;
    }

    currentList++;
    list[currentList] = new svtkIdType[(shapeSize + 1) * shapesPerList];
    currentShape = 0;
  }

  int idx = (shapeSize + 1) * currentShape;
  list[currentList][idx + 0] = cellId;
  list[currentList][idx + 1] = v1;
  list[currentList][idx + 2] = v2;
  list[currentList][idx + 3] = v3;
  list[currentList][idx + 4] = v4;
  currentShape++;
}

svtkTableBasedClipperTriList::svtkTableBasedClipperTriList()
  : svtkTableBasedClipperShapeList(3)
{
}

svtkTableBasedClipperTriList::~svtkTableBasedClipperTriList() = default;

void svtkTableBasedClipperTriList::AddTri(svtkIdType cellId, svtkIdType v1, svtkIdType v2, svtkIdType v3)
{
  if (currentShape >= shapesPerList)
  {
    if ((currentList + 1) >= listSize)
    {
      svtkIdType** tmpList = new svtkIdType*[2 * listSize];
      for (int i = 0; i < listSize; i++)
      {
        tmpList[i] = list[i];
      }

      for (int i = listSize; i < listSize * 2; i++)
      {
        tmpList[i] = nullptr;
      }

      listSize *= 2;
      delete[] list;
      list = tmpList;
    }

    currentList++;
    list[currentList] = new svtkIdType[(shapeSize + 1) * shapesPerList];
    currentShape = 0;
  }

  int idx = (shapeSize + 1) * currentShape;
  list[currentList][idx + 0] = cellId;
  list[currentList][idx + 1] = v1;
  list[currentList][idx + 2] = v2;
  list[currentList][idx + 3] = v3;
  currentShape++;
}

svtkTableBasedClipperLineList::svtkTableBasedClipperLineList()
  : svtkTableBasedClipperShapeList(2)
{
}

svtkTableBasedClipperLineList::~svtkTableBasedClipperLineList() = default;

void svtkTableBasedClipperLineList::AddLine(svtkIdType cellId, svtkIdType v1, svtkIdType v2)
{
  if (currentShape >= shapesPerList)
  {
    if ((currentList + 1) >= listSize)
    {
      svtkIdType** tmpList = new svtkIdType*[2 * listSize];
      for (int i = 0; i < listSize; i++)
      {
        tmpList[i] = list[i];
      }

      for (int i = listSize; i < listSize * 2; i++)
      {
        tmpList[i] = nullptr;
      }

      listSize *= 2;
      delete[] list;
      list = tmpList;
    }

    currentList++;
    list[currentList] = new svtkIdType[(shapeSize + 1) * shapesPerList];
    currentShape = 0;
  }

  int idx = (shapeSize + 1) * currentShape;
  list[currentList][idx + 0] = cellId;
  list[currentList][idx + 1] = v1;
  list[currentList][idx + 2] = v2;
  currentShape++;
}

svtkTableBasedClipperVertexList::svtkTableBasedClipperVertexList()
  : svtkTableBasedClipperShapeList(1)
{
}

svtkTableBasedClipperVertexList::~svtkTableBasedClipperVertexList() = default;

void svtkTableBasedClipperVertexList::AddVertex(svtkIdType cellId, svtkIdType v1)
{
  if (currentShape >= shapesPerList)
  {
    if ((currentList + 1) >= listSize)
    {
      svtkIdType** tmpList = new svtkIdType*[2 * listSize];
      for (int i = 0; i < listSize; i++)
      {
        tmpList[i] = list[i];
      }

      for (int i = listSize; i < listSize * 2; i++)
      {
        tmpList[i] = nullptr;
      }

      listSize *= 2;
      delete[] list;
      list = tmpList;
    }

    currentList++;
    list[currentList] = new svtkIdType[(shapeSize + 1) * shapesPerList];
    currentShape = 0;
  }

  int idx = (shapeSize + 1) * currentShape;
  list[currentList][idx + 0] = cellId;
  list[currentList][idx + 1] = v1;
  currentShape++;
}

void svtkTableBasedClipperVolumeFromVolume::ConstructDataSet(
  svtkDataSet* input, svtkUnstructuredGrid* output, double* pts_ptr)
{
  TableBasedClipperCommonPointsStructure cps;
  cps.hasPtsList = true;
  cps.pts_ptr = pts_ptr;
  ConstructDataSet(input, output, cps);
}

void svtkTableBasedClipperVolumeFromVolume::ConstructDataSet(
  svtkDataSet* input, svtkUnstructuredGrid* output, int* dims, double* X, double* Y, double* Z)
{
  TableBasedClipperCommonPointsStructure cps;
  cps.hasPtsList = false;
  cps.dims = dims;
  cps.X = X;
  cps.Y = Y;
  cps.Z = Z;
  ConstructDataSet(input, output, cps);
}

void svtkTableBasedClipperVolumeFromVolume::ConstructDataSet(
  svtkDataSet* input, svtkUnstructuredGrid* output, TableBasedClipperCommonPointsStructure& cps)
{
  int i, j, k, l;

  svtkPointData* inPD = input->GetPointData();
  svtkCellData* inCD = input->GetCellData();

  svtkPointData* outPD = output->GetPointData();
  svtkCellData* outCD = output->GetCellData();

  svtkIntArray* newOrigNodes = nullptr;
  svtkIntArray* origNodes = svtkArrayDownCast<svtkIntArray>(inPD->GetArray("avtOriginalNodeNumbers"));
  //
  // If the isovolume only affects a small part of the dataset, we can save
  // on memory by only bringing over the points from the original dataset
  // that are used with the output.  Determine which points those are here.
  //
  int* ptLookup = new int[numPrevPts];
  for (i = 0; i < numPrevPts; i++)
  {
    ptLookup[i] = -1;
  }

  int numUsed = 0;
  for (i = 0; i < nshapes; i++)
  {
    int nlists = shapes[i]->GetNumberOfLists();
    int npts_per_shape = shapes[i]->GetShapeSize();

    for (j = 0; j < nlists; j++)
    {
      const svtkIdType* list;
      int listSize = shapes[i]->GetList(j, list);

      for (k = 0; k < listSize; k++)
      {
        list++; // skip the cell id entry

        for (l = 0; l < npts_per_shape; l++)
        {
          int pt = *list;
          list++;

          if (pt >= 0 && pt < numPrevPts)
          {
            if (ptLookup[pt] == -1)
            {
              ptLookup[pt] = numUsed++;
            }
          }
        }
      }
    }
  }

  //
  // Set up the output points and its point data.
  //
  svtkPoints* outPts = svtkPoints::New();

  // set precision for the points in the output
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    svtkPointSet* inputPointSet = svtkPointSet::SafeDownCast(input);
    if (inputPointSet)
    {
      outPts->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      outPts->SetDataType(SVTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(SVTK_DOUBLE);
  }

  svtkIdType centroidStart = numUsed + pt_list.GetTotalNumberOfPoints();
  svtkIdType nOutPts = centroidStart + centroid_list.GetTotalNumberOfPoints();
  outPts->SetNumberOfPoints(nOutPts);
  outPD->CopyAllocate(inPD, nOutPts);

  if (origNodes != nullptr)
  {
    newOrigNodes = svtkIntArray::New();
    newOrigNodes->SetNumberOfComponents(origNodes->GetNumberOfComponents());
    newOrigNodes->SetNumberOfTuples(nOutPts);
    newOrigNodes->SetName(origNodes->GetName());
  }

  //
  // Copy over all the points from the input that are actually used in the
  // output.
  //
  for (i = 0; i < numPrevPts; i++)
  {
    if (ptLookup[i] == -1)
    {
      continue;
    }

    if (cps.hasPtsList)
    {
      outPts->SetPoint(ptLookup[i], cps.pts_ptr + 3 * i);
    }
    else
    {
      int I = i % cps.dims[0];
      int J = (i / cps.dims[0]) % cps.dims[1];
      int K = i / (cps.dims[0] * cps.dims[1]);
      outPts->SetPoint(ptLookup[i], cps.X[I], cps.Y[J], cps.Z[K]);
    }

    outPD->CopyData(inPD, i, ptLookup[i]);
    if (newOrigNodes)
    {
      newOrigNodes->SetTuple(ptLookup[i], origNodes->GetTuple(i));
    }
  }

  int ptIdx = numUsed;

  //
  // Now construct all the points that are along edges and new and add
  // them to the points list.
  //
  int nLists = pt_list.GetNumberOfLists();
  for (i = 0; i < nLists; i++)
  {
    const TableBasedClipperPointEntry* pe_list = nullptr;
    int nPts = pt_list.GetList(i, pe_list);
    for (j = 0; j < nPts; j++)
    {
      const TableBasedClipperPointEntry& pe = pe_list[j];
      double pt[3];
      int idx1 = pe.ptIds[0];
      int idx2 = pe.ptIds[1];

      // Construct the original points -- this will depend on whether
      // or not we started with a rectilinear grid or a point set.
      double* pt1 = nullptr;
      double* pt2 = nullptr;
      double pt1_storage[3];
      double pt2_storage[3];
      if (cps.hasPtsList)
      {
        pt1 = cps.pts_ptr + 3 * idx1;
        pt2 = cps.pts_ptr + 3 * idx2;
      }
      else
      {
        pt1 = pt1_storage;
        pt2 = pt2_storage;
        int I = idx1 % cps.dims[0];
        int J = (idx1 / cps.dims[0]) % cps.dims[1];
        int K = idx1 / (cps.dims[0] * cps.dims[1]);
        pt1[0] = cps.X[I];
        pt1[1] = cps.Y[J];
        pt1[2] = cps.Z[K];
        I = idx2 % cps.dims[0];
        J = (idx2 / cps.dims[0]) % cps.dims[1];
        K = idx2 / (cps.dims[0] * cps.dims[1]);
        pt2[0] = cps.X[I];
        pt2[1] = cps.Y[J];
        pt2[2] = cps.Z[K];
      }

      // Now that we have the original points, calculate the new one.
      double p = pe.percent;
      double bp = 1.0 - p;
      pt[0] = pt1[0] * p + pt2[0] * bp;
      pt[1] = pt1[1] * p + pt2[1] * bp;
      pt[2] = pt1[2] * p + pt2[2] * bp;
      outPts->SetPoint(ptIdx, pt);
      outPD->InterpolateEdge(inPD, ptIdx, pe.ptIds[0], pe.ptIds[1], bp);

      if (newOrigNodes)
      {
        int id = (bp <= 0.5 ? pe.ptIds[0] : pe.ptIds[1]);
        newOrigNodes->SetTuple(ptIdx, origNodes->GetTuple(id));
      }
      ptIdx++;
    }
  }

  //
  // Now construct the new "centroid" points and add them to the points list.
  //
  nLists = centroid_list.GetNumberOfLists();
  svtkIdList* idList = svtkIdList::New();
  for (i = 0; i < nLists; i++)
  {
    const TableBasedClipperCentroidPointEntry* ce_list = nullptr;
    int nPts = centroid_list.GetList(i, ce_list);
    for (j = 0; j < nPts; j++)
    {
      const TableBasedClipperCentroidPointEntry& ce = ce_list[j];
      idList->SetNumberOfIds(ce.nPts);
      double pts[8][3];
      double weights[8];
      double pt[3] = { 0.0, 0.0, 0.0 };
      double weight_factor = 1.0 / ce.nPts;
      for (k = 0; k < ce.nPts; k++)
      {
        weights[k] = 1.0 * weight_factor;
        svtkIdType id = 0;

        if (ce.ptIds[k] < 0)
        {
          id = centroidStart - 1 - ce.ptIds[k];
        }
        else if (ce.ptIds[k] >= numPrevPts)
        {
          id = numUsed + (ce.ptIds[k] - numPrevPts);
        }
        else
        {
          id = ptLookup[ce.ptIds[k]];
        }

        idList->SetId(k, id);
        outPts->GetPoint(id, pts[k]);
        pt[0] += pts[k][0];
        pt[1] += pts[k][1];
        pt[2] += pts[k][2];
      }
      pt[0] *= weight_factor;
      pt[1] *= weight_factor;
      pt[2] *= weight_factor;

      outPts->SetPoint(ptIdx, pt);
      outPD->InterpolatePoint(outPD, ptIdx, idList, weights);
      if (newOrigNodes)
      {
        // these 'created' nodes have no original designation
        for (int z = 0; z < newOrigNodes->GetNumberOfComponents(); z++)
        {
          newOrigNodes->SetComponent(ptIdx, z, -1);
        }
      }
      ptIdx++;
    }
  }
  idList->Delete();

  //
  // We are finally done constructing the points list.  Set it with our
  // output and clean up memory.
  //
  output->SetPoints(outPts);
  outPts->Delete();

  if (newOrigNodes)
  {
    // AddArray will overwrite an already existing array with
    // the same name, exactly what we want here.
    outPD->AddArray(newOrigNodes);
    newOrigNodes->Delete();
  }

  //
  // Now set up the shapes and the cell data.
  //
  int cellId = 0;
  int nlists;

  svtkIdType ncells = 0;
  svtkIdType conn_size = 0;
  for (i = 0; i < nshapes; i++)
  {
    svtkIdType ns = shapes[i]->GetTotalNumberOfShapes();
    ncells += ns;
    conn_size += static_cast<svtkIdType>(shapes[i]->GetShapeSize() + 1) * ns;
  }

  outCD->CopyAllocate(inCD, ncells);

  svtkIdTypeArray* nlist = svtkIdTypeArray::New();
  nlist->SetNumberOfValues(conn_size);
  svtkIdType* nl = nlist->GetPointer(0);

  svtkUnsignedCharArray* cellTypes = svtkUnsignedCharArray::New();
  cellTypes->SetNumberOfValues(ncells);
  unsigned char* ct = cellTypes->GetPointer(0);

  svtkIdType ids[1024]; // 8 (for hex) should be max, but...
  for (i = 0; i < nshapes; i++)
  {
    const svtkIdType* list;
    nlists = shapes[i]->GetNumberOfLists();
    int shapesize = shapes[i]->GetShapeSize();
    int svtk_type = shapes[i]->GetSVTKType();

    for (j = 0; j < nlists; j++)
    {
      int listSize = shapes[i]->GetList(j, list);

      for (k = 0; k < listSize; k++)
      {
        outCD->CopyData(inCD, list[0], cellId);

        for (l = 0; l < shapesize; l++)
        {
          if (list[l + 1] < 0)
          {
            ids[l] = centroidStart - 1 - list[l + 1];
          }
          else if (list[l + 1] >= numPrevPts)
          {
            ids[l] = numUsed + (list[l + 1] - numPrevPts);
          }
          else
          {
            ids[l] = ptLookup[list[l + 1]];
          }
        }
        list += shapesize + 1;
        *nl++ = shapesize;
        *ct++ = static_cast<unsigned char>(svtk_type);
        for (l = 0; l < shapesize; l++)
        {
          *nl++ = ids[l];
        }

        cellId++;
      }
    }
  }

  svtkCellArray* cells = svtkCellArray::New();
  cells->AllocateExact(ncells, nlist->GetNumberOfValues() - ncells);
  cells->ImportLegacyFormat(nlist);
  nlist->Delete();

  output->SetCells(cellTypes, cells);
  cellTypes->Delete();
  cells->Delete();

  delete[] ptLookup;
}

inline void GetPoint(
  double* pt, const double* X, const double* Y, const double* Z, const int* dims, const int& index)
{
  int cellI = index % dims[0];
  int cellJ = (index / dims[0]) % dims[1];
  int cellK = index / (dims[0] * dims[1]);
  pt[0] = X[cellI];
  pt[1] = Y[cellJ];
  pt[2] = Z[cellK];
}
// ============================================================================
// =============== svtkTableBasedClipperVolumeFromVolume ( end ) ===============
// ============================================================================

//-----------------------------------------------------------------------------
// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off.
svtkTableBasedClipDataSet::svtkTableBasedClipDataSet(svtkImplicitFunction* cf)
{
  this->Locator = nullptr;
  this->ClipFunction = cf;

  // setup a callback to report progress
  this->InternalProgressObserver = svtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(
    &svtkTableBasedClipDataSet::InternalProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);

  this->Value = 0.0;
  this->InsideOut = 0;
  this->MergeTolerance = 0.01;
  this->UseValueAsOffset = true;
  this->GenerateClipScalars = 0;
  this->GenerateClippedOutput = 0;

  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->SetNumberOfOutputPorts(2);
  svtkUnstructuredGrid* output2 = svtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();
  output2 = nullptr;

  // process active point scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
}

//-----------------------------------------------------------------------------
svtkTableBasedClipDataSet::~svtkTableBasedClipDataSet()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  this->SetClipFunction(nullptr);
  this->InternalProgressObserver->Delete();
  this->InternalProgressObserver = nullptr;
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::InternalProgressCallbackFunction(
  svtkObject* arg, unsigned long, void* clientdata, void*)
{
  reinterpret_cast<svtkTableBasedClipDataSet*>(clientdata)
    ->InternalProgressCallback(static_cast<svtkAlgorithm*>(arg));
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::InternalProgressCallback(svtkAlgorithm* algorithm)
{
  double progress = algorithm->GetProgress();
  this->UpdateProgress(progress);

  if (this->AbortExecute)
  {
    algorithm->SetAbortExecute(1);
  }
}

//-----------------------------------------------------------------------------
svtkMTimeType svtkTableBasedClipDataSet::GetMTime()
{
  svtkMTimeType time;
  svtkMTimeType mTime = this->Superclass::GetMTime();

  if (this->ClipFunction != nullptr)
  {
    time = this->ClipFunction->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

svtkUnstructuredGrid* svtkTableBasedClipDataSet::GetClippedOutput()
{
  if (!this->GenerateClippedOutput)
  {
    return nullptr;
  }

  return svtkUnstructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::SetLocator(svtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }

  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }

  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//-----------------------------------------------------------------------------
int svtkTableBasedClipDataSet::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkTableBasedClipDataSet::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // input and output information objects
  svtkInformation* inputInf = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfor = outputVector->GetInformationObject(0);

  // Get the input of which we have to create a copy since the clipper requires
  // that InterpolateAllocate() be invoked for the output based on its input in
  // terms of the point data. If the input and output arrays are different,
  // svtkCell3D's Clip will fail. The last argument of InterpolateAllocate makes
  // sure that arrays are shallow-copied from theInput to cpyInput.
  svtkDataSet* theInput = svtkDataSet::SafeDownCast(inputInf->Get(svtkDataObject::DATA_OBJECT()));
  svtkSmartPointer<svtkDataSet> cpyInput;
  cpyInput.TakeReference(theInput->NewInstance());
  cpyInput->CopyStructure(theInput);
  cpyInput->GetCellData()->PassData(theInput->GetCellData());
  cpyInput->GetFieldData()->PassData(theInput->GetFieldData());
  cpyInput->GetPointData()->InterpolateAllocate(theInput->GetPointData(), 0, 0, 1);

  // get the output (the remaining and the clipped parts)
  svtkUnstructuredGrid* outputUG =
    svtkUnstructuredGrid::SafeDownCast(outInfor->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* clippedOutputUG = this->GetClippedOutput();

  inputInf = nullptr;
  outInfor = nullptr;
  theInput = nullptr;
  svtkDebugMacro(<< "Clipping dataset" << endl);

  int i;
  svtkIdType numbPnts = cpyInput->GetNumberOfPoints();

  // handling exceptions
  if (numbPnts < 1)
  {
    svtkDebugMacro(<< "No data to clip" << endl);
    outputUG = nullptr;
    return 1;
  }

  if (!this->ClipFunction && this->GenerateClipScalars)
  {
    svtkErrorMacro(<< "Cannot generate clip scalars "
                  << "if no clip function defined" << endl);
    outputUG = nullptr;
    return 1;
  }

  svtkDataArray* clipAray = nullptr;
  svtkDoubleArray* pScalars = nullptr;

  // check whether the cells are clipped with input scalars or a clip function
  if (this->ClipFunction)
  {
    pScalars = svtkDoubleArray::New();
    pScalars->SetNumberOfTuples(numbPnts);
    pScalars->SetName("ClipDataSetScalars");

    // enable clipDataSetScalars to be passed to the output
    if (this->GenerateClipScalars)
    {
      cpyInput->GetPointData()->SetScalars(pScalars);
    }

    for (i = 0; i < numbPnts; i++)
    {
      double s = this->ClipFunction->FunctionValue(cpyInput->GetPoint(i));
      pScalars->SetTuple1(i, s);
    }

    clipAray = pScalars;
  }
  else // using input scalars
  {
    clipAray = this->GetInputArrayToProcess(0, inputVector);
    if (!clipAray)
    {
      svtkErrorMacro(<< "no input scalars." << endl);
      return 1;
    }
  }

  int gridType = cpyInput->GetDataObjectType();
  double isoValue = (!this->ClipFunction || this->UseValueAsOffset) ? this->Value : 0.0;
  if (gridType == SVTK_IMAGE_DATA || gridType == SVTK_STRUCTURED_POINTS)
  {
    this->ClipImageData(cpyInput, clipAray, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipImageData(cpyInput, clipAray, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (gridType == SVTK_POLY_DATA)
  {
    this->ClipPolyData(cpyInput, clipAray, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipPolyData(cpyInput, clipAray, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (gridType == SVTK_RECTILINEAR_GRID)
  {
    this->ClipRectilinearGridData(cpyInput, clipAray, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipRectilinearGridData(cpyInput, clipAray, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (gridType == SVTK_STRUCTURED_GRID)
  {
    this->ClipStructuredGridData(cpyInput, clipAray, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipStructuredGridData(cpyInput, clipAray, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else if (gridType == SVTK_UNSTRUCTURED_GRID)
  {
    this->ClipUnstructuredGridData(cpyInput, clipAray, isoValue, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipUnstructuredGridData(cpyInput, clipAray, isoValue, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }
  else
  {
    this->ClipDataSet(cpyInput, clipAray, outputUG);
    if (clippedOutputUG)
    {
      this->InsideOut = !(this->InsideOut);
      this->ClipDataSet(cpyInput, clipAray, clippedOutputUG);
      this->InsideOut = !(this->InsideOut);
    }
  }

  outputUG->Squeeze();
  outputUG->GetFieldData()->PassData(cpyInput->GetFieldData());

  if (clippedOutputUG)
  {
    clippedOutputUG->Squeeze();
    clippedOutputUG->GetFieldData()->PassData(cpyInput->GetFieldData());
  }

  if (pScalars)
  {
    pScalars->Delete();
  }
  pScalars = nullptr;
  outputUG = nullptr;
  clippedOutputUG = nullptr;
  clipAray = nullptr;

  return 1;
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::ClipDataSet(
  svtkDataSet* pDataSet, svtkDataArray* clipAray, svtkUnstructuredGrid* unstruct)
{
  svtkClipDataSet* clipData = svtkClipDataSet::New();
  clipData->SetInputData(pDataSet);
  clipData->SetValue(this->Value);
  clipData->SetInsideOut(this->InsideOut);
  clipData->SetClipFunction(this->ClipFunction);
  clipData->SetUseValueAsOffset(this->UseValueAsOffset);
  clipData->SetGenerateClipScalars(this->GenerateClipScalars);

  if (!this->ClipFunction)
  {
    pDataSet->GetPointData()->SetScalars(clipAray);
  }

  clipData->Update();
  unstruct->ShallowCopy(clipData->GetOutput());

  clipData->Delete();
  clipData = nullptr;
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::ClipImageData(
  svtkDataSet* inputGrd, svtkDataArray* clipAray, double isoValue, svtkUnstructuredGrid* outputUG)
{
  int i, j;
  int dataDims[3];
  double spacings[3];
  double tmpValue = 0.0;
  svtkRectilinearGrid* rectGrid = nullptr;
  svtkImageData* volImage = svtkImageData::SafeDownCast(inputGrd);
  volImage->GetDimensions(dataDims);
  volImage->GetSpacing(spacings);
  const double* dataBBox = volImage->GetBounds();

  svtkDoubleArray* pxCoords = svtkDoubleArray::New();
  svtkDoubleArray* pyCoords = svtkDoubleArray::New();
  svtkDoubleArray* pzCoords = svtkDoubleArray::New();
  svtkDoubleArray* tmpArays[3] = { pxCoords, pyCoords, pzCoords };
  for (j = 0; j < 3; j++)
  {
    tmpArays[j]->SetNumberOfComponents(1);
    tmpArays[j]->SetNumberOfTuples(dataDims[j]);
    for (tmpValue = dataBBox[j << 1], i = 0; i < dataDims[j]; i++, tmpValue += spacings[j])
    {
      tmpArays[j]->SetComponent(i, 0, tmpValue);
    }
    tmpArays[j] = nullptr;
  }

  rectGrid = svtkRectilinearGrid::New();
  rectGrid->SetDimensions(dataDims);
  rectGrid->SetXCoordinates(pxCoords);
  rectGrid->SetYCoordinates(pyCoords);
  rectGrid->SetZCoordinates(pzCoords);
  rectGrid->GetPointData()->ShallowCopy(volImage->GetPointData());
  rectGrid->GetCellData()->ShallowCopy(volImage->GetCellData());

  this->ClipRectilinearGridData(rectGrid, clipAray, isoValue, outputUG);

  pxCoords->Delete();
  pyCoords->Delete();
  pzCoords->Delete();
  rectGrid->Delete();
  pxCoords = nullptr;
  pyCoords = nullptr;
  pzCoords = nullptr;
  rectGrid = nullptr;
  volImage = nullptr;
  dataBBox = nullptr;
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::ClipPolyData(
  svtkDataSet* inputGrd, svtkDataArray* clipAray, double isoValue, svtkUnstructuredGrid* outputUG)
{
  svtkPolyData* polyData = svtkPolyData::SafeDownCast(inputGrd);
  svtkIdType numCells = polyData->GetNumberOfCells();

  svtkTableBasedClipperVolumeFromVolume* visItVFV =
    new svtkTableBasedClipperVolumeFromVolume(this->OutputPointsPrecision,
      polyData->GetNumberOfPoints(), int(pow(double(numCells), double(0.6667f))) * 5 + 100);

  svtkUnstructuredGrid* specials = svtkUnstructuredGrid::New();
  specials->SetPoints(polyData->GetPoints());
  specials->GetPointData()->ShallowCopy(polyData->GetPointData());
  specials->Allocate(numCells);

  svtkIdType i, j;
  svtkIdType numbPnts = 0;
  int numCants = 0; // number of cells not clipped by this filter

  for (i = 0; i < numCells; i++)
  {
    int cellType = polyData->GetCellType(i);
    bool bCanClip = false;
    const svtkIdType* pntIndxs = nullptr;
    polyData->GetCellPoints(i, numbPnts, pntIndxs);

    switch (cellType)
    {
      case SVTK_TETRA:
      case SVTK_PYRAMID:
      case SVTK_WEDGE:
      case SVTK_HEXAHEDRON:
      case SVTK_TRIANGLE:
      case SVTK_QUAD:
      case SVTK_LINE:
      case SVTK_VERTEX:
        bCanClip = true;
        break;

      default:
        bCanClip = false;
        break;
    }

    if (bCanClip)
    {
      double grdDiffs[8];
      int caseIndx = 0;

      for (j = numbPnts - 1; j >= 0; j--)
      {
        grdDiffs[j] = clipAray->GetComponent(pntIndxs[j], 0) - isoValue;
        caseIndx += ((grdDiffs[j] >= 0.0) ? 1 : 0);
        caseIndx <<= (1 - (!j));
      }

      int startIdx = 0;
      int nOutputs = 0;
      typedef int EDGEIDXS[2];
      const EDGEIDXS* edgeVtxs = nullptr;
      unsigned char* thisCase = nullptr;

      switch (cellType)
      {
        case SVTK_TETRA:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesTet[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesTet[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesTet[caseIndx];
          edgeVtxs = svtkTableBasedClipperTriangulationTables::TetVerticesFromEdges;
          break;

        case SVTK_PYRAMID:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesPyr[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesPyr[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesPyr[caseIndx];
          edgeVtxs = svtkTableBasedClipperTriangulationTables::PyramidVerticesFromEdges;
          break;

        case SVTK_WEDGE:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesWdg[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesWdg[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesWdg[caseIndx];
          edgeVtxs = svtkTableBasedClipperTriangulationTables::WedgeVerticesFromEdges;
          break;

        case SVTK_HEXAHEDRON:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesHex[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesHex[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesHex[caseIndx];
          edgeVtxs = svtkTableBasedClipperTriangulationTables::HexVerticesFromEdges;
          break;

        case SVTK_TRIANGLE:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesTri[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesTri[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesTri[caseIndx];
          edgeVtxs = svtkTableBasedClipperTriangulationTables::TriVerticesFromEdges;
          break;

        case SVTK_QUAD:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesQua[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesQua[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesQua[caseIndx];
          edgeVtxs = svtkTableBasedClipperTriangulationTables::QuadVerticesFromEdges;
          break;

        case SVTK_LINE:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesLin[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesLin[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesLin[caseIndx];
          edgeVtxs = svtkTableBasedClipperTriangulationTables::LineVerticesFromEdges;
          break;

        case SVTK_VERTEX:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesVtx[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesVtx[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesVtx[caseIndx];
          edgeVtxs = nullptr;
          break;
      }

      svtkIdType intrpIds[4];
      for (j = 0; j < nOutputs; j++)
      {
        int nCellPts = 0;
        int intrpIdx = -1;
        int theColor = -1;
        unsigned char theShape = *thisCase++;

        switch (theShape)
        {
          case ST_HEX:
            nCellPts = 8;
            theColor = *thisCase++;
            break;

          case ST_WDG:
            nCellPts = 6;
            theColor = *thisCase++;
            break;

          case ST_PYR:
            nCellPts = 5;
            theColor = *thisCase++;
            break;
          case ST_TET:
            nCellPts = 4;
            theColor = *thisCase++;
            break;

          case ST_QUA:
            nCellPts = 4;
            theColor = *thisCase++;
            break;

          case ST_TRI:
            nCellPts = 3;
            theColor = *thisCase++;
            break;

          case ST_LIN:
            nCellPts = 2;
            theColor = *thisCase++;
            break;

          case ST_VTX:
            nCellPts = 1;
            theColor = *thisCase++;
            break;

          case ST_PNT:
            intrpIdx = *thisCase++;
            theColor = *thisCase++;
            nCellPts = *thisCase++;
            break;

          default:
            svtkErrorMacro(<< "An invalid output shape was found in "
                          << "the ClipCases." << endl);
        }

        if ((!this->InsideOut && theColor == COLOR0) || (this->InsideOut && theColor == COLOR1))
        {
          // We don't want this one; it's the wrong side.
          thisCase += nCellPts;
          continue;
        }

        svtkIdType shapeIds[8];
        for (int p = 0; p < nCellPts; p++)
        {
          unsigned char pntIndex = *thisCase++;

          if (pntIndex <= P7)
          {
            shapeIds[p] = pntIndxs[pntIndex];
          }
          else if (pntIndex >= EA && pntIndex <= EL)
          {
            int pt1Index = edgeVtxs[pntIndex - EA][0];
            int pt2Index = edgeVtxs[pntIndex - EA][1];
            if (pt2Index < pt1Index)
            {
              int temp = pt2Index;
              pt2Index = pt1Index;
              pt1Index = temp;
            }
            double pt1ToPt2 = grdDiffs[pt2Index] - grdDiffs[pt1Index];
            double pt1ToIso = 0.0 - grdDiffs[pt1Index];
            double p1Weight = 1.0 - pt1ToIso / pt1ToPt2;

            svtkIdType pntIndx1 = pntIndxs[pt1Index];
            svtkIdType pntIndx2 = pntIndxs[pt2Index];

            shapeIds[p] = visItVFV->AddPoint(pntIndx1, pntIndx2, p1Weight);
          }
          else if (pntIndex >= N0 && pntIndex <= N3)
          {
            shapeIds[p] = static_cast<int>(intrpIds[pntIndex - N0]);
          }
          else
          {
            svtkErrorMacro(<< "An invalid output point value "
                          << "was found in the ClipCases." << endl);
          }
        }

        switch (theShape)
        {
          case ST_HEX:
            visItVFV->AddHex(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4],
              shapeIds[5], shapeIds[6], shapeIds[7]);
            break;

          case ST_WDG:
            visItVFV->AddWedge(
              i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4], shapeIds[5]);
            break;

          case ST_PYR:
            visItVFV->AddPyramid(
              i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4]);
            break;

          case ST_TET:
            visItVFV->AddTet(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3]);
            break;

          case ST_QUA:
            visItVFV->AddQuad(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3]);
            break;

          case ST_TRI:
            visItVFV->AddTri(i, shapeIds[0], shapeIds[1], shapeIds[2]);
            break;

          case ST_LIN:
            visItVFV->AddLine(i, shapeIds[0], shapeIds[1]);
            break;

          case ST_VTX:
            visItVFV->AddVertex(i, shapeIds[0]);
            break;

          case ST_PNT:
            intrpIds[intrpIdx] = visItVFV->AddCentroidPoint(nCellPts, shapeIds);
            break;
        }
      }

      edgeVtxs = nullptr;
      thisCase = nullptr;
    }
    else
    {
      if (numCants == 0)
      {
        specials->GetCellData()->CopyAllocate(polyData->GetCellData(), numCells);
      }

      specials->InsertNextCell(cellType, numbPnts, pntIndxs);
      specials->GetCellData()->CopyData(polyData->GetCellData(), i, numCants);
      numCants++;
    }

    pntIndxs = nullptr;
  }

  int toDelete = 0;
  double* theCords = nullptr;
  svtkPoints* inputPts = polyData->GetPoints();
  if (inputPts->GetDataType() == SVTK_DOUBLE)
  {
    theCords = static_cast<double*>(inputPts->GetVoidPointer(0));
  }
  else
  {
    toDelete = 1;
    numbPnts = inputPts->GetNumberOfPoints();
    theCords = new double[numbPnts * 3];
    for (i = 0; i < numbPnts; i++)
    {
      inputPts->GetPoint(i, theCords + (i << 1) + i);
    }
  }
  inputPts = nullptr;

  if (numCants > 0)
  {
    svtkUnstructuredGrid* svtkUGrid = svtkUnstructuredGrid::New();
    this->ClipDataSet(specials, clipAray, svtkUGrid);

    svtkUnstructuredGrid* visItGrd = svtkUnstructuredGrid::New();
    visItVFV->ConstructDataSet(polyData, visItGrd, theCords);

    svtkAppendFilter* appender = svtkAppendFilter::New();
    appender->AddInputData(svtkUGrid);
    appender->AddInputData(visItGrd);
    appender->Update();

    outputUG->ShallowCopy(appender->GetOutput());

    appender->Delete();
    svtkUGrid->Delete();
    visItGrd->Delete();
    appender = nullptr;
    svtkUGrid = nullptr;
    visItGrd = nullptr;
  }
  else
  {
    visItVFV->ConstructDataSet(polyData, outputUG, theCords);
  }

  specials->Delete();
  delete visItVFV;
  if (toDelete)
  {
    delete[] theCords;
  }
  specials = nullptr;
  visItVFV = nullptr;
  theCords = nullptr;
  polyData = nullptr;
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::ClipRectilinearGridData(
  svtkDataSet* inputGrd, svtkDataArray* clipAray, double isoValue, svtkUnstructuredGrid* outputUG)
{
  svtkRectilinearGrid* rectGrid = svtkRectilinearGrid::SafeDownCast(inputGrd);

  svtkIdType i, j;
  svtkIdType numCells = 0;
  int isTwoDim = 0;
  enum TwoDimType
  {
    XY,
    YZ,
    XZ
  };
  TwoDimType twoDimType;
  int rectDims[3];
  rectGrid->GetDimensions(rectDims);
  isTwoDim = int(rectDims[0] <= 1 || rectDims[1] <= 1 || rectDims[2] <= 1);
  if (rectDims[0] <= 1)
    twoDimType = YZ;
  else if (rectDims[1] <= 1)
    twoDimType = XZ;
  else
    twoDimType = XY;
  numCells = rectGrid->GetNumberOfCells();

  svtkTableBasedClipperVolumeFromVolume* visItVFV = new svtkTableBasedClipperVolumeFromVolume(
    this->OutputPointsPrecision, rectGrid->GetNumberOfPoints(),
    static_cast<svtkIdType>(pow(double(numCells), double(0.6667f)) * 5 + 100));

  int shiftLUTx[8] = { 0, 1, 1, 0, 0, 1, 1, 0 };
  int shiftLUTy[8] = { 0, 0, 1, 1, 0, 0, 1, 1 };
  int shiftLUTz[8] = { 0, 0, 0, 0, 1, 1, 1, 1 };

  int* shiftLUT[3];
  if (isTwoDim && twoDimType == XZ)
  {
    shiftLUT[0] = shiftLUTx;
    shiftLUT[1] = shiftLUTz;
    shiftLUT[2] = shiftLUTy;
  }
  else if (isTwoDim && twoDimType == YZ)
  {
    shiftLUT[0] = shiftLUTy;
    shiftLUT[1] = shiftLUTz;
    shiftLUT[2] = shiftLUTx;
  }
  else
  {
    shiftLUT[0] = shiftLUTx;
    shiftLUT[1] = shiftLUTy;
    shiftLUT[2] = shiftLUTz;
  }

  int cellDims[3] = { rectDims[0] - 1, rectDims[1] - 1, rectDims[2] - 1 };
  int cyStride = (cellDims[0] ? cellDims[0] : 1);
  int czStride = (cellDims[0] ? cellDims[0] : 1) * (cellDims[1] ? cellDims[1] : 1);
  int pyStride = rectDims[0];
  int pzStride = rectDims[0] * rectDims[1];

  for (i = 0; i < numCells; i++)
  {
    int caseIndx = 0;
    int nCellPts = isTwoDim ? 4 : 8;
    svtkIdType theCellI = (cellDims[0] > 0 ? i % cellDims[0] : 0);
    svtkIdType theCellJ = (cellDims[1] > 0 ? (i / cyStride) % cellDims[1] : 0);
    svtkIdType theCellK = (cellDims[2] > 0 ? (i / czStride) : 0);
    double grdDiffs[8];

    for (j = static_cast<svtkIdType>(nCellPts) - 1; j >= 0; j--)
    {
      grdDiffs[j] = clipAray->GetComponent((theCellK + shiftLUT[2][j]) * pzStride +
                        (theCellJ + shiftLUT[1][j]) * pyStride + (theCellI + shiftLUT[0][j]),
                      0) -
        isoValue;
      caseIndx += ((grdDiffs[j] >= 0.0) ? 1 : 0);
      caseIndx <<= (1 - (!j));
    }

    int nOutputs;
    int intrpIds[4];
    unsigned char* thisCase = nullptr;

    if (isTwoDim)
    {
      thisCase = &svtkTableBasedClipperClipTables::ClipShapesQua
                   [svtkTableBasedClipperClipTables::StartClipShapesQua[caseIndx]];
      nOutputs = svtkTableBasedClipperClipTables::NumClipShapesQua[caseIndx];
    }
    else
    {
      thisCase = &svtkTableBasedClipperClipTables::ClipShapesHex
                   [svtkTableBasedClipperClipTables::StartClipShapesHex[caseIndx]];
      nOutputs = svtkTableBasedClipperClipTables::NumClipShapesHex[caseIndx];
    }

    for (j = 0; j < nOutputs; j++)
    {
      int intrpIdx = -1;
      int theColor = -1;
      unsigned char theShape = *thisCase++;

      nCellPts = 0;
      switch (theShape)
      {
        case ST_HEX:
          nCellPts = 8;
          theColor = *thisCase++;
          break;

        case ST_WDG:
          nCellPts = 6;
          theColor = *thisCase++;
          break;

        case ST_PYR:
          nCellPts = 5;
          theColor = *thisCase++;
          break;

        case ST_TET:
          nCellPts = 4;
          theColor = *thisCase++;
          break;

        case ST_QUA:
          nCellPts = 4;
          theColor = *thisCase++;
          break;

        case ST_TRI:
          nCellPts = 3;
          theColor = *thisCase++;
          break;

        case ST_LIN:
          nCellPts = 2;
          theColor = *thisCase++;
          break;

        case ST_VTX:
          nCellPts = 1;
          theColor = *thisCase++;
          break;

        case ST_PNT:
          intrpIdx = *thisCase++;
          theColor = *thisCase++;
          nCellPts = *thisCase++;
          break;

        default:
          svtkErrorMacro(<< "An invalid output shape was found in "
                        << "the ClipCases." << endl);
      }

      if ((!this->InsideOut && theColor == COLOR0) || (this->InsideOut && theColor == COLOR1))
      {
        // We don't want this one; it's the wrong side.
        thisCase += nCellPts;
        continue;
      }

      svtkIdType shapeIds[8];
      for (int p = 0; p < nCellPts; p++)
      {
        unsigned char pntIndex = *thisCase++;

        if (pntIndex <= P7)
        {
          // We know pt P0 must be >P0 since we already
          // assume P0 == 0.  This is why we do not
          // bother subtracting P0 from pt here.
          shapeIds[p] =
            ((theCellI + shiftLUT[0][pntIndex]) + (theCellJ + shiftLUT[1][pntIndex]) * pyStride +
              (theCellK + shiftLUT[2][pntIndex]) * pzStride);
        }
        else if (pntIndex >= EA && pntIndex <= EL)
        {
          int pt1Index =
            svtkTableBasedClipperTriangulationTables::HexVerticesFromEdges[pntIndex - EA][0];
          int pt2Index =
            svtkTableBasedClipperTriangulationTables::HexVerticesFromEdges[pntIndex - EA][1];

          if (pt2Index < pt1Index)
          {
            int temp = pt2Index;
            pt2Index = pt1Index;
            pt1Index = temp;
          }

          double pt1ToPt2 = grdDiffs[pt2Index] - grdDiffs[pt1Index];
          double pt1ToIso = 0.0 - grdDiffs[pt1Index];
          double p1Weight = 1.0 - pt1ToIso / pt1ToPt2;

          int pntIndx1 =
            ((theCellI + shiftLUT[0][pt1Index]) + (theCellJ + shiftLUT[1][pt1Index]) * pyStride +
              (theCellK + shiftLUT[2][pt1Index]) * pzStride);
          int pntIndx2 =
            ((theCellI + shiftLUT[0][pt2Index]) + (theCellJ + shiftLUT[1][pt2Index]) * pyStride +
              (theCellK + shiftLUT[2][pt2Index]) * pzStride);

          /* We may have physically (though not logically) degenerate cells
          // if p1Weight == 0 or p1Weight == 1. We could pretty easily and
          // mostly safely clamp percent to the range [1e-4, 1 - 1e-4].
          if( p1Weight == 1.0)
            {
            shapeIds[p] = pntIndx1;
            }
          else
          if( p1Weight == 0.0 )
            {
            shapeIds[p] = pntIndx2;
            }
          else

            {
            shapeIds[p] = visItVFV->AddPoint( pntIndx1, pntIndx2, p1Weight );
            }
          */

          // Turning on the above code segment, the alternative, would cause
          // a bug with a synthetic Wavelet dataset (svtkImageData) when the
          // the clipping plane (x/y/z axis) is positioned exactly at (0,0,0).
          // The problem occurs in the form of an open 'box', as opposed to an
          // expected closed one. This is due to the use of hash instead of a
          // point-locator based detection of duplicate points.
          shapeIds[p] = visItVFV->AddPoint(pntIndx1, pntIndx2, p1Weight);
        }
        else if (pntIndex >= N0 && pntIndex <= N3)
        {
          shapeIds[p] = intrpIds[pntIndex - N0];
        }
        else
        {
          svtkErrorMacro(<< "An invalid output point value "
                        << "was found in the ClipCases." << endl);
        }
      }

      switch (theShape)
      {
        case ST_HEX:
          visItVFV->AddHex(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4],
            shapeIds[5], shapeIds[6], shapeIds[7]);
          break;

        case ST_WDG:
          visItVFV->AddWedge(
            i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4], shapeIds[5]);
          break;

        case ST_PYR:
          visItVFV->AddPyramid(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4]);
          break;

        case ST_TET:
          visItVFV->AddTet(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3]);
          break;

        case ST_QUA:
          visItVFV->AddQuad(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3]);
          break;

        case ST_TRI:
          visItVFV->AddTri(i, shapeIds[0], shapeIds[1], shapeIds[2]);
          break;

        case ST_LIN:
          visItVFV->AddLine(i, shapeIds[0], shapeIds[1]);
          break;

        case ST_VTX:
          visItVFV->AddVertex(i, shapeIds[0]);
          break;

        case ST_PNT:
          intrpIds[intrpIdx] = visItVFV->AddCentroidPoint(nCellPts, shapeIds);
          break;
      }
    }

    thisCase = nullptr;
  }

  int toDelete = 0;
  double* theCords[3] = { nullptr, nullptr, nullptr };
  svtkDataArray* theArays[3] = { nullptr, nullptr, nullptr };

  if (rectGrid->GetXCoordinates()->GetDataType() == SVTK_DOUBLE &&
    rectGrid->GetYCoordinates()->GetDataType() == SVTK_DOUBLE &&
    rectGrid->GetZCoordinates()->GetDataType() == SVTK_DOUBLE)
  {
    theCords[0] = static_cast<double*>(rectGrid->GetXCoordinates()->GetVoidPointer(0));
    theCords[1] = static_cast<double*>(rectGrid->GetYCoordinates()->GetVoidPointer(0));
    theCords[2] = static_cast<double*>(rectGrid->GetZCoordinates()->GetVoidPointer(0));
  }
  else
  {
    toDelete = 1;
    theArays[0] = rectGrid->GetXCoordinates();
    theArays[1] = rectGrid->GetYCoordinates();
    theArays[2] = rectGrid->GetZCoordinates();
    for (j = 0; j < 3; j++)
    {
      theCords[j] = new double[rectDims[j]];
      for (i = 0; i < rectDims[j]; i++)
      {
        theCords[j][i] = theArays[j]->GetComponent(i, 0);
      }
      theArays[j] = nullptr;
    }
  }

  visItVFV->ConstructDataSet(rectGrid, outputUG, rectDims, theCords[0], theCords[1], theCords[2]);

  delete visItVFV;
  visItVFV = nullptr;
  rectGrid = nullptr;

  for (i = 0; i < 3; i++)
  {
    if (toDelete)
    {
      delete[] theCords[i];
    }
    theCords[i] = nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::ClipStructuredGridData(
  svtkDataSet* inputGrd, svtkDataArray* clipAray, double isoValue, svtkUnstructuredGrid* outputUG)
{
  svtkStructuredGrid* strcGrid = svtkStructuredGrid::SafeDownCast(inputGrd);

  svtkIdType i, j;
  int isTwoDim = 0;
  enum TwoDimType
  {
    XY,
    YZ,
    XZ
  };
  TwoDimType twoDimType;
  svtkIdType numCells = 0;
  int gridDims[3] = { 0, 0, 0 };
  strcGrid->GetDimensions(gridDims);
  isTwoDim = int(gridDims[0] <= 1 || gridDims[1] <= 1 || gridDims[2] <= 1);
  if (gridDims[0] <= 1)
    twoDimType = YZ;
  else if (gridDims[1] <= 1)
    twoDimType = XZ;
  else
    twoDimType = XY;
  numCells = strcGrid->GetNumberOfCells();

  svtkTableBasedClipperVolumeFromVolume* visItVFV =
    new svtkTableBasedClipperVolumeFromVolume(this->OutputPointsPrecision,
      strcGrid->GetNumberOfPoints(), int(pow(double(numCells), double(0.6667f))) * 5 + 100);

  int shiftLUTx[8] = { 0, 1, 1, 0, 0, 1, 1, 0 };
  int shiftLUTy[8] = { 0, 0, 1, 1, 0, 0, 1, 1 };
  int shiftLUTz[8] = { 0, 0, 0, 0, 1, 1, 1, 1 };

  int* shiftLUT[3];
  if (isTwoDim && twoDimType == XZ)
  {
    shiftLUT[0] = shiftLUTx;
    shiftLUT[1] = shiftLUTz;
    shiftLUT[2] = shiftLUTy;
  }
  else if (isTwoDim && twoDimType == YZ)
  {
    shiftLUT[0] = shiftLUTy;
    shiftLUT[1] = shiftLUTz;
    shiftLUT[2] = shiftLUTx;
  }
  else
  {
    shiftLUT[0] = shiftLUTx;
    shiftLUT[1] = shiftLUTy;
    shiftLUT[2] = shiftLUTz;
  }

  svtkIdType numbPnts = 0;
  int cellDims[3] = { gridDims[0] - 1, gridDims[1] - 1, gridDims[2] - 1 };
  int cyStride = (cellDims[0] ? cellDims[0] : 1);
  int czStride = (cellDims[0] ? cellDims[0] : 1) * (cellDims[1] ? cellDims[1] : 1);
  int pyStride = gridDims[0];
  int pzStride = gridDims[0] * gridDims[1];

  for (i = 0; i < numCells; i++)
  {
    int caseIndx = 0;
    int theCellI = (cellDims[0] > 0 ? i % cellDims[0] : 0);
    int theCellJ = (cellDims[1] > 0 ? (i / cyStride) % cellDims[1] : 0);
    int theCellK = (cellDims[2] > 0 ? (i / czStride) : 0);
    double grdDiffs[8];

    numbPnts = isTwoDim ? 4 : 8;

    for (j = numbPnts - 1; j >= 0; j--)
    {
      int pntIndex = (theCellI + shiftLUT[0][j]) + (theCellJ + shiftLUT[1][j]) * pyStride +
        (theCellK + shiftLUT[2][j]) * pzStride;

      grdDiffs[j] = clipAray->GetComponent(pntIndex, 0) - isoValue;
      caseIndx += ((grdDiffs[j] >= 0.0) ? 1 : 0);
      caseIndx <<= (1 - (!j));
    }

    int nOutputs;
    svtkIdType intrpIds[4];
    unsigned char* thisCase = nullptr;

    if (isTwoDim)
    {
      thisCase = &svtkTableBasedClipperClipTables::ClipShapesQua
                   [svtkTableBasedClipperClipTables::StartClipShapesQua[caseIndx]];
      nOutputs = svtkTableBasedClipperClipTables::NumClipShapesQua[caseIndx];
    }
    else
    {
      thisCase = &svtkTableBasedClipperClipTables::ClipShapesHex
                   [svtkTableBasedClipperClipTables::StartClipShapesHex[caseIndx]];
      nOutputs = svtkTableBasedClipperClipTables::NumClipShapesHex[caseIndx];
    }

    for (j = 0; j < nOutputs; j++)
    {
      int nCellPts = 0;
      int intrpIdx = -1;
      int theColor = -1;
      unsigned char theShape = *thisCase++;

      switch (theShape)
      {
        case ST_HEX:
          nCellPts = 8;
          theColor = *thisCase++;
          break;

        case ST_WDG:
          nCellPts = 6;
          theColor = *thisCase++;
          break;

        case ST_PYR:
          nCellPts = 5;
          theColor = *thisCase++;
          break;

        case ST_TET:
          nCellPts = 4;
          theColor = *thisCase++;
          break;

        case ST_QUA:
          nCellPts = 4;
          theColor = *thisCase++;
          break;

        case ST_TRI:
          nCellPts = 3;
          theColor = *thisCase++;
          break;

        case ST_LIN:
          nCellPts = 2;
          theColor = *thisCase++;
          break;

        case ST_VTX:
          nCellPts = 1;
          theColor = *thisCase++;
          break;

        case ST_PNT:
          intrpIdx = *thisCase++;
          theColor = *thisCase++;
          nCellPts = *thisCase++;
          break;

        default:
          svtkErrorMacro(<< "An invalid output shape was found in "
                        << "the ClipCases." << endl);
      }

      if ((!this->InsideOut && theColor == COLOR0) || (this->InsideOut && theColor == COLOR1))
      {
        // We don't want this one; it's the wrong side.
        thisCase += nCellPts;
        continue;
      }

      svtkIdType shapeIds[8];
      for (int p = 0; p < nCellPts; p++)
      {
        unsigned char pntIndex = *thisCase++;

        if (pntIndex <= P7)
        {
          // We know pt P0 must be >P0 since we already
          // assume P0 == 0.  This is why we do not
          // bother subtracting P0 from pt here.
          shapeIds[p] =
            ((theCellI + shiftLUT[0][pntIndex]) + (theCellJ + shiftLUT[1][pntIndex]) * pyStride +
              (theCellK + shiftLUT[2][pntIndex]) * pzStride);
        }
        else if (pntIndex >= EA && pntIndex <= EL)
        {
          int pt1Index =
            svtkTableBasedClipperTriangulationTables::HexVerticesFromEdges[pntIndex - EA][0];
          int pt2Index =
            svtkTableBasedClipperTriangulationTables::HexVerticesFromEdges[pntIndex - EA][1];

          if (pt2Index < pt1Index)
          {
            int temp = pt2Index;
            pt2Index = pt1Index;
            pt1Index = temp;
          }

          double pt1ToPt2 = grdDiffs[pt2Index] - grdDiffs[pt1Index];
          double pt1ToIso = 0.0 - grdDiffs[pt1Index];
          double p1Weight = 1.0 - pt1ToIso / pt1ToPt2;

          int pntIndx1 =
            ((theCellI + shiftLUT[0][pt1Index]) + (theCellJ + shiftLUT[1][pt1Index]) * pyStride +
              (theCellK + shiftLUT[2][pt1Index]) * pzStride);
          int pntIndx2 =
            ((theCellI + shiftLUT[0][pt2Index]) + (theCellJ + shiftLUT[1][pt2Index]) * pyStride +
              (theCellK + shiftLUT[2][pt2Index]) * pzStride);

          shapeIds[p] = visItVFV->AddPoint(pntIndx1, pntIndx2, p1Weight);
        }
        else if (pntIndex >= N0 && pntIndex <= N3)
        {
          shapeIds[p] = intrpIds[pntIndex - N0];
        }
        else
        {
          svtkErrorMacro(<< "An invalid output point value "
                        << "was found in the ClipCases." << endl);
        }
      }

      switch (theShape)
      {
        case ST_HEX:
          visItVFV->AddHex(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4],
            shapeIds[5], shapeIds[6], shapeIds[7]);
          break;

        case ST_WDG:
          visItVFV->AddWedge(
            i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4], shapeIds[5]);
          break;

        case ST_PYR:
          visItVFV->AddPyramid(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4]);
          break;

        case ST_TET:
          visItVFV->AddTet(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3]);
          break;

        case ST_QUA:
          visItVFV->AddQuad(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3]);
          break;

        case ST_TRI:
          visItVFV->AddTri(i, shapeIds[0], shapeIds[1], shapeIds[2]);
          break;

        case ST_LIN:
          visItVFV->AddLine(i, shapeIds[0], shapeIds[1]);
          break;

        case ST_VTX:
          visItVFV->AddVertex(i, shapeIds[0]);
          break;

        case ST_PNT:
          intrpIds[intrpIdx] = visItVFV->AddCentroidPoint(nCellPts, shapeIds);
          break;
      }
    }

    thisCase = nullptr;
  }

  int toDelete = 0;
  double* theCords = nullptr;
  svtkPoints* inputPts = strcGrid->GetPoints();
  if (inputPts->GetDataType() == SVTK_DOUBLE)
  {
    theCords = static_cast<double*>(inputPts->GetVoidPointer(0));
  }
  else
  {
    toDelete = 1;
    numbPnts = inputPts->GetNumberOfPoints();
    theCords = new double[numbPnts * 3];
    for (i = 0; i < numbPnts; i++)
    {
      inputPts->GetPoint(i, theCords + (i << 1) + i);
    }
  }
  inputPts = nullptr;

  visItVFV->ConstructDataSet(strcGrid, outputUG, theCords);

  delete visItVFV;
  if (toDelete)
  {
    delete[] theCords;
  }
  visItVFV = nullptr;
  theCords = nullptr;
  strcGrid = nullptr;
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::ClipUnstructuredGridData(
  svtkDataSet* inputGrd, svtkDataArray* clipAray, double isoValue, svtkUnstructuredGrid* outputUG)
{
  svtkUnstructuredGrid* unstruct = svtkUnstructuredGrid::SafeDownCast(inputGrd);

  svtkIdType i, j;
  svtkIdType numbPnts = 0;
  int numCants = 0; // number of cells not clipped by this filter
  svtkIdType numCells = unstruct->GetNumberOfCells();

  // volume from volume
  svtkTableBasedClipperVolumeFromVolume* visItVFV =
    new svtkTableBasedClipperVolumeFromVolume(this->OutputPointsPrecision,
      unstruct->GetNumberOfPoints(), int(pow(double(numCells), double(0.6667f))) * 5 + 100);

  // the stuffs that can not be clipped by this filter
  svtkUnstructuredGrid* specials = svtkUnstructuredGrid::New();
  specials->SetPoints(unstruct->GetPoints());
  specials->GetPointData()->ShallowCopy(unstruct->GetPointData());
  specials->Allocate(numCells);

  for (i = 0; i < numCells; i++)
  {
    int cellType = unstruct->GetCellType(i);
    const svtkIdType* pntIndxs = nullptr;
    unstruct->GetCellPoints(i, numbPnts, pntIndxs);

    bool bCanClip = false;
    switch (cellType)
    {
      case SVTK_TETRA:
      case SVTK_PYRAMID:
      case SVTK_WEDGE:
      case SVTK_HEXAHEDRON:
      case SVTK_VOXEL:
      case SVTK_TRIANGLE:
      case SVTK_QUAD:
      case SVTK_PIXEL:
      case SVTK_LINE:
      case SVTK_VERTEX:
        bCanClip = true;
        break;

      default:
        bCanClip = false;
        break;
    }

    if (bCanClip)
    {
      int caseIndx = 0;
      double grdDiffs[8];

      for (j = numbPnts - 1; j >= 0; j--)
      {
        grdDiffs[j] = clipAray->GetComponent(pntIndxs[j], 0) - isoValue;
        caseIndx += ((grdDiffs[j] >= 0.0) ? 1 : 0);
        caseIndx <<= (1 - (!j));
      }

      int startIdx = 0;
      int nOutputs = 0;
      typedef const int EDGEIDXS[2];
      EDGEIDXS* edgeVtxs = nullptr;
      unsigned char* thisCase = nullptr;

      // start index, split case, number of output, and vertices from edges
      switch (cellType)
      {
        case SVTK_TETRA:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesTet[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesTet[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesTet[caseIndx];
          edgeVtxs = (EDGEIDXS*)svtkTableBasedClipperTriangulationTables::TetVerticesFromEdges;
          break;

        case SVTK_PYRAMID:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesPyr[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesPyr[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesPyr[caseIndx];
          edgeVtxs = (EDGEIDXS*)svtkTableBasedClipperTriangulationTables::PyramidVerticesFromEdges;
          break;

        case SVTK_WEDGE:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesWdg[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesWdg[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesWdg[caseIndx];
          edgeVtxs = (EDGEIDXS*)svtkTableBasedClipperTriangulationTables::WedgeVerticesFromEdges;
          break;

        case SVTK_HEXAHEDRON:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesHex[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesHex[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesHex[caseIndx];
          edgeVtxs = (EDGEIDXS*)svtkTableBasedClipperTriangulationTables::HexVerticesFromEdges;
          break;

        case SVTK_VOXEL:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesVox[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesVox[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesVox[caseIndx];
          edgeVtxs = (EDGEIDXS*)svtkTableBasedClipperTriangulationTables::VoxVerticesFromEdges;
          break;

        case SVTK_TRIANGLE:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesTri[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesTri[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesTri[caseIndx];
          edgeVtxs = (EDGEIDXS*)svtkTableBasedClipperTriangulationTables::TriVerticesFromEdges;
          break;

        case SVTK_QUAD:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesQua[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesQua[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesQua[caseIndx];
          edgeVtxs = (EDGEIDXS*)svtkTableBasedClipperTriangulationTables::QuadVerticesFromEdges;
          break;

        case SVTK_PIXEL:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesPix[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesPix[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesPix[caseIndx];
          edgeVtxs = (EDGEIDXS*)svtkTableBasedClipperTriangulationTables::PixelVerticesFromEdges;
          break;

        case SVTK_LINE:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesLin[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesLin[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesLin[caseIndx];
          edgeVtxs = (EDGEIDXS*)svtkTableBasedClipperTriangulationTables::LineVerticesFromEdges;
          break;

        case SVTK_VERTEX:
          startIdx = svtkTableBasedClipperClipTables::StartClipShapesVtx[caseIndx];
          thisCase = &svtkTableBasedClipperClipTables::ClipShapesVtx[startIdx];
          nOutputs = svtkTableBasedClipperClipTables::NumClipShapesVtx[caseIndx];
          edgeVtxs = nullptr;
          break;
      }

      int intrpIds[4];
      for (j = 0; j < nOutputs; j++)
      {
        int nCellPts = 0;
        int theColor = -1;
        int intrpIdx = -1;
        unsigned char theShape = *thisCase++;

        // number of points and color
        switch (theShape)
        {
          case ST_HEX:
            nCellPts = 8;
            theColor = *thisCase++;
            break;

          case ST_WDG:
            nCellPts = 6;
            theColor = *thisCase++;
            break;

          case ST_PYR:
            nCellPts = 5;
            theColor = *thisCase++;
            break;

          case ST_TET:
            nCellPts = 4;
            theColor = *thisCase++;
            break;

          case ST_QUA:
            nCellPts = 4;
            theColor = *thisCase++;
            break;

          case ST_TRI:
            nCellPts = 3;
            theColor = *thisCase++;
            break;

          case ST_LIN:
            nCellPts = 2;
            theColor = *thisCase++;
            break;

          case ST_VTX:
            nCellPts = 1;
            theColor = *thisCase++;
            break;

          case ST_PNT:
            intrpIdx = *thisCase++;
            theColor = *thisCase++;
            nCellPts = *thisCase++;
            break;

          default:
            svtkErrorMacro(<< "An invalid output shape was found "
                          << "in the ClipCases." << endl);
        }

        if ((!this->InsideOut && theColor == COLOR0) || (this->InsideOut && theColor == COLOR1))
        {
          // We don't want this one; it's the wrong side.
          thisCase += nCellPts;
          continue;
        }

        svtkIdType shapeIds[8];
        for (int p = 0; p < nCellPts; p++)
        {
          unsigned char pntIndex = *thisCase++;

          if (pntIndex <= P7)
          {
            // We know pt P0 must be >P0 since we already
            // assume P0 == 0.  This is why we do not
            // bother subtracting P0 from pt here.
            shapeIds[p] = pntIndxs[pntIndex];
          }
          else if (pntIndex >= EA && pntIndex <= EL)
          {
            int pt1Index = edgeVtxs[pntIndex - EA][0];
            int pt2Index = edgeVtxs[pntIndex - EA][1];
            if (pt2Index < pt1Index)
            {
              int temp = pt2Index;
              pt2Index = pt1Index;
              pt1Index = temp;
            }
            double pt1ToPt2 = grdDiffs[pt2Index] - grdDiffs[pt1Index];
            double pt1ToIso = 0.0 - grdDiffs[pt1Index];
            double p1Weight = 1.0 - pt1ToIso / pt1ToPt2;

            svtkIdType pntIndx1 = pntIndxs[pt1Index];
            svtkIdType pntIndx2 = pntIndxs[pt2Index];

            shapeIds[p] = visItVFV->AddPoint(pntIndx1, pntIndx2, p1Weight);
          }
          else if (pntIndex >= N0 && pntIndex <= N3)
          {
            shapeIds[p] = intrpIds[pntIndex - N0];
          }
          else
          {
            svtkErrorMacro(<< "An invalid output point value was found "
                          << "in the ClipCases." << endl);
          }
        }

        switch (theShape)
        {
          case ST_HEX:
            visItVFV->AddHex(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4],
              shapeIds[5], shapeIds[6], shapeIds[7]);
            break;

          case ST_WDG:
            visItVFV->AddWedge(
              i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4], shapeIds[5]);
            break;

          case ST_PYR:
            visItVFV->AddPyramid(
              i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3], shapeIds[4]);
            break;

          case ST_TET:
            visItVFV->AddTet(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3]);
            break;

          case ST_QUA:
            visItVFV->AddQuad(i, shapeIds[0], shapeIds[1], shapeIds[2], shapeIds[3]);
            break;

          case ST_TRI:
            visItVFV->AddTri(i, shapeIds[0], shapeIds[1], shapeIds[2]);
            break;

          case ST_LIN:
            visItVFV->AddLine(i, shapeIds[0], shapeIds[1]);
            break;

          case ST_VTX:
            visItVFV->AddVertex(i, shapeIds[0]);
            break;

          case ST_PNT:
            intrpIds[intrpIdx] = visItVFV->AddCentroidPoint(nCellPts, shapeIds);
            break;
        }
      }

      edgeVtxs = nullptr;
      thisCase = nullptr;
    }
    else if (cellType == SVTK_POLYHEDRON)
    {
      if (numCants == 0)
      {
        specials->GetCellData()->CopyAllocate(unstruct->GetCellData(), numCells);
      }
      svtkIdType nfaces;
      const svtkIdType* facePtIds;
      unstruct->GetFaceStream(i, nfaces, facePtIds);
      specials->InsertNextCell(cellType, nfaces, facePtIds);
      specials->GetCellData()->CopyData(unstruct->GetCellData(), i, numCants);
      numCants++;
    }
    else
    {
      if (numCants == 0)
      {
        specials->GetCellData()->CopyAllocate(unstruct->GetCellData(), numCells);
      }
      specials->InsertNextCell(cellType, numbPnts, pntIndxs);
      specials->GetCellData()->CopyData(unstruct->GetCellData(), i, numCants);
      numCants++;
    }

    pntIndxs = nullptr;
  }

  int toDelete = 0;
  double* theCords = nullptr;
  svtkPoints* inputPts = unstruct->GetPoints();
  if (inputPts->GetDataType() == SVTK_DOUBLE)
  {
    theCords = static_cast<double*>(inputPts->GetVoidPointer(0));
  }
  else
  {
    toDelete = 1;
    numbPnts = inputPts->GetNumberOfPoints();
    theCords = new double[numbPnts * 3];
    for (i = 0; i < numbPnts; i++)
    {
      inputPts->GetPoint(i, theCords + (i << 1) + i);
    }
  }
  inputPts = nullptr;

  // the stuff that can not be clipped
  if (numCants > 0)
  {
    svtkUnstructuredGrid* svtkUGrid = svtkUnstructuredGrid::New();
    this->ClipDataSet(specials, clipAray, svtkUGrid);

    svtkUnstructuredGrid* visItGrd = svtkUnstructuredGrid::New();
    visItVFV->ConstructDataSet(unstruct, visItGrd, theCords);

    svtkAppendFilter* appender = svtkAppendFilter::New();
    appender->AddInputData(svtkUGrid);
    appender->AddInputData(visItGrd);
    appender->Update();

    outputUG->ShallowCopy(appender->GetOutput());

    appender->Delete();
    visItGrd->Delete();
    svtkUGrid->Delete();
    appender = nullptr;
    svtkUGrid = nullptr;
    visItGrd = nullptr;
  }
  else
  {
    visItVFV->ConstructDataSet(unstruct, outputUG, theCords);
  }

  specials->Delete();
  delete visItVFV;
  if (toDelete)
  {
    delete[] theCords;
  }
  specials = nullptr;
  visItVFV = nullptr;
  theCords = nullptr;
  unstruct = nullptr;
}

//-----------------------------------------------------------------------------
void svtkTableBasedClipDataSet::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
  if (this->ClipFunction)
  {
    os << indent << "Clip Function: " << this->ClipFunction << "\n";
  }
  else
  {
    os << indent << "Clip Function: (none)\n";
  }
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Generate Clip Scalars: " << (this->GenerateClipScalars ? "On\n" : "Off\n");

  os << indent << "Generate Clipped Output: " << (this->GenerateClippedOutput ? "On\n" : "Off\n");

  os << indent << "UseValueAsOffset: " << (this->UseValueAsOffset ? "On\n" : "Off\n");

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";
}
