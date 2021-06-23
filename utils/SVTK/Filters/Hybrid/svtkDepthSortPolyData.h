/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDepthSortPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDepthSortPolyData
 * @brief   sort poly data along camera view direction
 *
 * svtkDepthSortPolyData rearranges the order of cells so that certain
 * rendering operations (e.g., transparency or Painter's algorithms)
 * generate correct results. To use this filter you must specify the
 * direction vector along which to sort the cells. You can do this by
 * specifying a camera and/or prop to define a view direction; or
 * explicitly set a view direction.
 *
 * @warning
 * The sort operation will not work well for long, thin primitives, or cells
 * that intersect, overlap, or interpenetrate each other.
 */

#ifndef svtkDepthSortPolyData_h
#define svtkDepthSortPolyData_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkCamera;
class svtkProp3D;
class svtkTransform;

class SVTKFILTERSHYBRID_EXPORT svtkDepthSortPolyData : public svtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate object.
   */
  static svtkDepthSortPolyData* New();

  svtkTypeMacro(svtkDepthSortPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum Directions
  {
    SVTK_DIRECTION_BACK_TO_FRONT = 0,
    SVTK_DIRECTION_FRONT_TO_BACK = 1,
    SVTK_DIRECTION_SPECIFIED_VECTOR = 2
  };

  //@{
  /**
   * Specify the sort method for the polygonal primitives. By default, the
   * poly data is sorted from back to front.
   */
  svtkSetMacro(Direction, int);
  svtkGetMacro(Direction, int);
  void SetDirectionToFrontToBack() { this->SetDirection(SVTK_DIRECTION_FRONT_TO_BACK); }
  void SetDirectionToBackToFront() { this->SetDirection(SVTK_DIRECTION_BACK_TO_FRONT); }
  void SetDirectionToSpecifiedVector() { this->SetDirection(SVTK_DIRECTION_SPECIFIED_VECTOR); }
  //@}

  enum SortMode
  {
    SVTK_SORT_FIRST_POINT = 0,
    SVTK_SORT_BOUNDS_CENTER = 1,
    SVTK_SORT_PARAMETRIC_CENTER = 2
  };

  //@{
  /**
   * Specify the point to use when sorting. The fastest is to just
   * take the first cell point. Other options are to take the bounding
   * box center or the parametric center of the cell. By default, the
   * first cell point is used.
   */
  svtkSetMacro(DepthSortMode, int);
  svtkGetMacro(DepthSortMode, int);
  void SetDepthSortModeToFirstPoint() { this->SetDepthSortMode(SVTK_SORT_FIRST_POINT); }
  void SetDepthSortModeToBoundsCenter() { this->SetDepthSortMode(SVTK_SORT_BOUNDS_CENTER); }
  void SetDepthSortModeToParametricCenter() { this->SetDepthSortMode(SVTK_SORT_PARAMETRIC_CENTER); }
  //@}

  //@{
  /**
   * Specify a camera that is used to define a view direction along which
   * the cells are sorted. This ivar only has effect if the direction is set
   * to front-to-back or back-to-front, and a camera is specified.
   */
  virtual void SetCamera(svtkCamera*);
  svtkGetObjectMacro(Camera, svtkCamera);
  //@}

  /**
   * Specify a transformation matrix (via the svtkProp3D::GetMatrix() method)
   * that is used to include the effects of transformation. This ivar only
   * has effect if the direction is set to front-to-back or back-to-front,
   * and a camera is specified. Specifying the svtkProp3D is optional.
   */
  void SetProp3D(svtkProp3D*);
  svtkProp3D* GetProp3D() { return this->Prop3D; }

  //@{
  /**
   * Set/Get the sort direction. This ivar only has effect if the sort
   * direction is set to SetDirectionToSpecifiedVector(). The sort occurs
   * in the direction of the vector.
   */
  svtkSetVector3Macro(Vector, double);
  svtkGetVectorMacro(Vector, double, 3);
  //@}

  //@{
  /**
   * Set/Get the sort origin. This ivar only has effect if the sort
   * direction is set to SetDirectionToSpecifiedVector(). The sort occurs
   * in the direction of the vector, with this point specifying the
   * origin.
   */
  svtkSetVector3Macro(Origin, double);
  svtkGetVectorMacro(Origin, double, 3);
  //@}

  //@{
  /**
   * Set/Get a flag that controls the generation of scalar values
   * corresponding to the sort order. If enabled, the output of this
   * filter will include scalar values that range from 0 to (ncells-1),
   * where 0 is closest to the sort direction.
   */
  svtkSetMacro(SortScalars, svtkTypeBool);
  svtkGetMacro(SortScalars, svtkTypeBool);
  svtkBooleanMacro(SortScalars, svtkTypeBool);
  //@}

  /**
   * Return MTime also considering the dependent objects: the camera
   * and/or the prop3D.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkDepthSortPolyData();
  ~svtkDepthSortPolyData() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ComputeProjectionVector(double vector[3], double origin[3]);

  int Direction;
  int DepthSortMode;
  svtkCamera* Camera;
  svtkProp3D* Prop3D;
  svtkTransform* Transform;
  double Vector[3];
  double Origin[3];
  svtkTypeBool SortScalars;

private:
  svtkDepthSortPolyData(const svtkDepthSortPolyData&) = delete;
  void operator=(const svtkDepthSortPolyData&) = delete;
};

#endif
