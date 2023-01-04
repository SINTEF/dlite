Guideline for contributing documentation
========================================

The DLite documentation is written in [Markdown].
This include both the README files and documentation found in the `doc/` subdirectory.

Common to both is that the text should be as easy and natural as possible to read and write both from the terminal, in an editor and rendered in a web browser.
Hence, the following recommendations:

* Write one sentence per line, in order to get an easier to read output from `git diff`.
  This improves readability and understandability when performing reviews on GitHub.

* For README files, use the underline ([setext]) style format for main and sub-headers.
  For example:

      Header level 1
      ==============
      ...

      Header level 2
      --------------
      ...

  For documentation, follow the style used by the original contributor.

* Avoid the use of html tags.

* Links become more readable if you place them at the end of the document using square brackets.
  Example:

     ```
     A link to [SOFT].
     ...

     End of document.


     [SOFT]: https://www.sintef.no/en/publications/publication/1553408/
     ```

* Use `.md` as file extension for all Markdown files.

The README files are intended to document the overall project or the content in a subdirectory.
These should therefore render nicely on GitHub.
Hence, use [Basic Markdown] or possible [GitHub-flavored Markdown].
For README files, keep the maximum line length at 79 characters so that the source file easily can be viewed in a 80 character wide terminal.

The Markdown files in the `doc/` subdirectory are intended to be included in the sphinx-generated [online documentation].
Here we can make use of the [MyST Markdown extensions], like [admonitions].


Figures
-------
Please place figures in the `doc/figs/` directory.
If you use [drawio], it is recommended that you save the figure in svg format.
Then it renders well in browsers and is easy to find and edit for collaborators.


Examples
--------
Write your python examples in the same way as done in the [official Python documentation], by preceeding expressions with `>>> ` and the expected output on the next line below the `>>> `.
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
That makes it easy to copy the correct doctest output into your example.
For more info, see the guide for [documentation testing].


[Markdown]: https://en.wikipedia.org/wiki/Markdown
[setext]: https://github.com/DavidAnson/markdownlint/blob/main/doc/md003.md
[Basic Markdown]: https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet
[GitHub-flavored Markdown]: https://docs.github.com/en/get-started/writing-on-github
[MyST Markdown extensions]: https://myst-parser.readthedocs.io/en/latest/syntax/optional.html
[online documentation]: https://sintef.github.io/dlite/
[admonitions]: https://myst-parser.readthedocs.io/en/latest/syntax/optional.html#admonition-directives
[drawio]: https://app.diagrams.net/
[doctest]: https://docs.python.org/3/library/doctest.html
[official Python documentation]: https://docs.python.org/3/tutorial/introduction.html#numbers
[documentation testing]: documentation_testing.md
