Guideline for contributing documentation
========================================

The DLite documentation is written in [Markdown].
This include both the README files and documentation found in the `doc/` subdirectory.


Generate documentation locally
------------------------------
When writing documentation it is practically to build and check the documentation locally before submitting a pull request.

The following steps are needed for building the documentation:

1. Install dependencies.

   First you need [doxygen]. In Ubuntu it can be installed with

       sudo apt-install doxygen

   Python requirements can be installed with

       pip install --update -r requirements_doc.txt

2. Ask cmake to build documentation

   ```
   cd <build_directory>
   cmake -DWITH_DOC=YES .
   ```

   If you haven't build dlite before, you should replace the final dot with the
   path to the root of the DLite source directory.

3. Build the documentation

       cmake --build .

   Check and fix possible error and warning messages from doxygen and sphinx.
   The generated documentation can be found in `<build_directory>/doc/html/index.html`.


Style recommendations and guidelines
------------------------------------
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

* Avoid the use of raw HTML.

* Links become more readable if you place them at the end of the document using square brackets.
  Example:

```markdown
A link to [SOFT].
...

End of document.


[SOFT]: https://www.sintef.no/en/publications/publication/1553408/
```

* Use `.md` as file extension for all Markdown files.

The README files are intended to document the overall project or the content in a subdirectory.
These should therefore render nicely on GitHub.
Hence, use [CommonMark] or possible [GitHub-flavored Markdown].
For README files, keep the maximum line length at 79 characters for easy viewing in a 80 character wide terminal.

The Markdown files in the `doc/` subdirectory are intended to be included in the [Sphinx]-generated [online documentation].
We make use of the [MyST Markdown extension] to convert all markdown syntax to rich structured text (RST) prior to letting Sphinx generate the HTML (or other) documentation.


Figures
-------
Please place figures in the `doc/_static/` directory.
If you use [draw.io], it is recommended that you save the figure in svg format.
Then it renders well in browsers and is easy to find and edit for collaborators.


Examples
--------
Write your python examples in the same way as done in the [official Python documentation], by preceeding expressions with `>>> ` and the expected output on the next line below the `>>> `.
Add a blank line at the end of the code listing.
For example

    ```python
        # Here is a python example
        >>> x = 1 + 1
        >>> x
        2

    ```

This will show up nicely highlighted in the generated documentation:

```python
    # Here is a python example
    >>> x = 1 + 1
    >>> x
    2

```

and will facilitate checking the examples by running

```shell
python -m doctest <filename>
```

on your markdown file.

It is recommended to indent your code examples with 4 spaces.
That makes it easy to copy the correct doctest output into your example.
For more info, see the guide for [documentation testing].

Please note that examples from [Python sources](https://sintef.github.io/dlite/autoapi/dlite/mappings/index.html#dlite.mappings.match_factory) will show up with a small `>>>` button in the top right of the code listing box.
If you click that button, it will toggle the prompt and output on or off, making it easy to copy/paste from examples.



[doxygen]: https://www.doxygen.nl/
[Markdown]: https://en.wikipedia.org/wiki/Markdown
[setext]: https://github.com/DavidAnson/markdownlint/blob/main/doc/md003.md
[CommonMark]: https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet
[GitHub-flavored Markdown]: https://docs.github.com/en/get-started/writing-on-github
[MyST Markdown extension]: https://myst-parser.readthedocs.io/en/latest/syntax/optional.html
[Sphinx]: https://www.sphinx-doc.org/
[online documentation]: https://sintef.github.io/dlite/
[draw.io]: https://app.diagrams.net/
[doctest]: https://docs.python.org/3/library/doctest.html
[official Python documentation]: https://docs.python.org/3/tutorial/introduction.html#numbers
[documentation testing]: documentation_testing.md
