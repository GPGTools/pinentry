#!/bin/bash

[[ $# -eq 2 ]] || exit 2

cd "$(dirname "${BASH_SOURCE[0]}")" || exit 1

if [[ -d "$DEPLOY_RESOURCES_DIR" ]] ;then
    source "$DEPLOY_RESOURCES_DIR/config/versioning.sh"
else
    source "Version.config"
fi
if [[ -z "$VERSION" ]] ;then
	echo "Missing Version!"
	exit 1
fi

dest=$2
[[ -d "$dest" ]] && dest+=/$(basename "$1")

cp "$1" "$dest" || exit 1


/usr/libexec/PlistBuddy \
	-c "Set CommitHash '${COMMIT_HASH:--}'" \
	-c "Set BuildNumber '${BUILD_NUMBER:-0}'" \
	-c "Set CFBundleVersion '${BUILD_VERSION:-0n}'" \
	-c "Set CFBundleShortVersionString '$VERSION'" \
	"$dest" || exit 1
