/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoxClipDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkBoxClipDataSet
 * @brief   clip an unstructured grid
 *
 *
 * Clipping means that is actually 'cuts' through the cells of the dataset,
 * returning tetrahedral cells inside of the box.
 * The output of this filter is an unstructured grid.
 *
 * This filter can be configured to compute a second output. The
 * second output is the part of the cell that is clipped away. Set the
 * GenerateClippedData boolean on if you wish to access this output data.
 *
 * The svtkBoxClipDataSet will triangulate all types of 3D cells (i.e, create tetrahedra).
 * This is necessary to preserve compatibility across face neighbors.
 *
 * To use this filter,you can decide if you will be clipping with a box or a hexahedral box.
 * 1) Set orientation
 *    if(SetOrientation(0)): box (parallel with coordinate axis)
 *       SetBoxClip(xmin,xmax,ymin,ymax,zmin,zmax)
 *    if(SetOrientation(1)): hexahedral box (Default)
 *       SetBoxClip(n[0],o[0],n[1],o[1],n[2],o[2],n[3],o[3],n[4],o[4],n[5],o[5])
 *       PlaneNormal[] normal of each plane
 *       PlanePoint[] point on the plane
 * 2) Apply the GenerateClipScalarsOn()
 * 3) Execute clipping  Update();
 */

#ifndef svtkBoxClipDataSet_h
#define svtkBoxClipDataSet_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkCell3D;
class svtkCellArray;
class svtkCellData;
class svtkDataArray;
class svtkDataSetAttributes;
class svtkIdList;
class svtkGenericCell;
class svtkPointData;
class svtkIncrementalPointLocator;
class svtkPoints;

