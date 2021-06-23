/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLHyperTreeGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLHyperTreeGridReader
 * @brief   Read SVTK XML HyperTreeGrid files.
 *
 * svtkXMLHyperTreeGridReader reads the SVTK XML HyperTreeGrid file
 * format. The standard extension for this reader's file format is "htg".
 *
 * NOTE: HyperTree exists as separate units with all data within htg
 *       But each htg file is considered one piece for the parallel reader
 *       Later may want to treat individual HyperTrees as separate pieces.
 *
 * For developpers:
 * To ensure the durability of this storage format over time, at least,
 * the drive must continue to support playback of previous format.
 *
 * Understand:
 * - version 0.0 (P. Fasel and D. DeMarle Kitware US)
 * - version 1.0 (J-B Lekien CEA, DAM, DIF, F-91297 Arpajon, France)
 *   This version of the format offers extensive loading options.
 *   With these options, regardless of the size of the backed-up mesh,
 *   it is possible to view a "reduced" version either by setting the
 *   maximum level (by SetFixedLevel) or/and setting the HyperTrees
 *   to load (by SetCoordinatesBoundingBox, SetIndicesBoundingBox,
 *   ClearAndAddSelectedHT and AddSelectedHT.
 */

#ifndef svtkXMLHyperTreeGridReader_h
#define svtkXMLHyperTreeGridReader_h

#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLReader.h"

#include <limits.h> // Use internal
#include <map>      // Use internal

class svtkBitArray;
class svtkHyperTree;
class svtkHyperTreeGrid;
class svtkHyperTreeGridNonOrientedCursor;
class svtkIdTypeArray;

class SVTKIOXML_EXPORT svtkXMLHyperTreeGridReader : public svtkXMLReader
{
public:
  svtkTypeMacro(svtkXMLHyperTreeGridReader, svtkXMLReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLHyperTreeGridReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  svtkHyperTreeGrid* GetOutput();
  svtkHyperTreeGrid* GetOutput(int idx);
  //@}

  //@{
  /**
   * Set/Get the fixed level to read.
   * Option avaiblable in 1.0
   */
  svtkSetMacro(FixedLevel, unsigned int);
  svtkGetMacro(FixedLevel, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the selected HyperTrees (HTs) to read :
   * by default, all Hts, or
   * by set coordinates bounding box, exclusive or
   * by set indices coordinates bounding box, exclusive or
   * by set indices HTs (ClearAndAdd and more Add).
   * Only available for files whose major version > 1
   * Option avaiblable in 1.0
   */
  void SetCoordinatesBoundingBox(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);

  void SetIndicesBoundingBox(unsigned int imin, unsigned int imax, unsigned int jmin,
    unsigned int jmax, unsigned int kmin, unsigned int kmax);

  void ClearAndAddSelectedHT(unsigned int idg, unsigned int fixedLevel = UINT_MAX);
  void AddSelectedHT(unsigned int idg, unsigned int fixedLevel = UINT_MAX);
  //@}

  // These defer to the HyperTreeGrid output.
  svtkIdType GetNumberOfPoints();

  svtkIdType GetNumberOfPieces();

  void SetupUpdateExtent(int piece, int numberOfPieces);

  void CopyOutputInformation(svtkInformation* outInfo, int port) override;

  // The most important stuff is here.
  // Read the rest of the file and create the HyperTreeGrid.
  void ReadXMLData() override;

protected:
  svtkXMLHyperTreeGridReader();
  ~svtkXMLHyperTreeGridReader() override;

  // Finalize the selected HyperTrees by, for example, transform
  // coordinates bounding box in indices coordinates bounding box
  // after initialize HyperTreeGrid.
  void CalculateHTs(const svtkHyperTreeGrid* grid);

  // Return true if HyperTree identified by treeIndx is selected for
  // the load.
  bool IsSelectedHT(const svtkHyperTreeGrid* grid, unsigned int treeIndx) const;

  // Return the fixedLevel choice for this HyperTree
  svtkIdType GetFixedLevelOfThisHT(svtkIdType numberOfLevels, unsigned int treeIndx) const;

  const char* GetDataSetName() override;

  void DestroyPieces();

  void GetOutputUpdateExtent(int& piece, int& numberOfPieces);

  // Setup the output with no data available.  Used in error cases.
  void SetupEmptyOutput() override;

  // Initialize the total number of vertices
  void SetupOutputTotals();

  // Initialize global start of next piece
  void SetupNextPiece();

  // Initialize current output data
  void SetupOutputData() override;

  // Setup the output's information
  void SetupOutputInformation(svtkInformation* outInfo) override;

  // Setup the number of pieces
  void SetupPieces(int numPieces);

  // Pipeline execute data driver called by svtkXMLReader
  int ReadPrimaryElement(svtkXMLDataElement* ePrimary) override;

  // Declare that this reader produces HyperTreeGrids
  int FillOutputPortInformation(int, svtkInformation*) override;

  // Read the coordinates describing the grid
  void ReadGrid(svtkXMLDataElement* elem);

  //----------- Used for the major version < 1

  // Recover the structure of the HyperTreeGrid, used by ReadXMLData.
  void ReadTrees_0(svtkXMLDataElement* elem);

  // Used by ReadTopology to recursively build the tree
  void SubdivideFromDescriptor_0(svtkHyperTreeGridNonOrientedCursor* treeCursor, unsigned int level,
    int numChildren, svtkBitArray* desc, svtkIdTypeArray* posByLevel);

  //---------- Used for other the major version

  // Recover the structure of the HyperTreeGrid, used by ReadXMLData.
  void ReadTrees_1(svtkXMLDataElement* elem);

  // Number of vertices in HyperTreeGrid being read
  svtkIdType NumberOfPoints;
  svtkIdType NumberOfPieces;

  // Fixed the load maximum level
  unsigned int FixedLevel = UINT_MAX;

  bool Verbose = false;

  bool FixedHTs = false;
  enum SelectedType
  {
    ALL,
    COORDINATES_BOUNDING_BOX,
    INDICES_BOUNDING_BOX,
    IDS_SELECTED
  };
  SelectedType SelectedHTs = ALL;

  // Selected HTs by coordinates of bounding box
  double CoordinatesBoundingBox[6];
  // Selected HTs by indice coordinate of bounding box
  unsigned int IndicesBoundingBox[6];
  // Selected HTs by indice of HTs in the map.
  // The value is the fixedLevel, but if this value is
  // UINT_MAX, this is FixedLevel that is used.
  std::map<unsigned int, unsigned int> IdsSelected;

  int UpdatedPiece;
  int UpdateNumberOfPieces;

  int StartPiece;
  int EndPiece;
  int Piece;

private:
  svtkXMLHyperTreeGridReader(const svtkXMLHyperTreeGridReader&) = delete;
  void operator=(const svtkXMLHyperTreeGridReader&) = delete;
};

#endif
