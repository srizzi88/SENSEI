/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBooleanTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBooleanTexture
 * @brief   generate 2D texture map based on combinations of inside, outside, and on region boundary
 *
 *
 * svtkBooleanTexture is a filter to generate a 2D texture map based on
 * combinations of inside, outside, and on region boundary. The "region" is
 * implicitly represented via 2D texture coordinates. These texture
 * coordinates are normally generated using a filter like
 * svtkImplicitTextureCoords, which generates the texture coordinates for
 * any implicit function.
 *
 * svtkBooleanTexture generates the map according to the s-t texture
 * coordinates plus the notion of being in, on, or outside of a
 * region. An in region is when the texture coordinate is between
 * (0,0.5-thickness/2).  An out region is where the texture coordinate
 * is (0.5+thickness/2). An on region is between
 * (0.5-thickness/2,0.5+thickness/2). The combination in, on, and out
 * for each of the s-t texture coordinates results in 16 possible
 * combinations (see text). For each combination, a different value of
 * intensity and transparency can be assigned. To assign maximum intensity
 * and/or opacity use the value 255. A minimum value of 0 results in
 * a black region (for intensity) and a fully transparent region (for
 * transparency).
 *
 * @sa
 * svtkImplicitTextureCoords svtkThresholdTextureCoords
 */

#ifndef svtkBooleanTexture_h
#define svtkBooleanTexture_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingHybridModule.h" // For export macro

class SVTKIMAGINGHYBRID_EXPORT svtkBooleanTexture : public svtkImageAlgorithm
{
public:
  static svtkBooleanTexture* New();

  svtkTypeMacro(svtkBooleanTexture, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the X texture map dimension.
   */
  svtkSetMacro(XSize, int);
  svtkGetMacro(XSize, int);
  //@}

  //@{
  /**
   * Set the Y texture map dimension.
   */
  svtkSetMacro(YSize, int);
  svtkGetMacro(YSize, int);
  //@}

  //@{
  /**
   * Set the thickness of the "on" region.
   */
  svtkSetMacro(Thickness, int);
  svtkGetMacro(Thickness, int);
  //@}

  //@{
  /**
   * Specify intensity/transparency for "in/in" region.
   */
  svtkSetVector2Macro(InIn, unsigned char);
  svtkGetVectorMacro(InIn, unsigned char, 2);
  //@}

  //@{
  /**
   * Specify intensity/transparency for "in/out" region.
   */
  svtkSetVector2Macro(InOut, unsigned char);
  svtkGetVectorMacro(InOut, unsigned char, 2);
  //@}

  //@{
  /**
   * Specify intensity/transparency for "out/in" region.
   */
  svtkSetVector2Macro(OutIn, unsigned char);
  svtkGetVectorMacro(OutIn, unsigned char, 2);
  //@}

  //@{
  /**
   * Specify intensity/transparency for "out/out" region.
   */
  svtkSetVector2Macro(OutOut, unsigned char);
  svtkGetVectorMacro(OutOut, unsigned char, 2);
  //@}

  //@{
  /**
   * Specify intensity/transparency for "on/on" region.
   */
  svtkSetVector2Macro(OnOn, unsigned char);
  svtkGetVectorMacro(OnOn, unsigned char, 2);
  //@}

  //@{
  /**
   * Specify intensity/transparency for "on/in" region.
   */
  svtkSetVector2Macro(OnIn, unsigned char);
  svtkGetVectorMacro(OnIn, unsigned char, 2);
  //@}

  //@{
  /**
   * Specify intensity/transparency for "on/out" region.
   */
  svtkSetVector2Macro(OnOut, unsigned char);
  svtkGetVectorMacro(OnOut, unsigned char, 2);
  //@}

  //@{
  /**
   * Specify intensity/transparency for "in/on" region.
   */
  svtkSetVector2Macro(InOn, unsigned char);
  svtkGetVectorMacro(InOn, unsigned char, 2);
  //@}

  //@{
  /**
   * Specify intensity/transparency for "out/on" region.
   */
  svtkSetVector2Macro(OutOn, unsigned char);
  svtkGetVectorMacro(OutOn, unsigned char, 2);
  //@}

protected:
  svtkBooleanTexture();
  ~svtkBooleanTexture() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ExecuteDataWithInformation(svtkDataObject* data, svtkInformation* outInfo) override;

  int XSize;
  int YSize;

  int Thickness;
  unsigned char InIn[2];
  unsigned char InOut[2];
  unsigned char OutIn[2];
  unsigned char OutOut[2];
  unsigned char OnOn[2];
  unsigned char OnIn[2];
  unsigned char OnOut[2];
  unsigned char InOn[2];
  unsigned char OutOn[2];

private:
  svtkBooleanTexture(const svtkBooleanTexture&) = delete;
  void operator=(const svtkBooleanTexture&) = delete;
};

#endif
