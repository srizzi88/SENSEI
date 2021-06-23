/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollectPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCollectPolyData
 * @brief   Collect distributed polydata.
 *
 * This filter has code to collect polydat from across processes onto node 0.
 * Collection can be turned on or off using the "PassThrough" flag.
 */

#ifndef svtkCollectPolyData_h
#define svtkCollectPolyData_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkMultiProcessController;
class svtkSocketController;

class SVTKFILTERSPARALLEL_EXPORT svtkCollectPolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkCollectPolyData* New();
  svtkTypeMacro(svtkCollectPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  //@{
  /**
   * When this filter is being used in client-server mode,
   * this is the controller used to communicate between
   * client and server.  Client should not set the other controller.
   */
  virtual void SetSocketController(svtkSocketController*);
  svtkGetObjectMacro(SocketController, svtkSocketController);
  //@}

  //@{
  /**
   * To collect or just copy input to output. Off (collect) by default.
   */
  svtkSetMacro(PassThrough, svtkTypeBool);
  svtkGetMacro(PassThrough, svtkTypeBool);
  svtkBooleanMacro(PassThrough, svtkTypeBool);
  //@}

protected:
  svtkCollectPolyData();
  ~svtkCollectPolyData() override;

  svtkTypeBool PassThrough;

  // Data generation method
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMultiProcessController* Controller;
  svtkSocketController* SocketController;

private:
  svtkCollectPolyData(const svtkCollectPolyData&) = delete;
  void operator=(const svtkCollectPolyData&) = delete;
};

#endif
