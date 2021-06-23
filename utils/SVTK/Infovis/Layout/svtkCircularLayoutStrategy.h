/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCircularLayoutStrategy.h

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
 * @class   svtkCircularLayoutStrategy
 * @brief   Places vertices around a circle
 *
 *
 * Assigns points to the vertices around a circle with unit radius.
 */

#ifndef svtkCircularLayoutStrategy_h
#define svtkCircularLayoutStrategy_h

#include "svtkGraphLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class SVTKINFOVISLAYOUT_EXPORT svtkCircularLayoutStrategy : public svtkGraphLayoutStrategy
{
public:
  static svtkCircularLayoutStrategy* New();

  svtkTypeMacro(svtkCircularLayoutStrategy, svtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform the layout.
   */
  void Layout() override;

protected:
  svtkCircularLayoutStrategy();
  ~svtkCircularLayoutStrategy() override;

private:
  svtkCircularLayoutStrategy(const svtkCircularLayoutStrategy&) = delete;
  void operator=(const svtkCircularLayoutStrategy&) = delete;
};

#endif
