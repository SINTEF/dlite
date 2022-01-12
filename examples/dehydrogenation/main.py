#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

try:
    import ase
except:
    HAVE_ASE = False
else:
    HAVE_ASE = True

try:
    import ontopy
except:
    HAVE_ONTOPY = False
else:
    HAVE_ONTOPY = True


def run(script):
    thisdir = Path(__file__).absolute().parent
    path = thisdir / script
    subprocess.check_call([sys.executable, path])

if HAVE_ASE:
    run('1-simple-workflow/molecular_energies.py')
    run('1-simple-workflow/calculate_reaction.py')
    run('2-instance-mappings/calculate_reactions.py')
    run('3-property-mappings/mappings_hard_coded/run.py')

    # Commented out for now - there are some issues in relation with
    # EMMOntoPy that will be fixed in a separate issue...
    #
    #if HAVE_ONTOPY:
    #    run('3-property-mappings/mappings_from_ontology/run_w_onto.py')
    #else:
    #    print("** warning: 'ontopy' is required for running dehydrogenisation "
    #          "workflow 3")
else:
    print("** warning: 'ase' is required for running dehydrogenisation example")