class SVTKFILTERSGENERAL_EXPORT svtkBoxClipDataSet : public svtkUnstructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkBoxClipDataSet, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Constructor of the clipping box. The initial box is (0,1,0,1,0,1).
   * The hexahedral box and the parallel box parameters are set to match this
   * box.
   */
  static svtkBoxClipDataSet* New();

  //@{
  /**
   * Specify the Box with which to perform the clipping.
   * If the box is not parallel to axis, you need to especify
   * normal vector of each plane and a point on the plane.
   */
  void SetBoxClip(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  void SetBoxClip(const double* n0, const double* o0, const double* n1, const double* o1,
    const double* n2, const double* o2, const double* n3, const double* o3, const double* n4,
    const double* o4, const double* n5, const double* o5);
  //@}

  //@{
  /**
   * If this flag is enabled, then the output scalar values will be
   * interpolated, and not the input scalar data.
   */
  svtkSetMacro(GenerateClipScalars, svtkTypeBool);
  svtkGetMacro(GenerateClipScalars, svtkTypeBool);
  svtkBooleanMacro(GenerateClipScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Control whether a second output is generated. The second output
   * contains the polygonal data that's been clipped away.
   */
  svtkSetMacro(GenerateClippedOutput, svtkTypeBool);
  svtkGetMacro(GenerateClippedOutput, svtkTypeBool);
  svtkBooleanMacro(GenerateClippedOutput, svtkTypeBool);
  //@}

  /**
   * Set the tolerance for merging clip intersection points that are near
   * the vertices of cells. This tolerance is used to prevent the generation
   * of degenerate primitives. Note that only 3D cells actually use this
   * instance variable.
   * svtkSetClampMacro(MergeTolerance,double,0.0001,0.25);
   * svtkGetMacro(MergeTolerance,double);
   */

  //@{
  /**
   * Return the Clipped output.
   */
  svtkUnstructuredGrid* GetClippedOutput();
  virtual int GetNumberOfOutputs();
  //@}

  //@{
  /**
   * Specify a spatial locator for merging points. By default, an
   * instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  /**
   * Return the mtime also considering the locator.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Tells if clipping happens with a box parallel with coordinate axis
   * (0) or with an hexahedral box (1). Initial value is 1.
   */
  svtkGetMacro(Orientation, unsigned int);
  svtkSetMacro(Orientation, unsigned int);
  //@}

  static void InterpolateEdge(svtkDataSetAttributes* attributes, svtkIdType toId, svtkIdType fromId1,
    svtkIdType fromId2, double t);

  void MinEdgeF(const unsigned int* id_v, const svtkIdType* cellIds, unsigned int* edgF);
  void PyramidToTetra(
    const svtkIdType* pyramId, const svtkIdType* cellIds, svtkCellArray* newCellArray);
  void WedgeToTetra(const svtkIdType* wedgeId, const svtkIdType* cellIds, svtkCellArray* newCellArray);
  void CellGrid(
    svtkIdType typeobj, svtkIdType npts, const svtkIdType* cellIds, svtkCellArray* newCellArray);
  void CreateTetra(svtkIdType npts, const svtkIdType* cellIds, svtkCellArray* newCellArray);
  void ClipBox(svtkPoints* newPoints, svtkGenericCell* cell, svtkIncrementalPointLocator* locator,
    svtkCellArray* tets, svtkPointData* inPD, svtkPointData* outPD, svtkCellData* inCD,
    svtkIdType cellId, svtkCellData* outCD);
  void ClipHexahedron(svtkPoints* newPoints, svtkGenericCell* cell,
    svtkIncrementalPointLocator* locator, svtkCellArray* tets, svtkPointData* inPD,
    svtkPointData* outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData* outCD);
  void ClipBoxInOut(svtkPoints* newPoints, svtkGenericCell* cell, svtkIncrementalPointLocator* locator,
    svtkCellArray** tets, svtkPointData* inPD, svtkPointData** outPD, svtkCellData* inCD,
    svtkIdType cellId, svtkCellData** outCD);
  void ClipHexahedronInOut(svtkPoints* newPoints, svtkGenericCell* cell,
    svtkIncrementalPointLocator* locator, svtkCellArray** tets, svtkPointData* inPD,
    svtkPointData** outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData** outCD);

  void ClipBox2D(svtkPoints* newPoints, svtkGenericCell* cell, svtkIncrementalPointLocator* locator,
    svtkCellArray* tets, svtkPointData* inPD, svtkPointData* outPD, svtkCellData* inCD,
    svtkIdType cellId, svtkCellData* outCD);
  void ClipBoxInOut2D(svtkPoints* newPoints, svtkGenericCell* cell,
    svtkIncrementalPointLocator* locator, svtkCellArray** tets, svtkPointData* inPD,
    svtkPointData** outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData** outCD);
  void ClipHexahedron2D(svtkPoints* newPoints, svtkGenericCell* cell,
    svtkIncrementalPointLocator* locator, svtkCellArray* tets, svtkPointData* inPD,
    svtkPointData* outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData* outCD);
  void ClipHexahedronInOut2D(svtkPoints* newPoints, svtkGenericCell* cell,
    svtkIncrementalPointLocator* locator, svtkCellArray** tets, svtkPointData* inPD,
    svtkPointData** outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData** outCD);

  void ClipBox1D(svtkPoints* newPoints, svtkGenericCell* cell, svtkIncrementalPointLocator* locator,
    svtkCellArray* lines, svtkPointData* inPD, svtkPointData* outPD, svtkCellData* inCD,
    svtkIdType cellId, svtkCellData* outCD);
  void ClipBoxInOut1D(svtkPoints* newPoints, svtkGenericCell* cell,
    svtkIncrementalPointLocator* locator, svtkCellArray** lines, svtkPointData* inPD,
    svtkPointData** outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData** outCD);
  void ClipHexahedron1D(svtkPoints* newPoints, svtkGenericCell* cell,
    svtkIncrementalPointLocator* locator, svtkCellArray* lines, svtkPointData* inPD,
    svtkPointData* outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData* outCD);
  void ClipHexahedronInOut1D(svtkPoints* newPoints, svtkGenericCell* cell,
    svtkIncrementalPointLocator* locator, svtkCellArray** lines, svtkPointData* inPD,
    svtkPointData** outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData** outCD);

  void ClipBox0D(svtkGenericCell* cell, svtkIncrementalPointLocator* locator, svtkCellArray* verts,
    svtkPointData* inPD, svtkPointData* outPD, svtkCellData* inCD, svtkIdType cellId,
    svtkCellData* outCD);
  void ClipBoxInOut0D(svtkGenericCell* cell, svtkIncrementalPointLocator* locator,
    svtkCellArray** verts, svtkPointData* inPD, svtkPointData** outPD, svtkCellData* inCD,
    svtkIdType cellId, svtkCellData** outCD);
  void ClipHexahedron0D(svtkGenericCell* cell, svtkIncrementalPointLocator* locator,
    svtkCellArray* verts, svtkPointData* inPD, svtkPointData* outPD, svtkCellData* inCD,
    svtkIdType cellId, svtkCellData* outCD);
  void ClipHexahedronInOut0D(svtkGenericCell* cell, svtkIncrementalPointLocator* locator,
    svtkCellArray** verts, svtkPointData* inPD, svtkPointData** outPD, svtkCellData* inCD,
    svtkIdType cellId, svtkCellData** outCD);

protected:
  svtkBoxClipDataSet();
  ~svtkBoxClipDataSet() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkIncrementalPointLocator* Locator;
  svtkTypeBool GenerateClipScalars;

  svtkTypeBool GenerateClippedOutput;

  // double MergeTolerance;

  double BoundBoxClip[3][2];
  unsigned int Orientation;
  double PlaneNormal[6][3]; // normal of each plane
  double PlanePoint[6][3];  // point on the plane

private:
  svtkBoxClipDataSet(const svtkBoxClipDataSet&) = delete;
  void operator=(const svtkBoxClipDataSet&) = delete;
};

#endif
