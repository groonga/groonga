# Introduction

This document outlines the procedures for writing, generating, and managing Groonga's documentation.
By following the steps below, you can contribute to enhancing and updating Groonga's documentation.

- [How to fork and clone Groonga repository](#how-to-fork-and-clone-groonga-repository)
- [Install dependent software](#install-dependent-software)
- [Run `cmake` with `--preset=doc`](#run-cmake-with---presetdoc)
- [Generate HTML](#generate-html)
- [How to edit Documentation](#how-to-edit-documentation)
- [Preview changes on HTML files](#preview-changes-on-html-files)
- [Send patch](#send-patch)
- [Optional: Translate documentation](#optional-translate-documentation)

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

## Install dependent software

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

## Send patch

You can submit your patch to the Groonga repository on GitHub via a pull request.
Feel free to send a pull request by following two steps.

- Prepare your pull request
- Submit your pull request

### Prepare your pull request

Ensure your changes are committed and then push your changes to your fork repository on GitHub.
Follow these commands:

```console
% git switch -c your-working-branch
% git add doc
% git commit -m 'Describe your works here'
% git push origin your-working-branch
```

### Submit your pull request

Now you're ready to submit a pull request to the upstream Groonga repository.
Follow these steps:

1. Go to your fork repository on GitHub
2. Click the `Compare & pull request` button
3. Make sure your changes are reflected
4. Click the `Create Pull Request` button and send your pull request

## Optional: Translate documentation

This is an optional step.

After editing and previewing the Groonga documentation, the next step is to translate the documents to make them accessible to a wider range of Groonga community users. Translating into languages other than English ensures that non-English speakers can also understand the Groonga documentation. Follow these steps to translate Groonga documentation.

1. Translate the documentation in `.edit.po` files
2. Reflect translations to `.po` files
3. Preview translations on HTML files

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

### Reflect translations to `.po` files

After adding your translations to the `.edit.po` files, the next step is to reflect these translations to the `.po` files. `.po` files are the finalized translation files. These `.po` files are located in `doc/locale/${LANGUAGE}/LC_MESSAGES`. Each file corresponds to a `.rst` or `.md` file. If you want to edit your translations, edit the corresponding `.edit.po` file and then reflect your changes to the `.po` file.

To reflect translations from the `.edit.po` files to the `.po` files, use the following command:

```console
% cmake --build ../groonga.doc
```

For example, if you have added Japanese translations about the {doc}`introduction` page and then execute the command above, your translations will be reflected to `/doc/locale/ja/LC_MESSAGES/contribution/documentation/introduction.po` file.

```{note}
Actually, the command to generate translation `.edit.po` files is the same as the one used for reflecting translations.
Therefore, there's no need to memorize different commands for generating translation files and reflecting the translations.
```

### Preview translations on HTML files

You can preview your translations in your Web browser in HTML format. The step for reflecting translations into `.po` files also generates the corresponding HTML files. These files are located in `../groonga.doc/doc/${LANGUAGE}/html/`. Each file corresponds to a `.rst` or `.md` file. Open the generated HTML file in your Web browser to review your translations.

For example, to preview the Japanese translation of the {doc}`introduction` page, use the following command:

```console
% open ../groonga.doc/doc/ja/html/contribution/documentation/introduction.html
```

If the translations seem correct after preview, please send a patch. For more information on this process, see the [Send patch](#send-patch)
