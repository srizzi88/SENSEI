/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPExtractExodusGlobalTemporalVariables.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkPExtractExodusGlobalTemporalVariables
 * @brief parallel version of svtkExtractExodusGlobalTemporalVariables.
 *
 * svtkPExtractExodusGlobalTemporalVariables is a parallel version of
 * svtkExtractExodusGlobalTemporalVariables that handles synchronization between
 * multiple ranks. Since svtkPExodusIIReader has explicit sycnchronization
 * between ranks its essential that downstream filters make consistent requests
 * on all ranks to avoid deadlocks. Since global variables need not be provided
 * on all ranks, without explicit coordination
 * svtkExtractExodusGlobalTemporalVariables may end up not making requests on
 * certain ranks causing deadlocks.
 */

#ifndef svtkPExtractExodusGlobalTemporalVariables_h
#define svtkPExtractExodusGlobalTemporalVariables_h

#include "svtkExtractExodusGlobalTemporalVariables.h"
#include "svtkFiltersParallelModule.h" // For export macro

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkPExtractExodusGlobalTemporalVariables
  : public svtkExtractExodusGlobalTemporalVariables
{
public:
  static svtkPExtractExodusGlobalTemporalVariables* New();
  svtkTypeMacro(svtkPExtractExodusGlobalTemporalVariables, svtkExtractExodusGlobalTemporalVariables);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default
   * `svtkMultiProcessController::GlobalController` will be used.
   */
  void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}
protected:
  svtkPExtractExodusGlobalTemporalVariables();
  ~svtkPExtractExodusGlobalTemporalVariables() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkPExtractExodusGlobalTemporalVariables(
    const svtkPExtractExodusGlobalTemporalVariables&) = delete;
  void operator=(const svtkPExtractExodusGlobalTemporalVariables&) = delete;

  svtkMultiProcessController* Controller;
};

#endif
