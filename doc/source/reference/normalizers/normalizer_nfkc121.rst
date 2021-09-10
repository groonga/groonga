.. -*- rst -*-

.. groonga-command
.. database: normalisers

.. _normalizer-nfkc121:

``NormalizerNFKC121``
=====================

Summary
-------

.. versionadded:: 9.0.3

``NormalizerNFKC121`` normalizes text by Unicode NFKC (Normalization Form Compatibility Composition) for Unicode version 12.1.

This normalizer can change behavior by specifying options.

Syntax
------

``NormalizerNFKC121`` has optional parameter.

No options::

  NormalizerNFKC121

``NormalizerNFKC121`` normalizes text by Unicode NFKC (Normalization Form Compatibility Composition) for Unicode version 12.1.

Specify option::

  NormalizerNFKC121("unify_kana", true)

  NormalizerNFKC121("unify_kana_case", true)

  NormalizerNFKC121("unify_kana_voiced_sound_mark", true)

  NormalizerNFKC121("unify_hyphen", true)

  NormalizerNFKC121("unify_prolonged_sound_mark", true)

  NormalizerNFKC121("unify_hyphen_and_prolonged_sound_mark", true)

  NormalizerNFKC121("unify_middle_dot", true)

  NormalizerNFKC121("unify_katakana_v_sounds", true)

  NormalizerNFKC121("unify_katakana_bu_sound", true)

  NormalizerNFKC121("unify_to_romaji", true)

Specify multiple options::

  NormalizerNFKC121("unify_to_romaji", true, "unify_kana_case", true, "unify_hyphen_and_prolonged_sound_mark", true)

``NormalizerNFKC121`` also specify multiple options as above. You can also specify mingle multiple options except above example.

Usage
-----

Simple usage
------------

Here is an example of ``NormalizerNFKC121``. ``NormalizerNFKC121`` normalizes text by Unicode NFKC (Normalization Form Compatibility Composition) for Unicode version 12.1.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121.log
.. normalize NormalizerNFKC121 "©" WITH_TYPES

Here is an example of :ref:`normalizer-nfkc121-unify-kana` option.

This option enables that same pronounced characters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-kana.log
.. normalize   'NormalizerNFKC121("unify_kana", true)'   "あイｳｪおヽヾ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc121-unify-kana-case` option.

This option enables that large and small versions of same letters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-kana-case-hiragana.log
.. normalize   'NormalizerNFKC121("unify_kana_case", true)'   "ぁあぃいぅうぇえぉおゃやゅゆょよゎわゕかゖけ"   WITH_TYPES

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-kana-case-katakana.log
.. normalize   'NormalizerNFKC121("unify_kana_case", true)'   "ァアィイゥウェエォオャヤュユョヨヮワヵカヶケ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc121-unify-kana-voiced-sound-mark` option.

This option enables that letters with/without voiced sound mark and semi voiced sound mark in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character as below.


