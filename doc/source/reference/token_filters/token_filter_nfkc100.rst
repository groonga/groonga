.. -*- rst -*-

.. groonga-command
.. database: token_filters_nfkc100

``TokenFilterNFKC100``
======================

Summary
-------

.. versionadded:: 8.0.9

This token filter can use the same option by :ref:`normalizer-nfkc100`.
This token filter is used to normalize after tokenizing.
Because, if you normalize before tokenizing with ``TokenMecab`` , the meaning of a token may be lost.

Syntax
------

``TokenFilterNFKC100`` has optional parameter.

No options::

  TokenFilterNFKC100

``TokenFilterNFKC100`` normalizes text by Unicode NFKC (Normalization Form Compatibility Composition)
for Unicode version 10.0.

Specify option::

  TokenFilterNFKC100("unify_kana", true)

  TokenFilterNFKC100("unify_kana_case", true)

  TokenFilterNFKC100("unify_kana_voiced_sound_mark", true)

  TokenFilterNFKC100("unify_hyphen", true)

  TokenFilterNFKC100("unify_prolonged_sound_mark", true)

  TokenFilterNFKC100("unify_hyphen_and_prolonged_sound_mark", true)

  TokenFilterNFKC100("unify_middle_dot", true)

  TokenFilterNFKC100("unify_katakana_v_sounds", true)

  TokenFilterNFKC100("unify_katakana_bu_sound", true)

  TokenFilterNFKC100("unify_to_romaji", true)

Usage
-----

Simple usage
------------

Here is an example of ``TokenFilterNFKC100``. ``TokenFilterNFKC100`` normalizes text by Unicode NFKC (Normalization Form Compatibility Composition) for Unicode version 10.0.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100.log
.. tokenize TokenDelimit "©" --token_filters TokenFilterNFKC100

Here is an example of :ref:`token-filter-nfkc100-unify-kana` option.

This option enables that same pronounced characters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-kana.log
.. tokenize TokenDelimit "あイｳｪおヽヾ" --token_filters 'TokenFilterNFKC100("unify_kana", true)'

Here is an example of :ref:`token-filter-nfkc100-unify-kana-case` option.

This option enables that large and small versions of same letters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-kana-case-hiragana.log
.. tokenize TokenDelimit "ぁあぃいぅうぇえぉおゃやゅゆょよゎわゕかゖけ" --token_filters 'TokenFilterNFKC100("unify_kana_case", true)'

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-kana-case-katakana.log
.. tokenize TokenDelimit "ァアィイゥウェエォオャヤュユョヨヮワヵカヶケ" --token_filters 'TokenFilterNFKC100("unify_kana_case", true)'

Here is an example of :ref:`token-filter-nfkc100-unify-kana-voiced-sound-mark` option.

This option enables that letters with/without voiced sound mark and semi voiced sound mark in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character as below.


.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-voiced-sound-mark-hiragana.log
.. tokenize TokenDelimit "かがきぎくぐけげこごさざしじすずせぜそぞただちぢつづてでとどはばぱひびぴふぶぷへべぺほぼぽ" --token_filters 'TokenFilterNFKC100("unify_kana_voiced_sound_mark", true)'

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-voiced-sound-mark-katakana.log
.. tokenize TokenDelimit "カガキギクグケゲコゴサザシジスズセゼソゾタダチヂツヅテデトドハバパヒビピフブプヘベペホボポ" --token-fitlers 'TokenFilterNFKC100("unify_kana_voiced_sound_mark", true)'

Here is an example of :ref:`token-filter-nfkc100-unify-hyphen` option.
This option enables normalize hyphen to "-" (U+002D HYPHEN-MINUS) as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-hyphen.log
.. tokenize TokenDelimit "-˗֊‐‑‒–⁃⁻₋−" --token_filters 'TokenFilterNFKC100("unify_hyphen", true)'

Here is an example of :ref:`token-filter-nfkc100-unify-prolonged-sound-mark` option.
This option enables normalize prolonged sound to "-" (U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK) as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-prolonged-sound-mark.log
.. tokenize TokenDelimit "ー—―─━ｰ" --token_filters 'TokenFilterNFKC100("unify_prolonged_sound_mark", true)'

Here is an example of :ref:`token-filter-nfkc100-unify-hyphen-and-prolonged-sound-mark` option.
This option enables normalize hyphen and prolonged sound to "-" (U+002D HYPHEN-MINUS) as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-hyphen-and-prolonged-sound-mark.log
.. tokenize TokenDelimit "-˗֊‐‑‒–⁃⁻₋− ﹣－ ー—―─━ｰ" --token_filters 'TokenFilterNFKC100("unify_hyphen_and_prolonged_sound_mark", true)'

Here is an example of :ref:`token-filter-nfkc100-unify-middle-dot` option.
This option enables normalize middle dot to "·" (U+00B7 MIDDLE DOT) as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-middle-dot.log
.. tokenize TokenDelimit "·ᐧ•∙⋅⸱・･" --token_filters 'TokenFilterNFKC100("unify_middle_dot", true)'

Here is an example of :ref:`token-filter-nfkc100-unify-katakana-v-sounds` option.
This option enables normalize "ヴァヴィヴヴェヴォ" to "バビブベボ" as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-katakana-v-sounds.log
.. tokenize TokenDelimit "ヴァヴィヴヴェヴォヴ" --token_filters 'TokenFilterNFKC100("unify_katakana_v_sounds", true)'

Here is an example of :ref:`token-filter-nfkc100-unify-katakana-bu-sounds` option.
This option enables normalize "ヴァヴィヴゥヴェヴォ" to "ブ" as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-katakana-bu-sounds.log
.. tokenize TokenDelimit "ヴァヴィヴヴェヴォヴ" --token_filters 'TokenFilterNFKC100("unify_katakana_bu_sound", true)'

