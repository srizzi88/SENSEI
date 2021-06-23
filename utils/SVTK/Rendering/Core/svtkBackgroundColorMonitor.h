/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBackgroundColorMonitor

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBackgroundColorMonitor
 * tracks state of background color(s).
 *
 *
 * svtkBackgroundColorMonitor -- A helper for painters that
 * tracks state of background color(s). A Painter could use this
 * to skip expensive processing that is only needed when
 * background color changes. This class queries SVTK renderer
 * rather than OpenGL state in order to support SVTK's gradient
 * background.
 *
 * this is not intended to be shared. each object should use it's
 * own instance of this class. it's intended to be called once per
 * render.
 */

#ifndef svtkBackgroundColorMonitor_h
#define svtkBackgroundColorMonitor_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // for export macro

class svtkRenderer;

class SVTKRENDERINGCORE_EXPORT svtkBackgroundColorMonitor : public svtkObject
{
public:
  static svtkBackgroundColorMonitor* New();
  svtkTypeMacro(svtkBackgroundColorMonitor, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Fetches the current background color state and
   * updates the internal copies of the data. returns
   * true if any of the tracked colors or modes have
   * changed. Typically this is the only function a
   * user needs to call.
   */
  bool StateChanged(svtkRenderer* ren);

  /**
   * Update the internal state if anything changed. Note,
   * this is done automatically in SateChanged.
   */
  void Update(svtkRenderer* ren);

protected:
  svtkBackgroundColorMonitor();
  ~svtkBackgroundColorMonitor() override {}

private:
  unsigned int UpTime;
  bool Gradient;
  double Color1[3];
  double Color2[3];

private:
  svtkBackgroundColorMonitor(const svtkBackgroundColorMonitor&) = delete;
  void operator=(const svtkBackgroundColorMonitor&) = delete;
};

#endif
