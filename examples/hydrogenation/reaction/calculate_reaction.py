import dlite
import os

dlite.storage_path.append(f'../molecules/*.json')
#from ../config_paths import *

#dlite.storage_path.append(os.cwd)
# input from chemical engineer
reactants = {'c2h6':1}
products = {'c2h4':1,'h2':1}

molecule_colletion = dlite.Instance('json:../molecules/atomscaledata.json?mode=r#molecules')


''' Reaction hasReactants reactants'''
''' REaction hasProduts products'''


