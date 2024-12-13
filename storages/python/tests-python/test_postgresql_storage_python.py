"""Script to test the DLite plugin 'postgresql.py' in Python."""
import ast
import json
import sys
from pathlib import Path

sys.dont_write_bytecode = True

import dlite
from dlite.utils import instance_from_dict
from dlite.testutils import importskip

psycopg2 = importskip("psycopg2", env_exitcode=None)
from psycopg2 import sql

from run_python_storage_tests import print_test_exception


if __name__ in ('__main__', '<run_path>'):
    thisfile = Path(__file__)
    print(f'Running Python test <{thisfile.name}>...')
    thisdir = thisfile.absolute().parent
    input_path = thisdir / 'input'
    dlite_path = thisdir.parent.parent.parent
    plugin = thisdir.parent / 'python-storage-plugins/postgresql.py'

    try:
        with open(plugin, 'r') as orig:
            lines = orig.read().splitlines(keepends=True)
            # Alter the plugin methods open(), load(), save() and
            # close(), to use files for comparison, instead
            # of databases contained in a PostgreSQL server.
            # This removes the need for a server when running the test.
            #
            # The input comparisons are between JSON files and
            # PostgreSQL database dumps:
            # 1: 'test-entity.json' vs. '/input/test_meta.pgsql'
            # 2: 'test-data.json' vs. '/input/test_data.pgsql'
            #
            # The output comparisons are between SQL commands from the
            # save() method (that would have been executed on a server)
            # and text files containing the desired commands:
            # 1: '/input/postgresql_test_meta_save.txt'
            # 2: '/input/postgresql_test_data_save.txt'

            df = '    def '
            ind8 = '        ' # Indent of 8 spaces
            open_start = lines.index(df + 'open(self, uri, options=None):\n')
            close_start = lines.index(df + 'close(self):\n')
            load_start = lines.index(df + 'load(self, uuid):\n')
            save_start = lines.index(df + 'save(self, inst):\n')

            # open(): Don't connect to server - read 'uri' instead
            lines[open_start + 1] = ind8 + 'self.data = open_pgsql(uri)\n'
            lines[open_start + 2] = ind8 + 'self.d = {}\n'

            # close(): Don't disconnect from server - just pass
            lines[close_start + 1] = ind8 + 'pass\n'

            # load(): Don't access server - read from self.data
            lines[load_start + 3] = ind8 + 'self.d[uuid] = ' \
                + 'load_pgsql(self.data, uuid, ["L", "M", "N"])\n'
            lines[load_start + 4] = ind8 \
                + 'return instance_from_dict(self.d[uuid])\n'

            # save(): Don't write to database, but compare the writing
            # commands to the commands in the database dump file
            n = save_start + 2
            lines[n - 1] = ind8 + 'ret = {"uuid": inst.uuid}\n'
            while not lines[n].startswith(df + 'table_exists'):
                lines[n] = lines[n].replace('self.conn', '#')
                lines[n] = lines[n].replace('self.cur.execute(', \
                    'ret = extract_exec_args(ret, ')
                if lines[n].startswith(ind8 + 'if not self.table'):
                    lines[n] = '\n'
                    lines[n + 1] = '\n'
                n += 1
            lines[n - 1] = ind8 + 'return ret\n'

            del lines[(load_start + 5):(save_start - 1)]
            del lines[(close_start + 2):(load_start - 1)]
            del lines[(open_start + 3):(close_start - 1)]
            s = 'from test_postgresql_storage_python import open_pgsql, ' \
                + 'load_pgsql, extract_exec_args\n' + str().join(lines)
            s = s.replace('class postgresql', 'class dlite_postgresql')
        exec(s)

        # Load JSON metadata
        with open(dlite_path / 'src/tests/test-entity.json', 'r') as f:
            json_dict1 = json.load(f)
        json_dict1 = instance_from_dict(json_dict1).asdict()

        # Test loading PostgreSQL metadata
        postgresql_inst1 = dlite_postgresql()
        postgresql_inst1.open(input_path / 'test_meta.pgsql')
        inst = postgresql_inst1.load('2b10c236-eb00-541a-901c-046c202e52fa')
        postgresql_dict = inst.asdict()
        if postgresql_dict == json_dict1:
            print('...Loading metadata ok!')
        else:
            raise ValueError('...Loading metadata failed!')

        # Test saving PostgreSQL metadata
        with open(input_path / 'postgresql_test_meta_save.txt', 'r') as f:
            sql_dict = ast.literal_eval(f.read())
        info = postgresql_inst1.save(inst)
        if info == sql_dict:
            print('...Saving metadata ok!')
        else:
            raise ValueError('...Saving metadata failed!')

        # Load JSON data
        with open(dlite_path / 'src/tests/test-data.json', 'r') as f:
            json_data = f.readlines()
        n = json_data.index('  "e076a856-e36e-5335-967e-2f2fd153c17d": {\n')
        json_dict1 = json.loads('{' + ''.join(json_data[2:(n - 1)]) + '}')
        json_dict1['uuid'] = '204b05b2-4c89-43f4-93db-fd1cb70f54ef'
        json_dict1 = instance_from_dict(json_dict1).asdict()
        json_dict2 = json.loads('{' + ''.join(json_data[(n + 1):-1]))
        json_dict2['uuid'] = 'e076a856-e36e-5335-967e-2f2fd153c17d'
        json_dict2 = instance_from_dict(json_dict2).asdict()

        # Test loading PostgreSQL data
        postgresql_inst2 = dlite_postgresql()
        postgresql_inst2.open(input_path / 'test_data.pgsql')
        inst1 = postgresql_inst2.load('204b05b2-4c89-43f4-93db-fd1cb70f54ef')
        postgresql_dict1 = inst1.asdict()
        inst2 = postgresql_inst2.load('e076a856-e36e-5335-967e-2f2fd153c17d')
        postgresql_dict2 = inst2.asdict()
        if postgresql_dict1 == json_dict1 and postgresql_dict2 == json_dict2:
            print('...Loading data ok!')
        else:
            raise ValueError('...Loading data failed!')

        # Test saving PostgreSQL data
        with open(input_path / 'postgresql_test_data_save.txt', 'r') as f:
            data = f.read()
            n = data.find('"}')
            sql_dict1 = ast.literal_eval(data[:(n + 2)])
            sql_dict2 = ast.literal_eval(data[(n + 2):])
        info1 = postgresql_inst2.save(inst1)
        info2 = postgresql_inst2.save(inst2)
        if info1 == sql_dict1 and info2 == sql_dict2:
            print('...Saving data ok!')
        else:
            raise ValueError('...Saving data failed!')

        print(f'Test <{thisfile.name}> ran successfully')
    except Exception as err:
        if __name__ == '<run_path>':
            print_test_exception(err)
        else:
            raise
