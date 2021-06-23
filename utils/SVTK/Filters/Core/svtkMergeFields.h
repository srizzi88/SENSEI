/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeFields.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMergeFields
 * @brief   Merge multiple fields into one.
 *
 * svtkMergeFields is used to merge multiple field into one.
 * The new field is put in the same field data as the original field.
 * For example
 * @verbatim
 * mf->SetOutputField("foo", svtkMergeFields::POINT_DATA);
 * mf->SetNumberOfComponents(2);
 * mf->Merge(0, "array1", 1);
 * mf->Merge(1, "array2", 0);
 * @endverbatim
 * will tell svtkMergeFields to use the 2nd component of array1 and
 * the 1st component of array2 to create a 2 component field called foo.
 *
 * @sa
 * svtkFieldData svtkDataSet svtkDataObjectToDataSetFilter
 * svtkDataSetAttributes svtkDataArray svtkRearrangeFields
 * svtkSplitField svtkAssignAttribute
 */

#ifndef svtkMergeFields_h
#define svtkMergeFields_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class svtkDataArray;
class svtkFieldData;

class SVTKFILTERSCORE_EXPORT svtkMergeFields : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkMergeFields, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create a new svtkMergeFields.
   */
  static svtkMergeFields* New();

  /**
   * The output field will have the given name and it will be in
   * fieldLoc (the input fields also have to be in fieldLoc).
   */
  void SetOutputField(const char* name, int fieldLoc);

  /**
   * Helper method used by the other language bindings. Allows the caller to
   * specify arguments as strings instead of enums.Returns an operation id
   * which can later be used to remove the operation.
   */
  void SetOutputField(const char* name, const char* fieldLoc);

  /**
   * Add a component (arrayName,sourceComp) to the output field.
   */
  void Merge(int component, const char* arrayName, int sourceComp);

  //@{
  /**
   * Set the number of the components in the output field.
   * This has to be set before execution. Default value is 0.
   */
  svtkSetMacro(NumberOfComponents, int);
  svtkGetMacro(NumberOfComponents, int);
  //@}

  enum FieldLocations
  {
    DATA_OBJECT = 0,
    POINT_DATA = 1,
    CELL_DATA = 2
  };

  struct Component
  {
    int Index;
    int SourceIndex;
    char* FieldName;
    Component* Next; // linked list
    void SetName(const char* name)
    {
      delete[] this->FieldName;
      this->FieldName = nullptr;
      if (name)
      {
        size_t len = strlen(name) + 1;
        this->FieldName = new char[len];
#ifdef _MSC_VER
        strncpy_s(this->FieldName, len, name, len - 1);
#else
        strncpy(this->FieldName, name, len);
#endif
      }
    }
    Component() { FieldName = nullptr; }
    ~Component() { delete[] FieldName; }
  };

protected:
  enum FieldType
  {
    NAME,
    ATTRIBUTE
  };

  svtkMergeFields();
  ~svtkMergeFields() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FieldName;
  int FieldLocation;
  int NumberOfComponents;
  int OutputDataType;

  static char FieldLocationNames[3][12];

  int MergeArray(svtkDataArray* in, svtkDataArray* out, int inComp, int outComp);

  // Components are stored as a linked list.
  Component* Head;
  Component* Tail;

  // Methods to browse/modify the linked list.
  Component* GetNextComponent(Component* op) { return op->Next; }
  Component* GetFirst() { return this->Head; }
  void AddComponent(Component* op);
  Component* FindComponent(int index);
  void DeleteAllComponents();

  void PrintComponent(Component* op, ostream& os, svtkIndent indent);
  void PrintAllComponents(ostream& os, svtkIndent indent);

private:
  svtkMergeFields(const svtkMergeFields&) = delete;
  void operator=(const svtkMergeFields&) = delete;
};

#endif
