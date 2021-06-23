/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataSilhouette.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataSilhouette
 * @brief   sort polydata along camera view direction
 *
 *
 * svtkPolyDataSilhouette extracts a subset of a polygonal mesh edges to
 * generate an outline (silhouette) of the corresponding 3D object. In
 * addition, this filter can also extracts sharp edges (aka feature angles).
 * In order to use this filter you must specify the a point of view (origin) or
 * a direction (vector).  given this direction or origin, a silhouette is
 * generated wherever the surface's normal is orthogonal to the view direction.
 *
 * @warning
 * when the active camera is used, almost everything is recomputed for each
 * frame, keep this in mind when dealing with extremely large surface data
 * sets.
 *
 * @par Thanks:
 * Contribution by Thierry Carrard <br>
 * CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
 * BP12, F-91297 Arpajon, France. <br>
 */

#ifndef svtkPolyDataSilhouette_h
#define svtkPolyDataSilhouette_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkCamera;
class svtkProp3D;
class svtkTransform;
class svtkPolyDataEdges;

class SVTKFILTERSHYBRID_EXPORT svtkPolyDataSilhouette : public svtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate object.
   */
  static svtkPolyDataSilhouette* New();

  svtkTypeMacro(svtkPolyDataSilhouette, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Enables or Disables generation of silhouette edges along sharp edges
   */
  svtkSetMacro(EnableFeatureAngle, int);
  svtkGetMacro(EnableFeatureAngle, int);
  //@}

  //@{
  /**
   * Sets/Gets minimal angle for sharp edges detection. Default is 60
   */
  svtkSetMacro(FeatureAngle, double);
  svtkGetMacro(FeatureAngle, double);
  //@}

  //@{
  /**
   * Enables or Disables generation of border edges. Note: borders exist only
   * in case of non closed surface
   */
  svtkSetMacro(BorderEdges, svtkTypeBool);
  svtkGetMacro(BorderEdges, svtkTypeBool);
  svtkBooleanMacro(BorderEdges, svtkTypeBool);
  //@}

  //@{
  /**
   * Enables or Disables piece invariance. This is useful when dealing with
   * multi-block data sets. Note: requires one level of ghost cells
   */
  svtkSetMacro(PieceInvariant, svtkTypeBool);
  svtkGetMacro(PieceInvariant, svtkTypeBool);
  svtkBooleanMacro(PieceInvariant, svtkTypeBool);
  //@}

  enum Directions
  {
    SVTK_DIRECTION_SPECIFIED_VECTOR = 0,
    SVTK_DIRECTION_SPECIFIED_ORIGIN = 1,
    SVTK_DIRECTION_CAMERA_ORIGIN = 2,
    SVTK_DIRECTION_CAMERA_VECTOR = 3
  };

  //@{
  /**
   * Specify how view direction is computed. By default, the
   * camera origin (eye) is used.
   */
  svtkSetMacro(Direction, int);
  svtkGetMacro(Direction, int);
  void SetDirectionToSpecifiedVector() { this->SetDirection(SVTK_DIRECTION_SPECIFIED_VECTOR); }
  void SetDirectionToSpecifiedOrigin() { this->SetDirection(SVTK_DIRECTION_SPECIFIED_ORIGIN); }
  void SetDirectionToCameraVector() { this->SetDirection(SVTK_DIRECTION_CAMERA_VECTOR); }
  void SetDirectionToCameraOrigin() { this->SetDirection(SVTK_DIRECTION_CAMERA_ORIGIN); }
  //@}

  //@{
  /**
   * Specify a camera that is used to define the view direction.  This ivar
   * only has effect if the direction is set to SVTK_DIRECTION_CAMERA_ORIGIN or
   * SVTK_DIRECTION_CAMERA_VECTOR, and a camera is specified.
   */
  virtual void SetCamera(svtkCamera SVTK_WRAP_EXTERN*);
  svtkGetObjectMacro(Camera, svtkCamera SVTK_WRAP_EXTERN);
  //@}

  //@{
  /**
   * Specify a transformation matrix (via the svtkProp3D::GetMatrix() method)
   * that is used to include the effects of transformation. This ivar only has
   * effect if the direction is set to SVTK_DIRECTION_CAMERA_ORIGIN or
   * SVTK_DIRECTION_CAMERA_VECTOR, and a camera is specified. Specifying the
   * svtkProp3D is optional.
   */
  void SetProp3D(svtkProp3D SVTK_WRAP_EXTERN*);
  svtkProp3D SVTK_WRAP_EXTERN* GetProp3D();
  //@}

  //@{
  /**
   * Set/Get the sort direction. This ivar only has effect if the sort
   * direction is set to SetDirectionToSpecifiedVector(). The edge detection
   * occurs in the direction of the vector.
   */
  svtkSetVector3Macro(Vector, double);
  svtkGetVectorMacro(Vector, double, 3);
  //@}

  //@{
  /**
   * Set/Get the sort origin. This ivar only has effect if the sort direction
   * is set to SetDirectionToSpecifiedOrigin(). The edge detection occurs in
   * the direction of the origin to each edge's center.
   */
  svtkSetVector3Macro(Origin, double);
  svtkGetVectorMacro(Origin, double, 3);
  //@}

  /**
   * Return MTime also considering the dependent objects: the camera
   * and/or the prop3D.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkPolyDataSilhouette();
  ~svtkPolyDataSilhouette() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ComputeProjectionVector(double vector[3], double origin[3]);

  int Direction;
  svtkCamera* Camera;
  svtkProp3D* Prop3D;
  svtkTransform* Transform;
  double Vector[3];
  double Origin[3];

  int EnableFeatureAngle;
  double FeatureAngle;

  svtkTypeBool BorderEdges;
  svtkTypeBool PieceInvariant;

  svtkPolyDataEdges* PreComp; // precomputed data for a given point of view

private:
  svtkPolyDataSilhouette(const svtkPolyDataSilhouette&) = delete;
  void operator=(const svtkPolyDataSilhouette&) = delete;
};

#endif
