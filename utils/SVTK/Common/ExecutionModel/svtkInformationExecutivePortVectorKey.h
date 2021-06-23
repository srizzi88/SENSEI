/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInformationExecutivePortVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInformationExecutivePortVectorKey
 * @brief   Key for svtkExecutive/Port value pair vectors.
 *
 * svtkInformationExecutivePortVectorKey is used to represent keys in
 * svtkInformation for values that are vectors of svtkExecutive
 * instances paired with port numbers.
 */

#ifndef svtkInformationExecutivePortVectorKey_h
#define svtkInformationExecutivePortVectorKey_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkInformationKey.h"

#include "svtkFilteringInformationKeyManager.h" // Manage instances of this type.

class svtkExecutive;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkInformationExecutivePortVectorKey : public svtkInformationKey
{
public:
  svtkTypeMacro(svtkInformationExecutivePortVectorKey, svtkInformationKey);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkInformationExecutivePortVectorKey(const char* name, const char* location);
  ~svtkInformationExecutivePortVectorKey() override;

  /**
   * This method simply returns a new svtkInformationExecutivePortVectorKey,
   * given a name and a location. This method is provided for wrappers. Use
   * the constructor directly from C++ instead.
   */
  static svtkInformationExecutivePortVectorKey* MakeKey(const char* name, const char* location)
  {
    return new svtkInformationExecutivePortVectorKey(name, location);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Append(svtkInformation* info, svtkExecutive* executive, int port);
  void Remove(svtkInformation* info, svtkExecutive* executive, int port);
  void Set(svtkInformation* info, svtkExecutive** executives, int* ports, int length);
  svtkExecutive** GetExecutives(svtkInformation* info);
  int* GetPorts(svtkInformation* info);
  void Get(svtkInformation* info, svtkExecutive** executives, int* ports);
  int Length(svtkInformation* info);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(svtkInformation* from, svtkInformation* to) override;

  /**
   * Remove this key from the given information object.
   */
  void Remove(svtkInformation* info) override;

  /**
   * Report a reference this key has in the given information object.
   */
  void Report(svtkInformation* info, svtkGarbageCollector* collector) override;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, svtkInformation* info) override;

protected:
  //@{
  /**
   * Get the address at which the actual value is stored.  This is
   * meant for use from a debugger to add watches and is therefore not
   * a public method.
   */
  svtkExecutive** GetExecutivesWatchAddress(svtkInformation* info);
  int* GetPortsWatchAddress(svtkInformation* info);
  //@}

private:
  svtkInformationExecutivePortVectorKey(const svtkInformationExecutivePortVectorKey&) = delete;
  void operator=(const svtkInformationExecutivePortVectorKey&) = delete;
};

#endif
