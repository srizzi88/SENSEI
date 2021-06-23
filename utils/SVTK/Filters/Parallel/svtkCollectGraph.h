/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollectGraph.h

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
 * @class   svtkCollectGraph
 * @brief   Collect distributed graph.
 *
 * This filter has code to collect a graph from across processes onto vertex 0.
 * Collection can be turned on or off using the "PassThrough" flag.
 */

#ifndef svtkCollectGraph_h
#define svtkCollectGraph_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkGraphAlgorithm.h"

class svtkMultiProcessController;
class svtkSocketController;

class SVTKFILTERSPARALLEL_EXPORT svtkCollectGraph : public svtkGraphAlgorithm
{
public:
  static svtkCollectGraph* New();
  svtkTypeMacro(svtkCollectGraph, svtkGraphAlgorithm);
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

  enum
  {
    DIRECTED_OUTPUT,
    UNDIRECTED_OUTPUT,
    USE_INPUT_TYPE
  };

  //@{
  /**
   * Directedness flag, used to signal whether the output graph is directed or undirected.
   * DIRECTED_OUTPUT expects that this filter is generating a directed graph.
   * UNDIRECTED_OUTPUT expects that this filter is generating an undirected graph.
   * DIRECTED_OUTPUT and UNDIRECTED_OUTPUT flags should only be set on the client
   * filter.  Server filters should be set to USE_INPUT_TYPE since they have valid
   * input and the directedness is determined from the input type.
   */
  svtkSetMacro(OutputType, int);
  svtkGetMacro(OutputType, int);
  //@}

protected:
  svtkCollectGraph();
  ~svtkCollectGraph() override;

  svtkTypeBool PassThrough;
  int OutputType;

  // Data generation method
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMultiProcessController* Controller;
  svtkSocketController* SocketController;

private:
  svtkCollectGraph(const svtkCollectGraph&) = delete;
  void operator=(const svtkCollectGraph&) = delete;
};

#endif
