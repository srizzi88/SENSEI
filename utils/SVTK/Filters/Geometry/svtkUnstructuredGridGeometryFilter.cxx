/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkUnstructuredGridGeometryFilter.h"

#include "svtkBezierHexahedron.h"
#include "svtkBezierQuadrilateral.h"
#include "svtkBezierTetra.h"
#include "svtkBezierWedge.h"
#include "svtkBiQuadraticQuadraticHexahedron.h"
#include "svtkBiQuadraticQuadraticWedge.h"
#include "svtkBiQuadraticTriangle.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkCellTypes.h"
#include "svtkGenericCell.h"
#include "svtkHexagonalPrism.h"
#include "svtkHexahedron.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLagrangeCurve.h"
#include "svtkLagrangeHexahedron.h"
#include "svtkLagrangeQuadrilateral.h"
#include "svtkLagrangeTetra.h"
#include "svtkLagrangeTriangle.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPentagonalPrism.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyhedron.h"
#include "svtkPyramid.h"
#include "svtkQuadraticHexahedron.h"
#include "svtkQuadraticLinearWedge.h"
#include "svtkQuadraticPyramid.h"
#include "svtkQuadraticTetra.h"
#include "svtkQuadraticWedge.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkTetra.h"
#include "svtkTriQuadraticHexahedron.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVoxel.h"
#include "svtkWedge.h"

#include <cassert>
#include <vector>

svtkStandardNewMacro(svtkUnstructuredGridGeometryFilter);

#if 0
//-----------------------------------------------------------------------------
// Pool allocator: interface is defined in section 20.1.5
// "Allocator Requirement" in the C++ norm.
template <class G> class svtkPoolAllocator
{
public:
  // It is ugly but it is the norm...
  typedef  G *pointer;
  typedef  const G * const_pointer;
  typedef  G&        reference;
  typedef  G const & const_reference;
  typedef  G         value_type;
  typedef  size_t    size_type;
  typedef  ptrdiff_t difference_type;
  template<H> struct rebind
  {
    typedef svtkPoolAllocator<H> other;
  };

  pointer address(reference r) const
  {
      return &r;
  }
  const_pointer address(const_reference r) const
  {
      return &r;
  }

  // Space for n Gs.
  // Comment from the norm:
  // Memory is allocated for `n' objects of type G but objects are not
  // constructed. allocate may raise an exception. The result is a random
  // access iterator.
  pointer allocate(size_type n,
                   svtkPoolAllocator<void>::const_pointer SVTK_NOT_USED(hint)=0)
  {

  }

  // Deallocate n Gs, don't destroy.
  // Comment from the norm:
  // All `n' G objects in the area pointed by `p' must be destroyed prior to
  // this call. `n'  must match the value passed to allocate to obtain this
  // memory.
  // \pre p_exists: p!=0
  void deallocate(pointer p,
                  size_type n)
  {
      // Pre-conditions
      assert("p_exists" && p!=0);


  }

  // Comment from the norm:
  // The largest value that can meaningfully be passed to allocate().
  size_type max_size() const throw()
  {
      // Implementation is the same than in gcc:
      return size_t(-1)/sizeof(G);
  }

  // Default constructor.
  svtkPoolAllocator() throw()
  {
  }

  // Copy constructor.
  template<H> svtkPoolAllocator(const svtkPoolAllocator<H> &other) throw()
  {
      assert("check: NOT USED" & 0);
  }

  // Destructor.
  ~svtkPoolAllocator() throw()
  {
  }

  // Return the size of the chunks.
  // \post positive_result: result>0
  static int GetChunkSize()
  {
      return this.Allocator.GetChunkSize();
  }

  // Set the chunk size.
  // \pre positive_size: size>0.
  // \post is_set: value==GetChunkSize()
  static void SetChunkSize(int size)
  {
      // Pre-conditions.
      assert("pre: positive_size" && size>0);

      this.Allocator.SeChunkSize(size);

      // Post-conditions.
      assert("post: is_set" && value==this.GetChunkSize());
  }

  // Initialize *p by val.
  // Comment from the norm:
  // Effect: new((void *)p) G(val)
  void construct(pointer p,
                 const G &val);

  // Destroy *p but don't deallocate.
  // Comment from the norm:
  // Effect: ((G*)p)->~G()
  void destroy(pointer p);

protected:
  static svtkPoolManager<G> Allocator;
};

// Initialization of the static variable.
template <class G> svtkPoolManager<G> svtkPoolAllocator<G>::Allocator();

// Comment from the norm:
// Return true iff storage allocated from each can be deallocated via the
// other.
template <class G> bool operator==(const allocator<G>&SVTK_NOT_USED(a1),
                                   const allocator<G>&SVTK_NOT_USED(a2)) throw()
{
  return true;
}

// Comment from the norm:
// Same as !(a1==a2).
template <class G> bool operator!=(const allocator<G>&a1,
                                   const allocator<G>&a2) throw()
{
  return !(a1==a2);
}
#endif

const unsigned int SVTK_DEFAULT_CHUNK_SIZE = 50;
const int SVTK_DEFAULT_NUMBER_OF_CHUNKS = 100;

//-----------------------------------------------------------------------------
// Memory management with a pool of objects to make allocation of chunks of
// objects instead of slow per-object allocation.
// Assumption about class G: has a public default constructor.
template <class G>
class svtkPoolManager
{
public:
  // Default constructor.
  svtkPoolManager()
  {
    this->Chunks = nullptr;
    this->ChunkSize = SVTK_DEFAULT_CHUNK_SIZE;
  }

  // Initialize the pool with a set of empty chunks.
  void Init()
  {
    if (this->Chunks == nullptr)
    {
      this->Chunks = new std::vector<std::vector<G>*>();
      this->Chunks->reserve(SVTK_DEFAULT_NUMBER_OF_CHUNKS);
    }
  }

