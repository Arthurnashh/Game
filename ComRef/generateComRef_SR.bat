mkdir html_SR
copy /Y comref.html html_SR\comref.html
msxsl "full.xml" full.xsl -o "html_SR\full.html"
msxsl "header.xml" header.xsl -o "html_SR\header.html"
msxsl "index.xml" index.xsl -o "html_SR\index.html"
msxsl "indexCategory.xml" indexCategory.xsl -o "html_SR\indexCategory.html"
msxsl "toc.xml" toc.xsl -o "html_SR\toc.html"
start html_SR\comref.html
