"""Configuration file for the Sphinx documentation builder.

For the full list of built-in configuration values, see the documentation:
https://www.sphinx-doc.org/en/master/usage/configuration.html
"""
import os
from pathlib import Path
import re
from typing import TYPE_CHECKING

if TYPE_CHECKING:  # pragma: no cover
    from jinja2 import Environment

dlite_version_file = Path(__file__).resolve().parent.parent / "CMakeLists.txt"
if not dlite_version_file.exists():
    raise FileNotFoundError(
        f"Could not find {dlite_version_file} necessary to read DLite version."
    )
for line in dlite_version_file.read_text(encoding="utf8").splitlines():
    match = re.match(
        r"^\s+VERSION\s+(?P<version>[0-9]+(\.[0-9]+){2})$",
        line,
    )
    if match:
        dlite_version = match.group("version")
        break
else:
    raise ValueError(f"Could not determine DLite version from {dlite_version_file}")

dlite_share_plugins = [
    plugin_dir.name
    for plugin_dir in (
        Path(__file__).resolve().parent.parent
        / "build" / "bindings" / "python" / "dlite" / "share" / "dlite"
    ).iterdir()
    if plugin_dir.is_dir()
]

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "DLite"
copyright = "© SINTEF 2022"
author = "SINTEF"
version = ".".join(dlite_version.split(".")[2:])
release = dlite_version

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

# General configuration
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "**.ipynb_checkpoints",
    "_templates",
]

extensions = [
    "autoapi.extension",
    "breathe", # Doxygen bridge
    "myst_nb",  # markdown source support & support for Jupyter notebooks
    "sphinx.ext.graphviz",  # Graphviz
    "sphinx.ext.napoleon",  # API ref Google and NumPy style
    "sphinx.ext.viewcode",
    "sphinxcontrib.plantuml",  # PlantUml
    "sphinx_copybutton",  # Copy button for codeblocks
    "sphinx_design",  # Create panels in a grid layout or as drop-downs and more
    # "sphinx.ext.autosectionlabel",  # Auto-generate section labels.
]

root_doc = "index"

suppress_warnings = ["myst.mathjax"]

# Extension configuration
autoapi_dirs = [
    "../build/bindings/python/dlite"
] + [
    f"../build/bindings/python/dlite/share/dlite/{plugin_dir}"
    for plugin_dir in dlite_share_plugins
]
autoapi_type = "python"
autoapi_file_patterns = ["*.py", "*.pyi"]
autoapi_template_dir = "_templates/autoapi"
autoapi_add_toctree_entry = True  # Toggle the most top-level index file
autoapi_options = [
    "members",
    "undoc-members",
    "private-members",
    "show-inheritance",
    "show-module-summary",
    "special-members",
    "imported-members",
    # "show-inheritance-diagram",
    # "inherited-members",
]
autoapi_keep_files = True  # Should be False in production
autoapi_python_use_implicit_namespaces = True  # True to avoid namespace being `python.dlite`

autodoc_typehints = "description"
autodoc_typehints_format = "short"
autodoc_inherit_docstrings = True

# HTML output
html_theme = "sphinx_book_theme"
html_logo = "_static/logo.svg"
html_favicon = "_static/favicon.ico"
html_theme_options = {
    "path_to_docs": "pydoc",
    "repository_url": "https://github.com/SINTEF/dlite",
    "repository_branch": "master",
    "use_issues_button": True,
    "use_fullscreen_button": True,
    "use_repository_button": True,
    "logo_only": True,
    "show_navbar_depth": 1,
    "announcement": "This documentation is under development!",
}
html_static_path = ["_static"]
# html_css_files = ["custom.css"]

intersphinx_mapping = {
    "python": ("https://docs.python.org/3", None),
}

myst_heading_anchors = 5
myst_enable_extensions = ["colon_fence"]

napoleon_use_admonition_for_examples = True
napoleon_use_admonition_for_notes = True
napoleon_use_admonition_for_references = True
napoleon_preprocess_types = True
napoleon_attr_annotations = True

nb_execution_allow_errors = False
nb_execution_mode = "cache"
if os.getenv("CI"):
    nb_kernel_rgx_aliases = {".*": "python"}

plantuml = "java -jar lib/plantuml.jar"
plantuml_output_format = "svg_img"

# Jinja2 custom filters
def autoapi_prepare_jinja_env(jinja_env: "Environment") -> None:
    """Add custom Jinja2 filters for use with AutoAPI templates."""

    # Remove plugin path, removes the initial part of the module name if it is equal to
    # any of the folder names under `dlite/share/dlite`.
    jinja_env.filters["remove_plugin_path"] = lambda name: ".".join(
        name.split(".")[1:]
    ) if name.split(".")[0] in dlite_share_plugins else name
