#!/bin/sh

# Read the version from package.json
VERSION=$(node -pe "require('./package.json').version")

# Check if the version has been published on npm
if npm view jolt-physics@$VERSION version >/dev/null; then
  echo "Version $VERSION is already published on npm, nothing to do."
else
  echo "Version $VERSION has not been published yet, publishing to npm now..."
  npm publish
  echo "Published version $VERSION to npm."
fi

