/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextureMapToPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextureMapToPlane
 * @brief   generate texture coordinates by mapping points to plane
 *
 * svtkTextureMapToPlane is a filter that generates 2D texture coordinates
 * by mapping input dataset points onto a plane. The plane can either be
 * user specified or generated automatically. (A least squares method is
 * used to generate the plane automatically.)
 *
 * There are two ways you can specify the plane. The first is to provide a
 * plane normal. In this case the points are projected to a plane, and the
 * points are then mapped into the user specified s-t coordinate range. For
 * more control, you can specify a plane with three points: an origin and two
 * points defining the two axes of the plane. (This is compatible with the
 * svtkPlaneSource.) Using the second method, the SRange and TRange vectors
 * are ignored, since the presumption is that the user does not want to scale
 * the texture coordinates; and you can adjust the origin and axes points to
 * achieve the texture coordinate scaling you need. Note also that using the
 * three point method the axes do not have to be orthogonal.
 *
 * @sa
 *  svtkPlaneSource svtkTextureMapToCylinder
 * svtkTextureMapToSphere svtkThresholdTextureCoords
 */

#ifndef svtkTextureMapToPlane_h
#define svtkTextureMapToPlane_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersTextureModule.h" // For export macro

class SVTKFILTERSTEXTURE_EXPORT svtkTextureMapToPlane : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkTextureMapToPlane, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with s,t range=(0,1) and automatic plane generation turned on.
   */
  static svtkTextureMapToPlane* New();

  //@{
  /**
   * Specify a point defining the origin of the plane. Used in conjunction with
   * the Point1 and Point2 ivars to specify a map plane.
   */
  svtkSetVector3Macro(Origin, double);
  svtkGetVectorMacro(Origin, double, 3);
  //@}

  //@{
  /**
   * Specify a point defining the first axis of the plane.
   */
  svtkSetVector3Macro(Point1, double);
  svtkGetVectorMacro(Point1, double, 3);
  //@}

  //@{
  /**
   * Specify a point defining the second axis of the plane.
   */
  svtkSetVector3Macro(Point2, double);
  svtkGetVectorMacro(Point2, double, 3);
  //@}

  //@{
  /**
   * Specify plane normal. An alternative way to specify a map plane. Using
   * this method, the object will scale the resulting texture coordinate
   * between the SRange and TRange specified.
   */
  svtkSetVector3Macro(Normal, double);
  svtkGetVectorMacro(Normal, double, 3);
  //@}

  //@{
  /**
   * Specify s-coordinate range for texture s-t coordinate pair.
   */
  svtkSetVector2Macro(SRange, double);
  svtkGetVectorMacro(SRange, double, 2);
  //@}

  //@{
  /**
   * Specify t-coordinate range for texture s-t coordinate pair.
   */
  svtkSetVector2Macro(TRange, double);
  svtkGetVectorMacro(TRange, double, 2);
  //@}

  //@{
  /**
   * Turn on/off automatic plane generation.
   */
  svtkSetMacro(AutomaticPlaneGeneration, svtkTypeBool);
  svtkGetMacro(AutomaticPlaneGeneration, svtkTypeBool);
  svtkBooleanMacro(AutomaticPlaneGeneration, svtkTypeBool);
  //@}

protected:
  svtkTextureMapToPlane();
  ~svtkTextureMapToPlane() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ComputeNormal(svtkDataSet* output);

  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Normal[3];
  double SRange[2];
  double TRange[2];
  svtkTypeBool AutomaticPlaneGeneration;

private:
  svtkTextureMapToPlane(const svtkTextureMapToPlane&) = delete;
  void operator=(const svtkTextureMapToPlane&) = delete;
};

#endif
