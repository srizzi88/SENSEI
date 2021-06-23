/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAttributeDataToFieldDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAttributeDataToFieldDataFilter
 * @brief   map attribute data to field data
 *
 * svtkAttributeDataToFieldDataFilter is a class that maps attribute data into
 * field data. Since this filter is a subclass of svtkDataSetAlgorithm,
 * the output dataset (whose structure is the same as the input dataset),
 * will contain the field data that is generated. The filter will convert
 * point and cell attribute data to field data and assign it as point and
 * cell field data, replacing any point or field data that was there
 * previously. By default, the original non-field point and cell attribute
 * data will be passed to the output of the filter, although you can shut
 * this behavior down.
 *
 * @warning
 * Reference counting the underlying data arrays is used to create the field
 * data.  Therefore, no extra memory is utilized.
 *
 * @warning
 * The original field data (if any) associated with the point and cell
 * attribute data is placed into the generated fields along with the scalars,
 * vectors, etc.
 *
 * @sa
 * svtkFieldData svtkDataObject svtkDataSet svtkFieldDataToAttributeDataFilter
 */

#ifndef svtkAttributeDataToFieldDataFilter_h
#define svtkAttributeDataToFieldDataFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class SVTKFILTERSCORE_EXPORT svtkAttributeDataToFieldDataFilter : public svtkDataSetAlgorithm
{
public:
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkAttributeDataToFieldDataFilter, svtkDataSetAlgorithm);

  /**
   * Construct this object.
   */
  static svtkAttributeDataToFieldDataFilter* New();

  //@{
  /**
   * Turn on/off the passing of point and cell non-field attribute data to the
   * output of the filter.
   */
  svtkSetMacro(PassAttributeData, svtkTypeBool);
  svtkGetMacro(PassAttributeData, svtkTypeBool);
  svtkBooleanMacro(PassAttributeData, svtkTypeBool);
  //@}

protected:
  svtkAttributeDataToFieldDataFilter();
  ~svtkAttributeDataToFieldDataFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**,
    svtkInformationVector*) override; // generate output data

  svtkTypeBool PassAttributeData;

private:
  svtkAttributeDataToFieldDataFilter(const svtkAttributeDataToFieldDataFilter&) = delete;
  void operator=(const svtkAttributeDataToFieldDataFilter&) = delete;
};

#endif
