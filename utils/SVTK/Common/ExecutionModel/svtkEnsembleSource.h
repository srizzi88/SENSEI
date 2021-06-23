/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEnsembleSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEnsembleSource
 * @brief   source that manages dataset ensembles
 *
 * svtkEnsembleSource manages a collection of data sources in order to
 * represent a dataset ensemble. It has the ability to provide meta-data
 * about the ensemble in the form of a table, using the META_DATA key
 * as well as accept a pipeline request using the UPDATE_MEMBER key.
 * Note that it is expected that all ensemble members produce data of the
 * same type.
 */

#ifndef svtkEnsembleSource_h
#define svtkEnsembleSource_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

struct svtkEnsembleSourceInternal;
class svtkTable;
class svtkInformationDataObjectMetaDataKey;
class svtkInformationIntegerRequestKey;
class svtkInformationIntegerKey;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkEnsembleSource : public svtkAlgorithm
{
public:
  static svtkEnsembleSource* New();
  svtkTypeMacro(svtkEnsembleSource, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add an algorithm (source) that will produce the next ensemble member.
   * This algorithm will be passed the REQUEST_INFORMATION, REQUEST_UPDATE_EXTENT
   * and REQUEST_DATA pipeline passes for execution.
   */
  void AddMember(svtkAlgorithm*);

  /**
   * Removes all ensemble members.
   */
  void RemoveAllMembers();

  /**
   * Returns the number of ensemble members.
   */
  unsigned int GetNumberOfMembers();

  //@{
  /**
   * Set/Get the current ensemble member to process. Note that this data member
   * will not be used if the UPDATE_MEMBER key is present in the pipeline. Also,
   * this data member may be removed in the future. Unless it is absolutely necessary
   * to use this data member, use the UPDATE_MEMBER key instead.
   */
  svtkSetMacro(CurrentMember, unsigned int);
  svtkGetMacro(CurrentMember, unsigned int);
  //@}

  /**
   * Set the meta-data that will be propagated downstream. Make sure that this table
   * has as many rows as the ensemble members and the meta-data for each row matches
   * the corresponding ensemble source.
   */
  void SetMetaData(svtkTable*);

  /**
   * Meta-data for the ensemble. This is set with SetMetaData.
   */
  static svtkInformationDataObjectMetaDataKey* META_DATA();

  /**
   * Key used to request a particular ensemble member.
   */
  static svtkInformationIntegerRequestKey* UPDATE_MEMBER();

protected:
  svtkEnsembleSource();
  ~svtkEnsembleSource() override;

  static svtkInformationIntegerKey* DATA_MEMBER();

  friend class svtkInformationEnsembleMemberRequestKey;

  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  svtkAlgorithm* GetCurrentReader(svtkInformation*);

  svtkEnsembleSourceInternal* Internal;
  unsigned int CurrentMember;

  svtkTable* MetaData;

private:
  svtkEnsembleSource(const svtkEnsembleSource&) = delete;
  void operator=(const svtkEnsembleSource&) = delete;
};

#endif