.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-voiced-sound-mark-hiragana.log
.. normalize   'NormalizerNFKC121("unify_kana_voiced_sound_mark", true)'   "かがきぎくぐけげこごさざしじすずせぜそぞただちぢつづてでとどはばぱひびぴふぶぷへべぺほぼぽ"   WITH_TYPES

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-voiced-sound-mark-katakana.log
.. normalize   'NormalizerNFKC121("unify_kana_voiced_sound_mark", true)'   "カガキギクグケゲコゴサザシジスズセゼソゾタダチヂツヅテデトドハバパヒビピフブプヘベペホボポ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc121-unify-hyphen` option.
This option enables normalize hyphen to "-" (U+002D HYPHEN-MINUS) as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-hyphen.log
.. normalize   'NormalizerNFKC121("unify_hyphen", true)'   "-˗֊‐‑‒–⁃⁻₋−"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc121-unify-prolonged-sound-mark` option.
This option enables normalize prolonged sound to "-" (U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK) as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-prolonged-sound-mark.log
.. normalize   'NormalizerNFKC121("unify_prolonged_sound_mark", true)'   "ー—―─━ｰ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc121-unify-hyphen-and-prolonged-sound-mark` option.
This option enables normalize hyphen and prolonged sound to "-" (U+002D HYPHEN-MINUS) as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-hyphen-and-prolonged-sound-mark.log
.. normalize   'NormalizerNFKC121("unify_hyphen_and_prolonged_sound_mark", true)'   "-˗֊‐‑‒–⁃⁻₋− ﹣－ ー—―─━ｰ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc121-unify-middle-dot` option.
This option enables normalize middle dot to "·" (U+00B7 MIDDLE DOT) as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-middle-dot.log
.. normalize   'NormalizerNFKC121("unify_middle_dot", true)'   "·ᐧ•∙⋅⸱・･"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc121-unify-katakana-v-sounds` option.
This option enables normalize "ヴァヴィヴヴェヴォ" to "バビブベボ" as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-katakana-v-sounds.log
.. normalize   'NormalizerNFKC121("unify_katakana_v_sounds", true)'   "ヴァヴィヴヴェヴォヴ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc121-unify-katakana-bu-sounds` option.
This option enables normalize "ヴァヴィヴゥヴェヴォ" to "ブ" as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-katakana-bu-sounds.log
.. normalize   'NormalizerNFKC121("unify_katakana_bu_sound", true)'   "ヴァヴィヴヴェヴォヴ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc121-unify-to-romaji` option.
This option enables normalize hiragana and katakana to romaji as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-to-romaji.log
.. normalize   'NormalizerNFKC121("unify_to_romaji", true)'   "アァイィウゥエェオォ"   WITH_TYPES

Advanced usage
--------------

You can output romaji of specific a part of speech with using to combine
``TokenMecab`` and ``NormalizerNFKC121`` as below.

First of all, you extract reading of a noun with excluding non-independent word and suffix of person name with ``target_class`` option and ``include_reading`` option.

Next, you normalize reading of the noun that extracted with ``unify_to_romaji`` option of ``NormalizerNFKC121``.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc121-unify-to-romaji-complex.log
.. tokenize 'TokenMecab("target_class", "-名詞/非自立", "target_class", "-名詞/接尾/人名", "target_class", "名詞", "include_reading", true)' '彼の名前は山田さんのはずです。'
.. normalize   'NormalizerNFKC121("unify_to_romaji", true)'   "カレ"   WITH_TYPES
.. normalize   'NormalizerNFKC121("unify_to_romaji", true)'   "ナマエ"   WITH_TYPES
.. normalize   'NormalizerNFKC121("unify_to_romaji", true)'   "ヤマダ"   WITH_TYPES

Parameters
----------

Optional parameter
^^^^^^^^^^^^^^^^^^

There are optional parameters as below.

.. _normalizer-nfkc121-unify-kana:

``unify_kana``
""""""""""""""

This option enables that same pronounced characters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character.

.. _normalizer-nfkc121-unify-kana-case:

``unify_kana_case``
"""""""""""""""""""

This option enables that large and small versions of same letters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character.

.. _normalizer-nfkc121-unify-kana-voiced-sound-mark:

``unify_kana_voiced_sound_mark``
""""""""""""""""""""""""""""""""

This option enables that letters with/without voiced sound mark and semi voiced sound mark in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character.

.. _normalizer-nfkc121-unify-hyphen:

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

.. _normalizer-nfkc121-unify-prolonged-sound-mark:

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

.. _normalizer-nfkc121-unify-hyphen-and-prolonged-sound-mark:

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

.. _normalizer-nfkc121-unify-middle-dot:

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

.. _normalizer-nfkc121-unify-katakana-v-sounds:

``unify_katakana_v_sounds``
"""""""""""""""""""""""""""

This option enables normalize "ヴァヴィヴヴェヴォ" to "バビブベボ".

.. _normalizer-nfkc121-unify-katakana-bu-sounds:

``unify_katakana_bu_sound``
"""""""""""""""""""""""""""

This option enables normalize "ヴァヴィヴゥヴェヴォ" to "ブ".

.. _normalizer-nfkc121-unify-to-romaji:

``unify_to_romaji``
"""""""""""""""""""

This option enables normalize hiragana and katakana to romaji.

See also
----------

* :doc:`../commands/normalize`
