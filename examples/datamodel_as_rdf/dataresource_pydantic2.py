"""RDF serialisation of a OTEAPI data resource."""
from typing import Optional

from typing_extensions import Annotated

from pydantic import AnyUrl, BaseModel, Field, UrlConstraints, model_validator
from pydantic import __version__ as pydantic_version

import dlite
from dlite.rdf import from_rdf, to_rdf
from dlite.utils import pydantic_to_instance, pydantic_to_metadata


# Require Pydantic v2
if int(pydantic_version.split(".")[0]) != 2:
    print("This example requires pydantic v2")
    raise SystemExit(44)

HostlessAnyUrl = Annotated[AnyUrl, UrlConstraints(host_required=False)]


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
    mediaType: Annotated[
        Optional[str],
        Field(
            None,
            description=(
                """The media type of the distribution as defined by [IANA].

                Usage: This property *SHOULD* be used when the media
                type of the distribution is defined in [IANA].

                [IANA]: https://www.w3.org/TR/vocab-dcat-2/#bib-iana-media-types
                """
            ),
        ),
    ]
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
    accessService: Annotated[
        Optional[str],
        Field(
            None,
            description=(
                """A data service that gives access to the distribution of the
                dataset."""
            ),
        ),
    ]
    license: Annotated[
        Optional[str],
        Field(
            None,
            description=(
                "A legal document under which the distribution is made "
                "available."
            ),
        ),
    ]
    accessRights: Annotated[
        Optional[str],
        Field(
            None,
            description=(
                "A rights statement that concerns how the distribution is "
                "accessed."
            ),
        ),
    ]
    publisher: Annotated[
        Optional[str],
        Field(
            None,
            description=(
                "The entity responsible for making the resource/item available."
            ),
        ),
    ]

    @model_validator(mode="after")
    @classmethod
    def ensure_unique_url_pairs(cls, data: "Any]") -> "Any":
        """Ensure either downloadUrl/mediaType or accessUrl/accessService are
        defined.

        It's fine to define them all, but at least one complete pair
        MUST be specified.
        """
        if not (data.downloadUrl and data.mediaType) or (
            data.accessUrl and data.accessService
        ):
            raise ValueError(
                "Either of the pairs of attributes downloadUrl/mediaType or "
                "accessUrl/accessService MUST be specified."
            )
        return data


config = {
    "downloadUrl": "http://example.com/testdata.csv",
    "mediaType": "text/csv",
    "license": "CC-BY-4.0",
}
resource_config = ResourceConfig(**config)


# Serialise to RDF
DLiteResourceConfig = pydantic_to_metadata(ResourceConfig)
dlite_config = pydantic_to_instance(DLiteResourceConfig, resource_config)


rdf = to_rdf(dlite_config, format="turtle", include_meta=False, decode=True)
print(rdf)


# Try now to go from RDF to data model
turtle = """
@prefix dm: <http://emmo.info/datamodel/0.0.2#> .

<http://onto-ns.com/data/6de3394f-615f-4b18-bae6-efcff13a2f0f> a dm:DataInstance ;
    dm:hasProperty
        <http://onto-ns.com/data/5ecf9c76-5997-4923-b085-566753cb59d7#downloadUrl>,
        <http://onto-ns.com/data/5ecf9c76-5997-4923-b085-566753cb59d7#license>,
        <http://onto-ns.com/data/5ecf9c76-5997-4923-b085-566753cb59d7#mediaType> ;
    dm:hasUUID "6de3394f-615f-4b18-bae6-efcff13a2f0f" ;
    dm:instanceOf <http://onto-ns.com/meta/0.1/ResourceConfig> .

<http://onto-ns.com/data/5ecf9c76-5997-4923-b085-566753cb59d7#downloadUrl> dm:hasLabel "downloadUrl" ;
    dm:hasValue "http://example.com/testdata.csv" ;
    dm:instanceOf <http://onto-ns.com/meta/0.1/ResourceConfig#downloadUrl> .

<http://onto-ns.com/data/5ecf9c76-5997-4923-b085-566753cb59d7#license> dm:hasLabel "license" ;
    dm:hasValue "CC-BY-4.0" ;
    dm:instanceOf <http://onto-ns.com/meta/0.1/ResourceConfig#license> .

<http://onto-ns.com/data/5ecf9c76-5997-4923-b085-566753cb59d7#mediaType> dm:hasLabel "mediaType" ;
    dm:hasValue "text/csv" ;
    dm:instanceOf <http://onto-ns.com/meta/0.1/ResourceConfig#mediaType> .
"""

inst = from_rdf(data=turtle, format="turtle")
new_resource_config = ResourceConfig(**inst.properties)


# If we create our pydantic model from the rdf representation - check
# that we get back the original model
inst2 = from_rdf(data=rdf, format="turtle")
new_resource_config2 = ResourceConfig(**inst.properties)
assert new_resource_config2.model_dump() == resource_config.model_dump()
