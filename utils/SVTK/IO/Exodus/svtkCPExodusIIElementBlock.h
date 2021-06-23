/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCPExodusIIElementBlock.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCPExodusIIElementBlock
 * @brief   Uses an Exodus II element block as a
 *  svtkMappedUnstructuredGrid's implementation.
 *
 *
 * This class allows raw data arrays returned by the Exodus II library to be
 * used directly in SVTK without repacking the data into the svtkUnstructuredGrid
 * memory layout. Use the svtkCPExodusIIInSituReader to read an Exodus II file's
 * data into this structure.
 */

#ifndef svtkCPExodusIIElementBlock_h
#define svtkCPExodusIIElementBlock_h

#include "svtkIOExodusModule.h" // For export macro
#include "svtkObject.h"

#include "svtkMappedUnstructuredGrid.h" // For mapped unstructured grid wrapper

#include <string> // For std::string

class svtkGenericCell;

class SVTKIOEXODUS_EXPORT svtkCPExodusIIElementBlockImpl : public svtkObject
{
public:
  static svtkCPExodusIIElementBlockImpl* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkCPExodusIIElementBlockImpl, svtkObject);

  /**
   * Set the Exodus element block data. 'elements' is the array returned from
   * ex_get_elem_conn. 'type', 'numElements', and 'nodesPerElement' are obtained
   * from ex_get_elem_block. Returns true or false depending on whether or not
   * the element type can be translated into a SVTK cell type. This object takes
   * ownership of the elements array unless this function returns false.
   */
  bool SetExodusConnectivityArray(
    int* elements, const std::string& type, int numElements, int nodesPerElement);

  // API for svtkMappedUnstructuredGrid's implementation.
  svtkIdType GetNumberOfCells();
  int GetCellType(svtkIdType cellId);
  void GetCellPoints(svtkIdType cellId, svtkIdList* ptIds);
  void GetPointCells(svtkIdType ptId, svtkIdList* cellIds);
  int GetMaxCellSize();
  void GetIdsOfCellsOfType(int type, svtkIdTypeArray* array);
  int IsHomogeneous();

  // This container is read only -- these methods do nothing but print a
  // warning.
  void Allocate(svtkIdType numCells, int extSize = 1000);
  svtkIdType InsertNextCell(int type, svtkIdList* ptIds);
  svtkIdType InsertNextCell(int type, svtkIdType npts, const svtkIdType ptIds[])
    SVTK_SIZEHINT(ptIds, npts);
  svtkIdType InsertNextCell(int type, svtkIdType npts, const svtkIdType ptIds[], svtkIdType nfaces,
    const svtkIdType faces[]) SVTK_SIZEHINT(ptIds, npts) SVTK_SIZEHINT(faces, nfaces);
  void ReplaceCell(svtkIdType cellId, int npts, const svtkIdType pts[]) SVTK_SIZEHINT(pts, npts);

protected:
  svtkCPExodusIIElementBlockImpl();
  ~svtkCPExodusIIElementBlockImpl() override;

private:
  svtkCPExodusIIElementBlockImpl(const svtkCPExodusIIElementBlockImpl&) = delete;
  void operator=(const svtkCPExodusIIElementBlockImpl&) = delete;

  // Convert between Exodus node ids and SVTK point ids.
  static svtkIdType NodeToPoint(const int& id) { return static_cast<svtkIdType>(id - 1); }
  static int PointToNode(const svtkIdType& id) { return static_cast<int>(id + 1); }

  // Convenience methods to get pointers into the element array.
  int* GetElementStart(svtkIdType cellId) const
  {
    return this->Elements + (cellId * this->CellSize);
  }
  int* GetElementEnd(svtkIdType cellId) const
  {
    return this->Elements + (cellId * this->CellSize) + this->CellSize;
  }
  int* GetStart() const { return this->Elements; }
  int* GetEnd() const { return this->Elements + (this->NumberOfCells * this->CellSize); }

  int* Elements;
  int CellType;
  int CellSize;
  svtkIdType NumberOfCells;
};

svtkMakeExportedMappedUnstructuredGrid(
  svtkCPExodusIIElementBlock, svtkCPExodusIIElementBlockImpl, SVTKIOEXODUS_EXPORT);

#endif // svtkCPExodusIIElementBlock_h
