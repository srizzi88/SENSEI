/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLMappedUnstructuredGridIO

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
  This test was written by Menno Deij - van Rijswijk (MARIN).
----------------------------------------------------------------------------*/

#include "svtkCell.h" // for cell types
#include "svtkCellIterator.h"
#include "svtkDataArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMappedUnstructuredGrid.h"
#include "svtkNew.h"
#include "svtkPoints.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLUnstructuredGridReader.h"
#include "svtkXMLUnstructuredGridWriter.h"
#include "svtksys/FStream.hxx"

#include <algorithm>
#include <fstream>
#include <string>

namespace
{ // this namespace contains the supporting mapped grid definition used in the test

template <class I>
class MappedCellIterator : public svtkCellIterator
{
public:
  svtkTemplateTypeMacro(MappedCellIterator<I>, svtkCellIterator);
  typedef MappedCellIterator<I> ThisType;

  static MappedCellIterator<I>* New();

  void SetMappedUnstructuredGrid(svtkMappedUnstructuredGrid<I, ThisType>* grid);

  void PrintSelf(std::ostream& os, svtkIndent id) override;

  bool IsDoneWithTraversal() override;
  svtkIdType GetCellId() override;

protected:
  MappedCellIterator();
  ~MappedCellIterator() override;
  void ResetToFirstCell() override { this->CellId = 0; }
  void IncrementToNextCell() override { this->CellId++; }
  void FetchCellType() override;
  void FetchPointIds() override;
  void FetchPoints() override;
  void FetchFaces() override;

private:
  MappedCellIterator(const MappedCellIterator&) = delete;
  void operator=(const MappedCellIterator&) = delete;

  svtkIdType CellId;
  svtkIdType NumberOfCells;
  svtkSmartPointer<I> Impl;
  svtkSmartPointer<svtkPoints> GridPoints;
};

template <class I>
MappedCellIterator<I>* MappedCellIterator<I>::New()
{
  SVTK_STANDARD_NEW_BODY(ThisType);
}

template <class I>
MappedCellIterator<I>::MappedCellIterator()
  : CellId(0)
  , NumberOfCells(0)
  , Impl(nullptr)
  , GridPoints(nullptr)
{
}

template <class I>
void MappedCellIterator<I>::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "Mapped Internal Block" << endl;
}

template <class I>
MappedCellIterator<I>::~MappedCellIterator() = default;

template <class I>
void MappedCellIterator<I>::SetMappedUnstructuredGrid(svtkMappedUnstructuredGrid<I, ThisType>* grid)
{
  this->Impl = grid->GetImplementation();
  this->CellId = 0;
  this->GridPoints = grid->GetPoints();
  this->NumberOfCells = grid->GetNumberOfCells();
}

template <class I>
bool MappedCellIterator<I>::IsDoneWithTraversal()
{
  if (!this->Impl)
    return true;
  return CellId >= this->NumberOfCells;
}

template <class I>
svtkIdType MappedCellIterator<I>::GetCellId()
{
  return this->CellId;
}

template <class I>
void MappedCellIterator<I>::FetchCellType()
{
  this->CellType = Impl->GetCellType(this->CellId);
}

template <class I>
void MappedCellIterator<I>::FetchPointIds()
{
  this->Impl->GetCellPoints(this->CellId, this->PointIds);
}

template <class I>
void MappedCellIterator<I>::FetchPoints()
{
  this->GridPoints->GetPoints(this->GetPointIds(), this->Points);
}

template <class I>
void MappedCellIterator<I>::FetchFaces()
{
  this->Impl->GetFaceStream(this->CellId, this->Faces);
}

class MappedGrid;
class MappedGridImpl : public svtkObject
{
public:
  static MappedGridImpl* New();

  void Initialize(svtkUnstructuredGrid* ug)
  {
    ug->Register(this);
    _grid = ug;
  }

  void PrintSelf(std::ostream& os, svtkIndent id) override;

  // API for svtkMappedUnstructuredGrid implementation
  virtual int GetCellType(svtkIdType cellId);
  virtual void GetCellPoints(svtkIdType cellId, svtkIdList* ptIds);
  virtual void GetFaceStream(svtkIdType cellId, svtkIdList* ptIds);
  virtual void GetPointCells(svtkIdType ptId, svtkIdList* cellIds);
  virtual int GetMaxCellSize();
  virtual void GetIdsOfCellsOfType(int type, svtkIdTypeArray* array);
  virtual int IsHomogeneous();

  // This container is read only -- these methods do nothing but print a warning.
  void Allocate(svtkIdType numCells, int extSize = 1000);
  svtkIdType InsertNextCell(int type, svtkIdList* ptIds);
  svtkIdType InsertNextCell(int type, svtkIdType npts, const svtkIdType ptIds[])
    SVTK_SIZEHINT(ptIds, npts);
  svtkIdType InsertNextCell(int type, svtkIdType npts, const svtkIdType ptIds[], svtkIdType nfaces,
    const svtkIdType faces[]) SVTK_SIZEHINT(ptIds, npts) SVTK_SIZEHINT(faces, nfaces);
  void ReplaceCell(svtkIdType cellId, int npts, const svtkIdType pts[]) SVTK_SIZEHINT(pts, npts);

