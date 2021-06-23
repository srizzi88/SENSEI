/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeoTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkGeoTransform
 * @brief   A transformation between two geographic coordinate systems
 *
 * This class takes two geographic projections and transforms point
 * coordinates between them.
 */

#ifndef svtkGeoTransform_h
#define svtkGeoTransform_h

#include "svtkAbstractTransform.h"
#include "svtkGeovisCoreModule.h" // For export macro

class svtkGeoProjection;

class SVTKGEOVISCORE_EXPORT svtkGeoTransform : public svtkAbstractTransform
{
public:
  static svtkGeoTransform* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkGeoTransform, svtkAbstractTransform);

  //@{
  /**
   * The source geographic projection.
   */
  void SetSourceProjection(svtkGeoProjection* source);
  svtkGetObjectMacro(SourceProjection, svtkGeoProjection);
  //@}

  //@{
  /**
   * The target geographic projection.
   */
  void SetDestinationProjection(svtkGeoProjection* dest);
  svtkGetObjectMacro(DestinationProjection, svtkGeoProjection);
  //@}

  /**
   * Transform many points at once.
   */
  void TransformPoints(svtkPoints* src, svtkPoints* dst) override;

  /**
   * Invert the transformation.
   */
  void Inverse() override;

  //@{
  /**
   * This will calculate the transformation without calling Update.
   * Meant for use only within other SVTK classes.
   */
  void InternalTransformPoint(const float in[3], float out[3]) override;
  void InternalTransformPoint(const double in[3], double out[3]) override;
  //@}

  //@{
  /**
   * Computes Universal Transverse Mercator (UTM) zone given the
   * longitude and latitude of the point.
   * It correctly computes the zones in the two exception areas.
   * It returns an integer between 1 and 60 for valid long lat, or 0 otherwise.
   *
   */
  static int ComputeUTMZone(double lon, double lat);
  static int ComputeUTMZone(double* lonlat) { return ComputeUTMZone(lonlat[0], lonlat[1]); }
  //@}

  //@{
  /**
   * This will transform a point and, at the same time, calculate a
   * 3x3 Jacobian matrix that provides the partial derivatives of the
   * transformation at that point.  This method does not call Update.
   * Meant for use only within other SVTK classes.
   */
  void InternalTransformDerivative(
    const float in[3], float out[3], float derivative[3][3]) override;
  void InternalTransformDerivative(
    const double in[3], double out[3], double derivative[3][3]) override;
  //@}

  /**
   * Make another transform of the same type.
   */
  svtkAbstractTransform* MakeTransform() override;

protected:
  svtkGeoTransform();
  ~svtkGeoTransform() override;

  void InternalTransformPoints(double* ptsInOut, svtkIdType numPts, int stride);

  svtkGeoProjection* SourceProjection;
  svtkGeoProjection* DestinationProjection;

private:
  svtkGeoTransform(const svtkGeoTransform&) = delete;
  void operator=(const svtkGeoTransform&) = delete;
};

#endif // svtkGeoTransform_h
