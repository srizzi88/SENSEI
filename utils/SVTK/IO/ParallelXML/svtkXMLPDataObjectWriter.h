/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPDataObjectWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPDataObjectWriter
 * @brief   Write data in a parallel XML format.
 *
 * svtkXMLPDataWriter is the superclass for all XML parallel data object
 * writers.  It provides functionality needed for writing parallel
 * formats, such as the selection of which writer writes the summary
 * file and what range of pieces are assigned to each serial writer.
 *
 * @sa
 * svtkXMLDataObjectWriter
 */

#ifndef svtkXMLPDataObjectWriter_h
#define svtkXMLPDataObjectWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLWriter.h"

class svtkCallbackCommand;
class svtkMultiProcessController;

class SVTKIOPARALLELXML_EXPORT svtkXMLPDataObjectWriter : public svtkXMLWriter
{
public:
  svtkTypeMacro(svtkXMLPDataObjectWriter, svtkXMLWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the number of pieces that are being written in parallel.
   */
  svtkSetMacro(NumberOfPieces, int);
  svtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Get/Set the range of pieces assigned to this writer.
   */
  svtkSetMacro(StartPiece, int);
  svtkGetMacro(StartPiece, int);
  svtkSetMacro(EndPiece, int);
  svtkGetMacro(EndPiece, int);
  //@}

  //@{
  /**
   * Get/Set the ghost level used for this writer's piece.
   */
  svtkSetMacro(GhostLevel, int);
  svtkGetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * Get/Set whether to use a subdirectory to store the pieces
   */
  svtkSetMacro(UseSubdirectory, bool);
  svtkGetMacro(UseSubdirectory, bool);
  //@}

  //@{
  /**
   * Get/Set whether the writer should write the summary file that
   * refers to all of the pieces' individual files.
   * This is on by default. Note that only the first process writes
   * the summary file.
   */
  virtual void SetWriteSummaryFile(int flag);
  svtkGetMacro(WriteSummaryFile, int);
  svtkBooleanMacro(WriteSummaryFile, int);
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

  /**
   * Overridden to handle passing the CONTINUE_EXECUTING() flags to the
   * executive.
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkXMLPDataObjectWriter();
  ~svtkXMLPDataObjectWriter() override;

  /**
   * Override writing method from superclass.
   */
  int WriteInternal() override;

  /**
   * Write data from the input dataset. Call WritePData(svtkIndent indent)
   */
  int WriteData() override;

  /**
   * Write Data associated with the input dataset. It needs to be overridden by subclass
   */
  virtual void WritePData(svtkIndent indent) = 0;

  /**
   * Write a piece of the dataset on disk. Called by WritePieceInternal().
   * It needs to be overridden by subclass
   */
  virtual int WritePiece(int index) = 0;

  /**
   * Method called by WriteInternal(). It's used for writing a piece of the dataset.
   * It needs to be overridden by subclass.
   */
  virtual int WritePieceInternal() = 0;

  /**
   * Overridden to make appropriate piece request from upstream.
   */
  virtual int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  /**
   * Collect information between ranks before writing the summary file.
   * This method is called on all ranks while summary file is only written on 1
   * rank (rank 0).
   */
  virtual void PrepareSummaryFile();

  /**
   * Write the attributes of the piece at the given index
   */
  virtual void WritePPieceAttributes(int index);

  //@{
  /**
   * Methods for creating a filename for each piece in the dataset
   */
  char* CreatePieceFileName(int index, const char* path = nullptr);
  void SplitFileName();
  //@}

  /**
   * Callback registered with the InternalProgressObserver.
   */
  static void ProgressCallbackFunction(svtkObject*, unsigned long, void*, void*);

  /**
   * Valid at end of WriteInternal to indicate if we're going to continue
   * execution.
   */
  svtkGetMacro(ContinuingExecution, bool);

  /**
   * Get the current piece to write
   */
  svtkGetMacro(CurrentPiece, int);

  /**
   * Progress callback from internal writer.
   */
  virtual void ProgressCallback(svtkAlgorithm* w);

  /**
   * Method used to delete all written files.
   */
  void DeleteFiles();

  /**
   * The observer to report progress from the internal writer.
   */
  svtkCallbackCommand* InternalProgressObserver;

  svtkMultiProcessController* Controller;

  int StartPiece;
  int EndPiece;
  int NumberOfPieces;
  int GhostLevel;
  int WriteSummaryFile;
  bool UseSubdirectory;

  char* PathName;
  char* FileNameBase;
  char* FileNameExtension;
  char* PieceFileNameExtension;

  /**
   * Flags used to keep track of which pieces were written out.
   */
  unsigned char* PieceWrittenFlags;

  /**
   * Initializes PieceFileNameExtension.
   */
  virtual void SetupPieceFileNameExtension();

private:
  svtkXMLPDataObjectWriter(const svtkXMLPDataObjectWriter&) = delete;
  void operator=(const svtkXMLPDataObjectWriter&) = delete;

  /**
   * Indicates the piece currently being written.
   */
  int CurrentPiece;

  /**
   * Set in WriteInternal() to request continued execution from the executive to
   * write more pieces.
   */
  bool ContinuingExecution;
};

#endif
