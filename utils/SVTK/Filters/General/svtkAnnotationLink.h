/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAnnotationLink.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAnnotationLink
 * @brief   An algorithm for linking annotations among objects
 *
 * svtkAnnotationLink is a simple source filter which outputs the
 * svtkAnnotationLayers object stored internally.  Multiple objects may share
 * the same annotation link filter and connect it to an internal pipeline so
 * that if one object changes the annotation set, it will be pulled into all
 * the other objects when their pipelines update.
 *
 * The shared svtkAnnotationLayers object (a collection of annotations) is
 * shallow copied to output port 0.
 *
 * svtkAnnotationLink can also store a set of domain maps. A domain map is
 * simply a table associating values between domains. The domain of each
 * column is defined by the array name of the column. The domain maps are
 * sent to a multi-block dataset in output port 1.
 *
 * Output ports 0 and 1 can be set as input ports 0 and 1 to
 * svtkConvertSelectionDomain, which can use the domain maps to convert the
 * domains of selections in the svtkAnnotationLayers to match a particular
 * data object (set as port 2 on svtkConvertSelectionDomain).
 *
 * The shared svtkAnnotationLayers object also stores a "current selection"
 * normally interpreted as the interactive selection of an application.
 * As a convenience, this selection is sent to output port 2 so that it
 * can be connected to pipelines requiring a svtkSelection.
 */

#ifndef svtkAnnotationLink_h
#define svtkAnnotationLink_h

#include "svtkAnnotationLayersAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkCommand;
class svtkDataObjectCollection;
class svtkInformation;
class svtkInformationVector;
class svtkSelection;
class svtkTable;

class SVTKFILTERSGENERAL_EXPORT svtkAnnotationLink : public svtkAnnotationLayersAlgorithm
{
public:
  static svtkAnnotationLink* New();
  svtkTypeMacro(svtkAnnotationLink, svtkAnnotationLayersAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The annotations to be shared.
   */
  svtkGetObjectMacro(AnnotationLayers, svtkAnnotationLayers);
  virtual void SetAnnotationLayers(svtkAnnotationLayers* layers);
  //@}

  //@{
  /**
   * Set or get the current selection in the annotation layers.
   */
  virtual void SetCurrentSelection(svtkSelection* sel);
  virtual svtkSelection* GetCurrentSelection();
  //@}

  //@{
  /**
   * The domain mappings.
   */
  void AddDomainMap(svtkTable* map);
  void RemoveDomainMap(svtkTable* map);
  void RemoveAllDomainMaps();
  int GetNumberOfDomainMaps();
  svtkTable* GetDomainMap(int i);
  //@}

  /**
   * Get the mtime of this object.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkAnnotationLink();
  ~svtkAnnotationLink() override;

  /**
   * Called to process modified events from its svtkAnnotationLayers.
   */
  virtual void ProcessEvents(svtkObject* caller, unsigned long eventId, void* callData);

  /**
   * Set up input ports.
   */
  int FillInputPortInformation(int, svtkInformation*) override;

  /**
   * Set up output ports.
   */
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Copy the data to the output objects.
   */
  void ShallowCopyToOutput(
    svtkAnnotationLayers* input, svtkAnnotationLayers* output, svtkSelection* sel);

  /**
   * Shallow copy the internal selection to the output.
   */
  int RequestData(svtkInformation* info, svtkInformationVector** inVector,
    svtkInformationVector* outVector) override;

  /**
   * The shared selection.
   */
  svtkAnnotationLayers* AnnotationLayers;

  /**
   * The mappings between domains.
   */
  svtkDataObjectCollection* DomainMaps;

private:
  svtkAnnotationLink(const svtkAnnotationLink&) = delete;
  void operator=(const svtkAnnotationLink&) = delete;

  class Command;
  friend class Command;
  Command* Observer;
};

#endif
