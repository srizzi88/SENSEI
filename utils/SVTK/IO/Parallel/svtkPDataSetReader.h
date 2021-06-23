/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPDataSetReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPDataSetReader
 * @brief   Manages reading pieces of a data set.
 *
 * svtkPDataSetReader will read a piece of a file, it takes as input
 * a metadata file that lists all of the files in a data set.
 */

#ifndef svtkPDataSetReader_h
#define svtkPDataSetReader_h

#include "svtkDataSetAlgorithm.h"
#include "svtkIOParallelModule.h" // For export macro

class svtkDataSet;

class SVTKIOPARALLEL_EXPORT svtkPDataSetReader : public svtkDataSetAlgorithm
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkPDataSetReader, svtkDataSetAlgorithm);
  static svtkPDataSetReader* New();

  //@{
  /**
   * This file to open and read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * This is set when UpdateInformation is called.
   * It shows the type of the output.
   */
  svtkGetMacro(DataType, int);
  //@}

  /**
   * Called to determine if the file can be read by the reader.
   */
  int CanReadFile(const char* filename);

protected:
  svtkPDataSetReader();
  ~svtkPDataSetReader() override;

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  void ReadPSVTKFileInformation(istream* fp, svtkInformation* request,
    svtkInformationVector** inputVector, svtkInformationVector* outputVector);
  void ReadSVTKFileInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int PolyDataExecute(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  int UnstructuredGridExecute(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  int ImageDataExecute(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  int StructuredGridExecute(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  void CoverExtent(int ext[6], int* pieceMask);

  svtkDataSet* CheckOutput();
  void SetNumberOfPieces(int num);

  istream* OpenFile(const char*);

  int ReadXML(istream* file, char** block, char** param, char** value);
  int SVTKFileFlag;
  int StructuredFlag;
  char* FileName;
  int DataType;
  int NumberOfPieces;
  char** PieceFileNames;
  int** PieceExtents;

private:
  svtkPDataSetReader(const svtkPDataSetReader&) = delete;
  void operator=(const svtkPDataSetReader&) = delete;
};

#endif
