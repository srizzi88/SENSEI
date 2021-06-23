/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLSDynaPartCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#ifndef svtkLSDynaPartCollection_h
#define svtkLSDynaPartCollection_h
#ifndef __SVTK_WRAP__

#include "LSDynaMetaData.h"    //needed for LSDynaMetaData::LSDYNA_TYPES enum
#include "svtkIOLSDynaModule.h" // For export macro
#include "svtkObject.h"

class svtkDataArray;
class svtkUnstructuredGrid;
class svtkPoints;
class svtkUnsignedCharArray;
class svtkLSDynaPart;

class SVTKIOLSDYNA_EXPORT svtkLSDynaPartCollection : public svtkObject
{
public:
  class LSDynaPart;
  static svtkLSDynaPartCollection* New();

  svtkTypeMacro(svtkLSDynaPartCollection, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Description:
  // Pass in the metadata to setup this collection.
  // The optional min and max cell Id are used when in parallel to load balance the nodes.
  // Meaning the collection will only store subsections of parts that fall within
  // the range of the min and max
  // Note: min is included, and max is excluded from the valid range of cells.
  void InitCollection(
    LSDynaMetaData* metaData, svtkIdType* mins = nullptr, svtkIdType* maxs = nullptr);

  // Description:
  // For a given part type returns the number of cells to read and the number
  // of cells to skip first to not read
  void GetPartReadInfo(const int& partType, svtkIdType& numberOfCells, svtkIdType& numCellsToSkip,
    svtkIdType& numCellsToSkipEnd) const;

  // Description:
  // Finalizes the cell topology by mapping the cells point indexes
  // to a relative number based on the cells this collection is storing
  void FinalizeTopology();

  // Description: Register a cell of a given type and material index to the
  // correct part
  // NOTE: the cellIndex is relative to the collection. So in parallel
  // the cellIndex will be from 0 to MaxId-MinId
  void RegisterCellIndexToPart(const int& partType, const svtkIdType& matIdx,
    const svtkIdType& cellIndex, const svtkIdType& npts);

  void InitCellInsertion();

  void AllocateParts();

  // Description: Insert a cell of a given type and material index to the
  // collection.
  // NOTE: the cellIndex is relative to the collection. So in parallel
  // the cellIndex will be from 0 to MaxId-MinId
  void InsertCell(const int& partType, const svtkIdType& matIdx, const int& cellType,
    const svtkIdType& npts, svtkIdType conn[8]);

  // Description:
  // Set for each part type what cells are deleted/dead
  void SetCellDeadFlags(
    const int& partType, svtkUnsignedCharArray* death, const int& deadCellsAsGhostArray);

  bool IsActivePart(const int& id) const;

  // Description:
  // Given a part will return the unstructured grid for the part.
  // Note: You must call finalize before using this method
  svtkUnstructuredGrid* GetGridForPart(const int& index) const;

  int GetNumberOfParts() const;

  void DisbleDeadCells();

  // Description:
  void ReadPointUserIds(const svtkIdType& numTuples, const char* name);

  // Description:
  void ReadPointProperty(const svtkIdType& numTuples, const svtkIdType& numComps, const char* name,
    const bool& isProperty = true, const bool& isGeometryPoints = false,
    const bool& isRoadPoints = false);

  // Description:
  // Adds a property for all parts of a certain type
  void AddProperty(const LSDynaMetaData::LSDYNA_TYPES& type, const char* name, const int& offset,
    const int& numComps);
  void FillCellProperties(float* buffer, const LSDynaMetaData::LSDYNA_TYPES& type,
    const svtkIdType& startId, const svtkIdType& numCells, const int& numPropertiesInCell);
  void FillCellProperties(double* buffer, const LSDynaMetaData::LSDYNA_TYPES& type,
    const svtkIdType& startId, const svtkIdType& numCells, const int& numPropertiesInCell);

  // Description:
  // Adds User Ids for all parts of a certain type
  void ReadCellUserIds(const LSDynaMetaData::LSDYNA_TYPES& type, const int& status);

  template <typename T>
  void FillCellUserId(T* buffer, const LSDynaMetaData::LSDYNA_TYPES& type, const svtkIdType& startId,
    const svtkIdType& numCells)
  {
    this->FillCellUserIdArray(buffer, type, startId, numCells);
  }

protected:
  svtkLSDynaPartCollection();
  ~svtkLSDynaPartCollection() override;

  svtkIdType* MinIds;
  svtkIdType* MaxIds;

  // Builds up the basic meta information needed for topology storage
  void BuildPartInfo();

  // Description:
  // Breaks down the buffer of cell properties to the cell properties we
  // are interested in. This will remove all properties that aren't active or
  // for parts we are not loading
  template <typename T>
  void FillCellArray(T* buffer, const LSDynaMetaData::LSDYNA_TYPES& type, const svtkIdType& startId,
    svtkIdType numCells, const int& numTuples);

  template <typename T>
  void FillCellUserIdArray(T* buffer, const LSDynaMetaData::LSDYNA_TYPES& type,
    const svtkIdType& startId, svtkIdType numCells);

  // Description:
  // Methods for adding points to the collection
  void SetupPointPropertyForReading(const svtkIdType& numTuples, const svtkIdType& numComps,
    const char* name, const bool& isIdType, const bool& isProperty, const bool& isGeometryPoints,
    const bool& isRoadPoints);
  template <typename T>
  void FillPointProperty(const svtkIdType& numTuples, const svtkIdType& numComps,
    svtkLSDynaPart** parts, const svtkIdType numParts);

private:
  svtkLSDynaPartCollection(const svtkLSDynaPartCollection&) = delete;
  void operator=(const svtkLSDynaPartCollection&) = delete;

  LSDynaMetaData* MetaData;

  class LSDynaPartStorage;
  LSDynaPartStorage* Storage;
};

#endif
#endif // LSDYNAPARTS_H
