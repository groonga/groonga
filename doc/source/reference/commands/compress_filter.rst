.. -*- rst -*-

These flags are to increase compression rate of ``COMPRESS_ZLIB`` / ``COMPRESS_LZ4`` / ``COMPRESS_ZSTD``
to increase by filtering the data before the compression.

However it could be effective or not depending on the data.
It might decrease the compression rate.

Also, saving column and reference process are going to be slower because enabling these flags would require additional processing.

Regardless of setting ``COMPRESS_ZLIB`` , ``COMPRESS_LZ4`` , ``COMPRESS_ZSTD`` , these filters use `BloscLZ <https://www.blosc.org/pages/blosc-in-depth/#blosc-as-a-meta-compressor>`_ as the compression alogolizm, thus make the column size smaller than the noncompressed column in most cases.

Yet, there would be a case that some data would show sufficient work by only suetting ``COMPRESS_ZLIB`` , `COMPRESS_LZ4`` , ``COMPRESS_ZSTD`` .
So it is advised to set suitable flags depending on the data.

Note that ``COMPRESS_FILTER_SHUFFLE`` flag is ignored if Blosc support is invalid. Blosc support is valid in default in each packages.
However, We need to valid Blosc support explicitly when we build Groonga from source.
Please refer :doc:`/install/others` when we build Groonga from source.

These flags are only available with ``COLUMN_VECTOR`` and are ignored with ``COLUMN_SCALAR`` .
