/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLUtilities
 * @brief   XML utilities.
 *
 * svtkXMLUtilities provides XML-related convenience functions.
 * @sa
 * svtkXMLDataElement
 */

#ifndef svtkXMLUtilities_h
#define svtkXMLUtilities_h

#include "svtkIOXMLParserModule.h" // For export macro
#include "svtkObject.h"

class svtkXMLDataElement;

class SVTKIOXMLPARSER_EXPORT svtkXMLUtilities : public svtkObject
{
public:
  static svtkXMLUtilities* New();
  svtkTypeMacro(svtkXMLUtilities, svtkObject);

  /**
   * Encode a string from one format to another
   * (see SVTK_ENCODING_... constants).
   * If special_entites is true, convert some characters to their corresponding
   * character entities.
   */
  static void EncodeString(const char* input, int input_encoding, ostream& output,
    int output_encoding, int special_entities = 0);

  /**
   * Collate a svtkXMLDataElement's attributes to a stream as a series of
   * name="value" pairs (the separator between each pair can be specified,
   * if not, it defaults to a space).
   * Note that the resulting character-encoding will be UTF-8 (we assume
   * that this function is used to create XML files/streams).
   */
  static void CollateAttributes(svtkXMLDataElement*, ostream&, const char* sep = nullptr);

  /**
   * Flatten a svtkXMLDataElement to a stream, i.e. output a textual stream
   * corresponding to that XML element, its attributes and its
   * nested elements.
   * If 'indent' is not nullptr, it is used to indent the whole tree.
   * If 'indent' is not nullptr and 'indent_attributes' is true, attributes will
   * be indented as well.
   * Note that the resulting character-encoding will be UTF-8 (we assume
   * that this function is used to create XML files/streams).
   */
  static void FlattenElement(
    svtkXMLDataElement*, ostream&, svtkIndent* indent = nullptr, int indent_attributes = 1);

  /**
   * Write a svtkXMLDataElement to a file (in a flattened textual form)
   * Note that the resulting character-encoding will be UTF-8.
   * Return 1 on success, 0 otherwise.
   */
  static int WriteElementToFile(
    svtkXMLDataElement*, const char* filename, svtkIndent* indent = nullptr);

  //@{
  /**
   * Read a svtkXMLDataElement from a stream, string or file.
   * The 'encoding' parameter will be used to set the internal encoding of the
   * attributes of the data elements created by those functions (conversion
   * from the XML stream encoding to that new encoding will be performed
   * automatically). If set to SVTK_ENCODING_NONE, the encoding won't be
   * changed and will default to the default svtkXMLDataElement encoding.
   * Return the root element on success, nullptr otherwise.
   * Note that you have to call Delete() on the element returned by that
   * function to ensure it is freed properly.
   */
  static svtkXMLDataElement* ReadElementFromStream(istream&, int encoding = SVTK_ENCODING_NONE);
  static svtkXMLDataElement* ReadElementFromString(
    const char* str, int encoding = SVTK_ENCODING_NONE);
  static svtkXMLDataElement* ReadElementFromFile(
    const char* filename, int encoding = SVTK_ENCODING_NONE);
  //@}

  /**
   * Sets attributes of an element from an array of encoded attributes.
   * The 'encoding' parameter will be used to set the internal encoding of the
   * attributes of the data elements created by those functions (conversion
   * from the XML stream encoding to that new encoding will be performed
   * automatically). If set to SVTK_ENCODING_NONE, the encoding won't be
   * changed and will default to the default svtkXMLDataElement encoding.
   */
  static void ReadElementFromAttributeArray(
    svtkXMLDataElement* element, const char** atts, int encoding);

  /**
   * Find all elements in 'tree' that are similar to 'elem' (using the
   * svtkXMLDataElement::IsEqualTo() predicate).
   * Return the number of elements found and store those elements in
   * 'results' (automatically allocated).
   * Warning: the results do not include 'elem' if it was found in the tree ;
   * do not forget to deallocate 'results' if something was found.
   */
  static int FindSimilarElements(
    svtkXMLDataElement* elem, svtkXMLDataElement* tree, svtkXMLDataElement*** results);

  //@{
  /**
   * Factor and unfactor a tree. This operation looks for duplicate elements
   * in the tree, and replace them with references to a pool of elements.
   * Unfactoring a non-factored element is harmless.
   */
  static void FactorElements(svtkXMLDataElement* tree);
  static void UnFactorElements(svtkXMLDataElement* tree);
  //@}

protected:
  svtkXMLUtilities() {}
  ~svtkXMLUtilities() override {}

  static int FactorElementsInternal(
    svtkXMLDataElement* tree, svtkXMLDataElement* root, svtkXMLDataElement* pool);
  static int UnFactorElementsInternal(svtkXMLDataElement* tree, svtkXMLDataElement* pool);

private:
  svtkXMLUtilities(const svtkXMLUtilities&) = delete;
  void operator=(const svtkXMLUtilities&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkXMLUtilities.h