  // Is the pool initialized?
  int IsInitialized() { return this->Chunks != nullptr; }

  // Return a new `G' object.
  // \pre is_initialized: IsInitialized()
  G* Allocate()
  {
    assert("pre: is_initialized" && this->IsInitialized());
    G* result = nullptr;
    size_t c = this->Chunks->size();
    if (c == 0) // first Allocate()
    {
      this->Chunks->resize(1);
      (*this->Chunks)[0] = new std::vector<G>();
      // Allocate the first chunk
      (*this->Chunks)[0]->reserve(this->ChunkSize);
      (*this->Chunks)[0]->resize(1);
      result = &((*((*this->Chunks)[0]))[0]);
    }
    else
    {
      // At the end of the current chunk?
      if ((*this->Chunks)[c - 1]->size() == this->ChunkSize)
      {
        // No more chunk?
        if (this->Chunks->size() == this->Chunks->capacity())
        {
          // double the capacity.
          this->Chunks->reserve(this->Chunks->capacity() * 2);
        }
        // Allocate the next chunk.
        size_t chunkIdx = this->Chunks->size();
        this->Chunks->resize(chunkIdx + 1);
        (*this->Chunks)[chunkIdx] = new std::vector<G>();
        (*this->Chunks)[chunkIdx]->reserve(this->ChunkSize);
        // Return the first element of this new chunk.
        (*this->Chunks)[chunkIdx]->resize(1);
        result = &((*((*this->Chunks)[chunkIdx]))[0]);
      }
      else
      {
        size_t c2 = (*this->Chunks)[c - 1]->size();
        (*this->Chunks)[c - 1]->resize(c2 + 1);
        result = &((*((*this->Chunks)[c - 1]))[c2]);
      }
    }
    return result;
  }

  // Destructor.
  ~svtkPoolManager()
  {
    if (this->Chunks != nullptr)
    {
      size_t c = this->Chunks->size();
      size_t i = 0;
      while (i < c)
      {
        delete (*this->Chunks)[i];
        ++i;
      }
      delete Chunks;
    }
  }

  // Return the size of the chunks.
  // \post positive_result: result>0
  unsigned int GetChunkSize() { return this->ChunkSize; }

  // Set the chunk size.
  // \pre not_yet_initialized: !IsInitialized()
  // \pre positive_size: size>0.
  // \post is_set: size==GetChunkSize()
  void SetChunkSize(unsigned int size)
  {
    // Pre-conditions.
    assert("pre: not_yet_initialized" && !this->IsInitialized());
    assert("pre: positive_size" && size > 0);

    this->ChunkSize = size;

    // Post-conditions.
    assert("post: is_set" && size == this->GetChunkSize());
  }

protected:
  std::vector<std::vector<G>*>* Chunks;
  unsigned int ChunkSize;
};

//-----------------------------------------------------------------------------
// Surface element: face of a 3D cell.
//  As this is internal use only, we put variables as public.
class svtkSurfel
{
public:
  ~svtkSurfel()
  {
    delete[] Points;
    Points = nullptr;
  }

  // 2D cell type:
  // SVTK_TRIANGLE,
  // SVTK_POLYGON,
  // SVTK_PIXEL,
  // SVTK_QUAD,
  // SVTK_QUADRATIC_TRIANGLE,
  // SVTK_QUADRATIC_QUAD,
  // SVTK_BIQUADRATIC_QUAD,
  // SVTK_BIQUADRATIC_TRIANGLE
  // SVTK_QUADRATIC_LINEAR_QUAD
  // SVTK_LAGRANGE_TRIANGLE
  // SVTK_LAGRANGE_QUADRILATERAL
  // SVTK_BEZIER_TRIANGLE
  // SVTK_BEZIER_QUADRILATERAL
  svtkIdType Type;

  // Dataset point Ids that form the surfel.
  svtkIdType* Points;

  // Number of points defining the cell.
  // For cells with a fixed number of points like triangle, it looks redundant.
  // However, it is useful for polygon (pentagonal or hexagonal face).
  svtkIdType NumberOfPoints;

  // Index of the point with the smallest dataset point Id.
  // SmallestIdx>=0 && SmallestIdx<NumberOfPoints.
  // Its dataset point Id is given by Points[SmallestIdx]
  svtkIdType SmallestIdx;

  // Id of the 3D cell this surfel belongs to,
  // -1 if it belongs to more than one (it means the surfel is not on the
  // boundary of the dataset, so it will be not visible.
  svtkIdType Cell3DId;

  // A 2d double, containing the degrees
  // This is used for Bezier quad, to know which degree is involved.
  int Degrees[2];

  // A surfel is also an element of a one-way linked list: in the hashtable,
  // each key entry is a one-way linked list of Surfels.
  svtkSurfel* Next;

  svtkSurfel(const svtkSurfel&) = default;
  svtkSurfel() = default;
};

//-----------------------------------------------------------------------------
// Hashtable of surfels.
const int SVTK_HASH_PRIME = 31;
class svtkHashTableOfSurfels
{
public:
  // Constructor for the number of points in the dataset and an initialized
  // pool.
  // \pre positive_number: numberOfPoints>0
  // \pre pool_exists: pool!=0
  // \pre initialized_pool: pool->IsInitialized()
  svtkHashTableOfSurfels(int numberOfPoints, svtkPoolManager<svtkSurfel>* pool)
    : HashTable(numberOfPoints)
  {
    assert("pre: positive_number" && numberOfPoints > 0);
    assert("pre: pool_exists" && pool != nullptr);
    assert("pre: initialized_pool" && pool->IsInitialized());

    this->Pool = pool;
    int i = 0;
    int c = numberOfPoints;
    while (i < c)
    {
      this->HashTable[i] = nullptr;
      ++i;
    }
  }
  std::vector<svtkSurfel*> HashTable;

