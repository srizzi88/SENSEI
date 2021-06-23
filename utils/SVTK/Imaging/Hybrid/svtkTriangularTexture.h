/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTriangularTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTriangularTexture
 * @brief   generate 2D triangular texture map
 *
 * svtkTriangularTexture is a filter that generates a 2D texture map based on
 * the paper "Opacity-modulating Triangular Textures for Irregular Surfaces,"
 * by Penny Rheingans, IEEE Visualization '96, pp. 219-225.
 * The textures assume texture coordinates of (0,0), (1.0) and
 * (.5, sqrt(3)/2). The sequence of texture values is the same along each
 * edge of the triangular texture map. So, the assignment order of texture
 * coordinates is arbitrary.
 *
 * @sa
 * svtkTriangularTCoords
 */

#ifndef svtkTriangularTexture_h
#define svtkTriangularTexture_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingHybridModule.h" // For export macro

class SVTKIMAGINGHYBRID_EXPORT svtkTriangularTexture : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkTriangularTexture, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with XSize and YSize = 64; the texture pattern =1
   * (opaque at centroid); and the scale factor set to 1.0.
   */
  static svtkTriangularTexture* New();

  //@{
  /**
   * Set a Scale Factor.
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * Set the X texture map dimension. Default is 64.
   */
  svtkSetMacro(XSize, int);
  svtkGetMacro(XSize, int);
  //@}

  //@{
  /**
   * Set the Y texture map dimension. Default is 64.
   */
  svtkSetMacro(YSize, int);
  svtkGetMacro(YSize, int);
  //@}

  //@{
  /**
   * Set the texture pattern.
   * 1 = opaque at centroid (default)
   * 2 = opaque at vertices
   * 3 = opaque in rings around vertices
   */
  svtkSetClampMacro(TexturePattern, int, 1, 3);
  svtkGetMacro(TexturePattern, int);
  //@}

protected:
  svtkTriangularTexture();
  ~svtkTriangularTexture() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ExecuteDataWithInformation(svtkDataObject* data, svtkInformation* outInfo) override;

  int XSize;
  int YSize;
  double ScaleFactor;

  int TexturePattern;

private:
  svtkTriangularTexture(const svtkTriangularTexture&) = delete;
  void operator=(const svtkTriangularTexture&) = delete;
};

#endif
