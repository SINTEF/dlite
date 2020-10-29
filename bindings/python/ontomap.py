"""A dlite module facilitating generation of metadata based on an ontology.

This module uses Owlready2, but can also be used with EMMO-python.

How entities in the ontology are mapped to dlite is described with a
Mapping object, which essentially is a dict and is typically loaded
from a json file.  Valid keys are:

  * 'is_a': How is_a relations are mapped. The value must be a dict with
    the following keys:
      * 'target': Valid values are:
          - 'coll': add relation to the created collection.
          - 'entity': add relation to the relations of the dlite entity.
            This requires that the meta-metadata includes a property
            names 'relations' which is a 1D array of type DLiteRelation.
          - None: Do not map is_a relations (default).
      * 'dlite_name': name of dlite predicate that is_a relations are
        mapped to.  Default is 'is_a'.
  * 'restriction': How restrictions are mapped.  The value must be a dict
    with the following keys:
      * name: How the object property with this name/label is mapped.
        If `name` is of the form `inverse(name2)`, the value specifies
        how the inverse object property of `name2` is mapped. The
        value must be a dict with the following keys:
          * 'some': How existential restrictions are mapped.  A dict with
            the following keys:
              * 'target': Valid values are:
                  - 'property': Map restriction to a dlite property.  The
                    subject of the restriction defined the property iri.
                  - 'coll': Map restriction to a relation in the created
                    collection.
                  - 'entity': Map  restriction to a relation in the dlite
                    entity.  This requires that the meta-metadata includes a
                    property names 'relations' which is a 1D array of type
                    DLiteRelation.
                  - None: Do not map this restriction.
              * 'dlite_name': name of dlite property or relation predicate
                that the relations is mapped to.  Default is the name of
                the class if target is 'property' otherwise the name of
                the object property.
              * 'property': The restriction is mapped to a property. Implies
                ``target='property'``.  The value is a dict with the following
                keys:
                  * 'name_annotation': Map annotation with this label to the
                    dlite property name.
                  * 'type_annotation': Map annotation with this label to the
                    dlite property type.
                  * 'size_annotation': Map annotation with this label to the
                    dlite property size.
                  * 'dims_annotation': A comma-separated string of dimension
                    names.  Maps to dlite property dimensions.
                  * 'unit_annotation': Map annotation with this label to the
                    dlite property unit.
                  * 'description_annotation': Map annotation with this label
                    to the dlite property description.
          * 'only': How universal restrictions are mapped. Same values
            as 'some'.
          * 'exactly': How exact cardinality restrictions are mapped.  Same
            values as 'some'.
          * 'max': How max cardinality restrictions are mapped.  Same
            values as 'some'.
          * 'min': How min cardinality restrictions are mapped.  Same
            values as 'some'.
          * '*': The default if the restriction type is not specified. Same
            values as 'some'.
          * 'subject_ancestor': Only apply this mapping if the subject of this
            restriction has an ancestor with this name/label.
          * 'object_ancestor': Only apply this mapping if the object of this
            restriction has an ancestor with this name/label.
      * 'inverse(*)': How inverse object properties with no matching name are
        mapped.  Same values as 'name'.  The default is no mapping.
      * '*': How object properties with no matching name are mapped.  Same
        values as 'name'.  The default is no mapping.
  * 'equivalent_to': How equivalent_to is mapped.  Since, the reasoner will
    create implied restrictions and is_a relations from it, equivalent_to
    will only be mapped to dlite relations (and not to dlite properties,
    that are already handeled by the implied restrictions).  The value is a
    dict with the following keys:
      * 'target': Valid values are:
          - 'coll': add relation to the created collection.
          - 'entity': add relation to the relations of the dlite entity.
            This requires that the meta-metadata includes a property
            names 'relations' which is a 1D array of type DLiteRelation.
          - None: Do not map is_a relations (default).
      * 'dlite_name': name of dlite predicate that is_a relations are
        mapped to.  Default is 'is_a'.
  * 'annotation': How annotation properties are mapped.  The value must be
    a dict with the following keys:
      * name: How the annotation with this name/label is mapped.  A dict
        with the following keys:
          * 'target': The targets starting with 'property-' implies that
            the entity in the ontology associated with this annotation
            is mapped to a dlite property. Valid values are:
              - 'property-name': Map annotation to a dlite property name.
              - 'property-type': Map annotation to a dlite property type.
              - 'property-size': Map annotation to a dlite property size.
              - 'property-dims': A comma-separated string of dimension names.
                Maps to dlite property dimensions.
              - 'property-unit': Map annotation to a dlite property unit.
              - 'property-description': Map annotation to a dlite property
                description.
              - 'coll': Map annotation to a relation in the created
                collection.
              - 'entity': Map annotation to a relation in the dlite
                entity.  This requires that the meta-metadata includes a
                property names 'relations' which is a 1D array of type
                DLiteRelation.
              - None: Do not map this annotation.
          * 'dlite_name': name of dlite property or relation predicate
            that the annotation is mapped to.  Default is the annotation
            name/label.
      * '*': How annotations with no matching name are mapped.  Same
        values as 'name'.  The default is no mapping.
  * 'class': How classes are mapped.  A dict with the following keys:
      * name: How the class with this name/label is mapped.  A dict
        with the following keys:
          - 'dlite_name': Name of dlite metadata that the class is mapped to.
          - 'dlite_version': Version of dlite metadata that the class is
            mapped to.
          - 'dlite_namespace': Namespace of dlite metadata that the class is
            mapped to.
          - 'dlite_meta': uri or dlite meta-metadata that the class is
            mapped to.  Defaults to "http://meta.sintef.no/0.3/EntitySchema".
      * '*': How classes with no matching name are mapped.  Same values
        as 'name'.  Typically used to provide default dlite version and
        namespace.
  * 'labelname': Name of annotation attribute providing the label that should
    be mapped to the dlite name.  The default is "label".  If None or empty,
    the name of the entity is used.

"""
import sys
import re
import copy

