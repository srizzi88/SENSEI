/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssignCoordinatesLayoutStrategy.h

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
 * @class   svtkAssignCoordinatesLayoutStrategy
 * @brief   uses array values to set vertex locations
 *
 *
 * Uses svtkAssignCoordinates to use values from arrays as the x, y, and z coordinates.
 */

#ifndef svtkAssignCoordinatesLayoutStrategy_h
#define svtkAssignCoordinatesLayoutStrategy_h

#include "svtkGraphLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkSmartPointer.h"        // For SP ivars

class svtkAssignCoordinates;

class SVTKINFOVISLAYOUT_EXPORT svtkAssignCoordinatesLayoutStrategy : public svtkGraphLayoutStrategy
{
public:
  static svtkAssignCoordinatesLayoutStrategy* New();
  svtkTypeMacro(svtkAssignCoordinatesLayoutStrategy, svtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The array to use for the x coordinate values.
   */
  virtual void SetXCoordArrayName(const char* name);
  virtual const char* GetXCoordArrayName();
  //@}

  //@{
  /**
   * The array to use for the y coordinate values.
   */
  virtual void SetYCoordArrayName(const char* name);
  virtual const char* GetYCoordArrayName();
  //@}

  //@{
  /**
   * The array to use for the z coordinate values.
   */
  virtual void SetZCoordArrayName(const char* name);
  virtual const char* GetZCoordArrayName();
  //@}

  /**
   * Perform the random layout.
   */
  void Layout() override;

protected:
  svtkAssignCoordinatesLayoutStrategy();
  ~svtkAssignCoordinatesLayoutStrategy() override;

  svtkSmartPointer<svtkAssignCoordinates> AssignCoordinates;

private:
  svtkAssignCoordinatesLayoutStrategy(const svtkAssignCoordinatesLayoutStrategy&) = delete;
  void operator=(const svtkAssignCoordinatesLayoutStrategy&) = delete;
};

#endif
