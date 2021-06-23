/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEnSightMasterServerReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEnSightMasterServerReader
 * @brief   reader for compound EnSight files
 */

#ifndef svtkEnSightMasterServerReader_h
#define svtkEnSightMasterServerReader_h

#include "svtkGenericEnSightReader.h"
#include "svtkIOEnSightModule.h" // For export macro

class svtkCollection;

class SVTKIOENSIGHT_EXPORT svtkEnSightMasterServerReader : public svtkGenericEnSightReader
{
public:
  svtkTypeMacro(svtkEnSightMasterServerReader, svtkGenericEnSightReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkEnSightMasterServerReader* New();

  /**
   * Determine which file should be read for piece
   */
  int DetermineFileName(int piece);

  //@{
  /**
   * Get the file name that will be read.
   */
  svtkGetStringMacro(PieceCaseFileName);
  //@}

  //@{
  /**
   * Set or get the current piece.
   */
  svtkSetMacro(CurrentPiece, int);
  svtkGetMacro(CurrentPiece, int);
  //@}

  int CanReadFile(const char* fname) override;

protected:
  svtkEnSightMasterServerReader();
  ~svtkEnSightMasterServerReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkSetStringMacro(PieceCaseFileName);
  char* PieceCaseFileName;
  int MaxNumberOfPieces;
  int CurrentPiece;

private:
  svtkEnSightMasterServerReader(const svtkEnSightMasterServerReader&) = delete;
  void operator=(const svtkEnSightMasterServerReader&) = delete;
};

#endif
