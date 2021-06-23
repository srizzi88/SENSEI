/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInformationIntegerRequestKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInformationIntegerRequestKey
 * @brief   key that can used to request integer values from the pipeline
 * svtkInformationIntegerRequestKey is a svtkInformationIntegerKey that can
 * used to request integer values from upstream. A good example of this is
 * UPDATE_NUMBER_OF_PIECES where downstream can request that upstream provides
 * data partitioned into a certain number of pieces. There are several components
 * that make this work. First, the key will copy itself upstream during
 * REQUEST_UPDATE_EXTENT. Second, after a successful execution, it will stor
 * its value into a data object's information using a specific key defined by
 * its data member DataKey. Third, before execution, it will check if the requested
 * value matched the value in the data object's information. If not, it will ask
 * the pipeline to execute.
 *
 * The best way to use this class is to subclass it to set the DataKey data member.
 * This is usually done in the subclass' constructor.
 */

#ifndef svtkInformationIntegerRequestKey_h
#define svtkInformationIntegerRequestKey_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkInformationIntegerKey.h"

#include "svtkCommonInformationKeyManager.h" // Manage instances of this type.

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkInformationIntegerRequestKey
  : public svtkInformationIntegerKey
{
public:
  svtkTypeMacro(svtkInformationIntegerRequestKey, svtkInformationIntegerKey);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkInformationIntegerRequestKey(const char* name, const char* location);
  ~svtkInformationIntegerRequestKey() override;

  /**
   * This method simply returns a new svtkInformationIntegerRequestKey,
   * given a name and a location. This method is provided for wrappers. Use
   * the constructor directly from C++ instead.
   */
  static svtkInformationIntegerRequestKey* MakeKey(const char* name, const char* location)
  {
    return new svtkInformationIntegerRequestKey(name, location);
  }

  /**
   * Returns true if a value of type DataKey does not exist in dobjInfo
   * or if it is different that the value stored in pipelineInfo using
   * this key.
   */
  bool NeedToExecute(svtkInformation* pipelineInfo, svtkInformation* dobjInfo) override;

  /**
   * Copies the value stored in pipelineInfo using this key into
   * dobjInfo.
   */
  void StoreMetaData(
    svtkInformation* request, svtkInformation* pipelineInfo, svtkInformation* dobjInfo) override;

  /**
   * Copies the value stored in fromInfo using this key into toInfo
   * if request has the REQUEST_UPDATE_EXTENT key.
   */
  void CopyDefaultInformation(
    svtkInformation* request, svtkInformation* fromInfo, svtkInformation* toInfo) override;

protected:
  svtkInformationIntegerKey* DataKey;

private:
  svtkInformationIntegerRequestKey(const svtkInformationIntegerRequestKey&) = delete;
  void operator=(const svtkInformationIntegerRequestKey&) = delete;
};

#endif