Here is an example of :ref:`token-filter-nfkc100-unify-to-romaji` option.
This option enables normalize hiragana and katakana to romaji as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-unify-to-romaji.log
.. tokenize TokenDelimit "アァイィウゥエェオォ" --token_filters  'TokenFilterNFKC100("unify_to_romaji", true)'

Advanced usage
--------------

You can output all input string as hiragana with cimbining ``TokenFilterNFKC100`` with ``use_reading`` option of ``TokenMecab`` as below.

.. groonga-command
.. include:: ../../example/reference/token_filters/nfkc100-with-token-mecab.log
.. tokenize   'TokenMecab("use_reading", true)'   "私は林檎を食べます。"   --token_filters 'TokenFilterNFKC100("unify_kana", true)'

Parameters
----------

Optional parameter
^^^^^^^^^^^^^^^^^^

There are optional parameters as below.

.. _token-filter-nfkc100-unify-kana:

``unify_kana``
""""""""""""""

This option enables that same pronounced characters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character.

.. _token-filter-nfkc100-unify-kana-case:

``unify_kana_case``
"""""""""""""""""""

This option enables that large and small versions of same letters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character.

.. _token-filter-nfkc100-unify-kana-voiced-sound-mark:

``unify_kana_voiced_sound_mark``
""""""""""""""""""""""""""""""""

This option enables that letters with/without voiced sound mark and semi voiced sound mark in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character.

.. _token-filter-nfkc100-unify-hyphen:

``unify_hyphen``
""""""""""""""""

This option enables normalize hyphen to "-" (U+002D HYPHEN-MINUS).

Hyphen of the target of normalizing is as below.

* "-" (U+002D HYPHEN-MINUS)
* "֊" (U+058A ARMENIAN HYPHEN)
* "˗" (U+02D7 MODIFIER LETTER MINUS SIGN)
* "‐" (U+2010 HYPHEN)
* "—" (U+2014 EM DASH)
* "⁃" (U+2043 HYPHEN BULLET)
* "⁻" (U+207B SUPERSCRIPT MINUS)
* "₋" (U+208B SUBSCRIPT MINUS)
* "−" (U+2212 MINUS SIGN)

.. _token-filter-nfkc100-unify-prolonged-sound-mark:

``unify_prolonged_sound_mark``
""""""""""""""""""""""""""""""

This option enables normalize prolonged sound to "-" (U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK).

Prolonged sound of the target of normalizing is as below.

* "—" (U+2014 EM DASH)
* "―" (U+2015 HORIZONTAL BAR)
* "─" (U+2500 BOX DRAWINGS LIGHT HORIZONTAL)
* "━" (U+2501 BOX DRAWINGS HEAVY HORIZONTAL)
* "ー" (U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK)
* "ｰ" (U+FF70 HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK)

.. _token-filter-nfkc100-unify-hyphen-and-prolonged-sound-mark:

``unify_hyphen_and_prolonged_sound_mark``
"""""""""""""""""""""""""""""""""""""""""

This option enables normalize hyphen and prolonged sound to "-" (U+002D HYPHEN-MINUS).

Hyphen and prolonged sound of the target normalizing is below.

* "-" (U+002D HYPHEN-MINUS)
* "֊" (U+058A ARMENIAN HYPHEN)
* "˗" (U+02D7 MODIFIER LETTER MINUS SIGN)
* "‐" (U+2010 HYPHEN)
* "—" (U+2014 EM DASH)
* "⁃" (U+2043 HYPHEN BULLET)
* "⁻" (U+207B SUPERSCRIPT MINUS)
* "₋" (U+208B SUBSCRIPT MINUS)
* "−" (U+2212 MINUS SIGN)

* "—" (U+2014 EM DASH)
* "―" (U+2015 HORIZONTAL BAR)
* "─" (U+2500 BOX DRAWINGS LIGHT HORIZONTAL)
* "━" (U+2501 BOX DRAWINGS HEAVY HORIZONTAL)
* "ー" (U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK)
* "ｰ" (U+FF70 HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK)

.. _token-filter-nfkc100-unify-middle-dot:

``unify_middle_dot``
""""""""""""""""""""

This option enables normalize middle dot to "·" (U+00B7 MIDDLE DOT).

Middle dot of the target of normalizing is as below.

* "·" (U+00B7 MIDDLE DOT)
* "ᐧ" (U+1427 CANADIAN SYLLABICS FINAL MIDDLE DOT)
* "•" (U+2022 BULLET)
* "∙" (U+2219 BULLET OPERATOR)
* "⋅" (U+22C5 DOT OPERATOR)
* "⸱" (U+2E31 WORD SEPARATOR MIDDLE DOT)
* "・" (U+30FB KATAKANA MIDDLE DOT)
* "･" (U+FF65 HALFWIDTH KATAKANA MIDDLE DOT)

.. _token-filter-nfkc100-unify-katakana-v-sounds:

``unify_katakana_v_sounds``
"""""""""""""""""""""""""""

This option enables normalize "ヴァヴィヴヴェヴォ" to "バビブベボ".

.. _token-filter-nfkc100-unify-katakana-bu-sounds:

``unify_katakana_bu_sound``
"""""""""""""""""""""""""""

This option enables normalize "ヴァヴィヴゥヴェヴォ" to "ブ".

.. _token-filter-nfkc100-unify-to-romaji:

``unify_to_romaji``
"""""""""""""""""""

This option enables normalize hiragana and katakana to romaji.
