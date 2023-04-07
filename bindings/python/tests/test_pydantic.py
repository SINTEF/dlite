from datetime import datetime
from typing import List, Optional

import dlite
from dlite.utils import pydantic_to_metadata, pydantic_to_instance

# Skip test if pydantic isn't installed
try:
    from pydantic import BaseModel, Field
except ImportError:
    import sys
    sys.exit(44)


class TransformationStatus(BaseModel):
    """Return from transformation status."""

    id: str = Field(..., description="ID for the given transformation process.")
    status: Optional[str] = Field(
        None, description="Status for the transformation process."
    )
    messages: Optional[List[str]] = Field(
        None, description="Messages related to the transformation process.",
        shape=["N", "M"],
    )
    created: Optional[float] = Field(
        None,
        description="Time of creation for the transformation process. "
        "Given as POSIX time stamp.",
    )
    startTime: Optional[int] = Field(
        None, description="Time when the transformation process started. "
        "Given as POSIX time stamp.",
    )
    finishTime: Optional[datetime] = Field(
        None, description="Time when the tranformation process finished. "
        "Given as POSIX time stamp.",
    )


now = datetime.now().timestamp()

t = TransformationStatus(
    id="sim1",
    message="success",
    created=now - 3600,
    startTime=now - 3000,
    finishTime=now - 600,
)


class Foo(BaseModel):
    count: int
    size: Optional[float] = -1


class Bar(BaseModel):
    apple = 'x'
    banana = 'y'


class Spam(BaseModel):
    foo: Foo
    bars: List[Bar]

m = Spam(foo={'count': 4}, bars=[{'apple': 'x1'}, {'apple': 'x2'}])

from dlite.utils import pydantic_to_metadata, pydantic_to_property

MetaFoo = pydantic_to_metadata(Foo)
MetaBar = pydantic_to_metadata(Bar)
MetaSpam = pydantic_to_metadata(Spam)
print(MetaSpam)
print("---")

spam = pydantic_to_instance(MetaSpam, m)
print(spam)