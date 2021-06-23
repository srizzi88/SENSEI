/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXYZMolReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXYZMolReader
 * @brief   read Molecular Data files
 *
 * svtkXYZMolReader is a source object that reads Molecule files
 * The FileName must be specified
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class
 */

#ifndef svtkXYZMolReader_h
#define svtkXYZMolReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkMoleculeReaderBase.h"

class SVTKIOGEOMETRY_EXPORT svtkXYZMolReader : public svtkMoleculeReaderBase
{
public:
  svtkTypeMacro(svtkXYZMolReader, svtkMoleculeReaderBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkXYZMolReader* New();

  /**
   * Test whether the file with the given name can be read by this
   * reader.
   */
  virtual int CanReadFile(const char* name);

  //@{
  /**
   * Set the current time step. It should be greater than 0 and smaller than
   * MaxTimeStep.
   */
  svtkSetMacro(TimeStep, int);
  svtkGetMacro(TimeStep, int);
  //@}

  //@{
  /**
   * Get the maximum time step.
   */
  svtkGetMacro(MaxTimeStep, int);
  //@}

protected:
  svtkXYZMolReader();
  ~svtkXYZMolReader() override;

  void ReadSpecificMolecule(FILE* fp) override;

  /**
   * Get next line that is not a comment. It returns the beginning of data on
   * line (skips empty spaces)
   */
  char* GetNextLine(FILE* fp, char* line, int maxlen);

  int GetLine1(const char* line, int* cnt);
  int GetLine2(const char* line, char* name);
  int GetAtom(const char* line, char* atom, float* x);

  void InsertAtom(const char* atom, float* pos);

  svtkSetMacro(MaxTimeStep, int);

  int TimeStep;
  int MaxTimeStep;

private:
  svtkXYZMolReader(const svtkXYZMolReader&) = delete;
  void operator=(const svtkXYZMolReader&) = delete;
};

#endif
