/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBar2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBar2
 * @brief   Bar2 class for svtk
 *
 * None.
 */

#ifndef svtkBar2_h
#define svtkBar2_h

#include "svtkObject.h"
#include "svtkmyUnsortedModule.h" // For export macro

class SVTKMYUNSORTED_EXPORT svtkBar2 : public svtkObject
{
public:
  static svtkBar2* New();
  svtkTypeMacro(svtkBar2, svtkObject);

protected:
  svtkBar2() {}
  ~svtkBar2() override {}

private:
  svtkBar2(const svtkBar2&) = delete;
  void operator=(const svtkBar2&) = delete;
};

#endif
