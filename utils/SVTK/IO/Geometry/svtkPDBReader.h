/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPDBReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPDBReader
 * @brief   read Molecular Data files
 *
 * svtkPDBReader is a source object that reads Molecule files
 * The FileName must be specified
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class
 */

#ifndef svtkPDBReader_h
#define svtkPDBReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkMoleculeReaderBase.h"

class SVTKIOGEOMETRY_EXPORT svtkPDBReader : public svtkMoleculeReaderBase
{
public:
  svtkTypeMacro(svtkPDBReader, svtkMoleculeReaderBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPDBReader* New();

protected:
  svtkPDBReader();
  ~svtkPDBReader() override;

  void ReadSpecificMolecule(FILE* fp) override;

private:
  svtkPDBReader(const svtkPDBReader&) = delete;
  void operator=(const svtkPDBReader&) = delete;
};

#endif
