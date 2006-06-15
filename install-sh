#!/bin/sh
# 
# Install the tintin++ executable and help file.
# 

echo " "
echo "Copying the tintin++ executable to \"$1\"..."
strip tt++
cp tt++ "$1"

echo "Copying the tintin++ help file to \"$2\"..."
if [ "$3" ]; then 
  cp support/.tt_help.txt.Z "$2/"
else
  echo Uncompressing.
  uncompress -c support/.tt_help.txt.Z | cat > "$2"/.tt_help.txt
fi

echo 
echo "You'll have to do 1 thing before you continue with using tintin++."
echo "Look over support/.tintinrc and notice the #pathdir's..  Either add" 
echo "Them to your current .tintinrc in your home directory, or copy that"
echo "file to your home directory with something like:"
echo "cp support/.tintinrc ~"
echo " "
echo "If you have any questions, problems, or comments, you may email"
echo "us at the address listed in the README, or type 'gripe'."
echo "and enter what your gripe is (This will get mailed to the Devel Team)"
echo " "
echo "Enjoy!!!"
echo "--The tintin++ developement team"
exit 0


