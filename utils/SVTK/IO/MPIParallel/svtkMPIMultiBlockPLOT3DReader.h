/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPIMultiBlockPLOT3DReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMPIMultiBlockPLOT3DReader
 * @brief   svtkMultiBlockPLOT3DReader subclass that
 * uses MPI-IO to efficiently read binary files for 3D domains in parallel using
 * MPI-IO.
 *
 * svtkMPIMultiBlockPLOT3DReader extends svtkMultiBlockPLOT3DReader to use MPI-IO
 * instead of POSIX IO to read file in parallel.
 */

#ifndef svtkMPIMultiBlockPLOT3DReader_h
#define svtkMPIMultiBlockPLOT3DReader_h

#include "svtkIOMPIParallelModule.h" // For export macro
#include "svtkMultiBlockPLOT3DReader.h"

class SVTKIOMPIPARALLEL_EXPORT svtkMPIMultiBlockPLOT3DReader : public svtkMultiBlockPLOT3DReader
{
public:
  static svtkMPIMultiBlockPLOT3DReader* New();
  svtkTypeMacro(svtkMPIMultiBlockPLOT3DReader, svtkMultiBlockPLOT3DReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Use this to override using MPI-IO. When set to false (default is true),
   * this class will simply forward all method calls to the superclass.
   */
  svtkSetMacro(UseMPIIO, bool);
  svtkGetMacro(UseMPIIO, bool);
  svtkBooleanMacro(UseMPIIO, bool);
  //@}

protected:
  svtkMPIMultiBlockPLOT3DReader();
  ~svtkMPIMultiBlockPLOT3DReader() override;

  /**
   * Determines we should use MPI-IO for the current file. We don't use MPI-IO
   * for 2D files or ASCII files.
   */
  bool CanUseMPIIO();

  virtual int OpenFileForDataRead(void*& fp, const char* fname) override;
  virtual void CloseFile(void* fp) override;

  virtual int ReadIntScalar(void* vfp, int extent[6], int wextent[6], svtkDataArray* scalar,
    svtkTypeUInt64 offset, const svtkMultiBlockPLOT3DReaderRecord& currentRecord) override;
  virtual int ReadScalar(void* vfp, int extent[6], int wextent[6], svtkDataArray* scalar,
    svtkTypeUInt64 offset, const svtkMultiBlockPLOT3DReaderRecord& currentRecord) override;
  virtual int ReadVector(void* vfp, int extent[6], int wextent[6], int numDims,
    svtkDataArray* vector, svtkTypeUInt64 offset,
    const svtkMultiBlockPLOT3DReaderRecord& currentRecord) override;
  bool UseMPIIO;

private:
  svtkMPIMultiBlockPLOT3DReader(const svtkMPIMultiBlockPLOT3DReader&) = delete;
  void operator=(const svtkMPIMultiBlockPLOT3DReader&) = delete;
};

#endif
