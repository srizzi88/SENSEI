/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProteinRibbonFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkProteinRibbonFilter_h
#define svtkProteinRibbonFilter_h

/**
 * @class   svtkProteinRibbonFilter
 * @brief   generates protein ribbons
 *
 * svtkProteinRibbonFilter is a polydata algorithm that generates protein
 * ribbons.
 */

#include "svtkDomainsChemistryModule.h" // for export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkColor.h" // For svtkColor3ub.
#include <map>        // For element to color map.

class svtkVector3f;
class svtkStringArray;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkProteinRibbonFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkProteinRibbonFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkProteinRibbonFilter* New();

  //@{
  /**
   * Width of the ribbon coil. Default is 0.3.
   */
  svtkGetMacro(CoilWidth, float);
  svtkSetMacro(CoilWidth, float);
  //@}

  //@{
  /**
   * Width of the helix part of the ribbon. Default is 1.3.
   */
  svtkGetMacro(HelixWidth, float);
  svtkSetMacro(HelixWidth, float);
  //@}

  //@{
  /**
   * Smoothing factor of the ribbon. Default is 20.
   */
  svtkGetMacro(SubdivideFactor, int);
  svtkSetMacro(SubdivideFactor, int);
  //@}

  //@{
  /**
   * If enabled, small molecules (HETATMs) are drawn as spheres. Default is true.
   */
  svtkGetMacro(DrawSmallMoleculesAsSpheres, bool);
  svtkSetMacro(DrawSmallMoleculesAsSpheres, bool);
  //@}

  //@{
  /**
   * Resolution of the spheres for small molecules. Default is 20.
   */
  svtkGetMacro(SphereResolution, int);
  svtkSetMacro(SphereResolution, int);
  //@}

protected:
  svtkProteinRibbonFilter();
  ~svtkProteinRibbonFilter() override;

  int FillInputPortInformation(int, svtkInformation*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void CreateThinStrip(svtkPolyData* poly, svtkUnsignedCharArray* pointsColors, svtkPoints* p,
    std::vector<std::pair<svtkVector3f, bool> >& p1, std::vector<std::pair<svtkVector3f, bool> >& p2,
    std::vector<svtkColor3ub>& colors);

  void CreateAtomAsSphere(svtkPolyData* poly, svtkUnsignedCharArray* pointsColors, double* pos,
    const svtkColor3ub& color, float radius, float scale);

  static std::vector<svtkVector3f>* Subdivide(
    std::vector<std::pair<svtkVector3f, bool> >& p, int div);

  void SetColorByAtom(std::vector<svtkColor3ub>& colors, svtkStringArray* atomTypes);

  void SetColorByStructure(std::vector<svtkColor3ub>& colors, svtkStringArray* atomTypes,
    svtkUnsignedCharArray* ss, const svtkColor3ub& helixColor, const svtkColor3ub& sheetColor);

  std::map<std::string, svtkColor3ub> ElementColors;

  float CoilWidth;
  float HelixWidth;
  int SphereResolution;
  int SubdivideFactor;
  bool DrawSmallMoleculesAsSpheres;

private:
  svtkProteinRibbonFilter(const svtkProteinRibbonFilter&) = delete;
  void operator=(const svtkProteinRibbonFilter&) = delete;
};

#endif // svtkProteinRibbonFilter_h
