/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataSetMapper
 * @brief   map svtkDataSet and derived classes to graphics primitives
 *
 * svtkDataSetMapper is a mapper to map data sets (i.e., svtkDataSet and
 * all derived classes) to graphics primitives. The mapping procedure
 * is as follows: all 0D, 1D, and 2D cells are converted into points,
 * lines, and polygons/triangle strips and then mapped to the graphics
 * system. The 2D faces of 3D cells are mapped only if they are used by
 * only one cell, i.e., on the boundary of the data set.
 */

#ifndef svtkDataSetMapper_h
#define svtkDataSetMapper_h

#include "svtkMapper.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkPolyDataMapper;
class svtkDataSetSurfaceFilter;

class SVTKRENDERINGCORE_EXPORT svtkDataSetMapper : public svtkMapper
{
public:
  static svtkDataSetMapper* New();
  svtkTypeMacro(svtkDataSetMapper, svtkMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  void Render(svtkRenderer* ren, svtkActor* act) override;

  //@{
  /**
   * Get the internal poly data mapper used to map data set to graphics system.
   */
  svtkGetObjectMacro(PolyDataMapper, svtkPolyDataMapper);
  //@}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Get the mtime also considering the lookup table.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set the Input of this mapper.
   */
  void SetInputData(svtkDataSet* input);
  svtkDataSet* GetInput();
  //@}

protected:
  svtkDataSetMapper();
  ~svtkDataSetMapper() override;

  svtkDataSetSurfaceFilter* GeometryExtractor;
  svtkPolyDataMapper* PolyDataMapper;

  void ReportReferences(svtkGarbageCollector*) override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkDataSetMapper(const svtkDataSetMapper&) = delete;
  void operator=(const svtkDataSetMapper&) = delete;
};

#endif
