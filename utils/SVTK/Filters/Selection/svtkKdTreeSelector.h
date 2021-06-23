/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkKdTreeSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   svtkKdTreeSelector
 * @brief   Selects point ids using a kd-tree.
 *
 *
 * If SetKdTree is used, the filter ignores the input and selects based on that
 * kd-tree.  If SetKdTree is not used, the filter builds a kd-tree using the
 * input point set and uses that tree for selection.  The output is a
 * svtkSelection containing the ids found in the kd-tree using the specified
 * bounds.
 */

#ifndef svtkKdTreeSelector_h
#define svtkKdTreeSelector_h

#include "svtkFiltersSelectionModule.h" // For export macro
#include "svtkSelectionAlgorithm.h"

class svtkKdTree;

class SVTKFILTERSSELECTION_EXPORT svtkKdTreeSelector : public svtkSelectionAlgorithm
{
public:
  static svtkKdTreeSelector* New();
  svtkTypeMacro(svtkKdTreeSelector, svtkSelectionAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The kd-tree to use to find selected ids.
   * The kd-tree must be initialized with the desired set of points.
   * When this is set, the optional input is ignored.
   */
  void SetKdTree(svtkKdTree* tree);
  svtkGetObjectMacro(KdTree, svtkKdTree);
  //@}

  //@{
  /**
   * The bounds of the form (xmin,xmax,ymin,ymax,zmin,zmax).
   * To perform a search in 2D, use the bounds
   * (xmin,xmax,ymin,ymax,SVTK_DOUBLE_MIN,SVTK_DOUBLE_MAX).
   */
  svtkSetVector6Macro(SelectionBounds, double);
  svtkGetVector6Macro(SelectionBounds, double);
  //@}

  //@{
  /**
   * The field name to use when generating the selection.
   * If set, creates a VALUES selection.
   * If not set (or is set to nullptr), creates a INDICES selection.
   * By default this is not set.
   */
  svtkSetStringMacro(SelectionFieldName);
  svtkGetStringMacro(SelectionFieldName);
  //@}

  //@{
  /**
   * The field attribute to use when generating the selection.
   * If set, creates a PEDIGREEIDS or GLOBALIDS selection.
   * If not set (or is set to -1), creates a INDICES selection.
   * By default this is not set.
   * NOTE: This should be set a constant in svtkDataSetAttributes,
   * not svtkSelection.
   */
  svtkSetMacro(SelectionAttribute, int);
  svtkGetMacro(SelectionAttribute, int);
  //@}

  //@{
  /**
   * Whether to only allow up to one value in the result.
   * The item selected is closest to the center of the bounds,
   * if there are any points within the selection threshold.
   * Default is off.
   */
  svtkSetMacro(SingleSelection, bool);
  svtkGetMacro(SingleSelection, bool);
  svtkBooleanMacro(SingleSelection, bool);
  //@}

  //@{
  /**
   * The threshold for the single selection.
   * A single point is added to the selection if it is within
   * this threshold from the bounds center.
   * Default is 1.
   */
  svtkSetMacro(SingleSelectionThreshold, double);
  svtkGetMacro(SingleSelectionThreshold, double);
  //@}

  svtkMTimeType GetMTime() override;

protected:
  svtkKdTreeSelector();
  ~svtkKdTreeSelector() override;

  svtkKdTree* KdTree;
  double SelectionBounds[6];
  char* SelectionFieldName;
  bool BuildKdTreeFromInput;
  bool SingleSelection;
  double SingleSelectionThreshold;
  int SelectionAttribute;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkKdTreeSelector(const svtkKdTreeSelector&) = delete;
  void operator=(const svtkKdTreeSelector&) = delete;
};

#endif
