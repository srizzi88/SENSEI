/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssignAttribute.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAssignAttribute
 * @brief   Labels/marks a field as an attribute
 *
 * svtkAssignAttribute is used to label/mark a field (svtkDataArray) as an attribute.
 * A field name or an attribute to labeled can be specified. For example:
 * @verbatim
 * aa->Assign("foo", svtkDataSetAttributes::SCALARS,
 *            svtkAssignAttribute::POINT_DATA);
 * @endverbatim
 * tells svtkAssignAttribute to make the array in the point data called
 * "foo" the active scalars. On the other hand,
 * @verbatim
 * aa->Assign(svtkDataSetAttributes::VECTORS, svtkDataSetAttributes::SCALARS,
 *            svtkAssignAttribute::POINT_DATA);
 * @endverbatim
 * tells svtkAssignAttribute to make the active vectors also the active
 * scalars.
 *
 * @warning
 * When using Java, Python or Visual Basic bindings, the array name
 * can not be one of the  AttributeTypes when calling Assign() which takes
 * strings as arguments. The wrapped command will
 * always assume the string corresponds to an attribute type when
 * the argument is one of the AttributeTypes. In this situation,
 * use the Assign() which takes enums.
 *
 * @sa
 * svtkFieldData svtkDataSet svtkDataObjectToDataSetFilter
 * svtkDataSetAttributes svtkDataArray svtkRearrangeFields
 * svtkSplitField svtkMergeFields
 */

#ifndef svtkAssignAttribute_h
#define svtkAssignAttribute_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

#include "svtkDataSetAttributes.h" // Needed for NUM_ATTRIBUTES

class svtkFieldData;

class SVTKFILTERSCORE_EXPORT svtkAssignAttribute : public svtkPassInputTypeAlgorithm
{
public:
  svtkTypeMacro(svtkAssignAttribute, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create a new svtkAssignAttribute.
   */
  static svtkAssignAttribute* New();

  /**
   * Label an attribute as another attribute.
   */
  void Assign(int inputAttributeType, int attributeType, int attributeLoc);

  /**
   * Label an array as an attribute.
   */
  void Assign(const char* fieldName, int attributeType, int attributeLoc);

  /**
   * Helper method used by other language bindings. Allows the caller to
   * specify arguments as strings instead of enums.
   */
  void Assign(const char* name, const char* attributeType, const char* attributeLoc);

  // Always keep NUM_ATTRIBUTE_LOCS as the last entry
  enum AttributeLocation
  {
    POINT_DATA = 0,
    CELL_DATA = 1,
    VERTEX_DATA = 2,
    EDGE_DATA = 3,
    NUM_ATTRIBUTE_LOCS
  };

protected:
  enum FieldType
  {
    NAME,
    ATTRIBUTE
  };

  svtkAssignAttribute();
  ~svtkAssignAttribute() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  char* FieldName;
  int FieldTypeAssignment;
  int AttributeType;
  int InputAttributeType;
  int AttributeLocationAssignment;

  static char AttributeLocationNames[svtkAssignAttribute::NUM_ATTRIBUTE_LOCS][12];
  static char AttributeNames[svtkDataSetAttributes::NUM_ATTRIBUTES][20];

private:
  svtkAssignAttribute(const svtkAssignAttribute&) = delete;
  void operator=(const svtkAssignAttribute&) = delete;
};

#endif