else:
    def open_pgsql(uri):
        with open(uri, "r") as f:
            return f.read()

    def load_pgsql(data, uuid, dims_keys=None):
        datalines = data.splitlines()

        # Look for meta value for the given uuid
        line = 'COPY public.uuidtable (uuid, meta) FROM stdin;'
        n = datalines.index(line)
        for dataline in datalines[n:]:
            if dataline == '\.':
                raise KeyError(f"UUID '{uuid}' not found")
            elif dataline.startswith(uuid):
                meta = dataline.split('\t')[-1]
                line_start = 'COPY public."' + meta + '" ('
                break

        # Look for the line that starts with line_start
        keys = None
        for dataline in datalines:
            if dataline == '\.':
                raise ValueError(f"Values for UUID '{uuid}' not found")
            elif keys and dataline.startswith(uuid):
                values = dataline.split('\t')
                break
            elif dataline.startswith(line_start):
                keys = dataline[len(line_start):-13].split(', ')

        d = {keys[k]: values[k] for k in range(len(keys))}
        metadata = False
        if 'dimensions' in d.keys():
            metadata = True
            dims_str = d['dimensions']
            dims_str = dims_str.replace('{{', '[["')
            dims_str = dims_str.replace('}}', ']]')
            dims_str = dims_str.replace('},{', '],["')
            dims_str = dims_str.replace(',"', '","')
            dims = ast.literal_eval(dims_str)
            for dim in dims:
                dim = {'name': str(dim[0]), \
                    'description': str(dim[1])}
            d['dimensions'] = dims
        if 'properties' in d.keys():
            metadata = True
            keys = {0: 'name', 1: 'type', 2: 'dims', 3: 'unit', \
                    5: 'description'}
            prop_str = d['properties']
            prop_str = prop_str.replace('"', '')
            prop_str = prop_str.replace('{{', '[["')
            prop_str = prop_str.replace('}}', '"]]')
            prop_str = prop_str.replace(',', '","')
            prop_str = prop_str.replace('}","{', '"],["')
            prop = ast.literal_eval(prop_str)
            d['properties'] = []
            for n in range(len(prop)):
                d['properties'].append({})
                single_prop = prop[n]
                shape = None
                if single_prop[2] != '':
                    shape = single_prop[2:-3]
                    single_prop[2:-3] = ['']
                for m in range(len(single_prop)):
                    if single_prop[m] != '':
                        d['properties'][n][keys[m]] = single_prop[m]
                    elif m == 2 and shape:
                        d['properties'][n]['shape'] = shape

        if metadata:
            del d['dims']
        else:
            # Data instance
            if 'uri' in d.keys() and d['uri'] == '\\N':
                del d['uri']

            # Identify types from meta
            line = 'CREATE TABLE public."' + meta + '" ('
            n = datalines.index(line) + 1
            type_str = '('
            for dataline in datalines[n:]:
                if dataline == ');':
                    break
                type_str = type_str + '"' + dataline.lstrip() + '"'
            type_str = type_str + ')'
            type_str = type_str.replace(',"', '",')
            types = ast.literal_eval(type_str)
            type_dict = {}
            for t in types:
                s = t.split(' ', 1)
                type_dict[s[0]] = s[1]

            # Check types (only those in test-entity.json, for now)
            prop = {}
            keep = {}
            for key in d:
                d[key] = d[key].replace('{', '[').replace('}', ']')
                if key == 'dims':
                    dims_vals = ast.literal_eval(d[key])
                    keep['dimensions'] = {dims_keys[k]: dims_vals[k] \
                        for k in range(len(dims_keys))}
                    continue
                elif type_dict[key] == 'integer':
                    d[key] = int(d[key])
                elif type_dict[key] == 'double precision':
                    d[key] = float(d[key])
                elif type_dict[key].endswith('bytea'):
                    d[key] = d[key].lstrip('\\x')
                elif type_dict[key].endswith('[]'):
                    d[key] = ast.literal_eval(d[key])
                if key not in ('uuid', 'uri', 'meta'):
                    prop[key] = d[key]
                else:
                    keep[key] = d[key]
            d = keep
            d['properties'] = prop

        return d

    def extract_exec_args(d, arg0, arg1):
        """Expected types:
          d: dict,
          arg0: psycopg2.sql.SQL,
          arg1: list.
        """
        d['arg' + str(len(d))] = str(arg0)
        return d
