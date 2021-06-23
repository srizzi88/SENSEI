/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedFrustum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractSelectedFrustum
 * @brief   Returns the portion of the input dataset that
 * lies within a selection frustum.
 *
 *
 * This class intersects the input DataSet with a frustum and determines which
 * cells and points lie within the frustum. The frustum is defined with a
 * svtkPlanes containing six cutting planes. The output is a DataSet that is
 * either a shallow copy of the input dataset with two new "svtkInsidedness"
 * attribute arrays, or a completely new UnstructuredGrid that contains only
 * the cells and points of the input that are inside the frustum. The
 * PreserveTopology flag controls which occurs. When PreserveTopology is off
 * this filter adds a scalar array called svtkOriginalCellIds that says what
 * input cell produced each output cell. This is an example of a Pedigree ID
 * which helps to trace back results.
 *
 * @sa
 * svtkExtractGeometry, svtkAreaPicker, svtkExtractSelection, svtkSelection
 */

#ifndef svtkExtractSelectedFrustum_h
#define svtkExtractSelectedFrustum_h

#include "svtkExtractSelectionBase.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkPlanes;
class svtkInformation;
class svtkInformationVector;
class svtkCell;
class svtkPoints;
class svtkDoubleArray;

class SVTKFILTERSGENERAL_EXPORT svtkExtractSelectedFrustum : public svtkExtractSelectionBase
{
public:
  static svtkExtractSelectedFrustum* New();
  svtkTypeMacro(svtkExtractSelectedFrustum, svtkExtractSelectionBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return the MTime taking into account changes to the Frustum
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set the selection frustum. The planes object must contain six planes.
   */
  virtual void SetFrustum(svtkPlanes*);
  svtkGetObjectMacro(Frustum, svtkPlanes);
  //@}

  /**
   * Given eight vertices, creates a frustum.
   * each pt is x,y,z,1
   * in the following order
   * near lower left, far lower left
   * near upper left, far upper left
   * near lower right, far lower right
   * near upper right, far upper right
   */
  void CreateFrustum(double vertices[32]);

  //@{
  /**
   * Return eight points that define the selection frustum. Valid if
   * create Frustum was used, invalid if SetFrustum was.
   */
  svtkGetObjectMacro(ClipPoints, svtkPoints);
  //@}

  //@{
  /**
   * Sets/gets the intersection test type.
   */
  svtkSetMacro(FieldType, int);
  svtkGetMacro(FieldType, int);
  //@}

  //@{
  /**
   * Sets/gets the intersection test type. Only meaningful when fieldType is
   * svtkSelection::POINT
   */
  svtkSetMacro(ContainingCells, int);
  svtkGetMacro(ContainingCells, int);
  //@}

  /**
   * Does a quick test on the AABBox defined by the bounds.
   */
  int OverallBoundsTest(double* bounds);

  //@{
  /**
   * When On, this returns an unstructured grid that outlines selection area.
   * Off is the default.
   */
  svtkSetMacro(ShowBounds, svtkTypeBool);
  svtkGetMacro(ShowBounds, svtkTypeBool);
  svtkBooleanMacro(ShowBounds, svtkTypeBool);
  //@}

  //@{
  /**
   * When on, extracts cells outside the frustum instead of inside.
   */
  svtkSetMacro(InsideOut, svtkTypeBool);
  svtkGetMacro(InsideOut, svtkTypeBool);
  svtkBooleanMacro(InsideOut, svtkTypeBool);
  //@}

protected:
  svtkExtractSelectedFrustum(svtkPlanes* f = nullptr);
  ~svtkExtractSelectedFrustum() override;

  // sets up output dataset
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  // execution
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int ABoxFrustumIsect(double bounds[], svtkCell* cell);
  int FrustumClipPolygon(int nverts, double* ivlist, double* wvlist, double* ovlist);
  void PlaneClipPolygon(int nverts, double* ivlist, int pid, int& noverts, double* ovlist);
  void PlaneClipEdge(double* V0, double* V1, int pid, int& noverts, double* overts);
  int IsectDegenerateCell(svtkCell* cell);

  // used in CreateFrustum
  void ComputePlane(
    int idx, double v0[3], double v1[2], double v2[3], svtkPoints* points, svtkDoubleArray* norms);

  // modes
  int FieldType;
  int ContainingCells;
  svtkTypeBool InsideOut;

  // used internally
  svtkPlanes* Frustum;
  int np_vertids[6][2];

  // for debugging
  svtkPoints* ClipPoints;
  int NumRejects;
  int NumIsects;
  int NumAccepts;
  svtkTypeBool ShowBounds;

private:
  svtkExtractSelectedFrustum(const svtkExtractSelectedFrustum&) = delete;
  void operator=(const svtkExtractSelectedFrustum&) = delete;
};

#endif
