/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHeatmapItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHeatmapItem
 * @brief   A 2D graphics item for rendering a heatmap
 *
 *
 * This item draws a heatmap as a part of a svtkContextScene.
 *
 * .SEE ALSO
 * svtkTable
 */

#ifndef svtkHeatmapItem_h
#define svtkHeatmapItem_h

#include "svtkContextItem.h"
#include "svtkViewsInfovisModule.h" // For export macro

#include "svtkNew.h"          // For svtkNew ivars
#include "svtkSmartPointer.h" // For svtkSmartPointer ivars
#include "svtkStdString.h"    // For get/set ivars
#include "svtkVector.h"       // For svtkVector2f ivar
#include <map>               // For column ranges
#include <set>               // For blank row support
#include <vector>            // For row mapping

class svtkBitArray;
class svtkCategoryLegend;
class svtkColorLegend;
class svtkLookupTable;
class svtkStringArray;
class svtkTable;
class svtkTooltipItem;
class svtkVariantArray;

class SVTKVIEWSINFOVIS_EXPORT svtkHeatmapItem : public svtkContextItem
{
public:
  static svtkHeatmapItem* New();
  svtkTypeMacro(svtkHeatmapItem, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the table that this item draws.  The first column of the table
   * must contain the names of the rows.
   */
  virtual void SetTable(svtkTable* table);

  /**
   * Get the table that this item draws.
   */
  svtkTable* GetTable();

  /**
   * Get the table that this item draws.
   */
  svtkStringArray* GetRowNames();

  //@{
  /**
   * Get/Set the name of the column that specifies the name
   * of this table's rows.  By default, we assume this
   * column will be named "name".  If no such column can be
   * found, we then assume that the 1st column in the table
   * names the rows.
   */
  svtkGetMacro(NameColumn, svtkStdString);
  svtkSetMacro(NameColumn, svtkStdString);
  //@}

  /**
   * Set which way the table should face within the visualization.
   */
  void SetOrientation(int orientation);

  /**
   * Get the current heatmap orientation.
   */
  int GetOrientation();

  /**
   * Get the angle that row labels should be rotated for the corresponding
   * heatmap orientation.  For the default orientation (LEFT_TO_RIGHT), this
   * is 0 degrees.
   */
  double GetTextAngleForOrientation(int orientation);

  //@{
  /**
   * Set the position of the heatmap.
   */
  svtkSetVector2Macro(Position, float);
  void SetPosition(const svtkVector2f& pos);
  //@}

  //@{
  /**
   * Get position of the heatmap.
   */
  svtkGetVector2Macro(Position, float);
  svtkVector2f GetPositionVector();
  //@}

  //@{
  /**
   * Get/Set the height of the cells in our heatmap.
   * Default is 18 pixels.
   */
  svtkGetMacro(CellHeight, double);
  svtkSetMacro(CellHeight, double);
  //@}

  //@{
  /**
   * Get/Set the width of the cells in our heatmap.
   * Default is 36 pixels.
   */
  svtkGetMacro(CellWidth, double);
  svtkSetMacro(CellWidth, double);
  //@}

  /**
   * Get the bounds for this item as (Xmin,Xmax,Ymin,Ymax).
   */
  virtual void GetBounds(double bounds[4]);

  /**
   * Mark a row as blank, meaning that no cells will be drawn for it.
   * Used by svtkTreeHeatmapItem to represent missing data.
   */
  void MarkRowAsBlank(const std::string& rowName);

  /**
   * Paints the table as a heatmap.
   */
  bool Paint(svtkContext2D* painter) override;

  //@{
  /**
   * Get the width of the largest row or column label drawn by this
   * heatmap.
   */
  svtkGetMacro(RowLabelWidth, float);
  svtkGetMacro(ColumnLabelWidth, float);
  //@}

  /**
   * Enum for Orientation.
   */
  enum
  {
    LEFT_TO_RIGHT,
    UP_TO_DOWN,
    RIGHT_TO_LEFT,
    DOWN_TO_UP
  };

  /**
   * Returns true if the transform is interactive, false otherwise.
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  /**
   * Display a tooltip when the user mouses over a cell in the heatmap.
   */
  bool MouseMoveEvent(const svtkContextMouseEvent& event) override;

  /**
   * Display a legend for a column of data.
   */
  bool MouseDoubleClickEvent(const svtkContextMouseEvent& event) override;

protected:
  svtkHeatmapItem();
  ~svtkHeatmapItem() override;

  svtkVector2f PositionVector;
  float* Position;

  /**
   * Generate some data needed for painting.  We cache this information as
   * it only needs to be generated when the input data changes.
   */
  virtual void RebuildBuffers();

  /**
   * This function does the bulk of the actual work in rendering our heatmap.
   */
  virtual void PaintBuffers(svtkContext2D* painter);

  /**
   * This function returns a bool indicating whether or not we need to rebuild
   * our cached data before painting.
   */
  virtual bool IsDirty();

  /**
   * Generate a separate svtkLookupTable for each column in the table.
   */
  void InitializeLookupTables();

  /**
   * Helper function.  Find the prominent, distinct values in the specified
   * column of strings and add it to our "master list" of categorical values.
   * This list is then used to generate a svtkLookupTable for all categorical
   * data within the heatmap.
   */
  void AccumulateProminentCategoricalDataValues(svtkIdType column);

  /**
   * Setup the default lookup table to use for continuous (not categorical)
   * data.
   */
  void GenerateContinuousDataLookupTable();

  /**
   * Setup the default lookup table to use for categorical (not continuous)
   * data.
   */
  void GenerateCategoricalDataLookupTable();

  /**
   * Get the value for the cell of the heatmap located at scene position (x, y)
   * This function assumes the caller has already determined that (x, y) falls
   * within the heatmap.
   */
  std::string GetTooltipText(float x, float y);

  /**
   * Calculate the extent of the data that is visible within the window.
   * This information is used to ensure that we only draw details that
   * will be seen by the user.  This improves rendering speed, particularly
   * for larger data.
   */
  void UpdateVisibleSceneExtent(svtkContext2D* painter);

  /**
   * Returns true if any part of the line segment defined by endpoints
   * (x0, y0), (x1, y1) falls within the extent of the currently
   * visible scene.  Returns false otherwise.
   */
  bool LineIsVisible(double x0, double y0, double x1, double y1);

  /**
   * Compute the extent of the heatmap.  This does not include
   * the text labels.
   */
  void ComputeBounds();

  /**
   * Compute the width of our longest row label and the width of our
   * longest column label.  These values are used by GetBounds().
   */
  void ComputeLabelWidth(svtkContext2D* painter);

  // Setup the position, size, and orientation of this heatmap's color
  // legend based on the heatmap's current orientation.
  void PositionColorLegend(int orientation);

  // Setup the position, size, and orientation of this heatmap's
  // legends based on the heatmap's current orientation.
  void PositionLegends(int orientation);

  svtkSmartPointer<svtkTable> Table;
  svtkStringArray* RowNames;
  svtkStdString NameColumn;

private:
  svtkHeatmapItem(const svtkHeatmapItem&) = delete;
  void operator=(const svtkHeatmapItem&) = delete;

  unsigned long HeatmapBuildTime;
  svtkNew<svtkCategoryLegend> CategoryLegend;
  svtkNew<svtkColorLegend> ColorLegend;
  svtkNew<svtkTooltipItem> Tooltip;
  svtkNew<svtkLookupTable> ContinuousDataLookupTable;
  svtkNew<svtkLookupTable> CategoricalDataLookupTable;
  svtkNew<svtkLookupTable> ColorLegendLookupTable;
  svtkNew<svtkStringArray> CategoricalDataValues;
  svtkNew<svtkVariantArray> CategoryLegendValues;
  double CellWidth;
  double CellHeight;

  std::map<svtkIdType, std::pair<double, double> > ColumnRanges;
  std::vector<svtkIdType> SceneRowToTableRowMap;
  std::vector<svtkIdType> SceneColumnToTableColumnMap;
  std::set<std::string> BlankRows;

  double MinX;
  double MinY;
  double MaxX;
  double MaxY;
  double SceneBottomLeft[3];
  double SceneTopRight[3];
  float RowLabelWidth;
  float ColumnLabelWidth;

  svtkBitArray* CollapsedRowsArray;
  svtkBitArray* CollapsedColumnsArray;
  bool LegendPositionSet;
};

#endif
