Guideline for contributing documentation
========================================
The DLite documentation is written in Markdown.
This include both the README files and documentation found in the
`doc/` subdirectory.

Common to both is that the text should be as easy and natural as
possible to read from the terminal.
Hence, the following recommendations should be followed:

* Keep the maximum line length at 79 characters so that the source
  file easily can be viewed in a 80 character wide terminal.

* Write one sentence per line (while respecting the 79 character line
  length), in order to get an easier to read output from `git diff`.

* Use the underline style format for main and sub-headers. For example,
  start a new page with

      Overview
      ========

  instead of

      # Overview

* Add two newlines before headers to make them easier to recognise.

  This is especially useful for levels 3-headers and below (which are
  lines starting with 3 or more hashes (`#`)) to make them easier to
  recognise as sub-headers.

* Avoid the use of html tags.

* Links become more readable if you place them at the end of the document
  using square brackets.
  Example:

     ```
     A link to [SOFT].
     ...

     End of document.


     [SOFT]: https://www.sintef.no/en/publications/publication/1553408/
     ```

* Use `.md` as file extension for all Markdown files.

The README files are intended to document the overall project or the
content in a subdirectory.
These should therefore render nicely on GitHub.
Hence, use [Basic Markdown] or possible [GitHub-flavored Markdown].

The Markdown files in the `doc/` subdirectory are intended to be included
in the sphinx-generated [online documentation].
Here we can make use of the [MyST Markdown extensions], like [admonitions].


Figures
-------
Please place figures in the `doc/figs/` directory.
If you use [drawio], it is recommended that you save the figure in svg
format.
Then it renders well in browsers and is easy to find and edit for
collaborators.


Examples
--------
Write your python examples in the same way as done in the [official Python
documentation], by preceeding expressions with `>>> ` and the expected
output on the next line below the `>>> `.
For example

    ```python
        # Here is a python example
        >>> x = 1 + 1
        >>> x
        2

    ```
This allow to check your examples by running

    python -m doctest <filename>

on your markdown file.

It is recommended to indent your code with 4 spaces.
That makes it easy to copy the correct output into your example.
For instance, if you write

    ```python
        >>> 1 + 1  # doctest: +SKIP
        3

    ```

`python -m doctest documentation_contributors.md` would print the following
message:

    **********************************************************************
    File "documentation_contributors.md", line 95, in documentation_contributors.md
    Failed example:
        1 + 1
    Expected:
        3
    Got:
        2
    **********************************************************************

Now you copy the output following `Got:`, including the 4 indentation spaces,
into your example.

Note that the comment `#doctest: +SKIP` was added to the above example
in order to not triggering a doctest failure when validating the markdown
file containing this guideline.



[Markdown]: https://en.wikipedia.org/wiki/Markdown
[Basic Markdown]: https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet
[GitHub-flavored Markdown]: https://docs.github.com/en/get-started/writing-on-github
[MyST Markdown extensions]: https://myst-parser.readthedocs.io/en/latest/syntax/optional.html
[online documentation]: https://sintef.github.io/dlite/
[admonitions]: https://myst-parser.readthedocs.io/en/latest/syntax/optional.html#admonition-directives
[drawio]: https://app.diagrams.net/
[doctest]: https://docs.python.org/3/library/doctest.html
[official Python documentation]: https://docs.python.org/3/tutorial/introduction.html#numbers
