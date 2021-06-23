/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenerateGlobalIds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkGenerateGlobalIds
 * @brief generates global point and cell ids.
 *
 * svtkGenerateGlobalIds generates global point and cell ids. This filter also
 * generated ghost-point information, flagging duplicate points appropriately.
 * svtkGenerateGlobalIds works across all blocks in the input datasets and across
 * all ranks.
 */

#ifndef svtkGenerateGlobalIds_h
#define svtkGenerateGlobalIds_h

#include "svtkFiltersParallelDIY2Module.h" // for export macros
#include "svtkPassInputTypeAlgorithm.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLELDIY2_EXPORT svtkGenerateGlobalIds : public svtkPassInputTypeAlgorithm
{
public:
  static svtkGenerateGlobalIds* New();
  svtkTypeMacro(svtkGenerateGlobalIds, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default
   * svtkMultiProcessController::GlobalController will be used.
   */
  void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkGenerateGlobalIds();
  ~svtkGenerateGlobalIds() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkGenerateGlobalIds(const svtkGenerateGlobalIds&) = delete;
  void operator=(const svtkGenerateGlobalIds&) = delete;

  svtkMultiProcessController* Controller;
};

#endif
