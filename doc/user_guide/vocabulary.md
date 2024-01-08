# Short vocabulary

The following terms have a special meaning in DLite:

* **Basic metadata schema**: Toplevel meta-metadata which describes itself.
* **Collection**: A specialised instance that contains references to set of instances and relations between them.
  Within a collection instances are labeled.
  See also the [SOFT5 nomenclauture].
* **Data instance**: A "leaf" instance that is not metadata.
* **Entity**: May be any kind of instance, including data instances, metadata instances or meta-metadata instances.
  However, for historical reasons it is often used for "standard" metadata that are instances of meta-metadata "http://onto-ns.com/meta/0.3/EntitySchema".
* **Instance**: The basic data object in DLite.
  All instances are described by their metadata which itself are instances.
  Instances are identified by an UUID.
* **Mapping**: A function that maps one or more input instances to an output instance.
  They are an important mechanism for interoperability.
  Mappings are called translators in SOFT5.
* **Metadata**: A special type of instances that describe other instances.
  All metadata are immutable and has an unique URI in addition to their UUID.
* **Meta-metadata**: Metadata that describes metadata.
* **Relation**: A subject-predicate-object triplet.
  Relations are immutable.
* **Storage**: A generic handle encapsulating actual storage backends.
* **Transaction**: An instance that has a reference to an immutable (frozen) parent instance is called a *transaction*.
  Transactions are very useful for ensuring data provenance and makes it easy to work with time series.
  Conceptually, they share many similarities with git.
  See also the [SOFT5 nomenclauture].
* **uri**: A [uniform resource identifier (URI)][URI] is a generalisation of URL, but follows the same syntax rules.
  In DLite, the term "uri" is used as an human readable identifier for instances (optional for data instances) and has the form `namespace/version/name`.
* **url**: A [uniform resource locator (URL)][URL] is a reference to a web resource, like a file (on a given computer), database entry, web page, etc.
  In DLite url's refer to a storage or even an specific instance in a storage using the general syntax `driver://location?options#fragment`, where `options` and `fragment` are optional.
  If `fragment` is provided, it should be the uuid or uri of an instance.
* **uuid**: A [universal unique identifier (UUID)][UUID] is commonly used to uniquely identify digital information.
  DLite uses the 36 character string representation of uuid's to uniquely identify instances.
  The uuid is generated from the uri for instances that has an uri, otherwise it is randomly generated.

[SOFT5 nomenclauture]: https://confluence.code.sintef.no/display/SOFT/Nomenclature
[URI]: https://en.wikipedia.org/wiki/Uniform_Resource_Identifier
[URL]: https://en.wikipedia.org/wiki/URL
[UUID]: https://en.wikipedia.org/wiki/Universally_unique_identifier
