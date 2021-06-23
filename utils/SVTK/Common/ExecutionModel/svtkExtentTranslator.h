/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtentTranslator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtentTranslator
 * @brief   Generates a structured extent from unstructured.
 *
 *
 * svtkExtentTranslator generates a structured extent from an unstructured
 * extent.  It uses a recursive scheme that splits the largest axis.  A hard
 * coded extent can be used for a starting point.
 */

#ifndef svtkExtentTranslator_h
#define svtkExtentTranslator_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkObject.h"

class svtkInformationIntegerRequestKey;
class svtkInformationIntegerKey;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkExtentTranslator : public svtkObject
{
public:
  static svtkExtentTranslator* New();

  svtkTypeMacro(svtkExtentTranslator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the Piece/NumPieces. Set the WholeExtent and then call PieceToExtent.
   * The result can be obtained from the Extent ivar.
   */
  svtkSetVector6Macro(WholeExtent, int);
  svtkGetVector6Macro(WholeExtent, int);
  svtkSetVector6Macro(Extent, int);
  svtkGetVector6Macro(Extent, int);
  svtkSetMacro(Piece, int);
  svtkGetMacro(Piece, int);
  svtkSetMacro(NumberOfPieces, int);
  svtkGetMacro(NumberOfPieces, int);
  svtkSetMacro(GhostLevel, int);
  svtkGetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * These are the main methods that should be called. These methods
   * are responsible for converting a piece to an extent. The signatures
   * without arguments are only thread safe when each thread accesses a
   * different instance. The signatures with arguments are fully thread
   * safe.
   */
  virtual int PieceToExtent();
  virtual int PieceToExtentByPoints();
  virtual int PieceToExtentThreadSafe(int piece, int numPieces, int ghostLevel, int* wholeExtent,
    int* resultExtent, int splitMode, int byPoints);
  //@}

  /**
   * How should the streamer break up extents. Block mode
   * tries to break an extent up into cube blocks.  It always chooses
   * the largest axis to split.
   * Slab mode first breaks up the Z axis.  If it gets to one slice,
   * then it starts breaking up other axes.
   */
  void SetSplitModeToBlock() { this->SplitMode = svtkExtentTranslator::BLOCK_MODE; }
  void SetSplitModeToXSlab() { this->SplitMode = svtkExtentTranslator::X_SLAB_MODE; }
  void SetSplitModeToYSlab() { this->SplitMode = svtkExtentTranslator::Y_SLAB_MODE; }
  void SetSplitModeToZSlab() { this->SplitMode = svtkExtentTranslator::Z_SLAB_MODE; }
  svtkGetMacro(SplitMode, int);

  /**
   * By default the translator creates N structured subextents by repeatedly
   * splitting the largest current dimension until there are N pieces.
   * If you do not want it always split the largest dimension, for instance when the
   * shortest dimension is the slowest changing and thus least coherent in memory,
   * use this to tell the translator which dimensions to split.
   */
  void SetSplitPath(int len, int* splitpath);

  // Don't change the numbers here - they are used in the code
  // to indicate array indices.
  enum Modes
  {
    X_SLAB_MODE = 0,
    Y_SLAB_MODE = 1,
    Z_SLAB_MODE = 2,
    BLOCK_MODE = 3
  };

  /**
   * Key used to request a particular split mode.
   * This is used by svtkStreamingDemandDrivenPipeline.
   */
  static svtkInformationIntegerRequestKey* UPDATE_SPLIT_MODE();

protected:
  svtkExtentTranslator();
  ~svtkExtentTranslator() override;

  static svtkInformationIntegerKey* DATA_SPLIT_MODE();

  friend class svtkInformationSplitModeRequestKey;

  //@{
  /**
   * Returns 0 if no data exist for a piece.
   * The whole extent Should be passed in as the extent.
   * It is modified to return the result.
   */
  int SplitExtent(int piece, int numPieces, int* extent, int splitMode);
  int SplitExtentByPoints(int piece, int numPieces, int* extent, int splitMode);
  //@}

  int Piece;
  int NumberOfPieces;
  int GhostLevel;
  int Extent[6];
  int WholeExtent[6];
  int SplitMode;

  int* SplitPath;
  int SplitLen;

private:
  svtkExtentTranslator(const svtkExtentTranslator&) = delete;
  void operator=(const svtkExtentTranslator&) = delete;
};

#endif
