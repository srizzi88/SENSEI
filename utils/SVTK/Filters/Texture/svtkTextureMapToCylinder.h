/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextureMapToCylinder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextureMapToCylinder
 * @brief   generate texture coordinates by mapping points to cylinder
 *
 * svtkTextureMapToCylinder is a filter that generates 2D texture coordinates
 * by mapping input dataset points onto a cylinder. The cylinder can either be
 * user specified or generated automatically. (The cylinder is generated
 * automatically by computing the axis of the cylinder.)  Note that the
 * generated texture coordinates for the s-coordinate ranges from (0-1)
 * (corresponding to angle of 0->360 around axis), while the mapping of
 * the t-coordinate is controlled by the projection of points along the axis.
 *
 * To specify a cylinder manually, you must provide two points that
 * define the axis of the cylinder. The length of the axis will affect the
 * t-coordinates.
 *
 * A special ivar controls how the s-coordinate is generated. If PreventSeam
 * is set to true, the s-texture varies from 0->1 and then 1->0 (corresponding
 * to angles of 0->180 and 180->360).
 *
 * @warning
 * Since the resulting texture s-coordinate will lie between (0,1), and the
 * origin of the texture coordinates is not user-controllable, you may want
 * to use the class svtkTransformTexture to linearly scale and shift the origin
 * of the texture coordinates.
 *
 * @sa
 * svtkTextureMapToPlane svtkTextureMapToSphere
 * svtkTransformTexture svtkThresholdTextureCoords
 */

#ifndef svtkTextureMapToCylinder_h
#define svtkTextureMapToCylinder_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersTextureModule.h" // For export macro

class SVTKFILTERSTEXTURE_EXPORT svtkTextureMapToCylinder : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkTextureMapToCylinder, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create object with cylinder axis parallel to z-axis (points (0,0,-0.5)
   * and (0,0,0.5)). The PreventSeam ivar is set to true. The cylinder is
   * automatically generated.
   */
  static svtkTextureMapToCylinder* New();

  //@{
  /**
   * Specify the first point defining the cylinder axis,
   */
  svtkSetVector3Macro(Point1, double);
  svtkGetVectorMacro(Point1, double, 3);
  //@}

  //@{
  /**
   * Specify the second point defining the cylinder axis,
   */
  svtkSetVector3Macro(Point2, double);
  svtkGetVectorMacro(Point2, double, 3);
  //@}

  //@{
  /**
   * Turn on/off automatic cylinder generation. This means it automatically
   * finds the cylinder center and axis.
   */
  svtkSetMacro(AutomaticCylinderGeneration, svtkTypeBool);
  svtkGetMacro(AutomaticCylinderGeneration, svtkTypeBool);
  svtkBooleanMacro(AutomaticCylinderGeneration, svtkTypeBool);
  //@}

  //@{
  /**
   * Control how the texture coordinates are generated. If PreventSeam is
   * set, the s-coordinate ranges from 0->1 and 1->0 corresponding to the
   * angle variation from 0->180 and 180->0. Otherwise, the s-coordinate
   * ranges from 0->1 from 0->360 degrees.
   */
  svtkSetMacro(PreventSeam, svtkTypeBool);
  svtkGetMacro(PreventSeam, svtkTypeBool);
  svtkBooleanMacro(PreventSeam, svtkTypeBool);
  //@}

protected:
  svtkTextureMapToCylinder();
  ~svtkTextureMapToCylinder() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Point1[3];
  double Point2[3];
  svtkTypeBool AutomaticCylinderGeneration;
  svtkTypeBool PreventSeam;

private:
  svtkTextureMapToCylinder(const svtkTextureMapToCylinder&) = delete;
  void operator=(const svtkTextureMapToCylinder&) = delete;
};

#endif
