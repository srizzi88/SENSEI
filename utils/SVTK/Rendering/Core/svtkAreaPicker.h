/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAreaPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAreaPicker
 * @brief   Picks props behind a selection rectangle on a viewport.
 *
 *
 * The svtkAreaPicker picks all svtkProp3Ds that lie behind the screen space
 * rectangle from x0,y0 and x1,y1. The selection is based upon the bounding
 * box of the prop and is thus not exact.
 *
 * Like svtkPicker, a pick results in a list of Prop3Ds because many props may
 * lie within the pick frustum. You can also get an AssemblyPath, which in this
 * case is defined to be the path to the one particular prop in the Prop3D list
 * that lies nearest to the near plane.
 *
 * This picker also returns the selection frustum, defined as either a
 * svtkPlanes, or a set of eight corner vertices in world space. The svtkPlanes
 * version is an ImplicitFunction, which is suitable for use with the
 * svtkExtractGeometry. The six frustum planes are in order: left, right,
 * bottom, top, near, far
 *
 * Because this picker picks everything within a volume, the world pick point
 * result is ill-defined. Therefore if you ask this class for the world pick
 * position, you will get the centroid of the pick frustum. This may be outside
 * of all props in the prop list.
 *
 * @sa
 * svtkInteractorStyleRubberBandPick, svtkExtractSelectedFrustum.
 */

#ifndef svtkAreaPicker_h
#define svtkAreaPicker_h

#include "svtkAbstractPropPicker.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;
class svtkPoints;
class svtkPlanes;
class svtkProp3DCollection;
class svtkAbstractMapper3D;
class svtkDataSet;
class svtkExtractSelectedFrustum;
class svtkProp;

class SVTKRENDERINGCORE_EXPORT svtkAreaPicker : public svtkAbstractPropPicker
{
public:
  static svtkAreaPicker* New();
  svtkTypeMacro(svtkAreaPicker, svtkAbstractPropPicker);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the default screen rectangle to pick in.
   */
  void SetPickCoords(double x0, double y0, double x1, double y1);

  /**
   * Set the default renderer to pick on.
   */
  void SetRenderer(svtkRenderer*);

  /**
   * Perform an AreaPick within the default screen rectangle and renderer.
   */
  virtual int Pick();

  /**
   * Perform pick operation in volume behind the given screen coordinates.
   * Props intersecting the selection frustum will be accessible via GetProp3D.
   * GetPlanes returns a svtkImplicitFunction suitable for svtkExtractGeometry.
   */
  virtual int AreaPick(double x0, double y0, double x1, double y1, svtkRenderer* renderer = nullptr);

  /**
   * Perform pick operation in volume behind the given screen coordinate.
   * This makes a thin frustum around the selected pixel.
   * Note: this ignores Z in order to pick everying in a volume from z=0 to z=1.
   */
  int Pick(double x0, double y0, double svtkNotUsed(z0), svtkRenderer* renderer = nullptr) override
  {
    return this->AreaPick(x0, y0, x0 + 1.0, y0 + 1.0, renderer);
  }

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

  /**
   * Return a collection of all the prop 3D's that were intersected
   * by the pick ray. This collection is not sorted.
   */
  svtkProp3DCollection* GetProp3Ds() { return this->Prop3Ds; }

  //@{
  /**
   * Return the six planes that define the selection frustum. The implicit
   * function defined by the planes evaluates to negative inside and positive
   * outside.
   */
  svtkGetObjectMacro(Frustum, svtkPlanes);
  //@}

  //@{
  /**
   * Return eight points that define the selection frustum.
   */
  svtkGetObjectMacro(ClipPoints, svtkPoints);
  //@}

protected:
  svtkAreaPicker();
  ~svtkAreaPicker() override;

  void Initialize() override;
  void DefineFrustum(double x0, double y0, double x1, double y1, svtkRenderer*);
  virtual int PickProps(svtkRenderer* renderer);
  int TypeDecipher(svtkProp*, svtkAbstractMapper3D**);

  int ABoxFrustumIsect(double bounds[], double& mindist);

  svtkPoints* ClipPoints;
  svtkPlanes* Frustum;

  svtkProp3DCollection* Prop3Ds; // candidate actors (based on bounding box)
  svtkAbstractMapper3D* Mapper;  // selected mapper (if the prop has a mapper)
  svtkDataSet* DataSet;          // selected dataset (if there is one)

  // used internally to do prop intersection tests
  svtkExtractSelectedFrustum* FrustumExtractor;

  double X0;
  double Y0;
  double X1;
  double Y1;

private:
  svtkAreaPicker(const svtkAreaPicker&) = delete;
  void operator=(const svtkAreaPicker&) = delete;
};

#endif
