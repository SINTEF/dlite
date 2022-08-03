# Creating a new release
You should not change the version number manually, since that is done
by the The [cd_release](.github/workflows/cd_release.yml) GitHub workflow.
The only thing you should do is:

1. Create a release on GitHub with a short release description.

   Set the tag to the version number prefixed with `"v"` and title to
   the version number as explained above.

2. Ensure the GitHub Action CD workflows run as expected.


#### :warning: If the workflow failed

If something is wrong and the workflow fails **before** publishing
the package to PyPI, make sure to remove all traces of the release
and tag, fix the bug, and try again.

If something is wrong and the workflow fails **after** publishing
the package to PyPI: **DO NOT REMOVE THE RELEASE OR TAG !**
