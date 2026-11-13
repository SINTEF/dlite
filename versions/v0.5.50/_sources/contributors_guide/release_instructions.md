# Creating a new release

You should not change the version number manually, since that is done
by the [cd_release.yml] GitHub workflow.
The only thing you should do is:

1. Create a release on GitHub with a short release description.

   Set the tag to the version number prefixed with `"v"` and title to
   the version number as explained above.

2. Ensure the GitHub Action CD workflows run as expected.


Documentation publishing workflow
---------------------------------
Documentation is published by a separate workflow: [cd_docs.yml].

To use this workflow, configure GitHub Pages to serve from the `gh-pages`
branch root (repository settings -> Pages).

The docs deployment workflow is triggered by:

1. Pushes to `master`.
2. Manual runs via workflow dispatch.
3. Published releases.

Publishing targets are:

1. `master` pushes -> latest docs at `/` and `/latest/`.
2. Release `vX.Y.Z` -> versioned docs at `/versions/vX.Y.Z/`.

The release workflow [cd_release.yml] does **not** deploy documentation directly.
However, it may update and push `master` (for example when bumping version metadata),
which then triggers [cd_docs.yml].


:::{admonition} If the workflow failed
:class: warning

If something is wrong and the workflow fails **before** publishing
the package to PyPI, make sure to remove all traces of the release
and tag, fix the bug, and try again.

If something is wrong and the workflow fails **after** publishing
the package to PyPI: **DO NOT REMOVE THE RELEASE OR TAG!**
:::


[cd_release.yml]: https://github.com/SINTEF/dlite/blob/master/.github/workflows/cd_release.yml
[cd_docs.yml]: https://github.com/SINTEF/dlite/blob/master/.github/workflows/cd_docs.yml
