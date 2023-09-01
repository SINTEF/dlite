"""
"""
from typing import Mapping, Sequence

import yaml

from tripper import RDF, Namespace, Triplestore
from tripper.convert import save_container, load_container
from otelib import OTEClient


OTEIO = Namespace("http://emmo.info/oteio#")


def populate_triplestore(
    ts: Triplestore,
    yamlfile: str,
) -> None:
    """Populate the triplestore with data documentation from a
    standardised yaml file.

    Arguments:
        ts: Tripper triplestore documenting data sources and sinks.
        yamlfile: Standardised YAML file to load the data documentation
            from.
    """
    with open(yamlfile) as f:
        document = yaml.safe_load(f)

    datadoc = document["data_documentation"]
    for iri, resource in datadoc.items():
        save_container(ts, resource, iri, recognised_keys="basic")

        # Some heuristics to categorise partial pipelines as either
        # data sources or data sinks
        #if "dataresource" in resource:
        #    ts.add((iri, RDF.type, OTEIO.DataSource))
        #else:
        #    ts.add((iri, RDF.type, OTEIO.DataSink))


def get_data(
    ts: Triplestore,
    steps: Sequence[str],
    client_iri: str = "python",
):
    """Get data.

    Arguments:
        ts: Tripper triplestore documenting data sources and sinks.
        steps: Sequence of names of data sources and sinks to combine.
            The order is important and should go from source to sink.
        client_iri: IRI of OTELib client to use.
    """
    client = OTEClient(client_iri)
    pipeline = None

    for step in steps:
        strategies = load_container(ts, step, recognised_keys="basic")
        for filtertype, config in strategies.items():
            creator = getattr(client, f"create_{filtertype}")
            pipe = creator(**config)
            if pipeline:
                pipeline = pipeline >> pipe
            else:
                pipeline = pipe

    pipeline.get()


def generate_partial_pipeline(
    client: OTEClient,
    ts: Triplestore,
    iri: str,
):
    """Create a OTEAPI partial pipeline from data documentation.

    Arguments:
        client: OTELib client.
        ts: Tripper triplestore.
        iri: IRI of the partial pipeline to generate.

    Returns:
        OTEAPI pipeline.
    """
    strategies = load_container(ts, iri)

    pipeline = None
    for filtertype, config in strategies.items():
        creator = getattr(client, f"create_{filtertype}")
        pipe = creator(**config)
        if pipeline:
            pipeline = pipeline >> pipe
        else:
            pipeline = pipe
    return pipeline


def save_partial_pipeline(
    ts: Triplestore,
    strategies: Mapping[str, dict],
    iri: str,
) -> None:
    """Save partial pipeline to triplestore.

    Arguments:
        ts: Tripper triplestore.
        strategies: Maps OTEAPI stratepy types name to configurations
            (i.e. to data documentation).
        iri: IRI of individual standing for the partial pipeline to save.
    """
    save_container(ts, strategies, iri, recognised_keys="basic")



def load_partial_pipeline(
    ts: Triplestore,
    iri: str,
) -> None:
    """Save partial pipeline to triplestore.

    Arguments:
        ts: Tripper triplestore.
        iri: IRI of individual standing for the partial pipeline to load.
    """
    return load_container(ts, iri, recognised_keys="basic")
