"""
"""
import warnings
from typing import Mapping, Sequence

import yaml

from tripper import RDF, Namespace, Triplestore
from tripper.convert import save_container, load_container
from otelib import OTEClient


# Namespaces
OTEIO = Namespace("http://emmo.info/oteio#")

# Get rid of FutureWarning from csv.py
warnings.filterwarnings("ignore", category=FutureWarning)


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

    datadoc = document["data_resources"]
    for iri, resource in datadoc.items():
        save_container(ts, resource, iri, recognised_keys="basic")

        # Some heuristics to categorise partial pipelines as either
        # data sources or data sinks
        if "dataresource" in resource:
            ts.add((iri, RDF.type, OTEIO.DataSource))
        else:
            ts.add((iri, RDF.type, OTEIO.DataSink))


def get_data(
    ts: Triplestore,
    steps: Sequence[str],
    client_iri: str = "python",
):
    """Get the data specified by the user.

    From the sequence of IRIs provided in the `steps` argument, this
    function ensembles an OTEAPI pipeline and calls its `get()` method.

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
            pipeline = pipeline >> pipe if pipeline else pipe

    pipeline.get()
