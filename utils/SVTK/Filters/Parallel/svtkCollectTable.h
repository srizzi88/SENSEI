/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollectTable.h

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
 * @class   svtkCollectTable
 * @brief   Collect distributed table.
 *
 * This filter has code to collect a table from across processes onto node 0.
 * Collection can be turned on or off using the "PassThrough" flag.
 */

#ifndef svtkCollectTable_h
#define svtkCollectTable_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class svtkMultiProcessController;
class svtkSocketController;

class SVTKFILTERSPARALLEL_EXPORT svtkCollectTable : public svtkTableAlgorithm
{
public:
  static svtkCollectTable* New();
  svtkTypeMacro(svtkCollectTable, svtkTableAlgorithm);
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
  svtkCollectTable();
  ~svtkCollectTable() override;

  svtkTypeBool PassThrough;

  // Data generation method
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMultiProcessController* Controller;
  svtkSocketController* SocketController;

private:
  svtkCollectTable(const svtkCollectTable&) = delete;
  void operator=(const svtkCollectTable&) = delete;
};

#endif
