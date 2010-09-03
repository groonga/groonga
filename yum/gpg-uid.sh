#!/bin/sh

gpg --list-secret-keys | grep uid | sed -e 's/^uid *//' | tail -1