import owlready2

import dlite
from dlite import Instance, Dimension, Property


# We want an ordered dict.  From Python 3.7 (unofficially 3.6) the
# built-in dict conserves insertion order.
if sys.version_info >= (3, 7):
    odict = dict
else:
    from collections import OrderedDict as odict  # noqa: F401


# Default mapping
default_mapping = odict(
    is_a=odict(
        target='coll',
    ),
)


class Mapping(odict):
    """Describes how the ontology is mapped to dlite.

    The first argument may be a json file name, json file object, dict
    or sequence.

    Supports attribute access.
    """
    def __init__(self, mapping=None, **kwargs):
        super().__init__()
        if isinstance(mapping, str):
            with open(mapping, 'rt') as f:
                json.load(f)
        elif hasattr(mapping, 'read'):
            json.load(f)
        elif hasattr(mapping, 'items'):
            for key, value in mapping.items():
                self.__setitem__(key, value)
        elif mapping is not None:
            for key, value in mapping:
                self.__setitem__(key, value)
        for key, value in kwargs.items():
            self.__setitem__(key, value)

    def __getitem__(self, key):
        haskey = super().__contains__(key)
        if isinstance(key, str) and not haskey and '.' in key:
            head, sep, tail = key.partition('.')
            return self[head][tail]
        else:
            return super().__getitem__(key)

    def __setitem__(self, key, value):
        if isinstance(value, dict):
            value = Mapping(value)
        haskey = super().__contains__(key)
        if isinstance(key, str) and not haskey and '.' in key:
            head, sep, tail = key.partition('.')
            m = self.get(head, Mapping())
            m[tail] = value
        else:
            super().__setitem__(key, value)

    def __delitem__(self, key):
        haskey = super().__contains__(key)
        if isinstance(key, str) and not haskey and '.' in key:
            head, sep, tail = key.partition('.')
            del self[head][tail]
        else:
            super().__delitem__(key)

    def __contains__(self, key):
        haskey = super().__contains__(key)
        if isinstance(key, str) and not haskey and '.' in key:
            head, sep, tail = key.partition('.')
            return tail in self[head]
        else:
            return haskey

    def __dir__(self):
        d = super().__dir__()
        return super().__dir__() + list(self.keys())

    def __getattr__(self, key):
        try:
            return self.__getitem__(key)
        except KeyError:
            raise AttributeError(key)

    __setattr__ = __setitem__
    __delattr__ = __delitem__

    def __repr__(self):
        s = super().__repr__()
        return 'Mapping(%s)' % s[1: -1]

    def get(self, key, default=None):
        """Returns the value for key if key is in the dict, else default."""
        return self[key] if key in self else default

    def get_restriction(self, label, type=None):
        """Returns the sub-mapping corresponding to the restriction matching
        `label` or None, if no matching label is found.

        If `type` is given, the sub-mapping matching `label` and `type`
        is returned, or None if match is found.
        """
        if 'restriction' in self:
            if label in self.restriction:
                res = self.restriction[label]
            elif '*' in self.restriction:
                res = self.restriction['*']
            else:
                return None
            if not type:
                return res
            if type in res:
                return res.type
            return res.get('*', None)
        return None

    def get_class(self, label):
        """Returns the sub-mapping corresponding to the class matching
        `label` or None, if no matching label is found.
        """
        if 'class' in self:
            if label in self['class']:
                return self['class'][label]
            else:
                return self['class'].get('*', None)
        return None

    def get_annotation(self, label):
        """Returns the sub-mapping corresponding to the annotation matching
        `label` or None, if no matching label is found.
        """
        if 'annotation' in self:
            if label in self.annotation:
                return self.annotation[label]
            else:
                return self.annotation.get('*', None)
        return None


