:py:mod:`dlite.protocol`
========================

.. py:module:: dlite.protocol

.. autoapi-nested-parse::

   Protocol plugins.

   The module also include a few help functions for calling
   functions/class methods and using zipfile to transparently
   representing a file directory.



Module Contents
---------------

Classes
~~~~~~~

.. autoapisummary::

   dlite.protocol.Protocol



Functions
~~~~~~~~~

.. autoapisummary::

   dlite.protocol.call
   dlite.protocol.load_path
   dlite.protocol.save_path
   dlite.protocol.is_archive
   dlite.protocol.archive_check
   dlite.protocol.archive_names
   dlite.protocol.archive_extract
   dlite.protocol.archive_add



Attributes
~~~~~~~~~~

.. autoapisummary::

   dlite.protocol.zip_compressions


.. py:class:: Protocol(protocol, location, options=None)

   Provides an interface to protocol plugins.

   :param protocol: Name of protocol.
   :param location: Location of resource.  Typically a URL or file path.
   :param options: Options passed to the protocol plugin.

   .. py:attribute:: conn
      

      

   .. py:attribute:: protocol
      

      

   .. py:attribute:: closed
      :annotation: = False

      

   .. py:method:: close()

      Close connection.


   .. py:method:: load(uuid=None)

      Load data from connection and return it as a bytes object.


   .. py:method:: save(data, uuid=None)

      Save bytes object `data` to connection.


   .. py:method:: delete(uuid)

      Delete instance with given `uuid` from connection.


   .. py:method:: query(pattern=None)

      Generator method that iterates over all UUIDs in the connection
      who"s metadata URI matches glob pattern `pattern`.

      :param pattern: Glob pattern for matching metadata URIs.

      :Yields: UUIDs of instances who's metadata URI matches glob pattern `pattern`.
               If no `pattern` is given, the UUIDs of all instances in the
               connection are yielded.


   .. py:method:: load_plugins()
      :classmethod:

      Load all protocol plugins.

      The names of all plugin files that have been attempted to load
      are cached (regardless whether loading succeeded or
      not). Hence, it is safe to call this class method multiple
      times.  It won't reload already loaded plugins or try to
      reload plugins that already failed to load.



   .. py:method:: loaded_plugins()
      :classmethod:

      Return a set with the names of already loaded plugins.


   .. py:method:: failed_plugins()
      :classmethod:

      Return a set with the stem of the plugin files that have failed ot
      load.  This is typically the same as the plugin names.


   .. py:method:: __enter__()


   .. py:method:: __exit__(*exc)


   .. py:method:: __del__()



.. py:function:: call(func, *args, **kwargs)

   Call a function.  Keyword arguments that are None, will not be passed
   to `func` if `func` does not have this argument.


.. py:data:: zip_compressions
   

   

.. py:function:: load_path(path, include=None, exclude=None, compression='lzma', compresslevel=None)

   Returns content of path as bytes.

   If `path` is a directory, it will be zipped first.

   :param path: File-like object or name to a regular file, directory or
                socket to load from .
   :param include: Regular expression matching file names to be included
                   when path is a directory.
   :param exclude: Regular expression matching file names to be excluded
                   when path is a directory.  May exclude files included with
                   `include`.
   :param compression: Compression to use if path is a directory.
                       Should be "none", "deflated", "bzip2" or "lzma".
   :param compresslevel: Integer compression level.
                         For compresison "none" or "lzma"; no effect.
                         For compresison "deflated"; 0 to 9 are valid.
                         For compresison "bzip2"; 1 to 9 are valid.

   :returns: Bytes object with content of `path`.


.. py:function:: save_path(data, path, overwrite=False, include=None, exclude=None)

   Save `data` to file path.

   :param data: Bytes object to save.
   :param path: File-like object or name to a regular file, directory or
                socket to save to.
   :param overwrite: Whether to overwrite an existing path.
   :param include: Regular expression matching file names to be included
                   when path is a directory.
   :param exclude: Regular expression matching file names to be excluded
                   when path is a directory.  May exclude files included with
                   `include`.


.. py:function:: is_archive(data)

   Returns true if binary data `data` is an archieve, otherwise false.


.. py:function:: archive_check(data)

   Raises an exception if `data` is not an archive.  Otherwise return
   a io.BytesIO object for data.


.. py:function:: archive_names(data)

   If `data` is an archive, return a list of archive file names.


.. py:function:: archive_extract(data, name)

   If `data` is an archieve, return object with given name.


.. py:function:: archive_add(data, name, content)

   If `data` is an archieve, return a new bytes object with
   `content` added to `data` using the given name.


