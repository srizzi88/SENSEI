/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPassSelectedArrays.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkPassSelectedArrays
 * @brief pass through chosen arrays
 *
 * svtkPassSelectedArrays can be used to pass through chosen arrays. It is
 * intended as a replacement for svtkPassArrays filter with a more standard API
 * that uses `svtkDataArraySelection` to choose arrays to pass through.
 *
 * To enable/disable arrays to pass, get the appropriate `svtkDataArraySelection`
 * instance using `GetArraySelection` or the association specific get methods
 * such as `GetPointDataArraySelection`, `GetCellDataArraySelection` etc. and
 * then enable/disable arrays using `svtkDataArraySelection` API. Using
 * svtkDataArraySelection::SetUnknownArraySetting` one also dictate how arrays
 * not explicitly listed are to be handled.
 *
 */

#ifndef svtkPassSelectedArrays_h
#define svtkPassSelectedArrays_h

#include "svtkDataObject.h"           // for svtkDataObject::FieldAssociations
#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"
#include "svtkSmartPointer.h" // for ivar

class svtkDataArraySelection;

class SVTKFILTERSGENERAL_EXPORT svtkPassSelectedArrays : public svtkPassInputTypeAlgorithm
{
public:
  static svtkPassSelectedArrays* New();
  svtkTypeMacro(svtkPassSelectedArrays, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Enable/disable this filter. When disabled, this filter passes all input arrays
   * irrespective of the array selections. Default is `true`.
   */
  svtkSetMacro(Enabled, bool);
  svtkGetMacro(Enabled, bool);
  svtkBooleanMacro(Enabled, bool);
  //@}

  /**
   * Returns the svtkDataArraySelection instance associated with a particular
   * array association type (svtkDataObject::FieldAssociations). Returns nullptr
   * if the association type is invalid others the corresponding
   * svtkDataArraySelection instance is returned.
   */
  svtkDataArraySelection* GetArraySelection(int association);

  //@{
  /**
   * Convenience methods that call `GetArraySelection` with corresponding
   * association type.
   */
  svtkDataArraySelection* GetPointDataArraySelection()
  {
    return this->GetArraySelection(svtkDataObject::FIELD_ASSOCIATION_POINTS);
  }
  svtkDataArraySelection* GetCellDataArraySelection()
  {
    return this->GetArraySelection(svtkDataObject::FIELD_ASSOCIATION_CELLS);
  }
  svtkDataArraySelection* GetFieldDataArraySelection()
  {
    return this->GetArraySelection(svtkDataObject::FIELD_ASSOCIATION_NONE);
  }
  svtkDataArraySelection* GetVertexDataArraySelection()
  {
    return this->GetArraySelection(svtkDataObject::FIELD_ASSOCIATION_VERTICES);
  }
  svtkDataArraySelection* GetEdgeDataArraySelection()
  {
    return this->GetArraySelection(svtkDataObject::FIELD_ASSOCIATION_EDGES);
  }
  svtkDataArraySelection* GetRowDataArraySelection()
  {
    return this->GetArraySelection(svtkDataObject::FIELD_ASSOCIATION_ROWS);
  }
  //@}

protected:
  svtkPassSelectedArrays();
  ~svtkPassSelectedArrays() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPassSelectedArrays(const svtkPassSelectedArrays&) = delete;
  void operator=(const svtkPassSelectedArrays&) = delete;

  bool Enabled;
  svtkSmartPointer<svtkDataArraySelection> ArraySelections[svtkDataObject::NUMBER_OF_ASSOCIATIONS];
};

#endif
