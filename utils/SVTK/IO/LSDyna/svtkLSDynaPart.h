/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLSDynaPart.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#ifndef svtkLSDynaPart_h
#define svtkLSDynaPart_h
#ifndef __SVTK_WRAP__

#include "LSDynaMetaData.h"    //needed for lsdyna types
#include "svtkIOLSDynaModule.h" // For export macro
#include "svtkObject.h"
#include "svtkStdString.h" //needed for string

class svtkUnstructuredGrid;
class svtkPoints;

class SVTKIOLSDYNA_EXPORT svtkLSDynaPart : public svtkObject
{
public:
  static svtkLSDynaPart* New();

  svtkTypeMacro(svtkLSDynaPart, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Description: Set the type of the part
  void SetPartType(int type);

  // Description: Returns the type of the part
  LSDynaMetaData::LSDYNA_TYPES PartType() const { return Type; }

  // Description: Returns if the type of the part is considered valid
  bool hasValidType() const;

  svtkIdType GetUserMaterialId() const { return UserMaterialId; }
  svtkIdType GetPartId() const { return PartId; }
  bool HasCells() const;

  // Setup the part with some basic information about what it holds
  void InitPart(svtkStdString name, const svtkIdType& partId, const svtkIdType& userMaterialId,
    const svtkIdType& numGlobalPoints, const int& sizeOfWord);

  // Reserves the needed space in memory for this part
  // that way we never over allocate memory
  void AllocateCellMemory(const svtkIdType& numCells, const svtkIdType& cellLen);

  // Add a cell to the part
  void AddCell(const int& cellType, const svtkIdType& npts, svtkIdType conn[8]);

  // Description:
  // Setups the part cell topology so that we can cache information
  // between timesteps.
  void BuildToplogy();

  // Description:
  // Returns if the toplogy for this part has been constructed
  bool IsTopologyBuilt() const { return TopologyBuilt; }

  // Description:
  // Constructs the grid for this part and returns it.
  svtkUnstructuredGrid* GenerateGrid();

  // Description:
  // allows the part to store dead cells
  void EnableDeadCells(const int& deadCellsAsGhostArray);

  // Description:
  // removes the dead cells array if it exists from the grid
  void DisableDeadCells();

  // Description:
  // We set cells as dead to make them not show up during rendering
  void SetCellsDeadState(unsigned char* dead, const svtkIdType& size);

  // Description:
  // allows the part to store user cell ids
  void EnableCellUserIds();

  // Description:
  // Set the user ids for the cells of this grid
  void SetNextCellUserIds(const svtkIdType& value);

  // Description:
  // Called to init point filling for a property
  // is also able to set the point position of the grid too as that
  // is stored as a point property
  void AddPointProperty(const char* name, const svtkIdType& numComps, const bool& isIdTypeProperty,
    const bool& isProperty, const bool& isGeometryPoints);

  // Description:
  // Given a chunk of point property memory copy it to the correct
  // property on the part
  void ReadPointBasedProperty(float* data, const svtkIdType& numTuples, const svtkIdType& numComps,
    const svtkIdType& currentGlobalPointIndex);

  void ReadPointBasedProperty(double* data, const svtkIdType& numTuples, const svtkIdType& numComps,
    const svtkIdType& currentGlobalPointIndex);

  // Description:
  // Adds a property to the part
  void AddCellProperty(const char* name, const int& offset, const int& numComps);

  // Description:
  // Given the raw data converts it to be the properties for this part
  // The cell properties are woven together as a block for each cell
  void ReadCellProperties(
    float* cellProperties, const svtkIdType& numCells, const svtkIdType& numPropertiesInCell);
  void ReadCellProperties(
    double* cellsProperties, const svtkIdType& numCells, const svtkIdType& numPropertiesInCell);

  // Description:
  // Get the id of the lowest global point this part needs
  // Note: Presumes topology has been built already
  svtkIdType GetMinGlobalPointId() const;

  // Description:
  // Get the id of the largest global point this part needs
  // Note: Presumes topology has been built already
  svtkIdType GetMaxGlobalPointId() const;

protected:
  svtkLSDynaPart();
  ~svtkLSDynaPart() override;

  svtkUnstructuredGrid* RemoveDeletedCells();

  void BuildUniquePoints();
  void BuildCells();

  void GetPropertyData(const char* name, const svtkIdType& numComps, const bool& isIdTypeArray,
    const bool& isProperty, const bool& isGeometry);

  template <typename T>
  void AddPointInformation(T* buffer, T* pointData, const svtkIdType& numTuples,
    const svtkIdType& numComps, const svtkIdType& currentGlobalPointIndex);

  // basic info about the part
  LSDynaMetaData::LSDYNA_TYPES Type;
  svtkStdString Name;
  svtkIdType UserMaterialId;
  svtkIdType PartId;

  svtkIdType NumberOfCells;
  svtkIdType NumberOfPoints;
  svtkIdType NumberOfGlobalPoints;

  bool DeadCellsAsGhostArray;
  bool HasDeadCells;

  bool TopologyBuilt;
  bool DoubleBased;

  svtkUnstructuredGrid* Grid;
  svtkUnstructuredGrid* ThresholdGrid;

  svtkPoints* Points;

  class InternalCells;
  InternalCells* Cells;

  class InternalCellProperties;
  InternalCellProperties* CellProperties;

  class InternalPointsUsed;
  class DensePointsUsed;
  class SparsePointsUsed;
  InternalPointsUsed* GlobalPointsUsed;

  // used when reading properties
  class InternalCurrentPointInfo;
  InternalCurrentPointInfo* CurrentPointPropInfo;

private:
  svtkLSDynaPart(const svtkLSDynaPart&) = delete;
  void operator=(const svtkLSDynaPart&) = delete;
};

#endif
#endif // SVTKLSDYNAPART