  // Add faces of cell type FaceType
  template <typename CellType, int FirstFace, int LastFace, int NumPoints, int FaceType>
  void InsertFaces(svtkIdType* pts, svtkIdType cellId)
  {
    svtkIdType points[NumPoints];
    for (int face = FirstFace; face < LastFace; ++face)
    {
      const svtkIdType* faceIndices = CellType::GetFaceArray(face);
      for (int pt = 0; pt < NumPoints; ++pt)
      {
        points[pt] = pts[faceIndices[pt]];
      }
      int degrees[2]{ 0, 0 };
      this->InsertFace(cellId, FaceType, NumPoints, points, degrees);
    }
  }

  // Add a face defined by its cell type 'faceType', its number of points,
  // its list of points and the cellId of the 3D cell it belongs to.
  // \pre positive number of points

  void InsertFace(
    svtkIdType cellId, svtkIdType faceType, int numberOfPoints, svtkIdType* points, int degrees[2])
  {
    assert("pre: positive number of points" && numberOfPoints >= 0);

    int numberOfCornerPoints;

    switch (faceType)
    {
      case SVTK_QUADRATIC_TRIANGLE:
      case SVTK_BIQUADRATIC_TRIANGLE:
      case SVTK_LAGRANGE_TRIANGLE:
      case SVTK_BEZIER_TRIANGLE:
        numberOfCornerPoints = 3;
        break;
      case SVTK_QUADRATIC_QUAD:
      case SVTK_QUADRATIC_LINEAR_QUAD:
      case SVTK_BIQUADRATIC_QUAD:
      case SVTK_LAGRANGE_QUADRILATERAL:
      case SVTK_BEZIER_QUADRILATERAL:
        numberOfCornerPoints = 4;
        break;
      default:
        numberOfCornerPoints = numberOfPoints;
        break;
    }

    // Compute the smallest id among the corner points.
    int smallestIdx = 0;
    svtkIdType smallestId = points[smallestIdx];
    for (int i = 1; i < numberOfCornerPoints; ++i)
    {
      if (points[i] < smallestId)
      {
        smallestIdx = i;
        smallestId = points[i];
      }
    }

    // Compute the hashkey/code
    size_t key = (faceType * SVTK_HASH_PRIME + smallestId) % (this->HashTable.size());

    // Get the list at this key (several not equal faces can share the
    // same hashcode). This is the first element in the list.
    svtkSurfel* first = this->HashTable[key];
    svtkSurfel* surfel;
    if (first == nullptr)
    {
      // empty list.
      surfel = this->Pool->Allocate();

      // Just add this new face.
      this->HashTable[key] = surfel;
    }
    else
    {
      int found = 0;
      svtkSurfel* current = first;
      svtkSurfel* previous = current;
      while (!found && current != nullptr)
      {
        found = current->Type == faceType;
        if (found)
        {
          if (faceType == SVTK_QUADRATIC_LINEAR_QUAD)
          {
            // weird case
            // the following four combinations are equivalent
            // 01 23, 45, smallestIdx=0, go->
            // 10 32, 45, smallestIdx=1, go<-
            // 23 01, 54, smallestIdx=2, go->
            // 32 10, 54, smallestIdx=3, go<-

            // if current=0 or 2, other face has to be 1 or 3
            // if current=1 or 3, other face has to be 0 or 2

            if (points[0] == current->Points[1])
            {
              found = (points[1] == current->Points[0] && points[2] == current->Points[3] &&
                points[3] == current->Points[2] && points[4] == current->Points[4] &&
                points[5] == current->Points[5]);
            }
            else
            {
              if (points[0] == current->Points[3])
              {
                found = (points[1] == current->Points[2] && points[2] == current->Points[1] &&
                  points[3] == current->Points[0] && points[4] == current->Points[5] &&
                  points[5] == current->Points[4]);
              }
              else
              {
                found = 0;
              }
            }
          }
          else
          {
            // If the face is already from another cell. The first
            // corner point with smallest id will match.

            // The other corner points
            // will be given in reverse order (opposite orientation)
            int i = 0;
            while (found && i < numberOfCornerPoints)
            {
              // we add numberOfPoints before modulo. Modulo does not work
              // with negative values.
              found = current->Points[(current->SmallestIdx - i + numberOfCornerPoints) %
                        numberOfCornerPoints] == points[(smallestIdx + i) % numberOfCornerPoints];
              ++i;
            }

            // Check for other kind of points for nonlinear faces.
            switch (faceType)
            {
              case SVTK_QUADRATIC_TRIANGLE:
                // the mid-edge points
                i = 0;
                while (found && i < 3)
                {
                  // we add numberOfPoints before modulo. Modulo does not work
                  // with negative values.
                  // -1: start at the end in reverse order.
                  found =
                    current
                      ->Points[numberOfCornerPoints + ((current->SmallestIdx - i + 3 - 1) % 3)] ==
                    points[numberOfCornerPoints + ((smallestIdx + i) % 3)];
                  ++i;
                }
                break;
              case SVTK_BIQUADRATIC_TRIANGLE:
                // the center point
                found = current->Points[6] == points[6];

                // the mid-edge points
                i = 0;
                while (found && i < 3)
                {
                  // we add numberOfPoints before modulo. Modulo does not work
                  // with negative values.
                  // -1: start at the end in reverse order.
                  found =
                    current
                      ->Points[numberOfCornerPoints + ((current->SmallestIdx - i + 3 - 1) % 3)] ==
                    points[numberOfCornerPoints + ((smallestIdx + i) % 3)];
                  ++i;
                }
                break;
              case SVTK_LAGRANGE_TRIANGLE:
              case SVTK_BEZIER_TRIANGLE:
                found &= (current->NumberOfPoints == numberOfPoints);
                // TODO: Compare all higher order points.
                break;
              case SVTK_QUADRATIC_QUAD:
                // the mid-edge points
                i = 0;
                while (found && i < 4)
                {
                  // we add numberOfPoints before modulo. Modulo does not work
                  // with negative values.
                  found =
                    current
                      ->Points[numberOfCornerPoints + ((current->SmallestIdx - i + 4 - 1) % 4)] ==
                    points[numberOfCornerPoints + ((smallestIdx + i) % 4)];
                  ++i;
                }
                break;
              case SVTK_BIQUADRATIC_QUAD:
                // the center point
                found = current->Points[8] == points[8];

                // the mid-edge points
                i = 0;
                while (found && i < 4)
                {
                  // we add numberOfPoints before modulo. Modulo does not work
                  // with negative values.
                  found =
                    current
                      ->Points[numberOfCornerPoints + ((current->SmallestIdx - i + 4 - 1) % 4)] ==
                    points[numberOfCornerPoints + ((smallestIdx + i) % 4)];
                  ++i;
                }
                break;
              case SVTK_LAGRANGE_QUADRILATERAL:
              case SVTK_BEZIER_QUADRILATERAL:
                found &= (current->NumberOfPoints == numberOfPoints);
                // TODO: Compare all higher order points.
                break;
              default: // other faces are linear: we are done.
                break;
            }
          }
        }
        previous = current;
        current = current->Next;
      }
      if (found)
      {
        previous->Cell3DId = -1;
        surfel = nullptr;
      }
      else
      {
        surfel = this->Pool->Allocate();
        previous->Next = surfel;
      }
    }
    if (surfel != nullptr)
    {
      surfel->Degrees[0] = degrees[0];
      surfel->Degrees[1] = degrees[1];
      surfel->Next = nullptr;
      surfel->Type = faceType;
      surfel->NumberOfPoints = numberOfPoints;
      surfel->Points = new svtkIdType[numberOfPoints];
      surfel->SmallestIdx = smallestIdx;
      surfel->Cell3DId = cellId;
      for (int i = 0; i < numberOfPoints; ++i)
      {
        surfel->Points[i] = points[i];
      }
    }
  }

protected:
  svtkPoolManager<svtkSurfel>* Pool;
};

