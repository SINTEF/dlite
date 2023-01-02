"""PostgreSQL storage"""
import fnmatch
from typing import TYPE_CHECKING

import psycopg2
from psycopg2 import sql

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict

if TYPE_CHECKING:  # pragma: no cover
    from typing import Generator, Optional


# Mapping from DLite types to PostgreSQL types
pgtypes = {
    "blob": "bytea",
    "bool": "bool",
    "int": "integer",
    "int8": "bytea",
    "int16": "smallint",
    "int32": "integer",
    "int64": "bigint",
    "uint16": "integer",
    "uint32": "bigint",
    "float": "real",
    "double": "float8",
    "float32": "real",
    "float64": "float8",
    "string": "varchar",
    "dimension": "varchar[2]",
    "property": "varchar[5]",
    "relation": "varchar[3]",
}

def to_pgtype(typename: str) -> str:
    """Returns PostgreSQL type corresponding to dlite typename."""
    return pgtypes.get(
        typename,
        pgtypes[typename.rstrip("0123456789")]
    )


class postgresql(dlite.DLiteStorageBase):
    """DLite storage plugin for PostgreSQL."""

    def open(self, uri: str, options: "Optional[str]" = None) -> None:
        """Opens `uri`.

        After the options are passed, this method may set attribute
        `writable` to `True` if it is writable and to `False` otherwise.
        If `writable` is not set, it is assumed to be true.

        Parameters:
            uri: A fully resolved URI to the PostgreSQL database.
            options: This Argument provides additional input to the driver.
                Which options that are supported varies between the plugins. It
                should be a valid URL query string of the form:

                ```python
                options = "key1=value1;key2=value2;..."
                ```

                An ampersand (`&`) may be used instead of the semicolon (`;`) as a separator
                between the key/value-pairs.

                Typical options supported by most drivers include:

                - `database`: Name of database to connect to (default: dlite).
                - `user`: User name.
                - `password`: User password.
                - `mode`: Mode for opening.
                  Valid values are:

                  - `append`: Append to existing file or create new file (default).
                  - `r`: Open existing file for read-only.

        """
        self.options = Options(options, defaults="database=dlite;mode=append")
        self.options.setdefault("password", None)
        self.writable = self.options.mode != "r"

        # Connect to existing database
        self.connection = psycopg2.connect(
            host=uri,
            database=self.options.database,
            user=self.options.user,
            password=self.options.password,
        )

        # Open a cursor to perform database operations
        self.cursor = self.connection.cursor()

    def close(self) -> None:
        """Closes this storage."""
        self.cursor.close()
        self.connection.close()

    def load(self, uuid: str) -> dlite.Instance:
        """Loads `uuid` from current storage and returns it as a new instance.

        Parameters:
            uuid: The DLite Instance UUID.

        Returns:
            The DLite Instance corresponding to the UUID given as it exists in the
            PostgreSQL database.

        """
        uuid = dlite.get_uuid(uuid)

        sql_query = sql.SQL("SELECT meta FROM uuidtable WHERE uuid = %s")
        self.cursor.execute(sql_query, [uuid])
        metaid = self.cursor.fetchone()

        if metaid is None or len(metaid) != 1:
            raise RuntimeError(f"Could not retrieve meta ID for UUID {uuid}")

        sql_query = sql.SQL("SELECT * FROM {} WHERE uuid = %s").format(
            sql.Identifier(metaid[0])
        )
        self.cursor.execute(sql_query, [uuid])
        tokens = self.cursor.fetchone()
        uuid_, uri, metaid_, dims = tokens[:4]
        values = tokens[4:]

        if uuid_ != uuid:
            raise RuntimeError(
                f"Fetching {uuid} from PostgreSQL database results in a different "
                f"UUID to be returned: {uuid_}"
            )
        if metaid_ != metaid:
            raise RuntimeError(
                f"Fetching {metaid} from PostgreSQL database results in a different "
                f"meta ID to be returned: {metaid_}"
            )

        # Make sure we have a metadata object corresponding to metaid
        try:
            with dlite.err():
                meta = dlite.get_instance(metaid)
        except RuntimeError:
            dlite.errclr()
            meta = self.load(metaid)

        inst: dlite.Instance = dlite.Instance.from_metaid(metaid, dims, uri)

        for index, meta_property in enumerate(inst.meta["properties"]):
            inst.set_property(meta_property.name, values[index])

        # The UUID will be wrong for data instances, so override it
        if not inst.is_metameta:
            inst_dict = inst.asdict()
            inst_dict["uuid"] = uuid
            inst = instance_from_dict(inst_dict)

        return inst

    def save(self, inst: dlite.Instance) -> None:
        """Stores `inst` in current storage.

        Parameters:
            inst: The DLite Instance to store in the PostgreSQL database.

        """
        # Save to metadata table
        if not self._table_exists(inst.meta.uri):
            self._table_create(inst.meta)
        colnames = ["uuid", "uri", "meta", "dims"] + [
            meta_property.name for meta_property in inst.meta["properties"]
        ]
        sql_query = sql.SQL("INSERT INTO {0} ({1}) VALUES ({2});").format(
            sql.Identifier(inst.meta.uri),
            sql.SQL(", ").join(map(sql.Identifier, colnames)),
            (sql.Placeholder() * len(colnames)).join(", ")
        )
        values = [
            inst.uuid,
            inst.uri,
            inst.meta.uri,
            list(inst.dimensions.values()),
        ] + [
            dlite.standardise(value, inst.get_property_descr(key), asdict=False)
            for key, value in inst.properties.items()
        ]
        try:
            self.cursor.execute(sql_query, values)
        except psycopg2.IntegrityError:
            self.connection.rollback()  # Instance already in database
            return

        # Save to uuidtable
        if not self._table_exists("uuidtable"):
            self._uuidtable_create()
        sql_query = sql.SQL("INSERT INTO uuidtable (uuid, meta) VALUES (%s, %s);")
        self.cursor.execute(sql_query, [inst.uuid, inst.meta.uri])
        self.connection.commit()

    def instances(self, pattern: str) -> "Generator[str, None, None]":
        """Generator method that iterates over all UUIDs in the storage
        whose metadata URI matches glob pattern `pattern`.

        Parameters:
            pattern: Regular expression to identify DLite Instance UUIDs.

        Yields:
            DLite Instance UUIDs matching the regular expression UUID pattern or all
            instance UUIDs in the storage if no pattern is given.

        """
        if pattern:
            # Convert glob patter to PostgreSQL regular expression
            regex = "^{}".format(
                fnmatch.translate(pattern).replace("\\Z(?ms)", "$")
            )
            sql_query = sql.SQL("SELECT uuid from uuidtable WHERE uuid ~ %s;")
            self.cursor.execute(sql_query, [regex])
        else:
            sql_query = sql.SQL("SELECT uuid from uuidtable;")
            self.cursor.execute(sql_query)

        tokens = self.cursor.fetchone()

        while tokens:
            uuid, = tokens
            yield uuid
            tokens = self.cursor.fetchone()

    def _table_exists(self, table_name: str) -> str:
        """Returns true if a table named `table_name` exists."""
        self.cursor.execute(
            "SELECT EXISTS(SELECT * FROM information_schema.tables "
            "WHERE table_name=%s);",
            [table_name],
        )
        return self.cursor.fetchone()[0]

    def _table_create(self, meta: dlite.Metadata) -> None:
        """Creates a table for storing instances of `meta`.

        Parameters:
            meta: Metadata around which to create an SQL table.

        """
        table_name = meta.uri
        if self._table_exists(table_name):
            raise ValueError(f"Table already exists: {table_name!r}")
        cols = [
            "uuid char(36) PRIMARY KEY",
            "uri varchar",
            "meta varchar",
            f"dims integer[{meta.ndimensions}]"
        ]
        for meta_property in meta["properties"]:
            decl = f"{meta_property.name} {to_pgtype(meta_property.type)}"
            if len(meta_property.dims):
                decl += "[]" * len(meta_property.dims)
            cols.append(decl)

        sql_query = sql.SQL(
            f"CREATE TABLE {{}} (%s);" %
            ", ".join(cols)).format(sql.Identifier(meta.uri))
        self.cursor.execute(sql_query)
        self.connection.commit()

    def _uuidtable_create(self) -> None:
        """Creates the uuidtable - a table mapping all uuid"s to their
        metadata uri."""
        sql_query = sql.SQL(
            "CREATE TABLE uuidtable (uuid char(36) PRIMARY KEY, meta varchar);"
        )
        self.cursor.execute(sql_query)
        self.connection.commit()
