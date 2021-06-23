/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetToDataObjectFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataSetToDataObjectFilter
 * @brief   map dataset into data object (i.e., a field)
 *
 * svtkDataSetToDataObjectFilter is an class that transforms a dataset into
 * data object (i.e., a field). The field will have labeled data arrays
 * corresponding to the topology, geometry, field data, and point and cell
 * attribute data.
 *
 * You can control what portions of the dataset are converted into the
 * output data object's field data. The instance variables Geometry,
 * Topology, FieldData, PointData, and CellData are flags that control
 * whether the dataset's geometry (e.g., points, spacing, origin);
 * topology (e.g., cell connectivity, dimensions); the field data
 * associated with the dataset's superclass data object; the dataset's
 * point data attributes; and the dataset's cell data attributes. (Note:
 * the data attributes include scalars, vectors, tensors, normals, texture
 * coordinates, and field data.)
 *
 * The names used to create the field data are as follows. For svtkPolyData,
 * "Points", "Verts", "Lines", "Polys", and "Strips". For svtkUnstructuredGrid,
 * "Cells" and "CellTypes". For svtkStructuredPoints, "Dimensions", "Spacing",
 * and "Origin". For svtkStructuredGrid, "Points" and "Dimensions". For
 * svtkRectilinearGrid, "XCoordinates", "YCoordinates", and "ZCoordinates".
 * for point attribute data, "PointScalars", "PointVectors", etc. For cell
 * attribute data, "CellScalars", "CellVectors", etc. Field data arrays retain
 * their original name.
 *
 * @sa
 * svtkDataObject svtkFieldData svtkDataObjectToDataSetFilter
 */

#ifndef svtkDataSetToDataObjectFilter_h
#define svtkDataSetToDataObjectFilter_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class svtkDataSet;

class SVTKFILTERSCORE_EXPORT svtkDataSetToDataObjectFilter : public svtkDataObjectAlgorithm
{
public:
  svtkTypeMacro(svtkDataSetToDataObjectFilter, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate the object to transform all data into a data object.
   */
  static svtkDataSetToDataObjectFilter* New();

  //@{
  /**
   * Turn on/off the conversion of dataset geometry to a data object.
   */
  svtkSetMacro(Geometry, svtkTypeBool);
  svtkGetMacro(Geometry, svtkTypeBool);
  svtkBooleanMacro(Geometry, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the conversion of dataset topology to a data object.
   */
  svtkSetMacro(Topology, svtkTypeBool);
  svtkGetMacro(Topology, svtkTypeBool);
  svtkBooleanMacro(Topology, svtkTypeBool);
  //@}

  //@{
  /**
   * If LegacyTopology and Topology are both true, print out the legacy topology
   * arrays. Default is true.
   */
  svtkSetMacro(LegacyTopology, svtkTypeBool);
  svtkGetMacro(LegacyTopology, svtkTypeBool);
  svtkBooleanMacro(LegacyTopology, svtkTypeBool);
  //@}

  //@{
  /**
   * If ModernTopology and Topology are both true, print out the modern topology
   * arrays. Default is true.
   */
  svtkSetMacro(ModernTopology, svtkTypeBool);
  svtkGetMacro(ModernTopology, svtkTypeBool);
  svtkBooleanMacro(ModernTopology, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the conversion of dataset field data to a data object.
   */
  svtkSetMacro(FieldData, svtkTypeBool);
  svtkGetMacro(FieldData, svtkTypeBool);
  svtkBooleanMacro(FieldData, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the conversion of dataset point data to a data object.
   */
  svtkSetMacro(PointData, svtkTypeBool);
  svtkGetMacro(PointData, svtkTypeBool);
  svtkBooleanMacro(PointData, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the conversion of dataset cell data to a data object.
   */
  svtkSetMacro(CellData, svtkTypeBool);
  svtkGetMacro(CellData, svtkTypeBool);
  svtkBooleanMacro(CellData, svtkTypeBool);
  //@}

protected:
  svtkDataSetToDataObjectFilter();
  ~svtkDataSetToDataObjectFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**,
    svtkInformationVector*) override; // generate output data
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int, svtkInformation*) override;

  svtkTypeBool Geometry;
  svtkTypeBool Topology;
  svtkTypeBool LegacyTopology;
  svtkTypeBool ModernTopology;
  svtkTypeBool PointData;
  svtkTypeBool CellData;
  svtkTypeBool FieldData;

private:
  svtkDataSetToDataObjectFilter(const svtkDataSetToDataObjectFilter&) = delete;
  void operator=(const svtkDataSetToDataObjectFilter&) = delete;
};

#endif