class Metadata:
    """Represents a class in an ontology as a dlite metadata.

    Parameters
    ----------
    name : string
        Name of dlite metadata to create.
    version : string
        Version of dlite metadata to create.
    namespace : string
        Namespace of dlite metadata to create.
    schema : string
        IRI to the meta-metadata describing the dlite metadata to create.
    entity : owlready2.ThingClass instance
        The owlready2 representation of the class in the ontology to represent.
    """
    def __init__(self, name, version, namespace, schema, entity):
        assert isinstance(entity, owlready2.ThingClass)

        self.name = name
        self.version = version
        self.namespace = namespace
        self.schema = schema
        self.entity = entity

        self.onto = entity.namespace.ontology
        self.dimensions = []  # list of dimension dicts
        self.properties = []  # list of property dicts
        self.relations = []

    def copy(self):
        """Returns a deep copy of self."""
        return copy.deepcopy(self)

    def __repr__(self):
        return 'Metadata(%r, %r, %r, %r, %r)' % (
            self.name, self.version, self.namespace, self.schema, self.entity)

    def get_meta(self):
        """Returns a dlite metadata."""
        dims = [dlite.Dimension(**d) for d in self.dimensions]
        props = [dlite.Property(**p) for p in self.properties]
        rels = [dlite.Relation(*rel) for rel in self.relations]
        uri = '%s/%s/%s' % (self.namespace, self.version, self.name)
        iri = self.entity.iri
        descr = get_description(self.entity)
        return dlite.Instance(uri, dims, props, iri, descr)


