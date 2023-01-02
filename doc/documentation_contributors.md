Overview
========
The DLite documentation is written in Markdown.  This include both the
README files and documentation found in the `doc/` subdirectory.
Common to both is that the text should be as easy and natural as
possible to read from the terminal, hence, the following
recommendations should be followed:

* Keep the line length below 80 characters.

* Use the underline style format for main and sub-headers. For example,
  start a new page with

      Overview
      ========

  instead of

      # Overview

* Add two newlines before headers to make them easier to recognise.  This
  is especially useful for levels 3-headers and below, that are only lines
  starting with 3 or more hashes (`#`).

* Avoid the use of html tags.

* Use `.md` as file extension for all Markdown files.

* Links become more readable if you place them at the end of the document
  using square brackets.  Example:

     ```
     A link to [SOFT].
     ...

     End of document.


     [SOFT]: https://www.sintef.no/en/publications/publication/1553408/
     ```


The README files are intended to document the overall project or the
content in a subdirectory.  These should therefore render nicely on GitHub.
Hence, use [Basic Markdown] or possible [GitHub-flavored Markdown].

The Markdown files in the `doc/` subdirectory are intended to be included
in the sphinx-generated [online documentation].  Here we can make use of
the [MyST Markdown extensions], like [admonitions].




[Markdown]: https://en.wikipedia.org/wiki/Markdown
[Basic Markdown]: https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet
[GitHub-flavored Markdown]: https://docs.github.com/en/get-started/writing-on-github
[MyST Markdown extensions]: https://myst-parser.readthedocs.io/en/latest/syntax/optional.html[online documentation]: https://sintef.github.io/dlite/
[admonitions]: https://myst-parser.readthedocs.io/en/latest/syntax/optional.html#admonition-directives
