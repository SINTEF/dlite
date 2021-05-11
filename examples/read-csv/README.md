DLite example: Read CSV
=======================
Simple example of how to create a csv reader plugin.  The actual
implementation of the plugin is in the `python-storage-plugins`
directory.

This example requires that you have pandas installed.  Get it with
`pip install pandas`.  The plugin actually support all formats
supported by pandas, so csv may not be the most descriptive name.

To test the example, just run

    python readcsv.py

Note that `readcsv.py` imports `config_paths.py` before importing
dlite.  This is nessesary, since `config_paths.py` sets the
DLITE_PYTHON_STORAGE_PLUGIN_DIRS such that dlite is able to locate the
plugin in the `python-storage-plugins` directory.


Credits
-------
The example csv file was downloaded from
https://people.sc.fsu.edu/~jburkardt/data/csv/csv.html