//-----------------------------------------------------------------------------
// Object used to traverse an hashtable of surfels.
class svtkHashTableOfSurfelsCursor
{
public:
  // Initialize the cursor with the table to traverse.
  // \pre table_exists: table!=0
  void Init(svtkHashTableOfSurfels* table)
  {
    assert("pre: table_exists" && table != nullptr);
    this->Table = table;
    this->AtEnd = 1;
  }

  // Move the cursor to the first surfel.
  // If the table is empty, the cursor is at the end of the table.
  void Start()
  {
    this->CurrentKey = 0;
    this->CurrentSurfel = nullptr;

    size_t c = Table->HashTable.size();
    int done = this->CurrentKey >= c;
    if (!done)
    {
      this->CurrentSurfel = this->Table->HashTable[this->CurrentKey];
      done = this->CurrentSurfel != nullptr;
    }
    while (!done)
    {
      ++this->CurrentKey;
      done = this->CurrentKey >= c;
      if (!done)
      {
        this->CurrentSurfel = this->Table->HashTable[this->CurrentKey];
        done = this->CurrentSurfel != nullptr;
      }
    }
    this->AtEnd = this->CurrentSurfel == nullptr;
  }

  // Is the cursor at the end of the table? (ie. no more surfel?)
  svtkTypeBool IsAtEnd() { return this->AtEnd; }

  // Return the surfel the cursor is pointing to.
  svtkSurfel* GetCurrentSurfel()
  {
    assert("pre: not_at_end" && !IsAtEnd());
    return this->CurrentSurfel;
  }

  // Move the cursor to the next available surfel.
  // If there is no more surfel, the cursor is at the end of the table.
  void Next()
  {
    assert("pre: not_at_end" && !IsAtEnd());
    CurrentSurfel = CurrentSurfel->Next;
    size_t c = Table->HashTable.size();
    if (this->CurrentSurfel == nullptr)
    {
      ++this->CurrentKey;
      int done = this->CurrentKey >= c;
      if (!done)
      {
        this->CurrentSurfel = this->Table->HashTable[this->CurrentKey];
        done = this->CurrentSurfel != nullptr;
      }
      while (!done)
      {
        ++this->CurrentKey;
        done = this->CurrentKey >= c;
        if (!done)
        {
          this->CurrentSurfel = this->Table->HashTable[this->CurrentKey];
          done = this->CurrentSurfel != nullptr;
        }
      }
      this->AtEnd = this->CurrentSurfel == nullptr;
    }
  }

protected:
  svtkHashTableOfSurfels* Table;
  size_t CurrentKey;
  svtkSurfel* CurrentSurfel;
  int AtEnd;
};

