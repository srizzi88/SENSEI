/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTrivialConsumer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTrivialConsumer
 * @brief   Consumer to consume data off of a pipeline.
 *
 * svtkTrivialConsumer caps off a pipeline so that no output data is left
 * hanging around when a pipeline executes when data is set to be released (see
 * svtkDataObject::SetGlobalReleaseDataFlag). This is intended to be used for
 * tools such as Catalyst and not end users.
 */

#ifndef svtkTrivialConsumer_h
#define svtkTrivialConsumer_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkTrivialConsumer : public svtkAlgorithm
{
public:
  static svtkTrivialConsumer* New();
  svtkTypeMacro(svtkTrivialConsumer, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkTrivialConsumer();
  ~svtkTrivialConsumer() override;

  int FillInputPortInformation(int, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkTrivialConsumer(const svtkTrivialConsumer&) = delete;
  void operator=(const svtkTrivialConsumer&) = delete;
};

#endif
