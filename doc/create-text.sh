# This is a temporary script for test. This shoud be removed

pushd locale/ja
        DOCUMENT_VERSION="12.1.1" DOCUMENT_VERSION_FULL="12.1.1" LOCALE="ja" /home/t-hashida/.local/bin/sphinx-build -j auto  \
        -Dlanguage=ja                       \
        -d doctrees/html               \
        -b text                                \
        -E /home/t-hashida/gitdir/groonga/doc/source                               \
        text
popd

pushd locale/en
        DOCUMENT_VERSION="12.1.1" DOCUMENT_VERSION_FULL="12.1.1" LOCALE="en" /home/t-hashida/.local/bin/sphinx-build -j auto  \
        -Dlanguage=en                       \
        -d doctrees/html               \
        -b text                                \
        -E /home/t-hashida/gitdir/groonga/doc/source                               \
        text
popd