/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPipelineGraphSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPipelineGraphSource
 * @brief   a graph constructed from a SVTK pipeline
 *
 *
 */

#ifndef svtkPipelineGraphSource_h
#define svtkPipelineGraphSource_h

#include "svtkDirectedGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkStdString.h"

class svtkCollection;

class SVTKINFOVISCORE_EXPORT svtkPipelineGraphSource : public svtkDirectedGraphAlgorithm
{
public:
  static svtkPipelineGraphSource* New();
  svtkTypeMacro(svtkPipelineGraphSource, svtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void AddSink(svtkObject* object);
  void RemoveSink(svtkObject* object);

  /**
   * Generates a GraphViz DOT file that describes the SVTK pipeline
   * terminating at the given sink.
   */
  static void PipelineToDot(
    svtkAlgorithm* sink, ostream& output, const svtkStdString& graph_name = "");
  /**
   * Generates a GraphViz DOT file that describes the SVTK pipeline
   * terminating at the given sinks.
   */
  static void PipelineToDot(
    svtkCollection* sinks, ostream& output, const svtkStdString& graph_name = "");

protected:
  svtkPipelineGraphSource();
  ~svtkPipelineGraphSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkCollection* Sinks;

private:
  svtkPipelineGraphSource(const svtkPipelineGraphSource&) = delete;
  void operator=(const svtkPipelineGraphSource&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkPipelineGraphSource.h
