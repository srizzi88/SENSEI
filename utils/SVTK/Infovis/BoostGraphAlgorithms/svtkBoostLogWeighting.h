/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostLogWeighting.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkBoostLogWeighting
 * @brief   Given an arbitrary-dimension array of doubles,
 * replaces each value x with one of:
 *
 * * The natural logarithm of 1 + x (the default)
 * * The base-2 logarithm of 1 + x
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkBoostLogWeighting_h
#define svtkBoostLogWeighting_h

#include "svtkArrayDataAlgorithm.h"
#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostLogWeighting : public svtkArrayDataAlgorithm
{
public:
  static svtkBoostLogWeighting* New();
  svtkTypeMacro(svtkBoostLogWeighting, svtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    BASE_E = 0,
    BASE_2 = 1
  };

  //@{
  /**
   * Specify the logarithm base to apply
   */
  svtkSetMacro(Base, int);
  svtkGetMacro(Base, int);
  //@}

  //@{
  /**
   * Specify whether this filter should emit progress events
   */
  svtkSetMacro(EmitProgress, bool);
  svtkGetMacro(EmitProgress, bool);
  svtkBooleanMacro(EmitProgress, bool);
  //@}

protected:
  svtkBoostLogWeighting();
  ~svtkBoostLogWeighting() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkBoostLogWeighting(const svtkBoostLogWeighting&) = delete;
  void operator=(const svtkBoostLogWeighting&) = delete;

  int Base;
  bool EmitProgress;
};

#endif
