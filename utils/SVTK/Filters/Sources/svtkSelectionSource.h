/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSelectionSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSelectionSource
 * @brief   Generate selection from given set of ids
 * svtkSelectionSource generates a svtkSelection from a set of
 * (piece id, cell id) pairs. It will only generate the selection values
 * that match UPDATE_PIECE_NUMBER (i.e. piece == UPDATE_PIECE_NUMBER).
 *
 * User-supplied, application-specific selections (with a ContentType of
 * svtkSelectionNode::USER) are not supported.
 */

#ifndef svtkSelectionSource_h
#define svtkSelectionSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkSelectionAlgorithm.h"

class svtkSelectionSourceInternals;

class SVTKFILTERSSOURCES_EXPORT svtkSelectionSource : public svtkSelectionAlgorithm
{
public:
  static svtkSelectionSource* New();
  svtkTypeMacro(svtkSelectionSource, svtkSelectionAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Add a (piece, id) to the selection set. The source will generate
   * only the ids for which piece == UPDATE_PIECE_NUMBER.
   * If piece == -1, the id applies to all pieces.
   */
  void AddID(svtkIdType piece, svtkIdType id);
  void AddStringID(svtkIdType piece, const char* id);
  //@}

  /**
   * Add a point in world space to probe at.
   */
  void AddLocation(double x, double y, double z);

  /**
   * Add a value range to threshold within.
   */
  void AddThreshold(double min, double max);

  /**
   * Set a frustum to choose within.
   */
  void SetFrustum(double* vertices);

  /**
   * Add the flat-index/composite index for a block.
   */
  void AddBlock(svtkIdType blockno);

  //@{
  /**
   * Removes all IDs.
   */
  void RemoveAllIDs();
  void RemoveAllStringIDs();
  //@}

  /**
   * Remove all thresholds added with AddThreshold.
   */
  void RemoveAllThresholds();

  /**
   * Remove all locations added with AddLocation.
   */
  void RemoveAllLocations();

  /**
   * Remove all blocks added with AddBlock.
   */
  void RemoveAllBlocks();

  //@{
  /**
   * Set the content type for the generated selection.
   * Possible values are as defined by
   * svtkSelection::SelectionContent.
   */
  svtkSetMacro(ContentType, int);
  svtkGetMacro(ContentType, int);
  //@}

  //@{
  /**
   * Set the field type for the generated selection.
   * Possible values are as defined by
   * svtkSelection::SelectionField.
   */
  svtkSetMacro(FieldType, int);
  svtkGetMacro(FieldType, int);
  //@}

  //@{
  /**
   * When extracting by points, extract the cells that contain the
   * passing points.
   */
  svtkSetMacro(ContainingCells, int);
  svtkGetMacro(ContainingCells, int);
  //@}

  //@{
  /**
   * Specify number of layers to extract connected to the selected elements.
   */
  svtkSetClampMacro(NumberOfLayers, int, 0, SVTK_INT_MAX);
  svtkGetMacro(NumberOfLayers, int);
  //@}

  //@{
  /**
   * Determines whether the selection describes what to include or exclude.
   * Default is 0, meaning include.
   */
  svtkSetMacro(Inverse, int);
  svtkGetMacro(Inverse, int);
  //@}

  //@{
  /**
   * Access to the name of the selection's subset description array.
   */
  svtkSetStringMacro(ArrayName);
  svtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Access to the component number for the array specified by ArrayName.
   * Default is component 0. Use -1 for magnitude.
   */
  svtkSetMacro(ArrayComponent, int);
  svtkGetMacro(ArrayComponent, int);
  //@}

  //@{
  /**
   * If CompositeIndex < 0 then COMPOSITE_INDEX() is not added to the output.
   */
  svtkSetMacro(CompositeIndex, int);
  svtkGetMacro(CompositeIndex, int);
  //@}

  //@{
  /**
   * If HierarchicalLevel or HierarchicalIndex < 0 , then HIERARCHICAL_LEVEL()
   * and HIERARCHICAL_INDEX() keys are not added to the output.
   */
  svtkSetMacro(HierarchicalLevel, int);
  svtkGetMacro(HierarchicalLevel, int);
  svtkSetMacro(HierarchicalIndex, int);
  svtkGetMacro(HierarchicalIndex, int);
  //@}

  //@{
  /**
   * Set/Get the query expression string.
   */
  svtkSetStringMacro(QueryString);
  svtkGetStringMacro(QueryString);
  //@}

protected:
  svtkSelectionSource();
  ~svtkSelectionSource() override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  svtkSelectionSourceInternals* Internal;

  int ContentType;
  int FieldType;
  int ContainingCells;
  int PreserveTopology;
  int Inverse;
  int CompositeIndex;
  int HierarchicalLevel;
  int HierarchicalIndex;
  char* ArrayName;
  int ArrayComponent;
  char* QueryString;
  int NumberOfLayers;

private:
  svtkSelectionSource(const svtkSelectionSource&) = delete;
  void operator=(const svtkSelectionSource&) = delete;
};

#endif
