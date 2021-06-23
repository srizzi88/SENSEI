/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScaledTextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkScaledTextActor
 * @brief   create text that will scale as needed
 *
 * svtkScaledTextActor is deprecated. New code should use svtkTextActor with
 * the Scaled = true option.
 *
 * @sa
 * svtkTextActor svtkActor2D svtkTextMapper
 */

#ifndef svtkScaledTextActor_h
#define svtkScaledTextActor_h

#include "svtkRenderingFreeTypeModule.h" // For export macro
#include "svtkTextActor.h"

class SVTKRENDERINGFREETYPE_EXPORT svtkScaledTextActor : public svtkTextActor
{
public:
  svtkTypeMacro(svtkScaledTextActor, svtkTextActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with a rectangle in normaled view coordinates
   * of (0.2,0.85, 0.8, 0.95).
   */
  static svtkScaledTextActor* New();

protected:
  svtkScaledTextActor();

private:
  svtkScaledTextActor(const svtkScaledTextActor&) = delete;
  void operator=(const svtkScaledTextActor&) = delete;
};

#endif
