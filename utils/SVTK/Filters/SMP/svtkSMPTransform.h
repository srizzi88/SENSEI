/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSMPTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSMPTransform
 * @brief   Transform that uses the SMP framework
 *
 * Just like its parent, svtkTransform, svtkSMPTransform calculates and
 * manages transforms. Its main difference is that it performs various
 * transform operations over a set of points in parallel using the SMP
 * framework.
 * @sa
 * svtkTransform
 */

#ifndef svtkSMPTransform_h
#define svtkSMPTransform_h

#include "svtkFiltersSMPModule.h" // For export macro
#include "svtkTransform.h"

#if !defined(SVTK_LEGACY_REMOVE)
class SVTKFILTERSSMP_EXPORT svtkSMPTransform : public svtkTransform
{
public:
  static svtkSMPTransform* New();
  svtkTypeMacro(svtkSMPTransform, svtkTransform);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Apply the transformation to a series of points, and append the
   * results to outPts.
   */
  void TransformPoints(svtkPoints* inPts, svtkPoints* outPts) override;

  /**
   * Apply the transformation to a series of normals, and append the
   * results to outNms.
   */
  void TransformNormals(svtkDataArray* inNms, svtkDataArray* outNms) override;

  /**
   * Apply the transformation to a series of vectors, and append the
   * results to outVrs.
   */
  void TransformVectors(svtkDataArray* inVrs, svtkDataArray* outVrs) override;

  /**
   * Apply the transformation to a combination of points, normals
   * and vectors.
   */
  void TransformPointsNormalsVectors(svtkPoints* inPts, svtkPoints* outPts, svtkDataArray* inNms,
    svtkDataArray* outNms, svtkDataArray* inVrs, svtkDataArray* outVrs, int nOptionalVectors = 0,
    svtkDataArray** inVrsArr = nullptr, svtkDataArray** outVrsArr = nullptr) override;

protected:
  svtkSMPTransform();
  ~svtkSMPTransform() override {}

private:
  svtkSMPTransform(const svtkSMPTransform&) = delete;
  void operator=(const svtkSMPTransform&) = delete;
};

#endif // SVTK_LEGACY_REMOVE
#endif
