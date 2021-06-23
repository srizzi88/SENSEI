/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPDFExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkPDFExporter
 * @brief Exports svtkContext2D scenes to PDF.
 *
 * This exporter draws context2D scenes into a PDF file.
 *
 * If ActiveRenderer is specified then it exports contents of
 * ActiveRenderer. Otherwise it exports contents of all renderers.
 */

#ifndef svtkPDFExporter_h
#define svtkPDFExporter_h

#include "svtkExporter.h"
#include "svtkIOExportPDFModule.h" // For export macro

class svtkContextActor;
class svtkRenderer;

class SVTKIOEXPORTPDF_EXPORT svtkPDFExporter : public svtkExporter
{
public:
  static svtkPDFExporter* New();
  svtkTypeMacro(svtkPDFExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /** The title of the exported document. @{ */
  svtkSetStringMacro(Title);
  svtkGetStringMacro(Title);
  /** @} */

  /** The name of the exported file. @{ */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  /** @} */

protected:
  svtkPDFExporter();
  ~svtkPDFExporter() override;

  void WriteData() override;

  void WritePDF();
  void PrepareDocument();
  void RenderContextActors();
  void RenderContextActor(svtkContextActor* actor, svtkRenderer* renderer);

  char* Title;
  char* FileName;

private:
  svtkPDFExporter(const svtkPDFExporter&) = delete;
  void operator=(const svtkPDFExporter&) = delete;

  struct Details;
  Details* Impl;
};

#endif // svtkPDFExporter_h