class OntoMap:
    """A class for representing an ontology as a collection of metadata
    entities using DLite.

    Parameters
    ----------
    ontology : string
        URI or path to the ontology with the classes to represent in dlite.
    mapping : None | Mapping instance | dict | fileobj | filename
        Default mappings from ontology to dlite.
    """
    def __init__(self, ontology, mapping=default_mapping):
        if isinstance(ontology, owlready2.Ontology):
            self.onto = ontology
        else:
            self.onto = owlready2.get_ontology(ontology)
            self.onto.load()
        self.mapping = Mapping(mapping)

        self.metadata = {}  # maps iri to Metadata instance
        self.relations = set()  # set of (s, p, o) tuples

        # Cache of object properties specified in mapping.restriction
        self._restriction_maps = {}
        for k, v in mapping.get('restriction', {}).items():
            if k != '*':
                self._restriction_maps[self.get_entity(k)] = v

    def get_entity(self, label):
        """Return the entity in the ontology matching `label`."""
        labelname = self.mapping.get('labelname', 'label')
        if labelname:
            kw = {labelname: label}
            return self.onto.search(**kw).first()
        else:
            return self.onto.search(iri='*' + onto.base_iri[-1] + label)

    def get_restriction_map(self, label, type=None):
        """Returns mapping specification for restriction with given label
        or None if no matching label can be found."""
        if not 'restriction' in self.mapping:
            return None
        r = self.get_entity(label)
        rspec = None
        for p in r.mro():
            if p in self._restriction_maps:
                rspec = self._restriction_maps[p]
                break
        if not rspec and 'restriction' in self.mapping:
            rspec = self.mapping.restriction.get('*')
        if not type:
            return rspec
        if type in rspec:
            return rspec.type
        return rspec.get('*.' + type)

    def add_metadata(self, label, metadata=None):
        """Add mapping from ontology class `label` to `metadata`.
        By default is metadata created from the provided mappings.
        If given explicitely, no mappings are attempted.
        """
        e = self.get_entity(label)
        if metadata is not None:
            md = metadata.copy()
            md.iri = e.iri
            self.metadata[e.iri] = md
            return

        if isinstance(e, owlready2.ThingClass):
            m = self.mapping.get_class(label)
            if not m:
                raise ValueError('Missing mapping information for class %r' %
                                 label)
            if not 'dlite_version' in m:
                raise ValueError('Missing "dlite_version" for class %r' %
                                 label)
            if not 'dlite_namespace' in m:
                raise ValueError('Missing "dlite_namespace" for class %r' %
                                 namespace)
            name = m.get('dlite_name', label)
            meta = m.get('dlite_meta', 'http://meta.sintef.no/0.3/EntitySchema')
            md = Metadata(name=name, version=m.dlite_version,
                          namespace=m.dlite_namespace, schema=meta,
                          entity=e)
            self.metadata[e.iri] = md

            if hasattr(e, 'get_indirect_is_a'):
                is_a = e.get_indirect_is_a()
            else:
                is_a = e.is_a

            for r in is_a:
                if isinstance(r, owlready2.ThingClass):
                    self.add_is_a(e, r)
                elif isinstance(r, owlready2.Restriction):
                    self.add_restriction(e, r)
                elif isinstance(r, owlready2.ClassConstruct):
                    self.add_class_construct(e, r)

    def add_is_a(self, child, parent):
        """Add mapping for `child` isA `parent` relation."""
        target = self.mapping.get('is_a.target')
        if target is None:
            return

        assert child.iri in self.metadata
        if not parent.iri in self.metadata:
            self.add_metadata(parent.get_preferred_label())

        pred = self.mapping.get('is_a.dlite_name', 'is_a')
        rel = (child.get_preferred_label(), pred, parent.get_preferred_label())
        if target == 'coll':
            self.relations.add(rel)
        elif target == 'entity':
            self.metadata[child.iri].relations.append(rel)
        else:
            raise ValueError('is_a.target mappings must be either None, '
                            '"coll" or "entity".  Got %r' % (is_a, ))

    def add_restriction(self, e, r):
        """Add mapping for restriction `r` on entity `e`."""
        print('*** add_restriction:', e, r)

        inv = isinstance(r.property, owlready2.Inverse)
        prop = r.property.property if inv else r.property
        preflabel = prop.get_preferred_label()
        elabel = e.get_preferred_label()
        vlabel = r.value.get_preferred_label()

        m = self.mapping.get('restriction.%s' % preflabel)
        if not m:
            if inv:
                m = self.mapping.get('restriction.inverse(*)')
            else:
                m = self.mapping.get('restriction.*')
        if m:
            m2 = m.get(r.get_typename())
            if not m2 and '*' in m:
                m2 = m['*']
            if m2:
                target = m2.get('target')
                if 'property' in m2:
                    if target and target != 'property':
                        raise ValueError(
                            'mapping for restriction %r has target=%r, but '
                            'defines "property"' % (preflabel, target))
                    target = 'property'
                if not target:
                    return

                if e.iri not in self.metadata:
                    self.add_metadata(elabel)

                if target == 'property':
                    if 'property' not in m2:
                        raise ValueError(
                            'mapping for restriction %r has target '
                            '"property" but not "property" key' % preflabel)
                    p = m2.property
                    a = e.get_annotations()
                    if 'name_annotation' in p:
                        name = a[p.name_annotation]
                    else:
                        name = m2.get('dlite_name', vlabel)
                    d = {'name': name, 'type': a[p.type_annotation],
                         'iri': r.value.iri}
                    if 'size_annotation' in p:
                        d['size'] = a.get(p.size_annotation)
                    if 'dims_annotation' in p:
                        d['dims'] = a.get(p.dims_annotation)
                    if 'unit_annotation' in p:
                        d['unit'] = a.get(p.unit_annotation)
                    if 'description_annotation' in p:
                        d['description'] = a.get(p.description_annotation)
                    self.metadata[e.iri].properties.append(d)

                dlite_name = m2.get('dlite_name', preflabel)
                type = r.get_typename()
                if type in ('some', 'only', 'value'):
                    predicate = '%s.%s' % (dlite_name, type)
                else:
                    predicate = '%s.%s(%d)' % (dlite_name, type, r.cardinality)
                rel = (elabel, predicate, vlabel)
                if target == 'coll':
                    self.relations.add(rel)
                elif target == 'entity':
                    self.metadata[e.iri].relations.append(rel)
                else:
                    raise ValueError(
                        'mapping for restriction %r has invalid target: %r' %
                        (preflabel, target))

    def add_class_construct(self, e, c):
        """Add mapping for class construct `c` for entity `e`."""
        print('*** class_construct:', e, c)
        pass

    def get_collection(self, id=None):
        """Returns a collection with selected metadata entities and relations
        between them.  If `id` is given, the """
        coll = dlite.Collection(id)
        for metadata in self.metadata.values():
            coll.add(metadata.name, metadata.get_meta())
        for rel in self.relations:
            coll.add_relation(*rel)
        return coll