  svtkIdType GetNumberOfCells();
  void SetOwner(MappedGrid* owner) { this->Owner = owner; }

  svtkPoints* GetPoints() { return _grid->GetPoints(); }

protected:
  MappedGridImpl() = default;
  ~MappedGridImpl() override { _grid->UnRegister(this); }

private:
  svtkUnstructuredGrid* _grid;
  MappedGrid* Owner;
};

svtkStandardNewMacro(MappedGridImpl);

void MappedGridImpl::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "Mapped Grid Implementation" << endl;
}

int MappedGridImpl::GetCellType(svtkIdType cellId)
{
  return _grid->GetCellType(cellId);
}

int MappedGridImpl::GetMaxCellSize()
{
  return _grid->GetMaxCellSize();
}

void MappedGridImpl::GetCellPoints(svtkIdType cellId, svtkIdList* ptIds)
{
  _grid->GetCellPoints(cellId, ptIds);
}

void MappedGridImpl::GetFaceStream(svtkIdType cellId, svtkIdList* ptIds)
{
  _grid->GetFaceStream(cellId, ptIds);
}

void MappedGridImpl::GetPointCells(svtkIdType ptId, svtkIdList* cellIds)
{
  _grid->GetPointCells(ptId, cellIds);
}

int MappedGridImpl::IsHomogeneous()
{
  return _grid->IsHomogeneous();
}

svtkIdType MappedGridImpl::GetNumberOfCells()
{
  return _grid->GetNumberOfCells();
}

void MappedGridImpl::GetIdsOfCellsOfType(int type, svtkIdTypeArray* array)
{
  _grid->GetIdsOfCellsOfType(type, array);
}

void MappedGridImpl::Allocate(svtkIdType svtkNotUsed(numCells), int svtkNotUsed(extSize))
{
  svtkWarningMacro(<< "Read only block\n");
}

svtkIdType MappedGridImpl::InsertNextCell(int svtkNotUsed(type), svtkIdList* svtkNotUsed(ptIds))
{
  svtkWarningMacro(<< "Read only block\n");
  return -1;
}

svtkIdType MappedGridImpl::InsertNextCell(
  int svtkNotUsed(type), svtkIdType svtkNotUsed(npts), const svtkIdType svtkNotUsed(ptIds)[])
{
  svtkWarningMacro(<< "Read only block\n");
  return -1;
}

svtkIdType MappedGridImpl::InsertNextCell(int svtkNotUsed(type), svtkIdType svtkNotUsed(npts),
  const svtkIdType svtkNotUsed(ptIds)[], svtkIdType svtkNotUsed(nfaces),
  const svtkIdType svtkNotUsed(faces)[])
{
  svtkWarningMacro(<< "Read only block\n");
  return -1;
}

void MappedGridImpl::ReplaceCell(
  svtkIdType svtkNotUsed(cellId), int svtkNotUsed(npts), const svtkIdType svtkNotUsed(pts)[])
{
  svtkWarningMacro(<< "Read only block\n");
}

class MappedGrid
  : public svtkMappedUnstructuredGrid<MappedGridImpl, MappedCellIterator<MappedGridImpl> >
{
public:
  typedef svtkMappedUnstructuredGrid<MappedGridImpl, MappedCellIterator<MappedGridImpl> > _myBase;

  int GetDataObjectType() override { return SVTK_UNSTRUCTURED_GRID_BASE; }

  static MappedGrid* New();

  svtkPoints* GetPoints() override { return this->GetImplementation()->GetPoints(); }

  svtkIdType GetNumberOfPoints() override
  {
    return this->GetImplementation()->GetPoints()->GetNumberOfPoints();
  }

protected:
  MappedGrid()
  {
    MappedGridImpl* ig = MappedGridImpl::New();
    ig->SetOwner(this);
    this->SetImplementation(ig);
    ig->Delete();
  }
  ~MappedGrid() override = default;

private:
  MappedGrid(const MappedGrid&) = delete;
  void operator=(const MappedGrid&) = delete;
};

svtkStandardNewMacro(MappedGrid);

} // end anonymous namespace

using namespace std;

bool compareFiles(const string& p1, const string& p2)
{
  svtksys::ifstream f1(p1.c_str(), ifstream::binary | ifstream::ate);
  svtksys::ifstream f2(p2.c_str(), ifstream::binary | ifstream::ate);

  if (f1.fail() || f2.fail())
  {
    return false; // file problem
  }

  if (f1.tellg() != f2.tellg())
  {
    return false; // size mismatch
  }

  // seek back to beginning and use equal to compare contents
  f1.seekg(0, svtksys::ifstream::beg);
  f2.seekg(0, svtksys::ifstream::beg);
  return equal(istreambuf_iterator<char>(f1.rdbuf()), istreambuf_iterator<char>(),
    istreambuf_iterator<char>(f2.rdbuf()));
}

