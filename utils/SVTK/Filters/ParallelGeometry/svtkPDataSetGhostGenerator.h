/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkPDataSetGhostGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkPDataSetGhostGenerator
 *
 *
 *  An abstract class that provides common functionality and implements an
 *  interface for all parallel ghost data generators.
 *
 * @sa
 * svtkDataSetGhostGenerator, svtkPUniformGridGhostDataGenerator,
 * svtkPStructuredGridGhostDataGenerator, svtkPRectilinearGridGhostDataGenerator
 */

#ifndef svtkPDataSetGhostGenerator_h
#define svtkPDataSetGhostGenerator_h

#include "svtkDataSetGhostGenerator.h"
#include "svtkFiltersParallelGeometryModule.h" // For export macro

class svtkMultiProcessController;
class svtkMultiBlockDataSet;

class SVTKFILTERSPARALLELGEOMETRY_EXPORT svtkPDataSetGhostGenerator : public svtkDataSetGhostGenerator
{
public:
  svtkTypeMacro(svtkPDataSetGhostGenerator, svtkDataSetGhostGenerator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set macro for the multi-process controller. If a controller is not
   * supplied, then, the global controller is assumed.
   */
  svtkSetMacro(Controller, svtkMultiProcessController*);
  svtkGetMacro(Controller, svtkMultiProcessController*);
  //@}

  /**
   * Initializes
   */
  void Initialize();

  /**
   * Barrier synchronization
   */
  void Barrier();

protected:
  svtkPDataSetGhostGenerator();
  ~svtkPDataSetGhostGenerator() override;

  /**
   * Creates ghost layers. Implemented by concrete implementations.
   */
  virtual void GenerateGhostLayers(
    svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out) override = 0;

  int Rank;
  bool Initialized;
  svtkMultiProcessController* Controller;

private:
  svtkPDataSetGhostGenerator(const svtkPDataSetGhostGenerator&) = delete;
  void operator=(const svtkPDataSetGhostGenerator&) = delete;
};

#endif /* svtkPDataSetGhostGenerator_h */
