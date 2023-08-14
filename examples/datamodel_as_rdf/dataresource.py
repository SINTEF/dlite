"""RDF serialisation of a OTEAPI data resource."""
from typing import Optional

from pydantic import AnyUrl, BaseModel, Field, root_validator
from pydantic import __version__ as pydantic_version

import dlite
from dlite.rdf import from_rdf, to_rdf
from dlite.utils import pydantic_to_instance, pydantic_to_metadata


# Require Pydantic v1
if int(pydantic_version.split(".")[0]) != 1:
    print("This example requires pydantic v1")
    raise SystemExit(44)


class HostlessAnyUrl(AnyUrl):
    """AnyUrl, but allow not having a host."""
    host_required = False


class ResourceConfig(BaseModel):
    """Resource Strategy Data Configuration.

    Important:
        Either of the pairs of attributes `downloadUrl`/`mediaType` or
        `accessUrl`/`accessService` MUST be specified.
    """

    downloadUrl: Optional[HostlessAnyUrl] = Field(
        None,
        description=(
            """Definition: The URL of the downloadable file in a given
            format. E.g. CSV " "file or RDF file.

            Usage: `downloadURL` *SHOULD* be used for the URL at which
            this distribution is available directly, typically through a
            HTTPS GET request or SFTP."""
        ),
    )
    mediaType: Optional[str] = Field(
        None,
        description=(
            """The media type of the distribution as defined by [IANA].

            Usage: This property *SHOULD* be used when the media
            type of the distribution is defined in [IANA].

            [IANA]: https://www.w3.org/TR/vocab-dcat-2/#bib-iana-media-types
            """
        ),
    )
    accessUrl: Optional[HostlessAnyUrl] = Field(
        None,
        description=(
            """A URL of the resource that gives access to a distribution of
            the dataset. E.g. landing page, feed, SPARQL endpoint.

            Usage: `accessURL` *SHOULD* be used for the URL of a
            service or location that can provide access to this
            distribution, typically through a Web form, query or API
            call.

            `downloadURL` is preferred for direct links to downloadable
            resources."""
        ),
    )
    accessService: Optional[str] = Field(
        None,
        description=(
            """A data service that gives access to the distribution of the
            dataset."""
        ),
    )
    license: Optional[str] = Field(
        None,
        description=(
            "A legal document under which the distribution is made available."
        ),
    )
    accessRights: Optional[str] = Field(
        None,
        description=(
            "A rights statement that concerns how the distribution is accessed."
        ),
    )
    publisher: Optional[str] = Field(
        None,
        description=(
            "The entity responsible for making the resource/item available."
        ),
    )

    @root_validator(skip_on_failure=True)
    def ensure_unique_url_pairs(cls, values: "Dict[str, Any]") -> "Dict[str, Any]":
        """Ensure either downloadUrl/mediaType or accessUrl/accessService are
        defined.

        It's fine to define them all, but at least one complete pair
        MUST be specified.
        """
        if not (
            all(values.get(_) for _ in ["downloadUrl", "mediaType"])
            or all(values.get(_) for _ in ["accessUrl", "accessService"])
        ):
            raise ValueError(
                "Either of the pairs of attributes downloadUrl/mediaType or "
                "accessUrl/accessService MUST be specified."
            )
        return values


config = {
    "downloadUrl": "http://example.com/testdata.csv",
    "mediaType": "text/csv",
    "license": "CC-BY-4.0",
}
resource_config = ResourceConfig(**config)


# Serialise to RDF
DLiteResourceConfig = pydantic_to_metadata(ResourceConfig)
dlite_config = pydantic_to_instance(DLiteResourceConfig, resource_config)


print(to_rdf(dlite_config, format="turtle", include_meta=False, decode=True))


# Try now to go from RDF to data model
turtle = """
@prefix dm: <http://emmo.info/datamodel/0.0.2#> .

<6de3394f-615f-4b18-bae6-efcff13a2f0f> a dm:DataInstance ;
    dm:hasProperty
        <5ecf9c76-5997-4923-b085-566753cb59d7#downloadUrl>,
        <5ecf9c76-5997-4923-b085-566753cb59d7#license>,
        <5ecf9c76-5997-4923-b085-566753cb59d7#mediaType> ;
    dm:hasUUID "6de3394f-615f-4b18-bae6-efcff13a2f0f" ;
    dm:instanceOf <http://onto-ns.com/meta/0.1/ResourceConfig> .

<5ecf9c76-5997-4923-b085-566753cb59d7#downloadUrl> dm:hasLabel "downloadUrl" ;
    dm:hasValue "http://example.com/testdata.csv" ;
    dm:instanceOf <http://onto-ns.com/meta/0.1/ResourceConfig#downloadUrl> .

<5ecf9c76-5997-4923-b085-566753cb59d7#license> dm:hasLabel "license" ;
    dm:hasValue "CC-BY-4.0" ;
    dm:instanceOf <http://onto-ns.com/meta/0.1/ResourceConfig#license> .

<5ecf9c76-5997-4923-b085-566753cb59d7#mediaType> dm:hasLabel "mediaType" ;
    dm:hasValue "text/csv" ;
    dm:instanceOf <http://onto-ns.com/meta/0.1/ResourceConfig#mediaType> .
"""

inst = from_rdf(data=turtle, format="turtle")
new_resource_config = ResourceConfig(**inst.properties)
