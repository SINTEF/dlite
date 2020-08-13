
#include "dlite.h"
#include "chemistry_schema.h"
#include "chemistry.h"

/* Forward declarations */
const ChemistrySchema *get_chemistry();



int main()
{
  const ChemistrySchema *chem = get_chemistry();
  dlite_instance_print((DLiteInstance *)chem);




  return 0;
}
