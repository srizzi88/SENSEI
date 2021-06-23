/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSMPWarpVector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSMPWarpVector
 * @brief   multithreaded svtkWarpVector
 *
 * Just like parent, but uses the SMP framework to do the work on many threads.
 */

#ifndef svtkSMPWarpVector_h
#define svtkSMPWarpVector_h

#include "svtkFiltersSMPModule.h" // For export macro
#include "svtkWarpVector.h"

class svtkInformation;
class svtkInformationVector;

#if !defined(SVTK_LEGACY_REMOVE)
class SVTKFILTERSSMP_EXPORT svtkSMPWarpVector : public svtkWarpVector
{
public:
  svtkTypeMacro(svtkSMPWarpVector, svtkWarpVector);
  static svtkSMPWarpVector* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkSMPWarpVector();
  ~svtkSMPWarpVector() override;

  /**
   * Overridden to use threads.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkSMPWarpVector(const svtkSMPWarpVector&) = delete;
  void operator=(const svtkSMPWarpVector&) = delete;
};

#endif // SVTK_LEGACY_REMOVE
#endif // svtkSMPWarpVector_h