//-----------------------------------------------------------------------------
// Construct with all types of clipping turned off.
svtkUnstructuredGridGeometryFilter::svtkUnstructuredGridGeometryFilter()
{
  this->PointMinimum = 0;
  this->PointMaximum = SVTK_ID_MAX;

  this->CellMinimum = 0;
  this->CellMaximum = SVTK_ID_MAX;

  this->Extent[0] = -SVTK_DOUBLE_MAX;
  this->Extent[1] = SVTK_DOUBLE_MAX;
  this->Extent[2] = -SVTK_DOUBLE_MAX;
  this->Extent[3] = SVTK_DOUBLE_MAX;
  this->Extent[4] = -SVTK_DOUBLE_MAX;
  this->Extent[5] = SVTK_DOUBLE_MAX;

  this->PointClipping = 0;
  this->CellClipping = 0;
  this->ExtentClipping = 0;
  this->DuplicateGhostCellClipping = 1;

  this->PassThroughCellIds = 0;
  this->PassThroughPointIds = 0;
  this->OriginalCellIdsName = nullptr;
  this->OriginalPointIdsName = nullptr;

  this->Merging = 1;
  this->Locator = nullptr;

  this->HashTable = nullptr;
}

//-----------------------------------------------------------------------------
svtkUnstructuredGridGeometryFilter::~svtkUnstructuredGridGeometryFilter()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }

  this->SetOriginalCellIdsName(nullptr);
  this->SetOriginalPointIdsName(nullptr);
}

//-----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void svtkUnstructuredGridGeometryFilter::SetExtent(
  double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
  double extent[6];

  extent[0] = xMin;
  extent[1] = xMax;
  extent[2] = yMin;
  extent[3] = yMax;
  extent[4] = zMin;
  extent[5] = zMax;

  this->SetExtent(extent);
}

//-----------------------------------------------------------------------------
// Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
void svtkUnstructuredGridGeometryFilter::SetExtent(double extent[6])
{
  int i;

  if (extent[0] != this->Extent[0] || extent[1] != this->Extent[1] ||
    extent[2] != this->Extent[2] || extent[3] != this->Extent[3] || extent[4] != this->Extent[4] ||
    extent[5] != this->Extent[5])
  {
    this->Modified();
    for (i = 0; i < 3; i++)
    {
      if (extent[2 * i + 1] < extent[2 * i])
      {
        extent[2 * i + 1] = extent[2 * i];
      }
      this->Extent[2 * i] = extent[2 * i];
      this->Extent[2 * i + 1] = extent[2 * i + 1];
    }
  }
}

