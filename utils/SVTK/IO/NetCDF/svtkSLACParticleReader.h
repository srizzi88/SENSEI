// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSLACParticleReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   svtkSLACParticleReader
 *
 *
 *
 * A reader for a data format used by Omega3p, Tau3p, and several other tools
 * used at the Standford Linear Accelerator Center (SLAC).  The underlying
 * format uses netCDF to store arrays, but also imposes some conventions
 * to store a list of particles in 3D space.
 *
 * This reader supports pieces, but in actuality only loads anything in
 * piece 0.  All other pieces are empty.
 *
 */

#ifndef svtkSLACParticleReader_h
#define svtkSLACParticleReader_h

#include "svtkIONetCDFModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkDataArraySelection;
class svtkIdTypeArray;
class svtkInformationIntegerKey;
class svtkInformationObjectBaseKey;

class SVTKIONETCDF_EXPORT svtkSLACParticleReader : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkSLACParticleReader, svtkPolyDataAlgorithm);
  static svtkSLACParticleReader* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkGetStringMacro(FileName);
  svtkSetStringMacro(FileName);

  /**
   * Returns true if the given file can be read by this reader.
   */
  static int CanReadFile(const char* filename);

protected:
  svtkSLACParticleReader();
  ~svtkSLACParticleReader() override;

  char* FileName;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Convenience function that checks the dimensions of a 2D netCDF array that
   * is supposed to be a set of tuples.  It makes sure that the number of
   * dimensions is expected and that the number of components in each tuple
   * agree with what is expected.  It then returns the number of tuples.  An
   * error is emitted and 0 is returned if the checks fail.
   */
  virtual svtkIdType GetNumTuplesInVariable(int ncFD, int varId, int expectedNumComponents);

private:
  svtkSLACParticleReader(const svtkSLACParticleReader&) = delete;
  void operator=(const svtkSLACParticleReader&) = delete;
};

#endif // svtkSLACParticleReader_h
