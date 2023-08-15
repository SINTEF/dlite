"""Main script running all tests in this repo."""
import pydantic


pydantic_major = int(pydantic.__version__.split(".")[0])

# Run examples
import pydantic_nested  # works for both Pydantic v1 and v2

if pydantic_major == 1:
    # Only Pydantic v1
    import dataresource
elif pydantic_major == 2:
    # Only Pydantic v2
    import dataresource_pydantic2
