/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAlgorithmOutput.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAlgorithmOutput
 * @brief   Proxy object to connect input/output ports.
 *
 * svtkAlgorithmOutput is a proxy object returned by the GetOutputPort
 * method of svtkAlgorithm.  It may be passed to the
 * SetInputConnection, AddInputConnection, or RemoveInputConnection
 * methods of another svtkAlgorithm to establish a connection between
 * an output and input port.  The connection is not stored in the
 * proxy object: it is simply a convenience for creating or removing
 * connections.
 */

#ifndef svtkAlgorithmOutput_h
#define svtkAlgorithmOutput_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkObject.h"

class svtkAlgorithm;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkAlgorithmOutput : public svtkObject
{
public:
  static svtkAlgorithmOutput* New();
  svtkTypeMacro(svtkAlgorithmOutput, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void SetIndex(int index);
  int GetIndex();

  svtkAlgorithm* GetProducer();
  void SetProducer(svtkAlgorithm* producer);

protected:
  svtkAlgorithmOutput();
  ~svtkAlgorithmOutput() override;

  int Index;
  svtkAlgorithm* Producer;

private:
  svtkAlgorithmOutput(const svtkAlgorithmOutput&) = delete;
  void operator=(const svtkAlgorithmOutput&) = delete;
};

#endif
