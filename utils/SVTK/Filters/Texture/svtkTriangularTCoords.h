/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTriangularTCoords.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTriangularTCoords
 * @brief   2D texture coordinates based for triangles.
 *
 * svtkTriangularTCoords is a filter that generates texture coordinates
 * for triangles. Texture coordinates for each triangle are:
 * (0,0), (1,0) and (.5,sqrt(3)/2). This filter assumes that the triangle
 * texture map is symmetric about the center of the triangle. Thus the order
 * Of the texture coordinates is not important. The procedural texture
 * in svtkTriangularTexture is designed with this symmetry. For more information
 * see the paper "Opacity-modulating Triangular Textures for Irregular
 * Surfaces,"  by Penny Rheingans, IEEE Visualization '96, pp. 219-225.
 * @sa
 * svtkTriangularTexture svtkThresholdPoints svtkTextureMapToPlane
 * svtkTextureMapToSphere svtkTextureMapToCylinder
 */

#ifndef svtkTriangularTCoords_h
#define svtkTriangularTCoords_h

#include "svtkFiltersTextureModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSTEXTURE_EXPORT svtkTriangularTCoords : public svtkPolyDataAlgorithm
{
public:
  static svtkTriangularTCoords* New();
  svtkTypeMacro(svtkTriangularTCoords, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkTriangularTCoords() {}
  ~svtkTriangularTCoords() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkTriangularTCoords(const svtkTriangularTCoords&) = delete;
  void operator=(const svtkTriangularTCoords&) = delete;
};

#endif
