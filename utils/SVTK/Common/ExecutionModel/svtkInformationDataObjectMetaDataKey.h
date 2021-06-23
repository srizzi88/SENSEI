/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInformationDataObjectMetaDataKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInformationDataObjectMetaDataKey
 * @brief   key used to define meta-data of type svtkDataObject
 * svtkInformationDataObjectMetaDataKey is a svtkInformationDataObjectKey
 * that (shallow) copies itself downstream during the REQUEST_INFORMATION pass. Hence
 * it can be used to provide meta-data of type svtkDataObject or any subclass.
 */

#ifndef svtkInformationDataObjectMetaDataKey_h
#define svtkInformationDataObjectMetaDataKey_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkInformationDataObjectKey.h"

#include "svtkCommonInformationKeyManager.h" // Manage instances of this type.

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkInformationDataObjectMetaDataKey
  : public svtkInformationDataObjectKey
{
public:
  svtkTypeMacro(svtkInformationDataObjectMetaDataKey, svtkInformationDataObjectKey);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkInformationDataObjectMetaDataKey(const char* name, const char* location);
  ~svtkInformationDataObjectMetaDataKey() override;

  /**
   * This method simply returns a new svtkInformationDataObjectMetaDataKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static svtkInformationDataObjectMetaDataKey* MakeKey(const char* name, const char* location)
  {
    return new svtkInformationDataObjectMetaDataKey(name, location);
  }

  /**
   * Simply shallow copies the key from fromInfo to toInfo if request
   * has the REQUEST_INFORMATION() key.
   * This is used by the pipeline to propagate this key downstream.
   */
  void CopyDefaultInformation(
    svtkInformation* request, svtkInformation* fromInfo, svtkInformation* toInfo) override;

private:
  svtkInformationDataObjectMetaDataKey(const svtkInformationDataObjectMetaDataKey&) = delete;
  void operator=(const svtkInformationDataObjectMetaDataKey&) = delete;
};

#endif
