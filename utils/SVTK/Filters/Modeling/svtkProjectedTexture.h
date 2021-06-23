/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProjectedTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProjectedTexture
 * @brief   assign texture coordinates for a projected texture
 *
 * svtkProjectedTexture assigns texture coordinates to a dataset as if
 * the texture was projected from a slide projected located somewhere in the
 * scene.  Methods are provided to position the projector and aim it at a
 * location, to set the width of the projector's frustum, and to set the
 * range of texture coordinates assigned to the dataset.
 *
 * Objects in the scene that appear behind the projector are also assigned
 * texture coordinates; the projected image is left-right and top-bottom
 * flipped, much as a lens' focus flips the rays of light that pass through
 * it.  A warning is issued if a point in the dataset falls at the focus
 * of the projector.
 */

#ifndef svtkProjectedTexture_h
#define svtkProjectedTexture_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersModelingModule.h" // For export macro

#define SVTK_PROJECTED_TEXTURE_USE_PINHOLE 0
#define SVTK_PROJECTED_TEXTURE_USE_TWO_MIRRORS 1

class SVTKFILTERSMODELING_EXPORT svtkProjectedTexture : public svtkDataSetAlgorithm
{
public:
  static svtkProjectedTexture* New();
  svtkTypeMacro(svtkProjectedTexture, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the position of the focus of the projector.
   */
  svtkSetVector3Macro(Position, double);
  svtkGetVectorMacro(Position, double, 3);
  //@}

  //@{
  /**
   * Set/Get the focal point of the projector (a point that lies along
   * the center axis of the projector's frustum).
   */
  void SetFocalPoint(double focalPoint[3]);
  void SetFocalPoint(double x, double y, double z);
  svtkGetVectorMacro(FocalPoint, double, 3);
  //@}

  //@{
  /**
   * Set/Get the camera mode of the projection -- pinhole projection or
   * two mirror projection.
   */
  svtkSetMacro(CameraMode, int);
  svtkGetMacro(CameraMode, int);
  void SetCameraModeToPinhole() { this->SetCameraMode(SVTK_PROJECTED_TEXTURE_USE_PINHOLE); }
  void SetCameraModeToTwoMirror() { this->SetCameraMode(SVTK_PROJECTED_TEXTURE_USE_TWO_MIRRORS); }
  //@}

  //@{
  /**
   * Set/Get the mirror separation for the two mirror system.
   */
  svtkSetMacro(MirrorSeparation, double);
  svtkGetMacro(MirrorSeparation, double);
  //@}

  //@{
  /**
   * Get the normalized orientation vector of the projector.
   */
  svtkGetVectorMacro(Orientation, double, 3);
  //@}

  //@{
  /**
   * Set/Get the up vector of the projector.
   */
  svtkSetVector3Macro(Up, double);
  svtkGetVectorMacro(Up, double, 3);
  //@}

  //@{
  /**
   * Set/Get the aspect ratio of a perpendicular cross-section of the
   * the projector's frustum.  The aspect ratio consists of three
   * numbers:  (x, y, z), where x is the width of the
   * frustum, y is the height, and z is the perpendicular
   * distance from the focus of the projector.

   * For example, if the source of the image is a pinhole camera with
   * view angle A, then you could set x=1, y=1, z=1/tan(A).
   */
  svtkSetVector3Macro(AspectRatio, double);
  svtkGetVectorMacro(AspectRatio, double, 3);
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

protected:
  svtkProjectedTexture();
  ~svtkProjectedTexture() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ComputeNormal();

  int CameraMode;

  double Position[3];
  double Orientation[3];
  double FocalPoint[3];
  double Up[3];
  double MirrorSeparation;
  double AspectRatio[3];
  double SRange[2];
  double TRange[2];

private:
  svtkProjectedTexture(const svtkProjectedTexture&) = delete;
  void operator=(const svtkProjectedTexture&) = delete;
};

#endif
