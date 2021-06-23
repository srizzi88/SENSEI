/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPWindBladeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPWindBladeReader
 * @brief   class for reading WindBlade data files
 *
 * svtkPWindBladeReader is a source object that reads WindBlade files
 * which are block binary files with tags before and after each block
 * giving the number of bytes within the block.  The number of data
 * variables dumped varies.  There are 3 output ports with the first
 * being a structured grid with irregular spacing in the Z dimension.
 * The second is an unstructured grid only read on on process 0 and
 * used to represent the blade.  The third is also a structured grid
 * with irregular spacing on the Z dimension.  Only the first and
 * second output ports have time dependent data.
 * Parallel version of svtkWindBladeReader.h
 */

#ifndef svtkPWindBladeReader_h
#define svtkPWindBladeReader_h

#include "svtkIOMPIParallelModule.h" // For export macro
#include "svtkWindBladeReader.h"

class PWindBladeReaderInternal;

class SVTKIOMPIPARALLEL_EXPORT svtkPWindBladeReader : public svtkWindBladeReader
{
public:
  static svtkPWindBladeReader* New();
  svtkTypeMacro(svtkPWindBladeReader, svtkWindBladeReader);

  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkPWindBladeReader();
  ~svtkPWindBladeReader() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  virtual void CalculatePressure(int pressure, int prespre, int tempg, int density) override;
  virtual void CalculateVorticity(int vort, int uvw, int density) override;
  virtual void LoadVariableData(int var) override;
  virtual bool ReadGlobalData() override;
  virtual bool FindVariableOffsets() override;
  virtual void CreateZTopography(float* zValues) override;
  virtual void SetupBladeData() override;
  virtual void LoadBladeData(int timeStep) override;

private:
  PWindBladeReaderInternal* PInternal;

  svtkPWindBladeReader(const svtkPWindBladeReader&) = delete;
  void operator=(const svtkPWindBladeReader&) = delete;
};

#endif
