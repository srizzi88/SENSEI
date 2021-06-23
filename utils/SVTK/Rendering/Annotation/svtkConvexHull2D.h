/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkConvexHull2D.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/**
 * @class   svtkConvexHull2D
 * @brief   Produce filled convex hulls around a set of points.
 *
 *
 * Produces a svtkPolyData comprised of a filled polygon of the convex hull
 * of the input points. You may alternatively choose to output a bounding
 * rectangle. Static methods are provided that calculate a (counter-clockwise)
 * hull based on a set of input points.
 *
 * To help maintain the property of <i>guaranteed visibility</i> hulls may be
 * artificially scaled by setting MinHullSizeInWorld. This is particularly
 * helpful in the case that there are only one or two points as it avoids
 * producing a degenerate polygon. This setting is also available as an
 * argument to the static methods.
 *
 * Setting a svtkRenderer on the filter enables the possibility to set
 * MinHullSizeInDisplay to the desired number of display pixels to cover in
 * each of the x- and y-dimensions.
 *
 * Setting OutlineOn() additionally produces an outline of the hull on output
 * port 1.
 *
 * @attention
 * This filter operates in the x,y-plane and as such works best with an
 * interactor style that does not permit camera rotation such as
 * svtkInteractorStyleRubberBand2D.
 *
 * @par Thanks:
 * Thanks to Colin Myers, University of Leeds for providing this implementation.
 */

#ifndef svtkConvexHull2D_h
#define svtkConvexHull2D_h

#include "svtkPolyDataAlgorithm.h"
#include "svtkRenderingAnnotationModule.h" // For export macro
#include "svtkSmartPointer.h"              // needed for ivars

class svtkCoordinate;
class svtkPoints;
class svtkPolygon;
class svtkPolyLine;
class svtkRenderer;
class svtkTransform;
class svtkTransformPolyDataFilter;

class SVTKRENDERINGANNOTATION_EXPORT svtkConvexHull2D : public svtkPolyDataAlgorithm
{
public:
  static svtkConvexHull2D* New();
  svtkTypeMacro(svtkConvexHull2D, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Scale the hull by the amount specified. Defaults to 1.0.
   */
  svtkGetMacro(ScaleFactor, double);
  svtkSetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * Produce an outline (polyline) of the hull on output port 1.
   */
  svtkGetMacro(Outline, bool);
  svtkSetMacro(Outline, bool);
  svtkBooleanMacro(Outline, bool);
  //@}

  enum HullShapes
  {
    BoundingRectangle = 0,
    ConvexHull
  };

  //@{
  /**
   * Set the shape of the hull to BoundingRectangle or ConvexHull.
   */
  svtkGetMacro(HullShape, int);
  svtkSetClampMacro(HullShape, int, 0, 1);
  //@}

  //@{
  /**
   * Set the minimum x,y-dimensions of each hull in world coordinates. Defaults
   * to 1.0. Set to 0.0 to disable.
   */
  svtkSetClampMacro(MinHullSizeInWorld, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(MinHullSizeInWorld, double);
  //@}

  //@{
  /**
   * Set the minimum x,y-dimensions of each hull in pixels. You must also set a
   * svtkRenderer. Defaults to 1. Set to 0 to disable.
   */
  svtkSetClampMacro(MinHullSizeInDisplay, int, 0, SVTK_INT_MAX);
  svtkGetMacro(MinHullSizeInDisplay, int);
  //@}

  //@{
  /**
   * Renderer needed for MinHullSizeInDisplay calculation. Not reference counted.
   */
  void SetRenderer(svtkRenderer* renderer);
  svtkRenderer* GetRenderer();
  //@}

  /**
   * The modified time of this filter.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Convenience methods to calculate a convex hull from a set of svtkPointS.
   */
  static void CalculateBoundingRectangle(
    svtkPoints* inPoints, svtkPoints* outPoints, double minimumHullSize = 1.0);
  static void CalculateConvexHull(
    svtkPoints* inPoints, svtkPoints* outPoints, double minimumHullSize = 1.0);
  //@}

protected:
  svtkConvexHull2D();
  ~svtkConvexHull2D() override;

  /**
   * This is called by the superclass. This is the method you should override.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkConvexHull2D(const svtkConvexHull2D&) = delete;
  void operator=(const svtkConvexHull2D&) = delete;

  void ResizeHullToMinimumInDisplay(svtkPolyData* hullPolyData);

  double ScaleFactor;
  bool Outline;
  int HullShape;
  int MinHullSizeInDisplay;
  double MinHullSizeInWorld;
  svtkRenderer* Renderer;

  svtkSmartPointer<svtkCoordinate> Coordinate;
  svtkSmartPointer<svtkTransform> Transform;
  svtkSmartPointer<svtkTransform> OutputTransform;
  svtkSmartPointer<svtkTransformPolyDataFilter> OutputTransformFilter;
  svtkSmartPointer<svtkPolyLine> OutlineSource;
  svtkSmartPointer<svtkPolygon> HullSource;
};

#endif // svtkConvexHull2D_h
