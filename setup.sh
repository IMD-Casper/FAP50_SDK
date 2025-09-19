#!/usr/bin/env bash

# Ensure the script is executed from the project root directory
if [ ! -d ".git" ]; then
  echo "Error: Please run this script from the project root directory."
  exit 1
fi

echo "Installing Git pre-commit hook..."

# Check if the .git/hooks directory exists; create it if it doesn't
if [ ! -d ".git/hooks" ]; then
  mkdir -p .git/hooks
fi

# Remove any existing pre-commit link or file to avoid conflicts
if [ -e ".git/hooks/pre-commit" ]; then
  rm .git/hooks/pre-commit
fi

# Create a symbolic link from githooks/pre-commit to .git/hooks/pre-commit
# Note: Git Bash on Windows handles Linux/Unix path formats
ln -s ../../githooks/pre-commit .git/hooks/pre-commit

echo "Git pre-commit hook installed successfully."