/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplitColumnComponents.h

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
 * @class   svtkSplitColumnComponents
 * @brief   split multicomponent table columns
 *
 *
 * Splits any columns in a table that have more than one component into
 * individual columns. Single component columns are passed through without
 * any data duplication.
 * NamingMode can be used to control how columns with multiple components are
 * labelled in the output, e.g., if column names "Points" had three components
 * this column would be split into "Points (0)", "Points (1)", and Points (2)"
 * when NamingMode is NUMBERS_WITH_PARENS, into Points_0, Points_1, and Points_2
 * when NamingMode is NUMBERS_WITH_UNDERSCORES, into "Points (X)", "Points (Y)",
 * and "Points (Z)" when NamingMode is NAMES_WITH_PARENS, and into Points_X,
 * Points_Y, and Points_Z when NamingMode is NAMES_WITH_UNDERSCORES.
 */

#ifndef svtkSplitColumnComponents_h
#define svtkSplitColumnComponents_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkTableAlgorithm.h"

#include <string> // for std::strin

class svtkInformationIntegerKey;
class svtkInformationStringKey;

class SVTKFILTERSGENERAL_EXPORT svtkSplitColumnComponents : public svtkTableAlgorithm
{
public:
  static svtkSplitColumnComponents* New();
  svtkTypeMacro(svtkSplitColumnComponents, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * If on this filter will calculate an additional magnitude column for all
   * columns it splits with two or more components.
   * Default is on.
   */
  svtkSetMacro(CalculateMagnitudes, bool);
  svtkGetMacro(CalculateMagnitudes, bool);
  svtkBooleanMacro(CalculateMagnitudes, bool);
  //@}

  enum
  {
    NUMBERS_WITH_PARENS = 0,      // e.g Points (0)
    NAMES_WITH_PARENS = 1,        // e.g. Points (X)
    NUMBERS_WITH_UNDERSCORES = 2, // e.g. Points_0
    NAMES_WITH_UNDERSCORES = 3    // e.g. Points_X
  };

  //@{
  /**
   * Get/Set the array naming mode.
   * Description is NUMBERS_WITH_PARENS.
   */
  svtkSetClampMacro(NamingMode, int, NUMBERS_WITH_PARENS, NAMES_WITH_UNDERSCORES);
  void SetNamingModeToNumberWithParens() { this->SetNamingMode(NUMBERS_WITH_PARENS); }
  void SetNamingModeToNumberWithUnderscores() { this->SetNamingMode(NUMBERS_WITH_UNDERSCORES); }
  void SetNamingModeToNamesWithParens() { this->SetNamingMode(NAMES_WITH_PARENS); }
  void SetNamingModeToNamesWithUnderscores() { this->SetNamingMode(NAMES_WITH_UNDERSCORES); }
  svtkGetMacro(NamingMode, int);
  //@}

  //@{
  /**
   * These are keys that get added to each output array to make it easier for
   * downstream filters to know which output array were extracted from which
   * input array.
   *
   * If either of these keys are missing, then the array was not extracted at
   * all.
   *
   * `ORIGINAL_COMPONENT_NUMBER` of -1 indicates magnitude.
   */
  static svtkInformationStringKey* ORIGINAL_ARRAY_NAME();
  static svtkInformationIntegerKey* ORIGINAL_COMPONENT_NUMBER();
  //@}

protected:
  svtkSplitColumnComponents();
  ~svtkSplitColumnComponents() override;

  /**
   * Returns the label to use for the specific component in the array based on
   * this->NamingMode. Use component_no==-1 for magnitude.
   */
  std::string GetComponentLabel(svtkAbstractArray* array, int component_no);

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkSplitColumnComponents(const svtkSplitColumnComponents&) = delete;
  void operator=(const svtkSplitColumnComponents&) = delete;

  bool CalculateMagnitudes;
  int NamingMode;
};

#endif
