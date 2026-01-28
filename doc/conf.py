# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html


import os
import sys
import textwrap
# import subprocess #Only needed for local Windows testing builds
from exhale import utils

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Snapdragon Telematics Application Framework API Reference'
copyright = 'Qualcomm Technologies, Inc '
author = 'Qualcomm Technologies, Inc.'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
              'breathe',
              'exhale',
              'sphinx_rtd_theme',
              'sphinx.ext.autosectionlabel']
#              'linuxdoc.rstFlatTable' requires linuxdoc package, but is useful for complicated tables in rst

autosectionlabel_prefix_document = False

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', '_ignore', '_scripts', 'doxy']

def initializeNodeFilenameAndLink(self, node):
    SPECIAL_CASES = ["dir", "file", "namespace", "page"]
    if node.kind in SPECIAL_CASES:
          if node.kind == "function":
               node.link_name ="{name}({parameters})".format(name=node.name, parameters=", ".join(node.parameters))

def specificationsForKind(kind):
    '''
    For a given input ``kind``, return the list of reStructuredText specifications
    for the associated Breathe directive.
    '''
    # Change the defaults for .. doxygenpage::
    if kind == "page":
        return [
          ":content-only:"
        ]
    elif kind == "file":
        return [
          ":content-only:",
          ":outline:"
        ]
    elif kind == "group":
        return [
          ":content-only:",
          ":outline:"
        ]
    # An empty list signals to Exhale to use the defaults
    else:
        return []


#############################
# Breathe extension
#############################

breathe_projects = {
    "Documentation"       : "_doxygen/xml/"
}

breathe_default_project = "Documentation"
breathe_default_members = ('content-only','members', 'undoc-members', 'protected-members', 'private-members')

breathe_show_include = False

breathe_function_custom = True


breathe_implementation_filename_extensions = [ '.cc']
breathe_use_project_refids = True
c_paren_attributes = ['__attribute__']
#c_id_attributes = ['__attribute__','((unused))','*LE_NONNULL','contextPtr','sessionRef']
c_extra_keywords = ['alignas', 'alignof', 'bool', 'complex', 'imaginary', 'noreturn', 'static_assert', 'thread_local','__attribute__','((unused))','*LE_NONNULL']

breathe_domain_by_extension = {
    "h" : "cpp"
}

#############################
# Exhale extension
#############################

exhale_args = {
    "kindsWithContentsDirectives": ["file", "namespace", "class", "struct", "page", "function"],
    "contentsSpecifiers":  [":local:", ":backlinks: none", ":content-only:"],
    #"unabridgedOrphanKinds": ["dir", "file","function", "page","typedef", "union"],
    "customSpecificationsMapping": utils.makeCustomSpecificationsMapping(specificationsForKind),
    "containmentFolder":     "_doxygen_rst",
    "rootFileName":          "EXCLUDE",
    "doxygenStripFromPath":  "..",
    "rootFileTitle":         "Snapdragon® Telematics Application Framework (TelAF) Interface Specification",
    "pageHierarchySubSectionTitle": "Services",
    "fullApiSubSectionTitle": " ",
    "contentsDirectives":    True,
    "createTreeView":        False,
    #"exhaleExecutesDoxygen": False,
    #"exhaleUseDoxyfile":     True,
    "minifyTreeView":        False,
    "fullToctreeMaxDepth":   5
    # "showFunctionSignatureAsTitle": True
}

# subprocess.run("doxygen telafDoxyConfig") # Only needed for local Windows testing builds

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

#master_doc = 'index'
html_theme = 'sphinx_rtd_theme'

html_title = ''

html_copy_source = False
html_show_sourcelink = False
html_show_sphinx = False