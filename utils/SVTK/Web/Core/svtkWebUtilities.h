/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWebUtilities
 * @brief   collection of utility functions for ParaView Web.
 *
 * svtkWebUtilities consolidates miscellaneous utility functions useful for
 * Python scripts designed for ParaView Web.
 */

#ifndef svtkWebUtilities_h
#define svtkWebUtilities_h

#include "svtkObject.h"
#include "svtkWebCoreModule.h" // needed for exports
#include <string>

class svtkDataSet;

class SVTKWEBCORE_EXPORT svtkWebUtilities : public svtkObject
{
public:
  static svtkWebUtilities* New();
  svtkTypeMacro(svtkWebUtilities, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static std::string WriteAttributesToJavaScript(int field_type, svtkDataSet*);
  static std::string WriteAttributeHeadersToJavaScript(int field_type, svtkDataSet*);

  //@{
  /**
   * This method is similar to the ProcessRMIs() method on the GlobalController
   * except that it is Python friendly in the sense that it will release the
   * Python GIS lock, so when run in a thread, this will truly work in the
   * background without locking the main one.
   */
  static void ProcessRMIs();
  static void ProcessRMIs(int reportError, int dont_loop = 0);
  //@}

protected:
  svtkWebUtilities();
  ~svtkWebUtilities() override;

private:
  svtkWebUtilities(const svtkWebUtilities&) = delete;
  void operator=(const svtkWebUtilities&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkWebUtilities.h
