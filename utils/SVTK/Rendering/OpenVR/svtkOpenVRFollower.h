/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenVRFollower
 * @brief   OpenVR Follower
 *
 * svtkOpenVRFollower a follower that aligns with PhysicalViewUp
 */

#ifndef svtkOpenVRFollower_h
#define svtkOpenVRFollower_h

#include "svtkFollower.h"
#include "svtkRenderingOpenVRModule.h" // For export macro

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRFollower : public svtkFollower
{
public:
  static svtkOpenVRFollower* New();
  svtkTypeMacro(svtkOpenVRFollower, svtkFollower);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  virtual void Render(svtkRenderer* ren) override;

  /**
   * Generate the matrix based on ivars. This method overloads its superclasses
   * ComputeMatrix() method due to the special svtkFollower matrix operations.
   */
  void ComputeMatrix() override;

protected:
  svtkOpenVRFollower();
  ~svtkOpenVRFollower() override;

  double LastViewUp[3];

private:
  svtkOpenVRFollower(const svtkOpenVRFollower&) = delete;
  void operator=(const svtkOpenVRFollower&) = delete;
};

#endif
