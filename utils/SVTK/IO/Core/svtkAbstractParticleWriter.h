/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractParticleWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAbstractParticleWriter
 * @brief   abstract class to write particle data to file
 *
 * svtkAbstractParticleWriter is an abstract class which is used by
 * svtkParticleTracerBase to write particles out during simulations.
 * This class is abstract and provides a TimeStep and FileName.
 * Subclasses of this should provide the necessary IO.
 *
 * @warning
 * See svtkWriter
 *
 * @sa
 * svtkParticleTracerBase
 */

#ifndef svtkAbstractParticleWriter_h
#define svtkAbstractParticleWriter_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkWriter.h"

class SVTKIOCORE_EXPORT svtkAbstractParticleWriter : public svtkWriter
{
public:
  svtkTypeMacro(svtkAbstractParticleWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get the TimeStep that is being written
   */
  svtkSetMacro(TimeStep, int);
  svtkGetMacro(TimeStep, int);
  //@}

  //@{
  /**
   * Before writing the current data out, set the TimeValue (optional)
   * The TimeValue is a float/double value that corresponds to the real
   * time of the data, it may not be regular, whereas the TimeSteps
   * are simple increments.
   */
  svtkSetMacro(TimeValue, double);
  svtkGetMacro(TimeValue, double);
  //@}

  //@{
  /**
   * Set/get the FileName that is being written to
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * When running in parallel, this writer may be capable of
   * Collective IO operations (HDF5). By default, this is off.
   */
  svtkSetMacro(CollectiveIO, int);
  svtkGetMacro(CollectiveIO, int);
  void SetWriteModeToCollective();
  void SetWriteModeToIndependent();
  //@}

  /**
   * Close the file after a write. This is optional but
   * may protect against data loss in between steps
   */
  virtual void CloseFile() = 0;

protected:
  svtkAbstractParticleWriter();
  ~svtkAbstractParticleWriter() override;

  void WriteData() override = 0; // internal method subclasses must respond to
  int CollectiveIO;
  int TimeStep;
  double TimeValue;
  char* FileName;

private:
  svtkAbstractParticleWriter(const svtkAbstractParticleWriter&) = delete;
  void operator=(const svtkAbstractParticleWriter&) = delete;
};

#endif
