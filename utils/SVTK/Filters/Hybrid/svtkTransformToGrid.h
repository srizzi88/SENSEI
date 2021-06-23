/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformToGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTransformToGrid
 * @brief   create a grid for a svtkGridTransform
 *
 * svtkTransformToGrid takes any transform as input and produces a grid
 * for use by a svtkGridTransform.  This can be used, for example, to
 * invert a grid transform, concatenate two grid transforms, or to
 * convert a thin plate spline transform into a grid transform.
 * @sa
 * svtkGridTransform svtkThinPlateSplineTransform svtkAbstractTransform
 */

#ifndef svtkTransformToGrid_h
#define svtkTransformToGrid_h

#include "svtkAlgorithm.h"
#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkImageData.h"           // makes things a bit easier

class svtkAbstractTransform;

class SVTKFILTERSHYBRID_EXPORT svtkTransformToGrid : public svtkAlgorithm
{
public:
  static svtkTransformToGrid* New();
  svtkTypeMacro(svtkTransformToGrid, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the transform which will be converted into a grid.
   */
  virtual void SetInput(svtkAbstractTransform*);
  svtkGetObjectMacro(Input, svtkAbstractTransform);
  //@}

  //@{
  /**
   * Get/Set the extent of the grid.
   */
  svtkSetVector6Macro(GridExtent, int);
  svtkGetVector6Macro(GridExtent, int);
  //@}

  //@{
  /**
   * Get/Set the origin of the grid.
   */
  svtkSetVector3Macro(GridOrigin, double);
  svtkGetVector3Macro(GridOrigin, double);
  //@}

  //@{
  /**
   * Get/Set the spacing between samples in the grid.
   */
  svtkSetVector3Macro(GridSpacing, double);
  svtkGetVector3Macro(GridSpacing, double);
  //@}

  //@{
  /**
   * Get/Set the scalar type of the grid.  The default is float.
   */
  svtkSetMacro(GridScalarType, int);
  svtkGetMacro(GridScalarType, int);
  void SetGridScalarTypeToDouble() { this->SetGridScalarType(SVTK_DOUBLE); }
  void SetGridScalarTypeToFloat() { this->SetGridScalarType(SVTK_FLOAT); }
  void SetGridScalarTypeToShort() { this->SetGridScalarType(SVTK_SHORT); }
  void SetGridScalarTypeToUnsignedShort() { this->SetGridScalarType(SVTK_UNSIGNED_SHORT); }
  void SetGridScalarTypeToUnsignedChar() { this->SetGridScalarType(SVTK_UNSIGNED_CHAR); }
  void SetGridScalarTypeToChar() { this->SetGridScalarType(SVTK_CHAR); }
  //@}

  //@{
  /**
   * Get the scale and shift to convert integer grid elements into
   * real values:  dx = scale*di + shift.  If the grid is of double type,
   * then scale = 1 and shift = 0.
   */
  double GetDisplacementScale()
  {
    this->UpdateShiftScale();
    return this->DisplacementScale;
  }
  double GetDisplacementShift()
  {
    this->UpdateShiftScale();
    return this->DisplacementShift;
  }
  //@}

  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkImageData* GetOutput();

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkTransformToGrid();
  ~svtkTransformToGrid() override;

  void RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  void RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  /**
   * Internal method to calculate the shift and scale values which
   * will provide maximum grid precision for a particular integer type.
   */
  void UpdateShiftScale();

  svtkMTimeType GetMTime() override;

  svtkAbstractTransform* Input;

  int GridScalarType;
  int GridExtent[6];
  double GridOrigin[3];
  double GridSpacing[3];

  double DisplacementScale;
  double DisplacementShift;
  svtkTimeStamp ShiftScaleTime;

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkTransformToGrid(const svtkTransformToGrid&) = delete;
  void operator=(const svtkTransformToGrid&) = delete;
};

#endif
