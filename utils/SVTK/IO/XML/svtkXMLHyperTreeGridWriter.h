/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLHyperTreeGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLHyperTreeGridWriter
 * @brief   Write SVTK XML HyperTreeGrid files.
 *
 * svtkXMLHyperTreeGridWriter writes the SVTK XML HyperTreeGrid file
 * format. The standard extension for this writer's file format is "htg".
 *
 * Note for developers:
 * The SVTK XML HyperTreeGrid file format is versioned.
 * Any evolution of the format must lead to:
 * - a move to a higher major version number, X+1.0, if the evolution is
 *   incompatible with the previous versions either at the level of the
 *   description of the information or the semantic understanding made by
 *   the reader;
 * - a move to a higher minor version number, X.y+1, if its consists of
 *   adding information without calling into question the general
 *   interpretation.
 *
 * Version 0.0 (P. Fasel and D. DeMarle)
 * ...
 * Version 1.0 (J-B Lekien CEA, DAM, DIF, F-91297 Arpajon, France)
 * - writing by HyperTree description and fields;
 * - saving the minimal tree (a hidden refined node becomes a hidden leaf node);
 * - saving the tree by level of refinement (course in width);
 * - the last null values in the binary description of the tree or mask (if
 *   defined) may not / are not explicitly described. The size of the table
 *   given elsewhere is authentic;
 * - all fields are copied to be saved in the implicit order, so even
 *   if an explicit global index map exists, it disappears;
 * - writing in this version requires more memory and CPU;
 * - reading of a part is accelerated (non iterative construction of the tree)
 *   and consumes potentially less memory (suppression of the global index
 *   map explicit);
 * - expanded possibility at the reader level, today these options allow to
 *   accelerate the obtaining of a result which will be less precise and to
 *   allow the loading of a part of a mesh which would not hold in memory:
 *      - loading by limiting the maximum level to load;
 *      - loading by selecting (differentes description possibilities are
 *        offered) the HTs to take into account.
 *
 * The default version of the SVTK XML HyperTreeGrid file format is the latest
 * version, now version 1.0.
 *
 * For developpers:
 * To ensure the durability of this storage format over time, at least, the drive
 * must continue to support playback of previous format.
 */

#ifndef svtkXMLHyperTreeGridWriter_h
#define svtkXMLHyperTreeGridWriter_h

#include "svtkBitArray.h"    // For ivar
#include "svtkIOXMLModule.h" // For export macro
#include "svtkXMLWriter.h"

#include <vector> // std::vector

class OffsetsManagerGroup;
class OffsetsManagerArray;
class svtkHyperTree;
class svtkHyperTreeGrid;
class svtkHyperTreeGridNonOrientedCursor;
class svtkUnsignedLongArray;

class SVTKIOXML_EXPORT svtkXMLHyperTreeGridWriter : public svtkXMLWriter
{
public:
  svtkTypeMacro(svtkXMLHyperTreeGridWriter, svtkXMLWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLHyperTreeGridWriter* New();

  /**
   * Get/Set the writer's input.
   */
  svtkHyperTreeGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

  /**
   * Methods to define the file's major and minor version numbers.
   * Major version incremented since v0.1 hypertreegrid data readers
   * cannot read the files written by this new reader.
   * A version is defined by defect, so there is no need to call
   * this function.
   * The default choice is usually the best choice.
   */
  svtkSetMacro(DataSetMajorVersion, int);
  svtkSetMacro(DataSetMinorVersion, int);

protected:
  svtkXMLHyperTreeGridWriter();
  ~svtkXMLHyperTreeGridWriter() override;

  const char* GetDataSetName() override;

  /**
   * Methods to define the file's major and minor version numbers.
   * Major version incremented since v0.1 hypertreegrid data readers
   * cannot read the files written by this new reader.
   * Actually, the default version is 1.0
   */
  int GetDataSetMajorVersion() override { return DataSetMajorVersion; }
  int GetDataSetMinorVersion() override { return DataSetMinorVersion; }

  // Specify that we require HyperTreeGrid input
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // The most important method, make the XML file for my input.
  int WriteData() override;

  // <HyperTreeGrid ...
  int StartPrimaryElement(svtkIndent);

  // ... dim, size, origin>
  void WritePrimaryElementAttributes(ostream&, svtkIndent) override;

  // Grid coordinates and mask
  int WriteGrid(svtkIndent);

  // Tree Descriptor and  PointData
  int WriteTrees_0(svtkIndent);
  int WriteTrees_1(svtkIndent);

  // </HyperTreeGrid>
  int FinishPrimaryElement(svtkIndent);

  // Descriptors for individual hypertrees
  std::vector<svtkBitArray*> Descriptors;

  // Descriptors for individual hypertrees
  std::vector<svtkUnsignedLongArray*> NbVerticesbyLevels;

  // Masks for individual hypertrees
  std::vector<svtkBitArray*> Masks;

  // Ids (index selection) for individual hypertrees
  std::vector<svtkIdList*> Ids;

  // Helper to simplify writing appended array data
  void WriteAppendedArrayDataHelper(svtkAbstractArray* array, OffsetsManager& offsets);

  void WritePointDataAppendedArrayDataHelper(
    svtkAbstractArray* array, svtkIdType treeCount, OffsetsManager& offsets, svtkHyperTree* tree);

  OffsetsManagerGroup* CoordsOMG;
  OffsetsManagerGroup* DescriptorOMG;
  OffsetsManagerGroup* NbVerticesbyLevelOMG;
  OffsetsManagerGroup* MaskOMG;
  OffsetsManagerGroup* PointDataOMG;

  int NumberOfTrees;

  // Default choice
  int DataSetMajorVersion = 1;
  int DataSetMinorVersion = 0;

private:
  svtkXMLHyperTreeGridWriter(const svtkXMLHyperTreeGridWriter&) = delete;
  void operator=(const svtkXMLHyperTreeGridWriter&) = delete;
};

#endif
