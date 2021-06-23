/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFrustumSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFrustumSource
 * @brief   create a polygonal representation of a frustum
 *
 * svtkFrustumSource creates a frustum defines by a set of planes. The frustum
 * is represented with four-sided polygons. It is possible to specify extra
 * lines to better visualize the field of view.
 *
 * @par Usage:
 * Typical use consists of 3 steps:
 * 1. get the planes coefficients from a svtkCamera with
 * svtkCamera::GetFrustumPlanes()
 * 2. initialize the planes with svtkPlanes::SetFrustumPlanes() with the planes
 * coefficients
 * 3. pass the svtkPlanes to a svtkFrustumSource.
 */

#ifndef svtkFrustumSource_h
#define svtkFrustumSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"
class svtkPlanes;

class SVTKFILTERSSOURCES_EXPORT svtkFrustumSource : public svtkPolyDataAlgorithm
{
public:
  static svtkFrustumSource* New();
  svtkTypeMacro(svtkFrustumSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Return the 6 planes defining the frustum. Initial value is nullptr.
   * The 6 planes are defined in this order: left,right,bottom,top,far,near.
   * If Planes==nullptr or if Planes->GetNumberOfPlanes()!=6 when RequestData()
   * is called, an error message will be emitted and RequestData() will
   * return right away.
   */
  svtkGetObjectMacro(Planes, svtkPlanes);
  //@}

  /**
   * Set the 6 planes defining the frustum.
   */
  virtual void SetPlanes(svtkPlanes* planes);

  //@{
  /**
   * Tells if some extra lines will be generated. Initial value is true.
   */
  svtkGetMacro(ShowLines, bool);
  svtkSetMacro(ShowLines, bool);
  svtkBooleanMacro(ShowLines, bool);
  //@}

  //@{
  /**
   * Length of the extra lines. This a stricly positive value.
   * Initial value is 1.0.
   */
  svtkGetMacro(LinesLength, double);
  svtkSetMacro(LinesLength, double);
  //@}

  /**
   * Modified GetMTime because of Planes.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  /**
   * Default constructor. Planes=nullptr. ShowLines=true. LinesLength=1.0.
   */
  svtkFrustumSource();

  ~svtkFrustumSource() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Compute the intersection of 3 planes.
   */
  void ComputePoint(int planes[3], double* pt);

  svtkPlanes* Planes;
  bool ShowLines;
  double LinesLength;
  int OutputPointsPrecision;

private:
  svtkFrustumSource(const svtkFrustumSource&) = delete;
  void operator=(const svtkFrustumSource&) = delete;
};

#endif
