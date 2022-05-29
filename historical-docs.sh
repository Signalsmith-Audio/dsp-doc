printf "" > version-notes.txt

current=$(git symbolic-ref --short HEAD)

for tag in $(git tag -l "v*" --sort=version:refname)
do
	printf "$tag: " >> version-notes.txt
	git log -n 1 --format="[%cs] %s" "$tag" -- >> version-notes.txt
	git log -n 1 --format="%b" "$tag" -- | sed  's/^/     /' >> version-notes.txt
     
	if [ -d "$tag" ]
	then
		echo "Already exists: $tag"
	else
		echo "Tag: $tag"
		git -c advice.detachedHead=false checkout "$tag"
		pushd ..
		git -c advice.detachedHead=false checkout "$tag";
		popd

		make clean test doxygen

		mv html "$tag"
	fi
done
git checkout "$current"
make clean test doxygen

for tag in $(git tag -l "v*" --sort=version:refname)
do
	cp extra-style.js "$tag/"
done
cp extra-style.js html/
