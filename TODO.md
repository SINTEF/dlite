TODO
====

High priority

  [ ] Put this TODO-list on jira.

  [ ] Check that dlite compiles correct on Windows, especially with the
      BUILD_HDF5 and BUILD_JANSSON cmake options turned on.

  [ ] Fix the WITH_HDF5 and WITH_JANSSON cmake options, such that the
      dlite is compiled without the corresponding libraries when they
      are set to OFF.

  [x] Finalise the json reader.  Handle n-dimensional data correct.

  [x] Make sure that entities can be read from json and used to
      instantiate instances.

  [x] Add reference counting to instances for sane and simple memory
      handling. Will be important for collections.  Metadata is already
      reference-counted, so this should be easy to do.

  [x] Make DLiteDimension, DLiteProperty and DLiteRelation the basic
      building blocks for describing all metadata and data.  Hence,
      there will be no need for the schema_form that Thomas Hagelien
      introduced but never implemented for the basic metadata schema.
      Merge DLiteBaseProperty with DLiteProperty.

  [x] Improve the definition of metadata such that dlite_instance_create()
      also can create metadata.  Reconsider DLiteProperty.  Based on
      this, correct the definition of schema_entity.

  [x] Implement an in-memory database, e.g. for storing instances that
      are added to collections or to cache metadata such that we only
      need to load it once.

  [ ] Implement Collections

  [ ] Add API for automatically look up and load metadata such that the
      user can load an instance with a given uuid without knowing its
      type (entity) a priori.  Add functions to register a set of storages
      that should be checked.

  [ ] Automatically lookup the metadata when dlite_instance_load() is
      called with NULL as metadata.

  [ ] Add variants of dlite_instance_create() and dlite_instance_load()
      whose `meta` argument are strings.

  [ ] Implement translators.  softpy has a working algorithm that can be
      copied.

  [ ] Implement a database of translator plugins.  Separate out the plugin
      system into independent plugins.c and plugins.h file, such that
      they can be reused.  Ensure it is cross-platform.

  [ ] Add Python bindings

  [ ] Add Fortran bindings

  [ ] Update the code generator to generate struct's that maps the
      memory layout of instances (and entities(?)), such that the same
      memory region could be accessed either via the generated struct or
      via the instance api, combining the flexibility of the api with
      the convenience and speed of struct access.  With this in mind,
      instances already store their data with alignment offsets
      determined at compile time.

  [x] Fix memory leaks in the json module. With the current test suite
      ``make memcheck`` reports 8 memory leaks, all related to json. I
      had to remove some calls to json_decref() to get rid of
      double-free's, resulting in memory leaks instead. I think the
      course is inconsistent handling of ownership to json objects. The
      easiest would to let the root node in the storage own everything.
      Then we only need to call json_decref() once.


Lower priprity

  [ ] Make the plugins dynamical loadable (ensure cross-platform)

  [ ] Add a plugin for postgresql storage (useful with AiiDA).

  [ ] Add a plugin for mongodb storage (really needed if we have postgresql?)

  [ ] Add a plugin for sqlite storage (might be useful for standalone
      applications)

  [ ] Make shared resources (e.g. the metadata database) tread-safe,
      by adding locks.  To ensure cross-platform,


Design questions
================

  [x] Except for reference counting, is it correct to consider all
      metadata as immutable?  Are there any exceptions (Python allows
      dynamic classes, but do we need such flexibility)?

      Answer: All metadata should be immutable. That why we have versioning.
      This is one of the key concepts of SOFT that we should stick to.

  [ ] Since metadata are instances of their meta-metadata, it should
      be possible to instantiate them with dlite_instance_create().

      This will e.g. allow adding metadata to a collection, store the
      collection the to disk, load the collection and finally retrieve
      the metadata by calling dlite_collection_get().

      In order to transparently handle instances of specialised
      metadata, like collections, that contain internal data structures
      two function pointers could be added to DLiteMeta_HEAD:

          int (*init)(DLiteInstance *inst);
          int (*deinit)(DLiteInstance *inst);

      that initiates and deinitiates the instances of the (meta)metadata.

      Pointers to the actual implementations of these functions, should
      be available via a lookup-table (use map) using the metadata uri
      as key.  This would even allow possible future plugins to define
      new types of (meta)metadata.  Might turn out to be very useful for
      interoperabililty between dlite and other semantic systems.

  [x] Should we add a boolean flag to DLiteMeta_HEAD that tells
      whether its instances are metadata?  This would e.g. allow an
      instance to check whether it itself is metadata.  Any use cases?

      Answer: No, a flag is not needed and will just bloat the DLiteMeta_HEAD.
      By inspecting the meta-metadata, it is easy to determine whether
      instances of a metadata are themselves metadata. See
      dlite_meta_is_metameta().

  [ ] Would it be smarter to change dlite_storage_uuids() to an iterator?

          int iter=0;  /* Type holding the iteration state. */
          char *uuid;
          while ((uuid = dlite_storage_next(storage, &iter))) {
            ...
          }

      Maybe not, since this would interact with the storage for each
      iteration, which potentially could be very slow if the storage
      is a database on a slow network...
