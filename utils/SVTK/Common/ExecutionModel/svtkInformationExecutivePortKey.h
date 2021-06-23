/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInformationExecutivePortKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInformationExecutivePortKey
 * @brief   Key for svtkExecutive/Port value pairs.
 *
 * svtkInformationExecutivePortKey is used to represent keys in
 * svtkInformation for values that are svtkExecutive instances paired
 * with port numbers.
 */

#ifndef svtkInformationExecutivePortKey_h
#define svtkInformationExecutivePortKey_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkInformationKey.h"

#include "svtkFilteringInformationKeyManager.h" // Manage instances of this type.

class svtkExecutive;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkInformationExecutivePortKey : public svtkInformationKey
{
public:
  svtkTypeMacro(svtkInformationExecutivePortKey, svtkInformationKey);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkInformationExecutivePortKey(const char* name, const char* location);
  ~svtkInformationExecutivePortKey() override;

  /**
   * This method simply returns a new svtkInformationExecutivePortKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static svtkInformationExecutivePortKey* MakeKey(const char* name, const char* location)
  {
    return new svtkInformationExecutivePortKey(name, location);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(svtkInformation* info, svtkExecutive*, int);
  svtkExecutive* GetExecutive(svtkInformation* info);
  int GetPort(svtkInformation* info);
  void Get(svtkInformation* info, svtkExecutive*& executive, int& port);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(svtkInformation* from, svtkInformation* to) override;

  /**
   * Report a reference this key has in the given information object.
   */
  void Report(svtkInformation* info, svtkGarbageCollector* collector) override;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, svtkInformation* info) override;

private:
  svtkInformationExecutivePortKey(const svtkInformationExecutivePortKey&) = delete;
  void operator=(const svtkInformationExecutivePortKey&) = delete;
};

#endif
