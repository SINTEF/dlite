"""A DLite storage plugin for BSON written in Python."""
import os
import subprocess
import sys

import binascii
from binascii import unhexlify
import bson as pybson # Must be pymongo.bson

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict


class bson(DLiteStorageBase):
    """DLite storage plugin for BSON."""

    def open(self, uri, options=None):
        """Open `uri`.

        The `options` argument provies additional input to the driver.
        Which options that are supported varies between the plugins.
        It should be a valid URL query string of the form:

            key1=value1;key2=value2...

        An ampersand (&) may be used instead of the semicolon (;).

        Typical options supported by most drivers include:
        - mode : append (default) | r | w
            Valid values are:
            - append   Append to existing file or create new file
            - r        Open existing file for read-only
            - w        Truncate existing file or create new file

        After the options are passed, this method may set attribute
        `writable` to True if it is writable and to False otherwise.
        If `writable` is not set, it is assumed to be True.
        
        The BSON data is translated to JSON.
        """
        self.options = Options(options, defaults='mode=append')
        self.mode = dict(r='rb', w='wb', append='rb+')[self.options.mode]
        self.writable = False if 'rb' in self.mode else True
        self.uri = uri
        self.d = {}
        if self.mode in ('rb', 'rb+') and os.path.exists(uri):
            with open(uri, self.mode) as f:
                bson_data = f.read()
                if pybson.is_valid(bson_data):
                    self.d = pybson.decode(bson_data)
                    if self.d == {}:
                        raise EOFError("Failed to read BSON data from " + uri)
                else:
                    raise EOFError("Invalid BSON data in source " + uri)

    def close(self):
        """Close this storage and write the data to file.

        Assumes the data to store is in JSON format.
        """
        if self.writable:
            if self.mode == 'rb+' and not os.path.exists(self.uri):
                mode = 'wb'
            else:
                mode = self.mode
            esc_seq = { # Escape sequences
                '5c30' : '00', # Null: '\\0' -> '\0' = hex 00
                '5c61' : '07', # Bell: '\\a' -> '\a' = hex 07
                '5c62' : '08', # Backspace: '\\b' -> '\b' = hex 08
                '5c74' : '09', # Horizontal Tab: '\\t' -> '\t' = hex 09
                '5c6e' : '0a', # Line Feed: '\\n' -> '\n' = hex 0a
                '5c76' : '0b', # Vertical Tab: '\\v' -> '\v' = hex 0b
                '5c66' : '0c', # Form Feed: '\\f' -> '\f' = hex 0c
                '5c72' : '0d', # Carriage Return: '\\r' -> '\r' = hex 0d
                '5c65' : '1b', # Escape: '\\e' -> '\e' = hex 1b
                '5c5c' : '5c', # Backslash: '\\\\' -> '\\' = hex 5c
                }
            entries = {}
            for uuid in self.queue():
                d = str(self.d[uuid])
                sp = d.split("bytearray(b'")
                d = sp.pop(0)
                # Convert binary strings in sp to hex strings
                for s in sp:
                    n = s.find("')")
                    raws = "r'" + s[:n] + "'"
                    # NOTE 02.12.2021:
                    # DLite corrupts binary strings in Python, but not
                    # raw strings, so s[:n].encode('utf-8').hex()
                    # does not work, but 'hexs' below does
                    hexs = raws.encode('utf-8').hex()
                    # hexs must be '7227' + h + '27', where h is a
                    # hexadecimal string of length divisible by two,
                    # since each binary value maps to a two-digit
                    # hexadecimal number.
                    # Each pair of hexadecimal digits HH in h are
                    # on one of the following forms:
                    # > '5cXX' = an escape sequence in esc_seq,
                    #            e.g. '5c6e' = b'\n'
                    # > 'XXXX' = two binary characters,
                    #            e.g. '3130' = b'10'
                    # > '5c78' = the first four of eight digits
                    #            representing one binary character
                    #            on the form '5c78XXXX',
                    #            e.g. '5c783130' = b'\x10'
                    hexs = hexs.lstrip('7227') # Remove "r'"
                    hexs = hexs.rstrip('27') # Remove "'"
                    d = d + "'"
                    pos = 0
                    while pos < len(hexs):
                        wrd = hexs[pos:(pos + 4)]
                        pos = pos + 4
                        if wrd[0:2] == '5c':
                            if wrd[2:4] == '78':
                                wrd = hexs[pos:(pos + 4)]
                                pos = pos + 4
                                d = d + unhexlify(wrd).decode('utf-8')
                            else:
                                d = d + esc_seq[wrd]
                        else:
                            d = d + wrd
                    d = d + "'" + s[(n + 2):]
                entries[uuid] = d
            with open(self.uri, mode) as f:
                f.write(pybson.encode(entries))

    def load(self, uuid):
        """Load `uuid` from current storage and return it
        as a new instance.
        """
        for key, value in self.d.items():
            if key == uuid:
                if type(value) == str:
                    value = eval(value) # Translate to dict
                # Create instance and return it
                meta = dlite.get_instance(value['meta'])
                if meta.is_metameta:
                    return instance_from_dict(value)
                dims = list(value['dimensions'].values())
                inst = dlite.Instance(meta.uri, dims, uuid)
                for p in meta['properties']:
                    if p.get_type().startswith('blob'): # Binary data
                        data = value['properties'][p.name]
                        try:
                            # Test for valid binary string 'data'
                            bin_data = bytes(data)
                        except:
                            # Assume hexadecimal string 'data'
                            bin_data = bytes(binascii.unhexlify(data))
                        inst[p.name] = bin_data
                    else:
                        inst[p.name] = value['properties'][p.name]
                return inst
        raise KeyError("Instance with id '" + uuid + "' not found")

    def save(self, inst):
        """Store `inst` in the current storage."""
        self.d[inst.uuid] = inst.asdict()

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in
        the storage who's metadata URI matches global pattern
        `pattern`.
        """
        for uuid, d in self.d.items():
            if pattern and dlite.globmatch(pattern, d['meta']):
                continue
            yield uuid

