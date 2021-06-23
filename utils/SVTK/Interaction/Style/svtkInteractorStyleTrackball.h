/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleTrackball.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyleTrackball
 * @brief   provides trackball motion control
 *
 *
 * svtkInteractorStyleTrackball is an implementation of svtkInteractorStyle
 * that defines the trackball style. It is now deprecated and as such a
 * subclass of svtkInteractorStyleSwitch
 *
 * @sa
 * svtkInteractorStyleSwitch svtkInteractorStyleTrackballActor svtkInteractorStyleJoystickCamera
 */

#ifndef svtkInteractorStyleTrackball_h
#define svtkInteractorStyleTrackball_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyleSwitch.h"

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleTrackball : public svtkInteractorStyleSwitch
{
public:
  static svtkInteractorStyleTrackball* New();
  svtkTypeMacro(svtkInteractorStyleTrackball, svtkInteractorStyleSwitch);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkInteractorStyleTrackball();
  ~svtkInteractorStyleTrackball() override;

private:
  svtkInteractorStyleTrackball(const svtkInteractorStyleTrackball&) = delete;
  void operator=(const svtkInteractorStyleTrackball&) = delete;
};

#endif