//-----------------------------------------------------------------------------
int svtkUnstructuredGridGeometryFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output. Input may just have the UnstructuredGridBase
  // interface, but output should be an unstructured grid.
  svtkUnstructuredGridBase* input =
    svtkUnstructuredGridBase::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  //  this->DebugOn();

  // Input
  svtkIdType numCells = input->GetNumberOfCells();
  if (numCells == 0)
  {
    svtkDebugMacro(<< "Nothing to extract");
    return 1;
  }
  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkPoints* inPts = input->GetPoints();
  svtkSmartPointer<svtkCellIterator> cellIter =
    svtkSmartPointer<svtkCellIterator>::Take(input->NewCellIterator());

  // Output
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  //  svtkUnsignedCharArray *types=svtkUnsignedCharArray::New();
  //  types->Allocate(numCells);
  //  svtkIdTypeArray *locs=svtkIdTypeArray::New();
  //  locs->Allocate(numCells);
  //  svtkCellArray *conn=svtkCellArray::New();
  //  conn->Allocate(numCells);

  unsigned char* cellGhostLevels = nullptr;
  svtkDataArray* temp = nullptr;
  if (cd != nullptr)
  {
    temp = cd->GetArray(svtkDataSetAttributes::GhostArrayName());
  }
  if (temp != nullptr && temp->GetDataType() == SVTK_UNSIGNED_CHAR &&
    temp->GetNumberOfComponents() == 1)
  {
    cellGhostLevels = static_cast<svtkUnsignedCharArray*>(temp)->GetPointer(0);
  }
  else
  {
    svtkDebugMacro("No appropriate ghost levels field available.");
  }

  // Visibility of cells.
  char* cellVis;
  int allVisible = (!this->CellClipping) && (!this->PointClipping) && (!this->ExtentClipping) &&
    (cellGhostLevels == nullptr);
  if (allVisible)
  {
    cellVis = nullptr;
  }
  else
  {
    cellVis = new char[numCells];
  }

  // Loop over the cells determining what's visible
  if (!allVisible)
  {
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      svtkIdType cellId = cellIter->GetCellId();
      svtkIdType npts = cellIter->GetNumberOfPoints();
      svtkIdType* pts = cellIter->GetPointIds()->GetPointer(0);
      if ((cellGhostLevels != nullptr &&
            (cellGhostLevels[cellId] & svtkDataSetAttributes::DUPLICATECELL) &&
            this->DuplicateGhostCellClipping) ||
        (this->CellClipping && (cellId < this->CellMinimum || cellId > this->CellMaximum)))
      {
        // the cell is a ghost cell or is clipped.
        cellVis[cellId] = 0;
      }
      else
      {
        double x[3];
        int i = 0;
        cellVis[cellId] = 1;
        while (i < npts && cellVis[cellId])
        {
          inPts->GetPoint(pts[i], x);
          cellVis[cellId] = !(
            (this->PointClipping && (pts[i] < this->PointMinimum || pts[i] > this->PointMaximum)) ||
            (this->ExtentClipping &&
              (x[0] < this->Extent[0] || x[0] > this->Extent[1] || x[1] < this->Extent[2] ||
                x[1] > this->Extent[3] || x[2] < this->Extent[4] || x[2] > this->Extent[5])));

          ++i;
        } // for each point
      }   // if point clipping needs checking
    }     // for all cells
  }       // if not all visible

  svtkIdList* cellIds = svtkIdList::New();
  svtkPoints* newPts = svtkPoints::New();
  newPts->Allocate(numPts);
  output->Allocate(numCells);
  outputPD->CopyAllocate(pd, numPts, numPts / 2);
  svtkSmartPointer<svtkIdTypeArray> originalPointIds;

  if (this->PassThroughPointIds)
  {
    originalPointIds = svtkSmartPointer<svtkIdTypeArray>::New();
    originalPointIds->SetName(this->GetOriginalPointIdsName());
    originalPointIds->SetNumberOfComponents(1);
    originalPointIds->Allocate(numPts, numPts / 2);
  }

  outputCD->CopyAllocate(cd, numCells, numCells / 2);
  svtkSmartPointer<svtkIdTypeArray> originalCellIds;
  if (this->PassThroughCellIds)
  {
    originalCellIds = svtkSmartPointer<svtkIdTypeArray>::New();
    originalCellIds->SetName(this->GetOriginalCellIdsName());
    originalCellIds->SetNumberOfComponents(1);
    originalCellIds->Allocate(numCells, numCells / 2);
  }

  svtkIdType* pointMap = nullptr;

  if (this->Merging)
  {
    if (this->Locator == nullptr)
    {
      this->CreateDefaultLocator();
    }
    this->Locator->InitPointInsertion(newPts, input->GetBounds());
  }
  else
  {
    pointMap = new svtkIdType[numPts];
    for (int i = 0; i < numPts; ++i)
    {
      pointMap[i] = -1; // initialize as unused
    }
  }

  // Traverse cells to extract geometry
  int progressCount = 0;
  int abort = 0;
  svtkIdType progressInterval = numCells / 20 + 1;

  svtkPoolManager<svtkSurfel>* pool = new svtkPoolManager<svtkSurfel>;
  pool->Init();
  this->HashTable = new svtkHashTableOfSurfels(numPts, pool);

  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal() && !abort;
       cellIter->GoToNextCell())
  {
    svtkIdType cellId = cellIter->GetCellId();
    // Progress and abort method support
    if (progressCount >= progressInterval)
    {
      svtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress((double)cellId / numCells);
      abort = this->GetAbortExecute();
      progressCount = 0;
    }
    progressCount++;

    svtkIdType npts = cellIter->GetNumberOfPoints();
    svtkIdType* pts = cellIter->GetPointIds()->GetPointer(0);
    if (allVisible || cellVis[cellId])
    {
      int cellType = cellIter->GetCellType();
      if ((cellType >= SVTK_EMPTY_CELL && cellType <= SVTK_QUAD) ||
        (cellType >= SVTK_QUADRATIC_EDGE && cellType <= SVTK_QUADRATIC_QUAD) ||
        (cellType == SVTK_BIQUADRATIC_QUAD) || (cellType == SVTK_QUADRATIC_LINEAR_QUAD) ||
        (cellType == SVTK_BIQUADRATIC_TRIANGLE) || (cellType == SVTK_CUBIC_LINE) ||
        (cellType == SVTK_QUADRATIC_POLYGON) || (cellType == SVTK_LAGRANGE_CURVE) ||
        (cellType == SVTK_LAGRANGE_QUADRILATERAL) || (cellType == SVTK_LAGRANGE_TRIANGLE) ||
        (cellType == SVTK_BEZIER_CURVE) || (cellType == SVTK_BEZIER_QUADRILATERAL) ||
        (cellType == SVTK_BEZIER_TRIANGLE))
      {
        svtkDebugMacro(<< "not 3D cell. type=" << cellType);
        // not 3D: just copy it
        cellIds->Reset();
        if (this->Merging)
        {
          double x[3];
          for (int i = 0; i < npts; ++i)
          {
            svtkIdType ptId = pts[i];
            input->GetPoint(ptId, x);
            svtkIdType newPtId;
            if (this->Locator->InsertUniquePoint(x, newPtId))
            {
              outputPD->CopyData(pd, ptId, newPtId);
              if (this->PassThroughPointIds)
              {
                originalPointIds->InsertValue(newPtId, ptId);
              }
            }
            cellIds->InsertNextId(newPtId);
          }
        } // merging coincident points
        else
        {
          for (int i = 0; i < npts; ++i)
          {
            svtkIdType ptId = pts[i];
            if (pointMap[ptId] < 0)
            {
              svtkIdType newPtId = newPts->InsertNextPoint(inPts->GetPoint(ptId));
              pointMap[ptId] = newPtId;
              outputPD->CopyData(pd, ptId, newPtId);
              if (this->PassThroughPointIds)
              {
                originalPointIds->InsertValue(newPtId, ptId);
              }
            }
            cellIds->InsertNextId(pointMap[ptId]);
          }
        } // keeping original point list

        svtkIdType newCellId = output->InsertNextCell(cellType, cellIds);
        outputCD->CopyData(cd, cellId, newCellId);
        if (this->PassThroughCellIds)
        {
          originalCellIds->InsertValue(newCellId, cellId);
        }
      }
      else // added the faces to the hashtable
      {
        svtkDebugMacro(<< "3D cell. type=" << cellType);
        switch (cellType)
        {
          case SVTK_TETRA:
            this->HashTable->InsertFaces<svtkTetra, 0, 4, 3, SVTK_TRIANGLE>(pts, cellId);
            break;
          case SVTK_VOXEL:
            // note, faces are PIXEL not QUAD. We don't need to convert
            //  to QUAD because PIXEL exist in an UnstructuredGrid.
            this->HashTable->InsertFaces<svtkVoxel, 0, 6, 4, SVTK_PIXEL>(pts, cellId);
            break;
          case SVTK_HEXAHEDRON:
            this->HashTable->InsertFaces<svtkHexahedron, 0, 6, 4, SVTK_QUAD>(pts, cellId);
            break;
          case SVTK_WEDGE:
            this->HashTable->InsertFaces<svtkWedge, 0, 2, 3, SVTK_TRIANGLE>(pts, cellId);
            this->HashTable->InsertFaces<svtkWedge, 2, 5, 4, SVTK_QUAD>(pts, cellId);
            break;
          case SVTK_PYRAMID:
            this->HashTable->InsertFaces<svtkPyramid, 0, 1, 4, SVTK_QUAD>(pts, cellId);
            this->HashTable->InsertFaces<svtkPyramid, 1, 5, 3, SVTK_TRIANGLE>(pts, cellId);
            break;
          case SVTK_PENTAGONAL_PRISM:
            this->HashTable->InsertFaces<svtkPentagonalPrism, 0, 2, 5, SVTK_POLYGON>(pts, cellId);
            this->HashTable->InsertFaces<svtkPentagonalPrism, 2, 7, 4, SVTK_QUAD>(pts, cellId);
            break;
          case SVTK_HEXAGONAL_PRISM:
            this->HashTable->InsertFaces<svtkHexagonalPrism, 0, 2, 6, SVTK_POLYGON>(pts, cellId);
            this->HashTable->InsertFaces<svtkHexagonalPrism, 2, 8, 4, SVTK_QUAD>(pts, cellId);
            break;
          case SVTK_QUADRATIC_TETRA:
            this->HashTable->InsertFaces<svtkQuadraticTetra, 0, 4, 6, SVTK_QUADRATIC_TRIANGLE>(
              pts, cellId);
            break;
          case SVTK_QUADRATIC_HEXAHEDRON:
            this->HashTable->InsertFaces<svtkQuadraticHexahedron, 0, 6, 8, SVTK_QUADRATIC_QUAD>(
              pts, cellId);
            break;
          case SVTK_QUADRATIC_WEDGE:
            this->HashTable->InsertFaces<svtkQuadraticWedge, 0, 2, 6, SVTK_QUADRATIC_TRIANGLE>(
              pts, cellId);
            this->HashTable->InsertFaces<svtkQuadraticWedge, 2, 5, 8, SVTK_QUADRATIC_QUAD>(
              pts, cellId);
            break;
          case SVTK_QUADRATIC_PYRAMID:
            this->HashTable->InsertFaces<svtkQuadraticPyramid, 0, 1, 8, SVTK_QUADRATIC_QUAD>(
              pts, cellId);
            this->HashTable->InsertFaces<svtkQuadraticPyramid, 1, 5, 6, SVTK_QUADRATIC_TRIANGLE>(
              pts, cellId);
            break;
          case SVTK_TRIQUADRATIC_HEXAHEDRON:
            this->HashTable->InsertFaces<svtkTriQuadraticHexahedron, 0, 6, 9, SVTK_BIQUADRATIC_QUAD>(
              pts, cellId);
            break;
          case SVTK_QUADRATIC_LINEAR_WEDGE:
            this->HashTable->InsertFaces<svtkQuadraticLinearWedge, 0, 2, 6, SVTK_QUADRATIC_TRIANGLE>(
              pts, cellId);
            this->HashTable
              ->InsertFaces<svtkQuadraticLinearWedge, 2, 5, 6, SVTK_QUADRATIC_LINEAR_QUAD>(
                pts, cellId);
            break;
          case SVTK_BIQUADRATIC_QUADRATIC_WEDGE:
            this->HashTable
              ->InsertFaces<svtkBiQuadraticQuadraticWedge, 0, 2, 6, SVTK_QUADRATIC_TRIANGLE>(
                pts, cellId);
            this->HashTable
              ->InsertFaces<svtkBiQuadraticQuadraticWedge, 2, 5, 9, SVTK_BIQUADRATIC_QUAD>(
                pts, cellId);
            break;
          case SVTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
            this->HashTable
              ->InsertFaces<svtkBiQuadraticQuadraticHexahedron, 0, 4, 9, SVTK_BIQUADRATIC_QUAD>(
                pts, cellId);
            this->HashTable
              ->InsertFaces<svtkBiQuadraticQuadraticHexahedron, 4, 6, 8, SVTK_QUADRATIC_QUAD>(
                pts, cellId);
            break;
          case SVTK_POLYHEDRON:
          {
            svtkIdList* faces = cellIter->GetFaces();
            int nFaces = cellIter->GetNumberOfFaces();
            for (int face = 0, fptr = 1; face < nFaces; ++face)
            {
              int pt = static_cast<int>(faces->GetId(fptr++));
              int degrees[2]{ 0, 0 };
              this->HashTable->InsertFace(
                cellId, SVTK_POLYGON, pt, faces->GetPointer(fptr), degrees);
              fptr += pt;
            }
            break;
          }
          case SVTK_LAGRANGE_HEXAHEDRON:
          case SVTK_LAGRANGE_WEDGE:
          case SVTK_LAGRANGE_TETRAHEDRON:
          case SVTK_BEZIER_HEXAHEDRON:
          case SVTK_BEZIER_WEDGE:
          case SVTK_BEZIER_TETRAHEDRON:
          {
            svtkNew<svtkGenericCell> genericCell;
            cellIter->GetCell(genericCell);
            input->SetCellOrderAndRationalWeights(cellId, genericCell);

            int nFaces = genericCell->GetNumberOfFaces();
            for (int face = 0; face < nFaces; ++face)
            {
              svtkCell* faceCell = genericCell->GetFace(face);
              svtkIdType nPoints = faceCell->GetPointIds()->GetNumberOfIds();
              svtkIdType* points = new svtkIdType[nPoints];
              for (int pt = 0; pt < nPoints; ++pt)
              {
                points[pt] = faceCell->GetPointIds()->GetId(pt);
              }

              int degrees[2]{ 0, 0 };
              if ((faceCell->GetCellType() == SVTK_BEZIER_QUADRILATERAL) ||
                (faceCell->GetCellType() == SVTK_LAGRANGE_QUADRILATERAL))
              {
                svtkHigherOrderQuadrilateral* facecellBezier =
                  dynamic_cast<svtkHigherOrderQuadrilateral*>(faceCell);
                degrees[0] = facecellBezier->GetOrder(0);
                degrees[1] = facecellBezier->GetOrder(1);
              }
              this->HashTable->InsertFace(
                cellId, faceCell->GetCellType(), nPoints, points, degrees);
              delete[] points;
            }
            break;
          }
          default:
            svtkErrorMacro(<< "Cell type " << svtkCellTypes::GetClassNameFromTypeId(cellType) << "("
                          << cellType << ")"
                          << " is not a 3D cell.");
        }
      }
    } // if cell is visible
  }   // for all cells

  // Loop over visible surfel (coming from a unique cell) in the hashtable:
  svtkHashTableOfSurfelsCursor cursor;
  cursor.Init(this->HashTable);
  cursor.Start();
  while (!cursor.IsAtEnd() && !abort)
  {
    svtkSurfel* surfel = cursor.GetCurrentSurfel();
    svtkIdType cellId = surfel->Cell3DId;
    if (cellId >= 0) // on dataset boundary
    {
      svtkIdType cellType2D = surfel->Type;
      svtkIdType npts = surfel->NumberOfPoints;
      // Dataset point Ids that form the surfel.
      svtkIdType* pts = surfel->Points;

      cellIds->Reset();
      if (this->Merging)
      {
        double x[3];
        for (int i = 0; i < npts; ++i)
        {
          svtkIdType ptId = pts[i];
          input->GetPoint(ptId, x);
          svtkIdType newPtId;
          if (this->Locator->InsertUniquePoint(x, newPtId))
          {
            outputPD->CopyData(pd, ptId, newPtId);
            if (this->PassThroughPointIds)
            {
              originalPointIds->InsertValue(newPtId, ptId);
            }
          }
          cellIds->InsertNextId(newPtId);
        }
      } // merging coincident points
      else
      {
        for (int i = 0; i < npts; ++i)
        {
          svtkIdType ptId = pts[i];
          if (pointMap[ptId] < 0)
          {
            svtkIdType newPtId = newPts->InsertNextPoint(inPts->GetPoint(ptId));
            pointMap[ptId] = newPtId;
            outputPD->CopyData(pd, ptId, newPtId);
            if (this->PassThroughPointIds)
            {
              originalPointIds->InsertValue(newPtId, ptId);
            }
          }
          cellIds->InsertNextId(pointMap[ptId]);
        }
      } // keeping original point list

      svtkIdType newCellId = output->InsertNextCell(cellType2D, cellIds);
      outputCD->CopyData(cd, cellId, newCellId);

      if (outputCD->SetActiveAttribute(
            "HigherOrderDegrees", svtkDataSetAttributes::AttributeTypes::HIGHERORDERDEGREES) != -1)
      {
        svtkDataArray* v = outputCD->GetHigherOrderDegrees();
        double degrees[3];
        degrees[0] = surfel->Degrees[0];
        degrees[1] = surfel->Degrees[1];
        degrees[2] = 0;
        v->SetTuple(newCellId, degrees);
      }

      if (this->PassThroughCellIds)
      {
        originalCellIds->InsertValue(newCellId, cellId);
      }
    }
    cursor.Next();
  }
  if (!this->Merging)
  {
    delete[] pointMap;
  }

  cellIds->Delete();
  delete this->HashTable;
  delete pool;

  // Set the output.
  output->SetPoints(newPts);
  newPts->Delete();

  if (this->PassThroughPointIds)
  {
    outputPD->AddArray(originalPointIds);
  }
  if (this->PassThroughCellIds)
  {
    outputCD->AddArray(originalCellIds);
  }

  if (!this->Merging && this->Locator)
  {
    this->Locator->Initialize();
  }

  output->Squeeze();
  delete[] cellVis;
  return 1;
}

