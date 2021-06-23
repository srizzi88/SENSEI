/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBar
 * @brief   Bar class for svtk
 *
 * None.
 */

#ifndef svtkBar_h
#define svtkBar_h

#include "svtkObject.h"
#include "svtkmyCommonModule.h" // For export macro

class SVTKMYCOMMON_EXPORT svtkBar : public svtkObject
{
public:
  static svtkBar* New();
  svtkTypeMacro(svtkBar, svtkObject);

protected:
  svtkBar() {}
  ~svtkBar() override {}

private:
  svtkBar(const svtkBar&) = delete;
  void operator=(const svtkBar&) = delete;
};

#endif
