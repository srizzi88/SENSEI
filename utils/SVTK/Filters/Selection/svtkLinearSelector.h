/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkLinearSelector.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLinearSelector
 * @brief   select cells intersecting a line (possibly broken)
 *
 *
 * This filter takes a svtkCompositeDataSet as input and a line segment as parameter.
 * It outputs a svtkSelection identifying all the cells intersecting the given line segment.
 *
 * @par Thanks:
 * This class has been initially developed in the frame of CEA's Love visualization software
 * development <br> CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br> BP12,
 * F-91297 Arpajon, France. <br> Modified and integrated into SVTK, Kitware SAS 2012 This class was
 * implemented by Thierry Carrard, Charles Pignerol, and Philippe Pebay.
 */

#ifndef svtkLinearSelector_h
#define svtkLinearSelector_h

#include "svtkFiltersSelectionModule.h" // For export macro
#include "svtkSelectionAlgorithm.h"

class svtkAlgorithmOutput;
class svtkDataSet;
class svtkDoubleArray;
class svtkIdTypeArray;
class svtkPoints;

class SVTKFILTERSSELECTION_EXPORT svtkLinearSelector : public svtkSelectionAlgorithm
{
public:
  svtkTypeMacro(svtkLinearSelector, svtkSelectionAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkLinearSelector* New();

  //@{
  /**
   * Set/Get starting point of intersecting segment
   */
  svtkSetVector3Macro(StartPoint, double);
  svtkGetVectorMacro(StartPoint, double, 3);
  //@}

  //@{
  /**
   * Set/Get end point of intersecting segment
   */
  svtkSetVector3Macro(EndPoint, double);
  svtkGetVectorMacro(EndPoint, double, 3);
  //@}

  //@{
  /**
   * Set/Get the list of points defining the intersecting broken line
   */
  virtual void SetPoints(svtkPoints*);
  svtkGetObjectMacro(Points, svtkPoints);
  //@}

  //@{
  /**
   * Set/Get tolerance to be used by intersection algorithm
   */
  svtkSetMacro(Tolerance, double);
  svtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Set/Get whether lines vertice are included in selection
   */
  svtkSetMacro(IncludeVertices, bool);
  svtkGetMacro(IncludeVertices, bool);
  svtkBooleanMacro(IncludeVertices, bool);
  //@}

  //@{
  /**
   * Set/Get relative tolerance for vertex elimination
   */
  svtkSetClampMacro(VertexEliminationTolerance, double, 0., .1);
  svtkGetMacro(VertexEliminationTolerance, double);
  //@}

protected:
  svtkLinearSelector();
  ~svtkLinearSelector() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * The main routine that iterates over cells and looks for those that
   * intersect at least one of the segments of interest
   */
  void SeekIntersectingCells(svtkDataSet* input, svtkIdTypeArray* outIndices);

private:
  svtkLinearSelector(const svtkLinearSelector&) = delete;
  void operator=(const svtkLinearSelector&) = delete;

  //@{
  /**
   * Start and end point of the intersecting line segment
   * NB: These are used if and only if Points is null.
   */
  double StartPoint[3];
  double EndPoint[3];
  //@}

  /**
   * The list of points defining the intersecting broken line
   * NB: The Start/EndPoint definition of a single line segment is used by default
   */
  svtkPoints* Points;

  /**
   * Tolerance to be used by intersection algorithm
   */
  double Tolerance;

  /**
   * Decide whether lines vertice are included in selection
   * Default: true
   */
  bool IncludeVertices;

  //@{
  /**
   * Relative tolerance for vertex elimination
   * Default: 1e-6
   */
  double VertexEliminationTolerance;
  //@}
};

#endif // svtkLinearSelector_h
