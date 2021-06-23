/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlockSortHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @brief Collection of comparison functions for std::sort.
 *
 */

#ifndef svtkBlockSortHelper_h
#define svtkBlockSortHelper_h

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkMatrix4x4.h"
#include "svtkNew.h"
#include "svtkRenderer.h"
#include "svtkVolumeMapper.h"

namespace svtkBlockSortHelper
{

/**
 *  operator() for back-to-front sorting.
 *
 *  \note Use as the 'comp' parameter of std::sort.
 *
 */
template <typename T>
struct BackToFront
{
  double CameraPosition[4];

  //----------------------------------------------------------------------------
  BackToFront(svtkRenderer* ren, svtkMatrix4x4* volMatrix)
  {
    svtkCamera* cam = ren->GetActiveCamera();
    double camWorldPos[4];

    cam->GetPosition(camWorldPos);
    camWorldPos[3] = 1.0;

    // Transform the camera position to the volume (dataset) coordinate system.
    svtkNew<svtkMatrix4x4> InverseVolumeMatrix;
    InverseVolumeMatrix->DeepCopy(volMatrix);
    InverseVolumeMatrix->Invert();
    InverseVolumeMatrix->MultiplyPoint(camWorldPos, this->CameraPosition);
  };

  //----------------------------------------------------------------------------
  bool operator()(T* first, T* second);

  /**
   * Compares distances from images (first, second) to the camera position.
   * Returns true if the distance of first is greater than the distance of
   * second (descending order according to the std::sort convention).
   *
   * Note this does not provide the correct rendering order all the time.
   * To get the correct rendering order (if there is one) you need a more
   * complex algorithm.
   */
  //----------------------------------------------------------------------------
  inline bool CompareByDistanceDescending(svtkImageData* first, svtkImageData* second)
  {
    double center[3];
    double bounds[6];

    first->GetBounds(bounds);
    this->ComputeCenter(bounds, center);
    double const dist1 = svtkMath::Distance2BetweenPoints(center, this->CameraPosition);

    second->GetBounds(bounds);
    this->ComputeCenter(bounds, center);
    double const dist2 = svtkMath::Distance2BetweenPoints(center, this->CameraPosition);

    return dist2 < dist1;
  };

  //----------------------------------------------------------------------------
  inline void ComputeCenter(double const* bounds, double* center)
  {
    center[0] = bounds[0] + std::abs(bounds[1] - bounds[0]) / 2.0;
    center[1] = bounds[2] + std::abs(bounds[3] - bounds[2]) / 2.0;
    center[2] = bounds[4] + std::abs(bounds[5] - bounds[4]) / 2.0;
  };
};

//----------------------------------------------------------------------------
template <>
inline bool BackToFront<svtkImageData>::operator()(svtkImageData* first, svtkImageData* second)
{
  return CompareByDistanceDescending(first, second);
};

//----------------------------------------------------------------------------
template <>
inline bool BackToFront<svtkVolumeMapper>::operator()(
  svtkVolumeMapper* first, svtkVolumeMapper* second)
{
  svtkImageData* firstIm = first->GetInput();
  svtkImageData* secondIm = second->GetInput();

  return CompareByDistanceDescending(firstIm, secondIm);
};
}

#endif // svtkBlockSortHelper_h
// SVTK-HeaderTest-Exclude: svtkBlockSortHelper.h
