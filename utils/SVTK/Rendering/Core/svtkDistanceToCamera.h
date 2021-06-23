/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistanceToCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkDistanceToCamera
 * @brief   calculates distance from points to the camera.
 *
 *
 * This filter adds a double array containing the distance from each point
 * to the camera. If Scaling is on, it will use the values in the input
 * array to process in order to scale the size of the points. ScreenSize
 * sets the size in screen pixels that you would want a rendered rectangle
 * at that point to be, if it was scaled by the output array.
 */

#ifndef svtkDistanceToCamera_h
#define svtkDistanceToCamera_h

#include "svtkPointSetAlgorithm.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;

class SVTKRENDERINGCORE_EXPORT svtkDistanceToCamera : public svtkPointSetAlgorithm
{
public:
  static svtkDistanceToCamera* New();
  svtkTypeMacro(svtkDistanceToCamera, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The renderer which will ultimately render these points.
   */
  void SetRenderer(svtkRenderer* ren);
  svtkGetObjectMacro(Renderer, svtkRenderer);
  //@}

  //@{
  /**
   * The desired screen size obtained by scaling glyphs by the distance
   * array. It assumes the glyph at each point will be unit size.
   */
  svtkSetMacro(ScreenSize, double);
  svtkGetMacro(ScreenSize, double);
  //@}

  //@{
  /**
   * Whether to scale the distance by the input array to process.
   */
  svtkSetMacro(Scaling, bool);
  svtkGetMacro(Scaling, bool);
  svtkBooleanMacro(Scaling, bool);
  //@}

  //@{
  /**
   * The name of the distance array. If not set, the array is
   * named 'DistanceToCamera'.
   */
  svtkSetStringMacro(DistanceArrayName);
  svtkGetStringMacro(DistanceArrayName);
  //@}

  /**
   * The modified time of this filter.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkDistanceToCamera();
  ~svtkDistanceToCamera() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkRenderer* Renderer;
  double ScreenSize;
  bool Scaling;
  int LastRendererSize[2];
  double LastCameraPosition[3];
  double LastCameraFocalPoint[3];
  double LastCameraViewUp[3];
  double LastCameraParallelScale;
  char* DistanceArrayName;

private:
  svtkDistanceToCamera(const svtkDistanceToCamera&) = delete;
  void operator=(const svtkDistanceToCamera&) = delete;
};

#endif
