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

  [x] Fix the json plugin such that we can load and save metadata.

  [ ] Implement Collections

  [ ] Add API for automatically look up and load metadata such that the
      user can load an instance with a given uuid without knowing its
      type (entity) a priori.  Add functions to register a set of storages
      that should be checked.

  [x] Automatically lookup the metadata when dlite_instance_load() is
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

  [x] Make handling of dliteStringPtr consistent.  Pointers to it should
      always be a (char **) regardless on it is a scalar or an array.

  [x] Fix bug in handling of dliteFixString that course that an additional
      byte must be allocated for the therminating NUL in
      dlite_instance_create().


Lower priprity

  [ ] Make the plugins dynamical loadable (ensure cross-platform)

  [ ] Add a plugin for postgresql storage (useful with AiiDA).

  [ ] Add a plugin for mongodb storage (really needed if we have postgresql?)

  [ ] Add a plugin for sqlite storage (might be useful for standalone
      applications)

  [ ] Make shared resources (e.g. the metadata database) tread-safe,
      by adding locks.  To ensure cross-platform,

  [ ] Add api for working with the actual data (both on individual
      elements and nd arrays), like:
        - compare
        - copy
        - free
        - array slicing (using strides)
        - pretty printing

  [ ] Improve the implementation of triplestore.c.  By adding a map
      to the TripleStore struct that maps the triplet uri's to the
      corresponding triplet, triplestore_add() can be implemented without
      sorting reducing it from O(n*log(n)) to O(1).  A
      triplestore_find_by_uri() can also be added with O(1) search time.

      To allow removing triplets during iteration, we should add an
      iteration counter to the TripleStore struct and iterate backwards.
      Deleted triplets should only be marked as deleted by setting their
      uri to NULL while any iteration is running.  When the last iteration
      is finished (the iteration counter reaches zero), a single sweep
      through the list of triplets is performed and any deleted triplet
      is swapped with the last triplet.  Finally the triplet list may be
      reallocated.

  [x] Formalise the options passed to dlite_storage_open(). Require that
      it should be a valid url query string, of the form:

          key1=value1;key2=value2...

      The semicolon may be replaced by an ampersand (&).

  [ ] Update dh5open() to use dlite_option_parse().

  [ ] Add way to encode/decode DLiteDimension, DLiteProperty and
      DLiteRelation as string arrays.  This would allow all plugins to
      transparently support these types without each plugin having to
      explicit implement it.

  [ ] Remove or use fmtSchema in the json plugin.



Design questions
================

  [x] Except for reference counting, is it correct to consider all
      metadata as immutable?  Are there any exceptions (Python allows
      dynamic classes, but do we need such flexibility)?

      Answer: All metadata should be immutable. That why we have versioning.
      This is one of the key concepts of SOFT that we should stick to.

  [x] Should we remove DLiteSchemaDimension and DLiteSchemaProperty
      and instead use DLiteDimension and DLiteProperty for all metadata?

      Implications:
        - Simplify the implementation.
        - Properties of meta-metadata will have units.
        - Disallow to define custom properties containing with new fields,
          essentially restricting the semantics.

      Thoughts: Seems to be a good idea.  But think through whether a
      restriction of the semantics may make interoperabililty between
      dlite and other semantic systems more difficult?

      Answer: Yes. Keep it stupid, simple!  Our metadata semantics
      contains only dimensions, properties and relations.  These basic
      building blocks should be well-defined, without any variations.

  [ ] Is it ok to require that the two first properties defined by
      meta-metadata always must be "dimensions" and "properties"?
      If so, should we also require that "relations" must be the third
      property if the instances of the meta-metadata has relations?

      This would simplify the implementation of e.g.
      dlite_instance_get_dimension_name_by_index() and
      dlite_instance_get_property_name_by_index().

      An argument for treating dimensions, properties and relations
      specially, is that they are a the building blocks of the semantics
      we use to define all data and metadata.

      Thoughts: Is currently implemented for simplicity.  But it is a
      constraint that might be nice to release, though.  If one want
      to define a new metadata with e.g. "comment" property, it will
      be unnatural and bug-prone to require to also define
      "relations".

  [ ] The current implementation duplicates the values of "ndimensions",
      "nproperties" and "nrelations" in all metadata ("nrelations" is
      only duplicated if the metadata has relations).

      Should we get rid of this duplication?

      Thoughts: Not sure, duplication seems to be the easiest
      solution.  It is very handy to have the dimension values defined
      in the metadata header such that we can refer to them directly.
      For simplicity we also want to store the dimension values after
      the header, such that metadata can be treated similar to data
      instances.  Since metadata are immutable, there are no risk for
      introducing inconsistencies.


  [x] Would it be useful to add a boolean flag to DLiteMeta_HEAD that
      tells whether its instances are metadata?  This would e.g. allow
      an instance to check whether it itself is metadata.

      This will make it possible for dlite_instance_create() to know
      whether additional initialisation for metadata should be
      performed.

      Thoughts: Probably not.  The definition of metadata does not
      contain such a flag, so how should dlite_instance_create() know
      how to set this on a new instance?

      However, given we agree to require "dimensions" to be the first
      property of meta-metadata and to remove DLiteSchemaDimension, an
      alternative would be to check the type of the first property of
      the metadata.  If that is dliteDimension, we know that the
      current instance is metadata.  Would this be robust enough?

      Answer: No need to add such a flag.  The dlite_meta_ismetameta()
      functions determines this by investigating the meta-metadata.

  [ ] Is it a good idea to add some hooks to DLiteMeta_HEAD?

          int (*init)(DLiteInstance *inst);
          int (*deinit)(DLiteInstance *inst);

          int (*load)(const DLiteDataModel *d, DLiteInstance *inst);
          int (*save)(DLiteDataModel *d, const DLiteInstance *inst);

      where init()/deinit() will be called as the last/first thing
      in dlite_instance_create()/dlite_instance_free() and
      load()/save() would, if defined, replacing the default
      interaction with the datamodel.

      These hooks should not be used for initialisation of metadata,
      since they are not described by the metadata semantics, but may
      provide very useful for specialised entities that needs
      additional initialisation, like creation of a triplestore in
      collections.

      Thoughts: Only add them if we see a need, e.g. for implementing
      collections.

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

      Thoughts: Maybe not, since this would interact with the storage
      for each iteration, which potentially could be very slow if the
      storage is a database on a slow network...

  [ ] Should we change the type of metadata arguments from
      DLiteEntity to DLiteMeta in the dlite_instance_*() functions?
      Are there any reasons to keep the DLiteEntity type as a
      special case of DLiteMeta?

  [ ] Do we need we distinguish between metadata whos instances are normal
      data and meta-metadata in the json plugin?  The alternative would
      be to drop the "schema_" prefix in "schema_dimensions",
      "schema_properties", etc. for meta-metadata.  Instead we could
      identify meta-metadata by checking if it has any property called
      "dimensions" and "properties".
