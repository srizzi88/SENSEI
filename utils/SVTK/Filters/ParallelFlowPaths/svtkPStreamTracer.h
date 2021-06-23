/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPStreamTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPStreamTracer
 * @brief    parallel streamline generators
 *
 * This class implements parallel streamline generators.  Note that all
 * processes must have access to the WHOLE seed source, i.e. the source must
 * be identical on all processes.
 * @sa
 * svtkStreamTracer
 */

#ifndef svtkPStreamTracer_h
#define svtkPStreamTracer_h

#include "svtkSmartPointer.h" // This is a leaf node. No need to use PIMPL to avoid compile time penalty.
#include "svtkStreamTracer.h"

class svtkAbstractInterpolatedVelocityField;
class svtkMultiProcessController;

class PStreamTracerPoint;
class svtkOverlappingAMR;
class AbstractPStreamTracerUtils;

#include "svtkFiltersParallelFlowPathsModule.h" // For export macro

class SVTKFILTERSPARALLELFLOWPATHS_EXPORT svtkPStreamTracer : public svtkStreamTracer
{
public:
  svtkTypeMacro(svtkPStreamTracer, svtkStreamTracer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the controller use in compositing (set to the global controller
   * by default) If not using the default, this must be called before any
   * other methods.
   */
  virtual void SetController(svtkMultiProcessController* controller);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  static svtkPStreamTracer* New();

protected:
  svtkPStreamTracer();
  ~svtkPStreamTracer() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestUpdateExtent(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMultiProcessController* Controller;

  svtkAbstractInterpolatedVelocityField* Interpolator;
  void SetInterpolator(svtkAbstractInterpolatedVelocityField*);

  int EmptyData;

private:
  svtkPStreamTracer(const svtkPStreamTracer&) = delete;
  void operator=(const svtkPStreamTracer&) = delete;

  void Trace(svtkDataSet* input, int vecType, const char* vecName, PStreamTracerPoint* pt,
    svtkSmartPointer<svtkPolyData>& output, svtkAbstractInterpolatedVelocityField* func,
    int maxCellSize);

  bool TraceOneStep(
    svtkPolyData* traceOut, svtkAbstractInterpolatedVelocityField*, PStreamTracerPoint* pt);

  void Prepend(svtkPolyData* path, svtkPolyData* headh);
  int Rank;
  int NumProcs;

  friend class AbstractPStreamTracerUtils;
  svtkSmartPointer<AbstractPStreamTracerUtils> Utils;
};
#endif