def get_description(entity):
    """Returns a description string for the provided ontology entity."""
    if hasattr(entity, 'get_annotations'):
        d = entity.get_annotations()
    elif hasattr(entity, 'get_individual_annotations'):
        d = entity.get_individual_annotations()
    elif hasattr(entity, 'comment'):
        d = {'comment': entity.comment}
    else:
        d = {}
    s = []
    s.extend(['DEFINITION:\n' + v for v in d.get('definition', [])])
    s.extend(['ELUCIDATION:\n' + v for v in d.get('elucidation', [])])
    s.extend(['COMMENT %d:\n' % (i + 1) + v
              for i, v in enumerate(d.get('comment', []))])
    s.extend(['EXAMPLE %d:\n' % (i + 1) + v
              for i, v in enumerate(d.get('example', []))])
    return '\n\n'.join(s)


    #def get_subclasses(self, cls):
    #    """Returns a generator yielding all subclasses of owl class `cls`."""
    #    yield cls
    #    for subcls in cls.subclasses():
    #        yield from self.get_subclasses(subcls)
    #
    #def get_uri(self, name, version=None):
    #    """Returns uri (namespace/version/name)."""
    #    if version is None:
    #        version = self.version
    #    return '%s/%s/%s' % (self.namespace, version, name)
    #
    #def get_uuid(self, uri=None):
    #    """Returns a UUID corresponding to `uri`.  If `uri` is None,
    #    a random UUID is returned."""
    #    return dlite.get_uuid(uri)
    #
    #def get_label(self, entity):
    #    """Returns a label for entity."""
    #    if hasattr(entity, 'label'):
    #        return entity.label.first()
    #    name = repr(entity)
    #    label, n = re.subn(r'emmo(-[a-z]+)?\.', '', name)
    #    return label
    #
    #def find_label(self, inst):
    #    """Returns label for class instance `inst` already added to the
    #    collection."""
    #    if hasattr(inst, 'uuid'):
    #        uuid = inst.uuid
    #    else:
    #        uuid = dlite.get_uuid(inst)
    #    rel = self.coll.find_first(p='_has-uuid', o=uuid)
    #    if not rel:
    #        raise ValueError('no class instance with UUID: %s' % uuid)
    #    return rel.s
    #
    #def add(self, entity):
    #    """Adds owl entity to collection and returns a reference to the
    #    new metadata."""
    #    if entity == owlready2.Thing:
    #        raise ValueError("invalid entity: %s" % entity)
    #    elif isinstance(entity, owlready2.ThingClass):
    #        return self.add_class(entity)
    #    elif isinstance(entity, owlready2.ClassConstruct):
    #        return self.add_class_construct(entity)
    #    else:
    #        raise ValueError("invalid entity: %s" % entity)
    #
    #def add_class(self, cls):
    #    """Adds owl class `cls` to collection and returns a reference to
    #    the new metadata."""
    #    if isinstance(cls, str):
    #        cls = self.onto[cls]
    #    label = cls.label.first()
    #    if not self.coll.has(label):
    #        uri = self.get_uri(label)
    #        dims, props = self.get_properties(cls)
    #        e = Instance(uri, dims, props, self.get_description(cls))
    #        self.coll.add(label, e)
    #        for r in cls.is_a:
    #            if r is owlready2.Thing:
    #                pass
    #            elif isinstance(r, owlready2.ThingClass):
    #                self.coll.add_relation(label, "is_a", r.label.first())
    #                self.add_class(r)
    #            elif isinstance(r, owlready2.Restriction):
    #                # Correct this test if EMMO reintroduce isPropertyOf
    #                if (isinstance(r.value, owlready2.ThingClass) and
    #                        isinstance(r.value, self.onto.Property) and
    #                        issubclass(r.property, self.onto.hasProperty)):
    #                    self.add_class(r.value)
    #                else:
    #                    self.add_restriction(r)
    #            elif isinstance(r, owlready2.ClassConstruct):
    #                self.add_class_construct(r)
    #            else:
    #                raise TypeError('Unexpected is_a member: %s' % type(r))
    #    return self.coll.get(label)
    #
    #def get_properties(self, cls):
    #    """Returns two lists with the dlite dimensions and properties
    #    correspinding to owl class `cls`."""
    #    dims = []
    #    props = []
    #    dimindices = {}
    #    propnames = set()
    #    types = dict(Integer='int', Real='double', String='string')
    #
    #    def get_dim(r, name, descr=None):
    #        """Returns dimension index corresponding to dimension name `name`
    #        for property `r.value`."""
    #        t = owlready2.class_construct._restriction_type_2_label[r.type]
    #        if (t in ('some', 'only', 'min') or
    #                (t in ('max', 'exactly') and r.cardinality > 1)):
    #            if name not in dimindices:
    #                dimindices[name] = len(dims)
    #                dims.append(Dimension(name, descr))
    #            return [dimindices[name]]
    #        else:
    #            return []
    #
    #    for c in cls.mro():
    #        if not isinstance(c, owlready2.ThingClass):
    #            continue
    #        for r in c.is_a:
    #            # Note that EMMO currently does not define an inverse for
    #            # hasProperty.  If we reintroduce that, we should replace
    #            #
    #            #     not isinstance(r.property, Inverse) and
    #            #     issubclass(r.property, self.onto.hasProperty)
    #            #
    #            # with
    #            #
    #            #     ((isinstance(r.property, Inverse) and
    #            #       issubclass(Inverse(r.property), onto.isPropertyFor)) or
    #            #      issubclass(r.property, self.onto.hasProperty))
    #            #
    #            if (isinstance(r, owlready2.Restriction) and
    #                    not isinstance(r.property, owlready2.Inverse) and
    #                    issubclass(r.property, self.onto.hasProperty) and
    #                    isinstance(r.value, owlready2.ThingClass) and
    #                    isinstance(r.value, self.onto.Property)):
    #                name = self.get_label(r.value)
    #                if name in propnames:
    #                    continue
    #                propnames.add(name)
    #
    #                # Default type, ndims and unit
    #                if isinstance(r.value, (self.onto.DescriptiveProperty,
    #                                        self.onto.QualitativeProperty,
    #                                        self.onto.SubjectiveProperty)):
    #                    ptype = 'string'
    #                else:
    #                    ptype = 'double'
    #                d = []
    #                d.extend(get_dim(r, 'n_%ss' % name, 'Number of %s.' %
    #                                 name))
    #                unit = None
    #
    #                # Update type, ndims and unit from relations
    #                for r2 in [r] + r.value.is_a:
    #                    if isinstance(r2, owlready2.Restriction):
    #                        if issubclass(r2.property, self.onto.hasType):
    #                            typelabel = self.get_label(r2.value)
    #                            ptype = types[typelabel]
    #                            d.extend(get_dim(r2, '%s_length' % name,
    #                                             'Length of %s' % name))
    #                        elif issubclass(r2.property, self.onto.hasUnit):
    #                            unit = self.get_label(r2.value)
    #
    #                descr = self.get_description(r.value)
    #                props.append(Property(name, type=ptype, dims=d,
    #                                      unit=unit, description=descr))
    #    return dims, props
    #
    #def add_restriction(self, r):
    #    """Adds owl restriction `r` to collection and returns a reference
    #    to it."""
    #    rtype = owlready2.class_construct._restriction_type_2_label[r.type]
    #    cardinality = r.cardinality if r.cardinality else 0
    #    e = self.add_restriction_entity()
    #    inst = e()
    #    inst.type = rtype
    #    inst.cardinality = cardinality
    #    label = inst.uuid
    #    vlabel = self.get_label(r.value)
    #    self.coll.add(label, inst)
    #    self.coll.add_relation(label, asstring(r.property), vlabel)
    #    if not self.coll.has(vlabel):
    #        self.add(r.value)
    #    return inst
    #
    #def add_restriction_entity(self):
    #    """Adds restriction metadata to collection and returns a reference
    #    to it."""
    #    uri = self.get_uri("Restriction")
    #    if not self.coll.has('Restriction'):
    #        props = [
    #            Property('type', type='string', description='Type of '
    #                     'restriction.  Valid values for `type` are: '
    #                     '"only", "some", "exact", "min" and "max".'),
    #            Property('cardinality', type='int', description='The '
    #                     'cardinality.  Unused for "only" and '
    #                     '"some" restrictions.'),
    #        ]
    #        e = Instance(uri, [], props,
    #                     "Class restriction.  For each instance of a class "
    #                     "restriction there should be a relation\n"
    #                     "\n"
    #                     "    (r.label, r.property, r.value.label)\n"
    #                     "\n"
    #                     "where `r.label` is the label associated with the "
    #                     "restriction, `r.property` is a relation and "
    #                     "`r.value.label` is the label of the value of the "
    #                     "restriction.")
    #        self.coll.add('Restriction', e)
    #    return self.coll.get('Restriction')
    #
    #def add_class_construct(self, c):
    #    """Adds owl class construct `c` to collection and returns a reference
    #    to it."""
    #    ctype = c.__class__.__name__
    #    e = self.add_class_construct_entity()
    #    inst = e()
    #    label = inst.uuid
    #    inst.type = ctype
    #    if isinstance(c, owlready2.LogicalClassConstruct):
    #        args = c.Classes
    #    else:
    #        args = [c.Class]
    #    for arg in args:
    #        self.coll.add_relation(label, 'has_argument', self.get_label(arg))
    #    self.coll.add(label, inst)
    #    return inst
    #
    #def add_class_construct_entity(self):
    #    """Adds class construct metadata to collection and returns a reference
    #    to it."""
    #    uri = self.get_uri("ClassConstruct")
    #    if not self.coll.has('ClassConstruct'):
    #        props = [
    #            Property('type', type='string', description='Type of '
    #                     'class construct.  Valid values for `type` are: '
    #                     '"not", "inverse", "and" or "or".'),
    #        ]
    #        e = Instance(uri, [], props,
    #                     "Class construct.  For each instance of a class "
    #                     "construct there should be one or more relations "
    #                     "of type\n"
    #                     "\n"
    #                     "    (c.label, \"has_argument\", c.value.label)\n"
    #                     "\n"
    #                     "where `c.label` is the label associated with the "
    #                     "class construct, `c.value.label` is the label of "
    #                     "an argument.")
    #        self.coll.add('ClassConstruct', e)
    #    return self.coll.get('ClassConstruct')
    #
    #def get_description(self, cls):
    #    """Returns description for OWL class `cls` by combining its
    #    annotations."""
    #    if isinstance(cls, str):
    #        cls = onto[cls]
    #    descr = []
    #    annotations = self.onto.get_annotations(cls)
    #    if 'definition' in annotations:
    #        descr.extend(annotations['definition'])
    #    if 'elucication' in annotations and annotations['elucidation']:
    #        for e in annotations['elucidation']:
    #            descr.extend(['', 'ELUCIDATION:', e])
    #    if 'axiom' in annotations and annotations['axiom']:
    #        for e in annotations['axiom']:
    #            descr.extend(['', 'AXIOM:', e])
    #    if 'comment' in annotations and annotations['comment']:
    #        for e in annotations['comment']:
    #            descr.extend(['', 'COMMENT:', e])
    #    if 'example' in annotations and annotations['example']:
    #        for e in annotations['example']:
    #            descr.extend(['', 'EXAMPLE:', e])
    #    return '\n'.join(descr).strip()
    #
    #def save(self, *args, **kw):
    #    """Saves collection to storage."""
    #    self.coll.save(*args, **kw)


def main():
    e = Onto2Meta(
        'https://emmo-repo.github.io/versions/1.0.0-alpha2/emmo-inferred.owl')
    return e


if __name__ == '__main__':
    e = main()
