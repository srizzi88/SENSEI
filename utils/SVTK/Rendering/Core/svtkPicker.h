/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPicker
 * @brief   superclass for 3D geometric pickers (uses ray cast)
 *
 * svtkPicker is used to select instances of svtkProp3D by shooting a ray
 * into a graphics window and intersecting with the actor's bounding box.
 * The ray is defined from a point defined in window (or pixel) coordinates,
 * and a point located from the camera's position.
 *
 * svtkPicker may return more than one svtkProp3D, since more than one bounding
 * box may be intersected. svtkPicker returns an unsorted list of props that
 * were hit, and a list of the corresponding world points of the hits.
 * For the svtkProp3D that is closest to the camera, svtkPicker returns the
 * pick coordinates in world and untransformed mapper space, the prop itself,
 * the data set, and the mapper.  For svtkPicker the closest prop is the one
 * whose center point (i.e., center of bounding box) projected on the view
 * ray is closest to the camera.  Subclasses of svtkPicker use other methods
 * for computing the pick point.
 *
 * @sa
 * svtkPicker is used for quick geometric picking. If you desire more precise
 * picking of points or cells based on the geometry of any svtkProp3D, use the
 * subclasses svtkPointPicker or svtkCellPicker.  For hardware-accelerated
 * picking of any type of svtkProp, use svtkPropPicker or svtkWorldPointPicker.
 */

#ifndef svtkPicker_h
#define svtkPicker_h

#include "svtkAbstractPropPicker.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkAbstractMapper3D;
class svtkCompositeDataSet;
class svtkDataSet;
class svtkTransform;
class svtkActorCollection;
class svtkProp3DCollection;
class svtkPoints;

class SVTKRENDERINGCORE_EXPORT svtkPicker : public svtkAbstractPropPicker
{
public:
  static svtkPicker* New();
  svtkTypeMacro(svtkPicker, svtkAbstractPropPicker);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify tolerance for performing pick operation. Tolerance is specified
   * as fraction of rendering window size. (Rendering window size is measured
   * across diagonal.)
   */
  svtkSetMacro(Tolerance, double);
  svtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Return position in mapper (i.e., non-transformed) coordinates of
   * pick point.
   */
  svtkGetVectorMacro(MapperPosition, double, 3);
  //@}

  //@{
  /**
   * Return mapper that was picked (if any).
   */
  svtkGetObjectMacro(Mapper, svtkAbstractMapper3D);
  //@}

  //@{
  /**
   * Get a pointer to the dataset that was picked (if any). If nothing
   * was picked then NULL is returned.
   */
  svtkGetObjectMacro(DataSet, svtkDataSet);
  //@}

  //@{
  /**
   * Get a pointer to the composite dataset that was picked (if any). If nothing
   * was picked or a non-composite data object was picked then NULL is returned.
   */
  svtkGetObjectMacro(CompositeDataSet, svtkCompositeDataSet);
  //@}

  //@{
  /**
   * Get the flat block index of the svtkDataSet in the composite dataset
   * that was picked (if any). If nothing
   * was picked or a non-composite data object was picked then -1 is returned.
   */
  svtkGetMacro(FlatBlockIndex, svtkIdType);
  //@}

  /**
   * Return a collection of all the prop 3D's that were intersected
   * by the pick ray. This collection is not sorted.
   */
  svtkProp3DCollection* GetProp3Ds() { return this->Prop3Ds; }

  /**
   * Return a collection of all the actors that were intersected.
   * This collection is not sorted. (This is a convenience method
   * to maintain backward compatibility.)
   */
  svtkActorCollection* GetActors();

  /**
   * Return a list of the points the actors returned by GetProp3Ds
   * were intersected at. The order of this list will match the order of
   * GetProp3Ds.
   */
  svtkPoints* GetPickedPositions() { return this->PickedPositions; }

  /**
   * Perform pick operation with selection point provided. Normally the
   * first two values for the selection point are x-y pixel coordinate, and
   * the third value is =0. Return non-zero if something was successfully
   * picked.
   */
  int Pick(double selectionX, double selectionY, double selectionZ, svtkRenderer* renderer) override;

  /**
   * Perform pick operation with selection point provided. Normally the first
   * two values for the selection point are x-y pixel coordinate, and the
   * third value is =0. Return non-zero if something was successfully picked.
   */
  int Pick(double selectionPt[3], svtkRenderer* ren)
  {
    return this->Pick(selectionPt[0], selectionPt[1], selectionPt[2], ren);
  }

  /**
   * Perform pick operation with selection point provided. The
   * selectionPt is in world coordinates.
   * Return non-zero if something was successfully picked.
   */
  int Pick3DPoint(double selectionPt[3], svtkRenderer* ren) override;

  /*
   * Pick a point in the scene with the selection point and focal point
   * provided. The two points are in world coordinates.
   *
   * Returns non-zero if something was successfully picked.
   */
  virtual int Pick3DPoint(double p1World[3], double p2World[3], svtkRenderer* ren);
  /**
   * Perform pick operation with selection point and orientation provided.
   * The selectionPt is in world coordinates.
   * Return non-zero if something was successfully picked.
   */
  int Pick3DRay(double selectionPt[3], double orient[4], svtkRenderer* ren) override;

protected:
  svtkPicker();
  ~svtkPicker() override;

  // shared code for picking
  virtual int Pick3DInternal(svtkRenderer* ren, double p1World[4], double p2World[4]);

  void MarkPicked(
    svtkAssemblyPath* path, svtkProp3D* p, svtkAbstractMapper3D* m, double tMin, double mapperPos[3]);
  void MarkPickedData(svtkAssemblyPath* path, double tMin, double mapperPos[3],
    svtkAbstractMapper3D* mapper, svtkDataSet* input, svtkIdType flatBlockIndex = -1);
  virtual double IntersectWithLine(const double p1[3], const double p2[3], double tol,
    svtkAssemblyPath* path, svtkProp3D* p, svtkAbstractMapper3D* m);
  void Initialize() override;
  static bool CalculateRay(
    const double p1[3], const double p2[3], double ray[3], double& rayFactor);

  double Tolerance;         // tolerance for computation (% of window)
  double MapperPosition[3]; // selection point in untransformed coordinates

  svtkAbstractMapper3D* Mapper; // selected mapper (if the prop has a mapper)
  svtkDataSet* DataSet;         // selected dataset (if there is one)
  svtkCompositeDataSet* CompositeDataSet;
  svtkIdType FlatBlockIndex; // flat block index, for a composite data set

  double GlobalTMin;            // parametric coordinate along pick ray where hit occurred
  svtkTransform* Transform;      // use to perform ray transformation
  svtkActorCollection* Actors;   // candidate actors (based on bounding box)
  svtkProp3DCollection* Prop3Ds; // candidate actors (based on bounding box)
  svtkPoints* PickedPositions;   // candidate positions

private:
  svtkPicker(const svtkPicker&) = delete;
  void operator=(const svtkPicker&) = delete;
};

#endif
