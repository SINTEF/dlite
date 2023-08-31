"""
"""
from typing import Mapping

from tripper import Triplestore
from tripper.convert import save_container, load_container
from otelib import OTEClient


def generate_partial_pipeline(
    client: OTEClient,
    strategies: Mapping[str, Mapping]
):
    """Create a OTEAPI partial pipeline from data documentation.

    Arguments:
        client: OTELib client.
        strategies: Maps OTEAPI stratepy types name to configurations
            (i.e. to data documentation).

    Returns:
        OTEAPI pipeline.
    """
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
