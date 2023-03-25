from datetime import datetime
from typing import List, Optional

from pydantic import BaseModel, Field

import dlite
from dlite.utils import to_metadata


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
    size: Optional[float] = None


class Bar(BaseModel):
    apple = 'x'
    banana = 'y'


class Spam(BaseModel):
    foo: Foo
    bars: list[Bar]

m = Spam(foo={'count': 4}, bars=[{'apple': 'x1'}, {'apple': 'x2'}])

from dlite.utils import pydantic_to_metadata, pydantic_to_property
