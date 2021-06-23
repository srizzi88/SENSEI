/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXML.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkXMLParser
// .SECTION Description
//

#include "svtkObjectFactory.h"
#include "svtkOutputWindow.h"
#include "svtkXMLParser.h"

class svtkMyXML : public svtkXMLParser
{
public:
  svtkTypeMacro(svtkMyXML, svtkXMLParser);
  static svtkMyXML* New();

protected:
  svtkMyXML() = default;
  void StartElement(const char*, const char**) override {}
  void EndElement(const char*) override {}

private:
  svtkMyXML(const svtkMyXML&) = delete;
  void operator=(const svtkMyXML&) = delete;
};

svtkStandardNewMacro(svtkMyXML);

int TestXML(int argc, char* argv[])
{
  int res = 0;
  svtkOutputWindow::GetInstance()->PromptUserOn();
  if (argc <= 1)
  {
    cout << "Usage: " << argv[0] << " <xml file>" << endl;
    return 1;
  }

  svtkMyXML* parser = svtkMyXML::New();
  parser->SetFileName(argv[1]);
  if (!parser->Parse())
  {
    cout << "Cannot parse the file: " << argv[1] << endl;
    res = 1;
  }
  parser->SetFileName(nullptr);

  if (!parser->Parse("<xml>This is an XML file</xml>"))
  {
    cout << "Cannot parse message" << endl;
    res = 1;
  }

  parser->Delete();

  return res;
}
