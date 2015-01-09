xombrero
========

xombrero is a minimalist web browser with sophisticated security
features designed-in, rather than through an add-on after-the-fact. In
particular, it provides both persistent and per-session controls for
scripts and cookies, making it easy to thwart tracking and scripting
attacks.  In additional to providing a familiar mouse-based interface
like other web browsers, it offers a set of vi-like keyboard commands
for users who prefer to keep their hands on their keyboard.  The default
settings provide a secure environment. With simple keyboard commands,
the user can "whitelist" specific sites, allowing cookies and scripts
from those sites.

For more information, please visit https://opensource.conformal.com/wiki/xombrero

## GPG Verification Key

All official release tags are signed by Conformal so users can ensure the code
has not been tampered with and is coming from Conformal.  To verify the
signature perform the following:

- Download the public key from the Conformal website at
  https://opensource.conformal.com/GIT-GPG-KEY-conformal.txt

- Import the public key into your GPG keyring:
  ```bash
  gpg --import GIT-GPG-KEY-conformal.txt
  ```

- Verify the release tag with the following command where `TAG_NAME` is a
  placeholder for the specific tag:
  ```bash
  git tag -v TAG_NAME
  ```
## Bookmarks folders

A folder is started by a line of the form

    *A foldername

and goes until a matching line

    *O [any text]

is found. Folders can be nested.

Convert from google chrome export:

    sed -n -e '/<DT><A HREF/ { s/.*HREF="\([^"]*\)".*>\(.*\)<\/A>.*/\2\n\1/p }' \
            -e 's/.*<DT><H3 .*>\(.*\)<\/H3>.*/*A \1/p' \
            -e 's/ .*<\/DL><p>.*/*O/p' bookmarks_1_9_15.html > ~/.xombrero/favorites

## License

xombrero is ISC licensed unless otherwise specified in individual files
