"""RDF serialisation of instance of nested Pydantic data model."""
from typing import List, Optional

from pydantic import BaseModel, Field

import dlite
from dlite.rdf import to_rdf
from dlite.utils import pydantic_to_instance, pydantic_to_metadata


# Some toy nested Pydantic data models
class Foo(BaseModel):
    count: int
    size: Optional[float] = -1


class Bar(BaseModel):
    apple: str = Field(..., description="An apple")
    banana: str = Field('European banana', description="A banana")


class Spam(BaseModel):
    foo: Foo
    bars: List[Bar]


# Create an instance of Spam that we want to serialise as RDF
m = Spam(foo={'count': 4}, bars=[{'apple': 'x1'}, {'apple': 'x2'}])

# Create DLite instance (data models must be created first)
MetaFoo = pydantic_to_metadata(Foo)
MetaBar = pydantic_to_metadata(Bar)
MetaSpam = pydantic_to_metadata(Spam)
spam = pydantic_to_instance(MetaSpam, m)
print(spam)
print("---")

# Serialise to RDF
print(to_rdf(spam, format="turtle", include_meta=True, decode=True))
