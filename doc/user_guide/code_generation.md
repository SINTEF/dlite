Code generation
===============

Content
-------
  1. [Introduction](#introduction)
  2. [Example](#example)
  3. [Custom extension of the metadata struct](#custom-extension-of-the-metadata-struct)
  4. [Code generation of meta-metadata](#code-generation-of-meta-metadata)
  5. [Writing a template](#writing-a-template)


TODO: write text!


Introduction
------------
xxx

Example
-------
xxx

Custom extension of the metadata struct
---------------------------------------
xxx


Code generation of meta-metadata
--------------------------------

There is a subtile issue with code generation of structures for
meta-metadata, which is a result of the memory layout of metadata in
dlite.  Lets say we want to generate a header file with a C struct
corresponding to the built-in EntitySchema, which describe the memory
layout of metadata.  Since last two fields of this struct are the
arrays `__propdiminds[]` and `__propoffsets[]`.  The length of these
arrays depends on the number of properties of the metadata instance,
which we don't know at the time we generate


Writing a template
------------------
xxx
