/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeHeatmapItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTreeHeatmapItem
 * @brief   A 2D graphics item for rendering a tree and
 * an associated heatmap.
 *
 *
 * This item draws a tree and a heatmap as a part of a svtkContextScene.
 * The input tree's vertex data must contain at least two arrays.
 * The first required array is a svtkStringArray called "node name".
 * This array corresponds to the first column of the input table.
 * The second required array is a scalar array called "node weight".
 * This array is used by svtkTreeLayoutStrategy to set any particular
 * node's distance from the root of the tree.
 *
 * The svtkNewickTreeReader automatically initializes both of these
 * required arrays in its output tree.
 *
 * .SEE ALSO
 * svtkDendrogramItem svtkHeatmapItem svtkTree svtkTable svtkNewickTreeReader
 */

#ifndef svtkTreeHeatmapItem_h
#define svtkTreeHeatmapItem_h

#include "svtkContextItem.h"
#include "svtkViewsInfovisModule.h" // For export macro

#include "svtkNew.h"          // For svtkNew ivars
#include "svtkSmartPointer.h" // For svtkSmartPointer ivars
#include <map>               // For string lookup tables
#include <vector>            // For lookup tables

class svtkDendrogramItem;
class svtkHeatmapItem;
class svtkTable;
class svtkTree;

class SVTKVIEWSINFOVIS_EXPORT svtkTreeHeatmapItem : public svtkContextItem
{
public:
  static svtkTreeHeatmapItem* New();
  svtkTypeMacro(svtkTreeHeatmapItem, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the tree that this item draws.  Note that this tree's vertex data
   * must contain a svtkStringArray called "node name".  Additionally, this
   * array must contain the same values as the first column of the input
   * table.  See SetTable for more information.  The svtkNewickTreeReader
   * automatically creates this required array for you.
   */
  virtual void SetTree(svtkTree* tree);

  /**
   * Get the tree that this item draws.
   */
  svtkTree* GetTree();

  /**
   * Set a tree to be drawn for the columns of the heatmap.  This tree's
   * vertex data must contain a svtkStringArray called "node name" that
   * corresponds to the names of the columns in the heatmap.
   */
  virtual void SetColumnTree(svtkTree* tree);

  /**
   * Get the tree that represents the columns of the heatmap (if one has
   * been set).
   */
  svtkTree* GetColumnTree();

  /**
   * Set the table that this item draws.  The first column of the table
   * must contain the names of the rows.  These names, in turn, must correspond
   * with the nodes names in the input tree.  See SetTree for more information.
   */
  virtual void SetTable(svtkTable* table);

  /**
   * Get the table that this item draws.
   */
  svtkTable* GetTable();

  //@{
  /**
   * Get/Set the dendrogram contained by this item.
   */
  svtkDendrogramItem* GetDendrogram();
  void SetDendrogram(svtkDendrogramItem* dendrogram);
  //@}

  //@{
  /**
   * Get/Set the heatmap contained by this item.
   */
  svtkHeatmapItem* GetHeatmap();
  void SetHeatmap(svtkHeatmapItem* heatmap);
  //@}

  /**
   * Reorder the rows in the table so they match the order of the leaf
   * nodes in our tree.
   */
  void ReorderTable();

  /**
   * Reverse the order of the rows in our input table.  This is used
   * to simplify the table layout for DOWN_TO_UP and RIGHT_TO_LEFT
   * orientations.
   */
  void ReverseTableRows();

  /**
   * Reverse the order of the rows in our input table.  This is used
   * to simplify the table layout for DOWN_TO_UP and UP_TO_DOWN
   * orientations.
   */
  void ReverseTableColumns();

  /**
   * Set which way the tree / heatmap should face within the visualization.
   * The default is for both components to be drawn left to right.
   */
  void SetOrientation(int orientation);

  /**
   * Get the current orientation.
   */
  int GetOrientation();

  /**
   * Get the bounds of this item (xMin, xMax, yMin, Max) in pixel coordinates.
   */
  void GetBounds(double bounds[4]);

  /**
   * Get the center point of this item in pixel coordinates.
   */
  void GetCenter(double center[2]);

  /**
   * Get the size of this item in pixel coordinates.
   */
  void GetSize(double size[2]);

  /**
   * Collapse subtrees until there are only n leaf nodes left in the tree.
   * The leaf nodes that remain are those that are closest to the root.
   * Any subtrees that were collapsed prior to this function being called
   * may be re-expanded.  Use this function instead of
   * this->GetDendrogram->CollapseToNumberOfLeafNodes(), as this function
   * also handles the hiding of heatmap rows that correspond to newly
   * collapsed subtrees.
   */
  void CollapseToNumberOfLeafNodes(unsigned int n);

  //@{
  /**
   * Get/Set how wide the edges of the trees should be.  Default is one pixel.
   */
  float GetTreeLineWidth();
  void SetTreeLineWidth(float width);
  //@}

  /**
   * Deprecated.  Use this->GetDendrogram()->GetPrunedTree() instead.
   */
  svtkTree* GetPrunedTree();

  /**
   * Deprecated.  Use this->GetDendrogram()->SetColorArray(const char *arrayName)
   * instead.
   */
  void SetTreeColorArray(const char* arrayName);

  /**
   * Returns true if the transform is interactive, false otherwise.
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  /**
   * Propagate any double click onto the dendrogram to check if any
   * subtrees should be collapsed or expanded.
   */
  bool MouseDoubleClickEvent(const svtkContextMouseEvent& event) override;

protected:
  svtkTreeHeatmapItem();
  ~svtkTreeHeatmapItem() override;

  /**
   * Paints the tree & associated table as a heatmap.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Mark heatmap rows as hidden when a subtree is collapsed.
   */
  void CollapseHeatmapRows();

  /**
   * Mark heatmap columns as hidden when a subtree is collapsed.
   */
  void CollapseHeatmapColumns();

  svtkSmartPointer<svtkDendrogramItem> Dendrogram;
  svtkSmartPointer<svtkDendrogramItem> ColumnDendrogram;
  svtkSmartPointer<svtkHeatmapItem> Heatmap;
  int Orientation;

private:
  svtkTreeHeatmapItem(const svtkTreeHeatmapItem&) = delete;
  void operator=(const svtkTreeHeatmapItem&) = delete;

  svtkMTimeType TreeHeatmapBuildTime;
};

#endif
