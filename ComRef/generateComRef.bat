mkdir html
copy /Y comref.html html\comref.html
msxsl "full.xml" full.xsl -o "html\full.html"
msxsl "header.xml" header.xsl -o "html\header.html"
msxsl "index.xml" index.xsl -o "html\index.html"
msxsl "indexCategory.xml" indexCategory.xsl -o "html\indexCategory.html"
msxsl "toc.xml" toc.xsl -o "html\toc.html"
start html\comref.html
