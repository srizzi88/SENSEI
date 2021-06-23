/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataStreamer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataStreamer
 * @brief   Streamer appends input pieces to the output.
 *
 * svtkPolyDataStreamer initiates streaming by requesting pieces from its
 * single input it appends these pieces to the requested output.
 * Note that since svtkPolyDataStreamer uses an append filter, all the
 * polygons generated have to be kept in memory before rendering. If
 * these do not fit in the memory, it is possible to make the svtkPolyDataMapper
 * stream. Since the mapper will render each piece separately, all the
 * polygons do not have to stored in memory.
 * @attention
 * The output may be slightly different if the pipeline does not handle
 * ghost cells properly (i.e. you might see seames between the pieces).
 * @sa
 * svtkAppendFilter
 */

#ifndef svtkPolyDataStreamer_h
#define svtkPolyDataStreamer_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkStreamerBase.h"

class svtkAppendPolyData;

class SVTKFILTERSGENERAL_EXPORT svtkPolyDataStreamer : public svtkStreamerBase
{
public:
  static svtkPolyDataStreamer* New();

  svtkTypeMacro(svtkPolyDataStreamer, svtkStreamerBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the number of pieces to divide the problem into.
   */
  void SetNumberOfStreamDivisions(int num);
  int GetNumberOfStreamDivisions() { return this->NumberOfPasses; }
  //@}

  //@{
  /**
   * By default, this option is off.  When it is on, cell scalars are generated
   * based on which piece they are in.
   */
  svtkSetMacro(ColorByPiece, svtkTypeBool);
  svtkGetMacro(ColorByPiece, svtkTypeBool);
  svtkBooleanMacro(ColorByPiece, svtkTypeBool);
  //@}

protected:
  svtkPolyDataStreamer();
  ~svtkPolyDataStreamer() override;

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int ExecutePass(svtkInformationVector** inputVector, svtkInformationVector* outputVector) override;

  int PostExecute(svtkInformationVector** inputVector, svtkInformationVector* outputVector) override;

  svtkTypeBool ColorByPiece;

private:
  svtkPolyDataStreamer(const svtkPolyDataStreamer&) = delete;
  void operator=(const svtkPolyDataStreamer&) = delete;

  svtkAppendPolyData* Append;
};

#endif
