printf "" > version-notes.txt

current=$(git symbolic-ref --short HEAD)

for tag in $(git tag -l "dev-v*" --sort=-version:refname)
do
	tag=`echo "$tag" | sed -e "s/^dev-//"`
	printf "$tag: " >> version-notes.txt
	git log -n 1 --format="[%cs] %s" "$tag" -- >> version-notes.txt
	git log -n 1 --format="%b" "$tag" -- | sed  's/^/     /' >> version-notes.txt
     
	if [ -d "$tag" ]
	then
		echo "Already exists: $tag"
	else
		echo "Tag: $tag"
		git -c advice.detachedHead=false checkout "dev-$tag"

		make clean test doxygen

		mv html "$tag"
	fi
done
git checkout "$current"

for tag in $(git tag -l "dev-v*" --sort=-version:refname)
do
	tag=`echo "$tag" | sed -e "s/^dev-//"`
	cp extra-style.js "$tag/"
done

echo "final clean test/doxygen"
if make clean test doxygen
then
	cp extra-style.js html/
else
	exit 1
fi
