/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPChacoReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkPChacoReader
 * @brief   Read Chaco files
 *
 * svtkPChacoReader is a unstructured grid source object that reads
 * Chaco files.  The file is read by process 0 and converted into
 * a svtkUnstructuredGrid.  The svtkDistributedDataFilter is invoked
 * to divide the grid among the processes.
 */

#ifndef svtkPChacoReader_h
#define svtkPChacoReader_h

#include "svtkChacoReader.h"
#include "svtkIOParallelModule.h" // For export macro

class svtkTimerLog;
class svtkMultiProcessController;

class SVTKIOPARALLEL_EXPORT svtkPChacoReader : public svtkChacoReader
{
public:
  static svtkPChacoReader* New();
  svtkTypeMacro(svtkPChacoReader, svtkChacoReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set/Get the communicator object (we'll use global World controller
   * if you don't set a different one).
   */

  void SetController(svtkMultiProcessController* c);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);

protected:
  svtkPChacoReader();
  ~svtkPChacoReader() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPChacoReader(const svtkPChacoReader&) = delete;
  void operator=(const svtkPChacoReader&) = delete;

  void SetUpEmptyGrid(svtkUnstructuredGrid* output);
  int DivideCells(svtkMultiProcessController* contr, svtkUnstructuredGrid* output, int source);
  int SendGrid(svtkMultiProcessController* c, int to, svtkUnstructuredGrid* grid);
  svtkUnstructuredGrid* GetGrid(svtkMultiProcessController* c, int from);
  svtkUnstructuredGrid* SubGrid(svtkUnstructuredGrid* ug, svtkIdType from, svtkIdType to);
  char* MarshallDataSet(svtkUnstructuredGrid* extractedGrid, svtkIdType& len);
  svtkUnstructuredGrid* UnMarshallDataSet(char* buf, svtkIdType size);

  int NumProcesses;
  int MyId;

  svtkMultiProcessController* Controller;
};

#endif
