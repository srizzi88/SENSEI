/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformTextureCoords.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTransformTextureCoords
 * @brief   transform (scale, rotate, translate) texture coordinates
 *
 * svtkTransformTextureCoords is a filter that operates on texture
 * coordinates. It ingests any type of dataset, and outputs a dataset of the
 * same type. The filter lets you scale, translate, and rotate texture
 * coordinates. For example, by using the Scale ivar, you can shift
 * texture coordinates that range from (0->1) to range from (0->10) (useful
 * for repeated patterns).
 *
 * The filter operates on texture coordinates of dimension 1->3. The texture
 * coordinates are referred to as r-s-t. If the texture map is two dimensional,
 * the t-coordinate (and operations on the t-coordinate) are ignored.
 *
 * @sa
 * svtkTextureMapToPlane  svtkTextureMapToCylinder
 * svtkTextureMapToSphere svtkThresholdTextureCoords svtkTexture
 */

#ifndef svtkTransformTextureCoords_h
#define svtkTransformTextureCoords_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersTextureModule.h" // For export macro

class SVTKFILTERSTEXTURE_EXPORT svtkTransformTextureCoords : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkTransformTextureCoords, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create instance with Origin (0.5,0.5,0.5); Position (0,0,0); and Scale
   * set to (1,1,1). Rotation of the texture coordinates is turned off.
   */
  static svtkTransformTextureCoords* New();

  //@{
  /**
   * Set/Get the position of the texture map. Setting the position translates
   * the texture map by the amount specified.
   */
  svtkSetVector3Macro(Position, double);
  svtkGetVectorMacro(Position, double, 3);
  //@}

  //@{
  /**
   * Incrementally change the position of the texture map (i.e., does a
   * translate or shift of the texture coordinates).
   */
  void AddPosition(double deltaR, double deltaS, double deltaT);
  void AddPosition(double deltaPosition[3]);
  //@}

  //@{
  /**
   * Set/Get the scale of the texture map. Scaling in performed independently
   * on the r, s and t axes.
   */
  svtkSetVector3Macro(Scale, double);
  svtkGetVectorMacro(Scale, double, 3);
  //@}

  //@{
  /**
   * Set/Get the origin of the texture map. This is the point about which the
   * texture map is flipped (e.g., rotated). Since a typical texture map ranges
   * from (0,1) in the r-s-t coordinates, the default origin is set at
   * (0.5,0.5,0.5).
   */
  svtkSetVector3Macro(Origin, double);
  svtkGetVectorMacro(Origin, double, 3);
  //@}

  //@{
  /**
   * Boolean indicates whether the texture map should be flipped around the
   * s-axis. Note that the flips occur around the texture origin.
   */
  svtkSetMacro(FlipR, svtkTypeBool);
  svtkGetMacro(FlipR, svtkTypeBool);
  svtkBooleanMacro(FlipR, svtkTypeBool);
  //@}

  //@{
  /**
   * Boolean indicates whether the texture map should be flipped around the
   * s-axis. Note that the flips occur around the texture origin.
   */
  svtkSetMacro(FlipS, svtkTypeBool);
  svtkGetMacro(FlipS, svtkTypeBool);
  svtkBooleanMacro(FlipS, svtkTypeBool);
  //@}

  //@{
  /**
   * Boolean indicates whether the texture map should be flipped around the
   * t-axis. Note that the flips occur around the texture origin.
   */
  svtkSetMacro(FlipT, svtkTypeBool);
  svtkGetMacro(FlipT, svtkTypeBool);
  svtkBooleanMacro(FlipT, svtkTypeBool);
  //@}

protected:
  svtkTransformTextureCoords();
  ~svtkTransformTextureCoords() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Origin[3];   // point around which map rotates
  double Position[3]; // controls translation of map
  double Scale[3];    // scales the texture map
  svtkTypeBool FlipR;  // boolean indicates whether to flip texture around r-axis
  svtkTypeBool FlipS;  // boolean indicates whether to flip texture around s-axis
  svtkTypeBool FlipT;  // boolean indicates whether to flip texture around t-axis
private:
  svtkTransformTextureCoords(const svtkTransformTextureCoords&) = delete;
  void operator=(const svtkTransformTextureCoords&) = delete;
};

#endif
