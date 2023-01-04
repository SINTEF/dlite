Testing Python examples in the documentation
============================================
We can use the Python doctest module for testing examples in the documentation.
Run it with

    python -m doctest <filename>

on your markdown file.

See [documentation contributions] for how to write your examples such that they can be tested with doctest.

For instance, assume that you have the following example in your documentation

    ```python
        >>> 1 + 1  # doctest: +SKIP
        3

    ```

`python -m doctest documentation_testing.md` would print the following message:

```shell
**********************************************************************
File "documentation_testing.md", line 13, in documentation_testing.md
Failed example:
    1 + 1
Expected:
    3
Got:
    2
**********************************************************************
```

Now you copy the output following `Got:`, including the 4 indentation spaces, into your example.

Note that the comment `#doctest: +SKIP` was added to the above example in order to not trigger a doctest failure when validating the markdown file containing this guideline.



[documentation contributions]: documentation_contributions.md
