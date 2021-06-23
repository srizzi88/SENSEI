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
 * @class   svtkOpenVRDefaultOverlay
 * @brief   OpenVR overlay
 *
 * svtkOpenVRDefaultOverlay support for VR overlays
 */

#ifndef svtkOpenVRDefaultOverlay_h
#define svtkOpenVRDefaultOverlay_h

#include "svtkOpenVROverlay.h"
#include "svtkRenderingOpenVRModule.h" // For export macro

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRDefaultOverlay : public svtkOpenVROverlay
{
public:
  static svtkOpenVRDefaultOverlay* New();
  svtkTypeMacro(svtkOpenVRDefaultOverlay, svtkOpenVROverlay);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Render the overlay, we set some opf the spots based on current settings
   */
  void Render() override;

protected:
  svtkOpenVRDefaultOverlay();
  ~svtkOpenVRDefaultOverlay() override;

  void SetupSpots() override;

private:
  svtkOpenVRDefaultOverlay(const svtkOpenVRDefaultOverlay&) = delete;
  void operator=(const svtkOpenVRDefaultOverlay&) = delete;
};

#endif
