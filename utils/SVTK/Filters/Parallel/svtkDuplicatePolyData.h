/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDuplicatePolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDuplicatePolyData
 * @brief   For distributed tiled displays.
 *
 * This filter collects poly data and duplicates it on every node.
 * Converts data parallel so every node has a complete copy of the data.
 * The filter is used at the end of a pipeline for driving a tiled
 * display.
 */

#ifndef svtkDuplicatePolyData_h
#define svtkDuplicatePolyData_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"
class svtkSocketController;
class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkDuplicatePolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkDuplicatePolyData* New();
  svtkTypeMacro(svtkDuplicatePolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  void InitializeSchedule(int numProcs);

  //@{
  /**
   * This flag causes sends and receives to be matched.
   * When this flag is off, two sends occur then two receives.
   * I want to see if it makes a difference in performance.
   * The flag is on by default.
   */
  svtkSetMacro(Synchronous, svtkTypeBool);
  svtkGetMacro(Synchronous, svtkTypeBool);
  svtkBooleanMacro(Synchronous, svtkTypeBool);
  //@}

  //@{
  /**
   * This duplicate filter works in client server mode when this
   * controller is set.  We have a client flag to differentiate the
   * client and server because the socket controller is odd:
   * Proth processes think their id is 0.
   */
  svtkSocketController* GetSocketController() { return this->SocketController; }
  void SetSocketController(svtkSocketController* controller);
  svtkSetMacro(ClientFlag, int);
  svtkGetMacro(ClientFlag, int);
  //@}

  //@{
  /**
   * This returns to size of the output (on this process).
   * This method is not really used.  It is needed to have
   * the same API as svtkCollectPolyData.
   */
  svtkGetMacro(MemorySize, unsigned long);
  //@}

protected:
  svtkDuplicatePolyData();
  ~svtkDuplicatePolyData() override;

  // Data generation method
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ClientExecute(svtkPolyData* output);

  svtkMultiProcessController* Controller;
  svtkTypeBool Synchronous;

  int NumberOfProcesses;
  int ScheduleLength;
  int** Schedule;

  // For client server mode.
  svtkSocketController* SocketController;
  int ClientFlag;

  unsigned long MemorySize;

private:
  svtkDuplicatePolyData(const svtkDuplicatePolyData&) = delete;
  void operator=(const svtkDuplicatePolyData&) = delete;
};

#endif
