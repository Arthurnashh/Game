pushd
attrib -A -R c:\DevP\Docs\ComRef\browserIndependent\comref_SR.xml
copy /Y c:\comref.xml c:\DevP\Docs\ComRef\browserIndependent\comref_SR.xml
attrib -A -R c:\DevP\Docs\ComRef\comref_SR.xml
copy /Y c:\comref.xml c:\DevP\Docs\ComRef\comref_SR.xml
c:
cd \DevP\Docs\ComRef\browserIndependent
call c:\DevP\Docs\ComRef\browserIndependent\generateComRef.bat noopen SR
cd \DevP\Docs\ComRef
svn ci -m "AB2: automatic comref update"
popd
pause
