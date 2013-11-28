#!/bin/bash

rm -f arcive.tar

res=$(git status | grep "nothing to commit, working directory clean" -c)

if [[ $res == "0" ]]; then
	echo "ERROR: unsynched changes with git repo"
else
	git archive --format=tar HEAD > archive.tar
	echo "Done"
fi