//-----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of svtkMergePoints is used.
void svtkUnstructuredGridGeometryFilter::SetLocator(svtkIncrementalPointLocator* locator)
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
void svtkUnstructuredGridGeometryFilter::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
  }
}

//-----------------------------------------------------------------------------
int svtkUnstructuredGridGeometryFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGridBase");
  return 1;
}

//-----------------------------------------------------------------------------
void svtkUnstructuredGridGeometryFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Point Minimum : " << this->PointMinimum << "\n";
  os << indent << "Point Maximum : " << this->PointMaximum << "\n";

  os << indent << "Cell Minimum : " << this->CellMinimum << "\n";
  os << indent << "Cell Maximum : " << this->CellMaximum << "\n";

  os << indent << "Extent: \n";
  os << indent << "  Xmin,Xmax: (" << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Extent[4] << ", " << this->Extent[5] << ")\n";

  os << indent << "PointClipping: " << (this->PointClipping ? "On\n" : "Off\n");
  os << indent << "CellClipping: " << (this->CellClipping ? "On\n" : "Off\n");
  os << indent << "ExtentClipping: " << (this->ExtentClipping ? "On\n" : "Off\n");

  os << indent << "PassThroughCellIds: " << this->PassThroughCellIds << endl;
  os << indent << "PassThroughPointIds: " << this->PassThroughPointIds << endl;

  os << indent << "OriginalCellIdsName: " << this->GetOriginalCellIdsName() << endl;
  os << indent << "OriginalPointIdsName: " << this->GetOriginalPointIdsName() << endl;

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}

//-----------------------------------------------------------------------------
svtkMTimeType svtkUnstructuredGridGeometryFilter::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

//-----------------------------------------------------------------------------
int svtkUnstructuredGridGeometryFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
  {
    ++ghostLevels;
  }

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}
