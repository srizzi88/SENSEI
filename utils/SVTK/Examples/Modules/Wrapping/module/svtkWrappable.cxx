#include "svtkWrappable.h"

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkWrappable);

svtkWrappable::svtkWrappable() {}

svtkWrappable::~svtkWrappable() {}

void svtkWrappable::PrintSelf(std::ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

std::string svtkWrappable::GetString() const
{
  return "wrapped";
}
