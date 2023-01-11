# CMake script that fixes markdown links in `infile` and writes it to `outfile`.

file(READ ${infile} content)

# Fix markdown links to .md files in subdirectories of doc/
string(REGEX REPLACE
  "\\[([^]]*)\\]\\(doc/([^/]*)/([^\\.]*)\\.md\\)"
  "\n@ref md_doc_\\2_\\3\n"
  replaced1
  "${content}"
  )

# Fix markdown links to .md files in doc/
string(REGEX REPLACE
  "\\[([^]]*)\\]\\(doc/([^\\.]*)\\.md\\)"
  "\n@ref md_doc_\\2\n"
  replaced2
  "${replaced1}"
  )

file(WRITE ${outfile} "${replaced2}")