int TestXMLMappedUnstructuredGridIO(int argc, char* argv[])
{
  svtkNew<svtkPoints> points;

  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 1, 0);

  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(1, 0, 1);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(0, 1, 1);

  points->InsertNextPoint(.5, .5, 2);
  points->InsertNextPoint(.5, .5, -1);

  svtkNew<svtkUnstructuredGrid> ug;
  ug->SetPoints(points);

  ug->Allocate(3); // allocate for 3 cells

  svtkNew<svtkIdList> ids;

  // add a hexahedron of the first 8 points (i.e. a cube)
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(2);
  ids->InsertNextId(3);
  ids->InsertNextId(4);
  ids->InsertNextId(5);
  ids->InsertNextId(6);
  ids->InsertNextId(7);
  ug->InsertNextCell(SVTK_HEXAHEDRON, ids.GetPointer());
  ids->Reset();

  // add a polyhedron comprise of the top hexahedron face
  // and four triangles to the 9th point
  ids->InsertNextId(4);
  ids->InsertNextId(5);
  ids->InsertNextId(6);
  ids->InsertNextId(7);
  ids->InsertNextId(8);

  svtkNew<svtkIdList> faces;
  // top face of four points
  faces->InsertNextId(4);

  faces->InsertNextId(4);
  faces->InsertNextId(5);
  faces->InsertNextId(6);
  faces->InsertNextId(7);

  // four triangle side faces, each of three points
  faces->InsertNextId(3);
  faces->InsertNextId(4);
  faces->InsertNextId(5);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(5);
  faces->InsertNextId(6);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(6);
  faces->InsertNextId(7);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(7);
  faces->InsertNextId(4);
  faces->InsertNextId(8);

  // insert the polyhedron cell
  ug->InsertNextCell(
    SVTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), 5, faces.GetPointer()->GetPointer(0));

  // put another pyramid on the bottom towards the 10th point
  faces->Reset();
  ids->Reset();

  // the list of points that the pyramid references
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(2);
  ids->InsertNextId(3);
  ids->InsertNextId(9);

  // bottom face of four points
  faces->InsertNextId(4);

  faces->InsertNextId(0);
  faces->InsertNextId(1);
  faces->InsertNextId(2);
  faces->InsertNextId(3);

  // four side faces, each of three points
  faces->InsertNextId(3);
  faces->InsertNextId(0);
  faces->InsertNextId(1);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(1);
  faces->InsertNextId(2);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(2);
  faces->InsertNextId(3);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(3);
  faces->InsertNextId(0);
  faces->InsertNextId(9);

  // insert the cell. We now have two pyramids with a cube in between
  ug->InsertNextCell(
    SVTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), 5, faces.GetPointer()->GetPointer(0));

  // for testing, we write in appended, ascii and binary mode and request that
  // the files are ** binary ** equal.
  //
  // first, find a file we can write to

  char* tempDir =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  string dir(tempDir);
  if (dir.empty())
  {
    cerr << "Could not determine temporary directory." << endl;
    return EXIT_FAILURE;
  }

  string f1 = dir + "/test_ug_input.vtu";
  string f2 = dir + "/test_mapped_input.vtu";

  svtkNew<svtkXMLUnstructuredGridWriter> w;
  w->SetInputData(ug);
  w->SetFileName(f1.c_str());

  w->Update();
  if (points->GetData()->GetInformation()->Has(svtkDataArray::L2_NORM_RANGE()))
  {
    // for the normal unstructured grid the L2_NORM_RANGE is added. This
    // makes file comparison impossible. therefore, after the first Update()
    // remove the L2_NORM_RANGE information key and write the file again.
    points->GetData()->GetInformation()->Remove(svtkDataArray::L2_NORM_RANGE());
  }
  w->Update();

  // create a mapped grid which basically takes the original grid
  // and uses it to map to.
  svtkNew<MappedGrid> mg;
  mg->GetImplementation()->Initialize(ug);

  svtkNew<svtkXMLUnstructuredGridWriter> w2;
  w2->SetInputData(mg);
  w2->SetFileName(f2.c_str());
  w2->Update();

  // compare the files in appended, then ascii, then binary mode.
  bool same = compareFiles(f1, f2);
  if (!same)
  {
    std::cerr << "Error comparing files in appended mode.\n";
    return EXIT_FAILURE;
  }
  w->SetDataModeToAscii();
  w2->SetDataModeToAscii();
  w->Update();
  w2->Update();

  same = compareFiles(f1, f2);
  if (!same)
  {
    std::cerr << "Error comparing files in ascii mode.\n";
    return EXIT_FAILURE;
  }
  w->SetDataModeToBinary();
  w2->SetDataModeToBinary();
  w->Update();
  w2->Update();

  same = compareFiles(f1, f2);
  if (!same)
  {
    std::cerr << "Error comparing files in binary mode.\n";
    return EXIT_FAILURE;
  }

  // clean up after ourselves: remove written files and free temp dir name
  remove(f1.c_str());
  remove(f2.c_str());

  delete[] tempDir;

  return 0;
}
