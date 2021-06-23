/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFeatureEdges.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFeatureEdges
 * @brief   extract interior, boundary, non-manifold, and/or
 *          sharp edges from polygonal data
 *
 * svtkFeatureEdges is a filter to extract special types of edges from
 * input polygonal data. These edges are either 1) boundary (used by
 * one polygon) or a line cell; 2) non-manifold (used by three or more
 * polygons); 3) feature edges (edges used by two triangles and whose
 * dihedral angle > FeatureAngle); or 4) manifold edges (edges used by
 * exactly two polygons). These edges may be extracted in any
 * combination. Edges may also be "colored" (i.e., scalar values assigned)
 * based on edge type. The cell coloring is assigned to the cell data of
 * the extracted edges.
 *
 * @warning
 * To see the coloring of the lines you may have to set the ScalarMode
 * instance variable of the mapper to SetScalarModeToUseCellData(). (This
 * is only a problem if there are point data scalars.)
 *
 * @sa
 * svtkExtractEdges
 */

#ifndef svtkFeatureEdges_h
#define svtkFeatureEdges_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkIncrementalPointLocator;

class SVTKFILTERSCORE_EXPORT svtkFeatureEdges : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  svtkTypeMacro(svtkFeatureEdges, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Construct an instance with feature angle = 30; all types of edges
   * (except manifold edges) are extracted and colored.
   */
  static svtkFeatureEdges* New();

  //@{
  /**
   * Methods for turning the extraction of all types of edges on;
   * and turning the extraction of all types of edges off.
   */
  void ExtractAllEdgeTypesOn();
  void ExtractAllEdgeTypesOff();
  //@}

  //@{
  /**
   * Turn on/off the extraction of boundary edges.
   */
  svtkSetMacro(BoundaryEdges, bool);
  svtkGetMacro(BoundaryEdges, bool);
  svtkBooleanMacro(BoundaryEdges, bool);
  //@}

  //@{
  /**
   * Turn on/off the extraction of feature edges.
   */
  svtkSetMacro(FeatureEdges, bool);
  svtkGetMacro(FeatureEdges, bool);
  svtkBooleanMacro(FeatureEdges, bool);
  //@}

  //@{
  /**
   * Specify the feature angle for extracting feature edges.
   */
  svtkSetClampMacro(FeatureAngle, double, 0.0, 180.0);
  svtkGetMacro(FeatureAngle, double);
  //@}

  //@{
  /**
   * Turn on/off the extraction of non-manifold edges.
   */
  svtkSetMacro(NonManifoldEdges, bool);
  svtkGetMacro(NonManifoldEdges, bool);
  svtkBooleanMacro(NonManifoldEdges, bool);
  //@}

  //@{
  /**
   * Turn on/off the extraction of manifold edges. This typically
   * correspond to interior edges.
   */
  svtkSetMacro(ManifoldEdges, bool);
  svtkGetMacro(ManifoldEdges, bool);
  svtkBooleanMacro(ManifoldEdges, bool);
  //@}

  //@{
  /**
   * Turn on/off the coloring of edges by type.
   */
  svtkSetMacro(Coloring, bool);
  svtkGetMacro(Coloring, bool);
  svtkBooleanMacro(Coloring, bool);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By
   * default an instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  /**
   * Return MTime also considering the locator.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/get the desired precision for the output point type. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkFeatureEdges();
  ~svtkFeatureEdges() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double FeatureAngle;
  bool BoundaryEdges;
  bool FeatureEdges;
  bool NonManifoldEdges;
  bool ManifoldEdges;
  bool Coloring;
  int OutputPointsPrecision;
  svtkIncrementalPointLocator* Locator;

private:
  svtkFeatureEdges(const svtkFeatureEdges&) = delete;
  void operator=(const svtkFeatureEdges&) = delete;
};

#endif
