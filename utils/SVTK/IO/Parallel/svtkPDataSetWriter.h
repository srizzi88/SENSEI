/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPDataSetWriter
 * @brief   Manages writing pieces of a data set.
 *
 * svtkPDataSetWriter will write a piece of a file, and will also create
 * a metadata file that lists all of the files in a data set.
 */

#ifndef svtkPDataSetWriter_h
#define svtkPDataSetWriter_h

#include "svtkDataSetWriter.h"
#include "svtkIOParallelModule.h" // For export macro

#include <map>    // for keeping track of extents
#include <vector> // for keeping track of extents

class svtkImageData;
class svtkRectilinearGrid;
class svtkStructuredGrid;
class svtkMultiProcessController;

class SVTKIOPARALLEL_EXPORT svtkPDataSetWriter : public svtkDataSetWriter
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkPDataSetWriter, svtkDataSetWriter);
  static svtkPDataSetWriter* New();

  /**
   * Write the psvtk file and cooresponding svtk files.
   */
  int Write() override;

  //@{
  /**
   * This is how many pieces the whole data set will be divided into.
   */
  void SetNumberOfPieces(int num);
  svtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Extra ghost cells will be written out to each piece file
   * if this value is larger than 0.
   */
  svtkSetMacro(GhostLevel, int);
  svtkGetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * This is the range of pieces that that this writer is
   * responsible for writing.  All pieces must be written
   * by some process.  The process that writes piece 0 also
   * writes the psvtk file that lists all the piece file names.
   */
  svtkSetMacro(StartPiece, int);
  svtkGetMacro(StartPiece, int);
  svtkSetMacro(EndPiece, int);
  svtkGetMacro(EndPiece, int);
  //@}

  //@{
  /**
   * This file pattern uses the file name and piece number
   * to construct a file name for the piece file.
   */
  svtkSetStringMacro(FilePattern);
  svtkGetStringMacro(FilePattern);
  //@}

  //@{
  /**
   * This flag determines whether to use absolute paths for the piece files.
   * By default the pieces are put in the main directory, and the piece file
   * names in the meta data psvtk file are relative to this directory.
   * This should make moving the whole lot to another directory, an easier task.
   */
  svtkSetMacro(UseRelativeFileNames, svtkTypeBool);
  svtkGetMacro(UseRelativeFileNames, svtkTypeBool);
  svtkBooleanMacro(UseRelativeFileNames, svtkTypeBool);
  //@}

  //@{
  /**
   * Controller used to communicate data type of blocks.
   * By default, the global controller is used. If you want another
   * controller to be used, set it with this.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPDataSetWriter();
  ~svtkPDataSetWriter() override;

  ostream* OpenFile();
  int WriteUnstructuredMetaData(
    svtkDataSet* input, char* root, char* str, size_t strSize, ostream* fptr);
  int WriteImageMetaData(svtkImageData* input, char* root, char* str, size_t strSize, ostream* fptr);
  int WriteRectilinearGridMetaData(
    svtkRectilinearGrid* input, char* root, char* str, size_t strSize, ostream* fptr);
  int WriteStructuredGridMetaData(
    svtkStructuredGrid* input, char* root, char* str, size_t strSize, ostream* fptr);

  int StartPiece;
  int EndPiece;
  int NumberOfPieces;
  int GhostLevel;

  svtkTypeBool UseRelativeFileNames;

  char* FilePattern;

  void DeleteFiles();

  typedef std::map<int, std::vector<int> > ExtentsType;
  ExtentsType Extents;

  svtkMultiProcessController* Controller;

private:
  svtkPDataSetWriter(const svtkPDataSetWriter&) = delete;
  void operator=(const svtkPDataSetWriter&) = delete;
};

#endif
