/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitTextureCoords.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImplicitTextureCoords
 * @brief   generate 1D, 2D, or 3D texture coordinates based on implicit function(s)
 *
 * svtkImplicitTextureCoords is a filter to generate 1D, 2D, or 3D texture
 * coordinates from one, two, or three implicit functions, respectively.
 * In combinations with a svtkBooleanTexture map (or another texture map of
 * your own creation), the texture coordinates can be used to highlight
 *(via color or intensity) or cut (via transparency) dataset geometry without
 * any complex geometric processing. (Note: the texture coordinates are
 * referred to as r-s-t coordinates.)
 *
 * The texture coordinates are automatically normalized to lie between (0,1).
 * Thus, no matter what the implicit functions evaluate to, the resulting
 * texture coordinates lie between (0,1), with the zero implicit function
 * value mapped to the 0.5 texture coordinates value. Depending upon the
 * maximum negative/positive implicit function values, the full (0,1) range
 * may not be occupied (i.e., the positive/negative ranges are mapped using
 * the same scale factor).
 *
 * A boolean variable InvertTexture is available to flip the texture
 * coordinates around 0.5 (value 1.0 becomes 0.0, 0.25->0.75). This is
 * equivalent to flipping the texture map (but a whole lot easier).
 *
 * @warning
 * You can use the transformation capabilities of svtkImplicitFunction to
 * orient, translate, and scale the implicit functions. Also, the dimension of
 * the texture coordinates is implicitly defined by the number of implicit
 * functions defined.
 *
 * @sa
 * svtkImplicitFunction svtkTexture svtkBooleanTexture svtkTransformTexture
 */

#ifndef svtkImplicitTextureCoords_h
#define svtkImplicitTextureCoords_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersTextureModule.h" // For export macro

class svtkImplicitFunction;

class SVTKFILTERSTEXTURE_EXPORT svtkImplicitTextureCoords : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkImplicitTextureCoords, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create object with texture dimension=2 and no r-s-t implicit functions
   * defined and FlipTexture turned off.
   */
  static svtkImplicitTextureCoords* New();

  //@{
  /**
   * Specify an implicit function to compute the r texture coordinate.
   */
  virtual void SetRFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(RFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * Specify an implicit function to compute the s texture coordinate.
   */
  virtual void SetSFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(SFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * Specify an implicit function to compute the t texture coordinate.
   */
  virtual void SetTFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(TFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * If enabled, this will flip the sense of inside and outside the implicit
   * function (i.e., a rotation around the r-s-t=0.5 axis).
   */
  svtkSetMacro(FlipTexture, svtkTypeBool);
  svtkGetMacro(FlipTexture, svtkTypeBool);
  svtkBooleanMacro(FlipTexture, svtkTypeBool);
  //@}

protected:
  svtkImplicitTextureCoords();
  ~svtkImplicitTextureCoords() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkImplicitFunction* RFunction;
  svtkImplicitFunction* SFunction;
  svtkImplicitFunction* TFunction;
  svtkTypeBool FlipTexture;

private:
  svtkImplicitTextureCoords(const svtkImplicitTextureCoords&) = delete;
  void operator=(const svtkImplicitTextureCoords&) = delete;
};

#endif
