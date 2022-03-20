import os
import sys
import warnings
import fnmatch

import psycopg2
from psycopg2 import sql

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict


# Translation table from dlite types to postgresql types
pgtypes = {
    'blob': 'bytea',
    'bool': 'bool',
    'int': 'integer',
    'int8': 'bytea',
    'int16': 'smallint',
    'int32': 'integer',
    'int64': 'bigint',
    'uint16': 'integer',
    'uint32': 'bigint',
    'float': 'real',
    'double': 'float8',
    'float32': 'real',
    'float64': 'float8',
    'string': 'varchar',
    'dimension': 'varchar[2]',
    'property': 'varchar[5]',
    'relation': 'varchar[3]',
}

def to_pgtype(typename):
    """Returns PostGreSQL type corresponding to dlite typename."""
    if typename in pgtypes:
        return pgtypes[typename]
    else:
        t = typename.rstrip('0123456789')
        return pgtypes[t]


class postgresql(dlite.DLiteStorageBase):
    """DLite storage plugin for PostgreSQL."""
    def open(self, uri, options=None):
        """Opens `uri`.

        The `options` argument provies additional input to the driver.
        Which options that are supported varies between the plugins.  It
        should be a valid URL query string of the form:

            key1=value1;key2=value2...

        An ampersand (&) may be used instead of the semicolon (;).

        Typical options supported by most drivers include:
        - database : Name of database to connect to (default: dlite)
        - user : User name.
        - password : Password.
        - mode : append | r
            Valid values are:
            - append   Append to existing file or create new file (default)
            - r        Open existing file for read-only

        After the options are passed, this method may set attribute
        `writable` to true if it is writable and to false otherwise.
        If `writable` is not set, it is assumed to be true.
        """
        self.options = Options(options, defaults='database=dlite;mode=append')
        opts = self.options
        opts.setdefault('password', None)
        self.writable = False if opts.mode == 'r' else True

        # Connect to existing database
        print('  host:', uri)
        print('  user:', opts.user)
        print('  database:', opts.database)
        #print('  password:', opts.password)
        self.conn = psycopg2.connect(host=uri, database=opts.database,
                                     user=opts.user, password=opts.password)

        # Open a cursor to perform database operations
        self.cur = self.conn.cursor()

    def close(self):
        """Closes this storage."""
        self.cur.close()
        self.conn.close()

    def load(self, uuid):
        """Loads `uuid` from current storage and return it as a new instance."""
        uuid = dlite.get_uuid(uuid)
        q = sql.SQL('SELECT meta FROM uuidtable WHERE uuid = %s')
        self.cur.execute(q, [uuid])
        metaid, = self.cur.fetchone()
        q = sql.SQL('SELECT * FROM {} WHERE uuid = %s').format(
            sql.Identifier(metaid))
        self.cur.execute(q, [uuid])
        tokens = self.cur.fetchone()
        uuid_, uri, metaid_, dims = tokens[:4]
        values = tokens[4:]
        assert uuid_ == uuid
        assert metaid_ == metaid

        # Make sure we have metadata object correcponding to metaid
        try:
            with dlite.err():
                meta = dlite.get_instance(metaid)
        except RuntimeError:
            dlite.errclr()
            meta = self.load(metaid)

        inst = dlite.Instance.from_metaid(metaid, dims, uri)

        for i, p in enumerate(inst.meta['properties']):
            inst.set_property(p.name, values[i])

        # The uuid will be wrong for data instances, so override it
        if not inst.is_metameta:
            d = inst.asdict()
            d['uuid'] = uuid
            inst = instance_from_dict(d)
        return inst

    def save(self, inst):
        """Stores `inst` in current storage."""

        # Save to metadata table
        if not self.table_exists(inst.meta.uri):
            self.table_create(inst.meta, inst.dimensions.values())
        colnames = ['uuid', 'uri', 'meta', 'dims'] +  [
            p.name for p in inst.meta['properties']]
        q = sql.SQL('INSERT INTO {0} ({1}) VALUES ({2});').format(
            sql.Identifier(inst.meta.uri),
            sql.SQL(', ').join(map(sql.Identifier, colnames)),
            (sql.Placeholder() * len(colnames)).join(', '))
        values = [inst.uuid,
                  inst.uri,
                  inst.meta.uri,
                  list(inst.dimensions.values()),
        ] + [dlite.standardise(v, inst.get_property_descr(k), asdict=False)
             for k, v in inst.properties.items()]
        try:
            self.cur.execute(q, values)
        except psycopg2.IntegrityError:
            self.conn.rollback()  # Instance already in database
            return

        # Save to uuidtable
        if not self.table_exists('uuidtable'):
            self.uuidtable_create()
        q = sql.SQL('INSERT INTO uuidtable (uuid, meta) VALUES (%s, %s);')
        self.cur.execute(q, [inst.uuid, inst.meta.uri])
        self.conn.commit()

    def table_exists(self, table_name):
        """Returns true if a table named `table_name` exists."""
        self.cur.execute(
            'select exists(select * from information_schema.tables '
            'where table_name=%s);', (table_name, ))
        return self.cur.fetchone()[0]

    def table_create(self, meta, dims=None):
        """Creates a table for storing instances of `meta`."""
        table_name = meta.uri
        if self.table_exists(table_name):
            raise ValueError('Table already exists: %r' % table_name)
        if dims:
            dims = list(dims)
        cols = [
            'uuid char(36) PRIMARY KEY',
            'uri varchar',
            'meta varchar',
            'dims integer[%d]' % meta.ndimensions
        ]
        for p in meta['properties']:
            decl = f'"{p.name}" {to_pgtype(p.type)}'
            if len(p.dims):
                decl += '[]' * len(p.dims)
            cols.append(decl)
        q = sql.SQL('CREATE TABLE {} (%s);' %
                    ', '.join(cols)).format(sql.Identifier(meta.uri))
        self.cur.execute(q)
        self.conn.commit()

    def uuidtable_create(self):
        """Creates the uuidtable - a table mapping all uuid's to their
        metadata uri."""
        q = sql.SQL('CREATE TABLE uuidtable ('
                    'uuid char(36) PRIMARY KEY, '
                    'meta varchar'
                    ');')
        self.cur.execute(q)
        self.conn.commit()

    def queue(self, pattern):
        """Generator method that iterates over all UUIDs in the storage
        who's metadata URI matches glob pattern `pattern`."""
        if pattern:
            # Convert glob patter to PostgreSQL regular expression
            regex = '^{}'.format(fnmatch.translate(globex).replace(
                '\\Z(?ms)', '$'))
            q = sql.SQL('SELECT uuid from uuidtable WHERE uuid ~ %s;')
            self.cur.execute(q, (regex, ))
        else:
            q = sql.SQL('SELECT uuid from uuidtable;')
            self.cur.execute(q)
        tokens = self.cur.fetchone()
        while tokens:
            uuid, = tokens
            yield uuid
            tokens = self.cur.fetchone()
