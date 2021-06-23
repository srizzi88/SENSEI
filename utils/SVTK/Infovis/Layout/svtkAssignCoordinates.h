/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssignCoordinates.h

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
 * @class   svtkAssignCoordinates
 * @brief   Given two(or three) arrays take the values
 * in those arrays and simply assign them to the coordinates of the vertices.
 *
 *
 * Given two(or three) arrays take the values in those arrays and simply assign
 * them to the coordinates of the vertices. Yes you could do this with the array
 * calculator, but your mom wears army boots so we're not going to.
 */

#ifndef svtkAssignCoordinates_h
#define svtkAssignCoordinates_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class SVTKINFOVISLAYOUT_EXPORT svtkAssignCoordinates : public svtkPassInputTypeAlgorithm
{
public:
  static svtkAssignCoordinates* New();

  svtkTypeMacro(svtkAssignCoordinates, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the x coordinate array name.
   */
  svtkSetStringMacro(XCoordArrayName);
  svtkGetStringMacro(XCoordArrayName);
  //@}

  //@{
  /**
   * Set the y coordinate array name.
   */
  svtkSetStringMacro(YCoordArrayName);
  svtkGetStringMacro(YCoordArrayName);
  //@}

  //@{
  /**
   * Set the z coordinate array name.
   */
  svtkSetStringMacro(ZCoordArrayName);
  svtkGetStringMacro(ZCoordArrayName);
  //@}

  //@{
  /**
   * Set if you want a random jitter
   */
  svtkSetMacro(Jitter, bool);
  //@}

protected:
  svtkAssignCoordinates();
  ~svtkAssignCoordinates() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  char* XCoordArrayName;
  char* YCoordArrayName;
  char* ZCoordArrayName;
  bool Jitter;

  svtkAssignCoordinates(const svtkAssignCoordinates&) = delete;
  void operator=(const svtkAssignCoordinates&) = delete;
};

#endif
