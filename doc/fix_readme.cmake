# CMake script that fixes markdown links in `infile` and writes it to `outfile`.

file(READ ${infile} content)

# Fix markdown links
string(REGEX REPLACE
  "\\[([^]]*)\\]\\(doc/([^\\.]*)\\.md\\)"
  "\n@ref md_doc_\\2\n"
  #"\n@ref md_doc_\\2 \"\\1\"\n"
  replaced
  "${content}"
  )

file(WRITE ${outfile} "${replaced}")
