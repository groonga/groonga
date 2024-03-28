# Introduction

This documentation describes about how to write, generate and manage
Groonga documentation.

## How to fork and clone Groonga repository

Contributing to Groonga's documentation begins with forking and cloning the Groonga repository.
These actions are essential first steps that enable personal modifications and experimentation in your personal repository.
And also, it enables you to submit them as your contributions to the Groonga repository. Follow these steps.

1. Go to https://github.com/groonga/groonga on GitHub
2. Click the `Fork` button to create a copy of the repository
3. Clone your Groonga repository with the following command

```console
% git clone --recursive git@github.com:${YOUR_GITHUB_ACCOUNT}/groonga.git
```

## Install depended software

Groonga uses [Sphinx](https://www.sphinx-doc.org/) as documentation tool.

Here are command lines to install Sphinx.

Debian GNU/Linux, Ubuntu:

```console
% ./setup.sh
% sudo pip install -r doc/requirements.txt
% (cd doc && bundle install)
```

AlmaLinux, Fedora:

```console
% sudo dnf install -y python-pip gettext
% sudo pip install -r doc/requirements.txt
% (cd doc && bundle install)
```

macOS:

```console
% brew bundle
% export PATH=$(brew --prefix gettext)/bin:$PATH
% pip install -r doc/requirements.txt
% (cd doc && bundle install)
```

## Run `cmake` with `--preset=doc`

Groonga disables documentation generation by default. You need to
enable it explicitly by adding `--preset=doc` option to
`cmake`:

```console
% cmake -S . -B ../groonga.doc --preset=doc
```

Now, your Groonga build is documentation ready.

## Generate HTML

You can generate HTML by the following command:

```console
% cmake --build ../groonga.doc
```

You can find generated HTML documentation at `../groonga.doc/doc/en/html/`.

## How to edit documentation

The Groonga documentation is written in [reStructuredText (.rst)](https://www.sphinx-doc.org/en/master/usage/restructuredtext/index.html) or [Markdown (.md)](https://myst-parser.readthedocs.io/en/latest/) . These files are located in the `doc/source`.
Each page of the documentation corresponds to a `.rst` file or a `.md` file. By modifying the corresponding file, you can edit the target document.
For Example, if you want to edit this {doc}`introduction` page, you should edit the `doc/source/contribution/documentation/introduction.rst` file.
Please find the file you wish to edit and make your changes.

## Preview changes on HTML files

You can preview your documentation changes in your browser in HTML format. Follow these two steps.

1. Generate the HTML files with your changes
2. Preview the generated HTML files in your Web browser

### Generate the HTML files with your changes

Use the following command to generate HTML files that reflect your changes. The generated files will be located in `../groonga.doc/doc/en/html/`. Each file corresponds to a `.rst` or `.md` file:

```console
% cmake --build ../groonga.doc
```

### Preview the generated HTML files in your Web browser

Open the generated file in your Web browser to preview your changes.
For example, if you have edited this {doc}`introduction` page, you can preview it by the following command:

```console
% open ../groonga.doc/doc/en/html/contribution/documentation/introduction.html
```

## Optional: Translate documentation

This is an optional step.

After editing and previewing the Groonga documentation, the next step is to translate the documents to make them accessible to a wider range of Groonga community users. Translating into languages other than English ensures that non-English speakers can also understand the Groonga documentation. Follow these steps to translate Groonga documentation.

1. Translate the documentation in `.edit.po` files

### Translate the documentation in `.edit.po` files

Use the following command to generate `.edit.po` files, which are translation files, corresponding to your changes. The generated files will be located in `../groonga.doc/doc/locale/${LANGUAGE}/LC_MESSAGES`. Each file corresponds to a `.rst` or `.md` file. Please add your translations to `.edit.po` files:

```console
% cmake --build ../groonga.doc
```

For example, if you have edited the {doc}`introduction` page and want to add Japanese translations, update this `../groonga.doc/doc/ja/LC_MESSAGES/contribution/documentation/introduction.edit.po` file.

```{caution}
Please do not modify the `.rst` or `.md` files while adding translations to the `.edit.po` files.
Editing `.rst` or `.md` files without first reflecting the translations in `.po` files will result in the loss of those translations.
If you want to edit `.rst` or `.md` files, ensure you first reflect your translations in `.po` files.
The method to reflect translations will be introduced in the next step.
```

## Update

You can find sources of documentation at `doc/source/`. The sources
should be written in English. See {doc}`i18n` about how to translate
documentation.

You can update the target file when you update the existing
documentation file.
