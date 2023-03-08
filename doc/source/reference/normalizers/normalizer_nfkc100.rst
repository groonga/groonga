.. -*- rst -*-

.. groonga-command
.. database: normalisers

.. _normalizer-nfkc100:

``NormalizerNFKC100``
=====================

Summary
-------

.. versionadded:: 8.0.2

``NormalizerNFKC100`` normalizes text by Unicode NFKC (Normalization Form Compatibility Composition) for Unicode version 10.0.

This normalizer can change behavior by specifying options.

Syntax
------

``NormalizerNFKC100`` has optional parameter.

No options::

  NormalizerNFKC100

``NormalizerNFKC100`` normalizes text by Unicode NFKC (Normalization Form Compatibility Composition) for Unicode version 10.0.

Specify option::

  NormalizerNFKC100("unify_kana", true)

  NormalizerNFKC100("unify_kana_case", true)

  NormalizerNFKC100("unify_kana_voiced_sound_mark", true)

  NormalizerNFKC100("unify_hyphen", true)

  NormalizerNFKC100("unify_prolonged_sound_mark", true)

  NormalizerNFKC100("unify_hyphen_and_prolonged_sound_mark", true)

  NormalizerNFKC100("unify_middle_dot", true)

  NormalizerNFKC100("unify_katakana_v_sounds", true)

  NormalizerNFKC100("unify_katakana_bu_sound", true)

  NormalizerNFKC100("unify_to_romaji", true)

  NormalizerNFKC100("unify_katakana_gu_small_sounds", true)

  NormalizerNFKC100("unify_katakana_di_sound", true)

  NormalizerNFKC100("unify_katakana_wo_sound", true)

  NormalizerNFKC100("unify_katakana_zu_small_sounds", true)

  NormalizerNFKC100("unify_katakana_du_sound", true)

  NormalizerNFKC100("unify_katakana_trailing_o", true)

  NormalizerNFKC100("unify_katakana_du_small_sounds", true)

  NormalizerNFKC100("unify_kana_prolonged_sound_mark", true)

  NormalizerNFKC100("unify_kana_hyphen", true)

.. versionadded:: 8.0.3

   :ref:`normalizer-nfkc100-unify-middle-dot` is added.

   :ref:`normalizer-nfkc100-unify-katakana-v-sounds` is added.

   :ref:`normalizer-nfkc100-unify-katakana-bu-sounds` is added.

.. versionadded:: 8.0.9

   :ref:`normalizer-nfkc100-unify-to-romaji` is added.

.. versionadded:: 13.0.0

  :ref:`normalizer-nfkc100-unify-katakana-gu-small-sounds` is added.

  :ref:`normalizer-nfkc100-unify-katakana-di-sound` is added.

  :ref:`normalizer-nfkc100-unify-katakana-wo-sound` is added.

  :ref:`normalizer-nfkc100-unify-katakana-zu-small-sounds` is added.

  :ref:`normalizer-nfkc100-unify-katakana-du-sound` is added.

  :ref:`normalizer-nfkc100-unify-katakana-trailing-o` is added.

  :ref:`normalizer-nfkc100-unify-katakana-du-small-sounds` is added.

.. versionadded:: 13.0.1

  :ref:`normalizer-nfkc100-unify-kana-prolonged-sound-mark` is added.

  :ref:`normalizer-nfkc100-unify-kana-hyphen` is added.

Specify multiple options::

  NormalizerNFKC100("unify_to_romaji", true, "unify_kana_case", true, "unify_hyphen_and_prolonged_sound_mark", true)

``NormalizerNFKC100`` also specify multiple options as above. You can also specify mingle multiple options except above example.

Usage
-----

Simple usage
^^^^^^^^^^^^

Here is an example of ``NormalizerNFKC100``. ``NormalizerNFKC100`` normalizes text by Unicode NFKC (Normalization Form Compatibility Composition) for Unicode version 10.0.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100.log
.. normalize NormalizerNFKC100 "©" WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-kana` option.

This option enables that same pronounced characters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-kana.log
.. normalize   'NormalizerNFKC100("unify_kana", true)'   "あイｳｪおヽヾ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-kana-case` option.

This option enables that large and small versions of same letters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-kana-case-hiragana.log
.. normalize   'NormalizerNFKC100("unify_kana_case", true)'   "ぁあぃいぅうぇえぉおゃやゅゆょよゎわゕかゖけ"   WITH_TYPES

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-kana-case-katakana.log
.. normalize   'NormalizerNFKC100("unify_kana_case", true)'   "ァアィイゥウェエォオャヤュユョヨヮワヵカヶケ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-kana-voiced-sound-mark` option.

This option enables that letters with/without voiced sound mark and semi voiced sound mark in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character as below.


.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-voiced-sound-mark-hiragana.log
.. normalize   'NormalizerNFKC100("unify_kana_voiced_sound_mark", true)'   "かがきぎくぐけげこごさざしじすずせぜそぞただちぢつづてでとどはばぱひびぴふぶぷへべぺほぼぽ"   WITH_TYPES

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-voiced-sound-mark-katakana.log
.. normalize   'NormalizerNFKC100("unify_kana_voiced_sound_mark", true)'   "カガキギクグケゲコゴサザシジスズセゼソゾタダチヂツヅテデトドハバパヒビピフブプヘベペホボポ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-hyphen` option.
This option enables normalize hyphen to "-" (U+002D HYPHEN-MINUS) as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-hyphen.log
.. normalize   'NormalizerNFKC100("unify_hyphen", true)'   "-˗֊‐‑‒–⁃⁻₋−"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-prolonged-sound-mark` option.
This option enables normalize prolonged sound to "-" (U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK) as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-prolonged-sound-mark.log
.. normalize   'NormalizerNFKC100("unify_prolonged_sound_mark", true)'   "ー—―─━ｰ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-hyphen-and-prolonged-sound-mark` option.
This option enables normalize hyphen and prolonged sound to "-" (U+002D HYPHEN-MINUS) as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-hyphen-and-prolonged-sound-mark.log
.. normalize   'NormalizerNFKC100("unify_hyphen_and_prolonged_sound_mark", true)'   "-˗֊‐‑‒–⁃⁻₋− ﹣－ ー—―─━ｰ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-middle-dot` option.
This option enables normalize middle dot to "·" (U+00B7 MIDDLE DOT) as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-middle-dot.log
.. normalize   'NormalizerNFKC100("unify_middle_dot", true)'   "·ᐧ•∙⋅⸱・･"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-katakana-v-sounds` option.
This option enables normalize "ヴァヴィヴヴェヴォ" to "バビブベボ" as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-katakana-v-sounds.log
.. normalize   'NormalizerNFKC100("unify_katakana_v_sounds", true)'   "ヴァヴィヴヴェヴォヴ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-katakana-bu-sounds` option.
This option enables normalize "ヴァヴィヴゥヴェヴォ" to "ブ" as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-katakana-bu-sounds.log
.. normalize   'NormalizerNFKC100("unify_katakana_bu_sound", true)'   "ヴァヴィヴヴェヴォヴ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-to-romaji` option.
This option enables normalize hiragana and katakana to romaji as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-to-romaji.log
.. normalize   'NormalizerNFKC100("unify_to_romaji", true)'   "アァイィウゥエェオォ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-katakana-gu-small-sounds` option.
This option enables to normalize "グァグィグェグォ" to "ガギゲゴ" as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-katakana-gu-small-sounds.log
.. normalize   'NormalizerNFKC100("unify_katakana_gu_small_sounds", true)'   "グァグィグェグォ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-katakana-di-sound` option.
This option enables to normalize "ヂ" to "ジ" as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-katakana-di-sound.log
.. normalize   'NormalizerNFKC100("unify_katakana_di_sound", true)'   "ヂ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-katakana-wo-sound` option.
This option enables to normalize "ヲ" to "オ" as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-katakana-wo-sound.log
.. normalize   'NormalizerNFKC100("unify_katakana_wo_sound", true)'   "ヲ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-katakana-zu-small-sounds` option.
This option enables to normalize "ズァズィズェズォ" to "ザジゼゾ" as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-katakana-zu-small-sounds.log
.. normalize   'NormalizerNFKC100("unify_katakana_zu_small_sounds", true)'   "ズァズィズェズォ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-katakana-du-sound` option.
This option enables to normalize "ヅ" to "ズ" as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-katakana-du-sound.log
.. normalize   'NormalizerNFKC100("unify_katakana_du_sound", true)'   "ヅ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-katakana-trailing-o` option.
This option enables to normalize "オ" to "ウ" when the vowel in the previous letter is "オ" as below.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-katakana-trailing-o.log
.. normalize   'NormalizerNFKC100("unify_katakana_trailing_o", true)'   "オオコオソオトオノオ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-katakana-du-small-sounds` option.
This option enables to normalize "ヅァヅィヅェヅォ" to "ザジゼゾ".

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-katakana-du-small-sounds.log
.. normalize   'NormalizerNFKC100("unify_katakana_du_small_sounds", true)'   "ヅァヅィヅェヅォ"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-kana-prolonged-sound-mark` option.
This option enables to normalize "ー" (U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK) 
to a vowel of a previous kana letter.

If a previous kana letter is "ん" , "ー" is normalized to "ん",
And a previous kana letter is "ン" , "ー" is normalized to "ン".

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-kana-prolonged-sound-mark.log
.. normalize   'NormalizerNFKC100("unify_kana_prolonged_sound_mark", true)'   "カーキークーケーコー"   WITH_TYPES

Here is an example of :ref:`normalizer-nfkc100-unify-kana-hyphen` option.
This option enables to normalize "-" (U+002D HYPHEN-MINUS) to a vowel of a previous kana letter.

If a previous kana letter is "ん" , "-" is normalized to "ん",
And a previous kana letter is "ン" , "-" is normalized to "ン".

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-kana-hyphen.log
.. normalize   'NormalizerNFKC100("unify_kana_hyphen", true)'   "カ-キ-ク-ケ-コ-"   WITH_TYPES

Advanced usage
^^^^^^^^^^^^^^

You can output romaji of specific a part of speech with using to combine
``TokenMecab`` and ``NormalizerNFKC100`` as below.

First of all, you extract reading of a noun with excluding non-independent word and suffix of person name with ``target_class`` option and ``include_reading`` option.

Next, you normalize reading of the noun that extracted with ``unify_to_romaji`` option of ``NormalizerNFKC100``.

.. groonga-command
.. include:: ../../example/reference/normalizers/normalizer-nfkc100-unify-to-romaji-complex.log
.. tokenize 'TokenMecab("target_class", "-名詞/非自立", "target_class", "-名詞/接尾/人名", "target_class", "名詞", "include_reading", true)' '彼の名前は山田さんのはずです。'
.. normalize   'NormalizerNFKC100("unify_to_romaji", true)'   "カレ"   WITH_TYPES
.. normalize   'NormalizerNFKC100("unify_to_romaji", true)'   "ナマエ"   WITH_TYPES
.. normalize   'NormalizerNFKC100("unify_to_romaji", true)'   "ヤマダ"   WITH_TYPES

Parameters
----------

Optional parameter
^^^^^^^^^^^^^^^^^^

There are optional parameters as below.

.. _normalizer-nfkc100-unify-kana:

``unify_kana``
""""""""""""""

This option enables that same pronounced characters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character.

.. _normalizer-nfkc100-unify-kana-case:

``unify_kana_case``
"""""""""""""""""""

This option enables that large and small versions of same letters in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character.

.. _normalizer-nfkc100-unify-kana-voiced-sound-mark:

``unify_kana_voiced_sound_mark``
""""""""""""""""""""""""""""""""

This option enables that letters with/without voiced sound mark and semi voiced sound mark in all of full-width Hiragana, full-width Katakana and half-width Katakana are regarded as the same character.

.. _normalizer-nfkc100-unify-hyphen:

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

.. _normalizer-nfkc100-unify-prolonged-sound-mark:

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

.. _normalizer-nfkc100-unify-hyphen-and-prolonged-sound-mark:

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

.. _normalizer-nfkc100-unify-middle-dot:

``unify_middle_dot``
""""""""""""""""""""

.. versionadded:: 8.0.3

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

.. _normalizer-nfkc100-unify-katakana-v-sounds:

``unify_katakana_v_sounds``
"""""""""""""""""""""""""""

.. versionadded:: 8.0.3

This option enables normalize "ヴァヴィヴヴェヴォ" to "バビブベボ".

.. _normalizer-nfkc100-unify-katakana-bu-sounds:

``unify_katakana_bu_sound``
"""""""""""""""""""""""""""

.. versionadded:: 8.0.3

This option enables normalize "ヴァヴィヴゥヴェヴォ" to "ブ".

.. _normalizer-nfkc100-unify-to-romaji:

``unify_to_romaji``
"""""""""""""""""""

.. versionadded:: 8.0.9

This option enables normalize hiragana and katakana to romaji.

.. _normalizer-nfkc100-unify-katakana-gu-small-sounds:

``unify_katakana_gu_small_sounds``
""""""""""""""""""""""""""""""""""

.. versionadded:: 13.0.0

This option enables to normalize "グァグィグェグォ" to "ガギゲゴ".

.. _normalizer-nfkc100-unify-katakana-di-sound:

``unify_katakana_di_sound``
"""""""""""""""""""""""""""

.. versionadded:: 13.0.0

This option enables to normalize "ヂ" to "ジ".

.. _normalizer-nfkc100-unify-katakana-wo-sound:

``unify_katakana_wo_sound``
"""""""""""""""""""""""""""

.. versionadded:: 13.0.0

This option enables to normalize "ヲ" to "オ".

.. _normalizer-nfkc100-unify-katakana-zu-small-sounds:

``unify_katakana_zu_small_sounds``
""""""""""""""""""""""""""""""""""

.. versionadded:: 13.0.0

This option enables to normalize "ズァズィズェズォ" to "ザジゼゾ".

.. _normalizer-nfkc100-unify-katakana-du-sound:

``unify_katakana_du_sound``
"""""""""""""""""""""""""""

.. versionadded:: 13.0.0

This option enables to normalize "ヅ" to "ズ".

.. _normalizer-nfkc100-unify-katakana-trailing-o:

``unify_katakana_trailing_o``
"""""""""""""""""""""""""""""

.. versionadded:: 13.0.0

This option enables to normalize "オ" to "ウ"
when the vowel in the previous letter is "オ".

* "ォオ" -> "ォウ"
* "オオ" -> "オウ"
* "コオ" -> "コウ"
* "ソオ" -> "ソウ"
* "トオ" -> "トウ"
* "ノオ" -> "ノウ"
* "ホオ" -> "ホウ"
* "モオ" -> "モウ"
* "ョオ" -> "ョオ"
* "ヨオ" -> "ヨウ"
* "ロオ" -> "ロウ"
* "ゴオ" -> "ゴウ"
* "ゾオ" -> "ゾウ"
* "ドオ" -> "ドウ"
* "ボオ" -> "ボウ"
* "ポオ" -> "ポウ"
* "ヺオ" -> "ヺウ"

.. _normalizer-nfkc100-unify-katakana-du-small-sounds:

``unify_katakana_du_small_sounds``
""""""""""""""""""""""""""""""""""

.. versionadded:: 13.0.0

This option enables to normalize "ヅァヅィヅェヅォ" to "ザジゼゾ".

.. _normalizer-nfkc100-unify-kana-prolonged-sound-mark:

``unify_kana_prolonged_sound_mark``
"""""""""""""""""""""""""""""""""""

.. versionadded:: 13.0.1

This option enables to normalize "ー" (U+30FC KATAKANA-HIRAGANA PROLONGED SOUND MARK) 
to a vowel of a previous kana letter.

If a previous kana letter is "ん" , "ー" is normalized to "ん",
And a previous kana letter is "ン" , "ー" is normalized to "ン".

.. code-block::

   ァー -> ァア, アー -> アア, ヵー -> ヵア, カー -> カア, ガー -> ガア, サー -> サア, ザー -> ザア, 
   ター -> タア, ダー -> ダア, ナー -> ナア, ハー -> ハア, バー -> バア, パー -> パア, マー -> マア, 
   ャー -> ャア, ヤー -> ヤア, ラー -> ラア, ヮー -> ヮア, ワー -> ワア, ヷー -> ヷア,
   ィー -> ィイ, イー -> イイ, キー -> キイ, ギー -> ギイ, シー -> シイ, ジー -> ジイ, チー -> チイ,
   ヂー -> ヂイ, ニー -> ニイ, ヒー -> ヒイ, ビー -> ビイ, ピー -> ピイ, ミー -> ミイ, リー -> リイ,
   ヰー -> ヰイ, ヸー -> ヸイ, 
   
   ゥー -> ゥウ, ウー -> ウウ, クー -> クウ, グー -> グウ, スー -> スウ, ズー -> ズウ, ツー -> ツウ,
   ヅー -> ヅウ, ヌー -> ヌウ, フー -> フウ, ブー -> ブウ, プー -> プウ, ムー -> ムウ, ュー -> ュウ,
   ユー -> ユウ, ルー -> ルウ, ヱー -> ヱウ, ヴー -> ヴウ,
   
   ェー -> ェエ, エー -> エエ, ヶー -> ヶエ, ケー -> ケエ, ゲー -> ゲエ, セー -> セエ, ゼー -> ゼエ,
   テー -> テエ, デー -> デエ, ネー -> ネエ, ヘー -> ヘエ, ベー -> ベエ, ペー -> ペエ, メー -> メエ,
   レー -> レエ, ヹー -> ヹエ,
   
   ォー -> ォオ, オー -> オオ, コー -> コオ, ゴー -> ゴオ, ソー -> ソオ, ゾー -> ゾオ, トー -> トオ,
   ドー -> ドオ, ノー -> ノオ, ホー -> ホオ, ボー -> ボオ, ポー -> ポオ, モー -> モオ, ョー -> ョオ,
   ヨー -> ヨオ, ロー -> ロオ, ヲー -> ヲオ, ヺー -> ヺオ, 
   
   ンー -> ンン
   
   ぁー -> ぁあ, あー -> ああ, ゕー -> ゕあ, かー -> かあ, がー -> があ, さー -> さあ, ざー -> ざあ, 
   たー -> たあ, だー -> だあ, なー -> なあ, はー -> はあ, ばー -> ばあ, ぱー -> ぱあ, まー -> まあ, 
   ゃー -> ゃあ, やー -> やあ, らー -> らあ, ゎー -> ゎあ, わー -> わあ 
   
   ぃー -> ぃい, いー -> いい, きー -> きい, ぎー -> ぎい, しー -> しい, じー -> じい, ちー -> ちい,
   ぢー -> ぢい, にー -> にい, ひー -> ひい, びー -> びい, ぴー -> ぴい, みー -> みい, りー -> りい,
   ゐー -> ゐい
   
   ぅー -> ぅう, うー -> うう, くー -> くう, ぐー -> ぐう, すー -> すう, ずー -> ずう, つー -> つう,
   づー -> づう, ぬー -> ぬう, ふー -> ふう, ぶー -> ぶう, ぷー -> ぷう, むー -> むう, ゅー -> ゅう,
   ゆー -> ゆう, るー -> るう, ゑー -> ゑう, ゔー -> ゔう
   
   ぇー -> ぇえ, えー -> ええ, ゖー -> ゖえ, けー -> けえ, げー -> げえ, せー -> せえ, ぜー -> ぜえ,
   てー -> てえ, でー -> でえ, ねー -> ねえ, へー -> へえ, べー -> べえ, ぺー -> ぺえ, めー -> めえ,
   れー -> れえ
   
   ぉー -> ぉお, おー -> おお, こー -> こお, ごー -> ごお, そー -> そお, ぞー -> ぞお, とー -> とお,
   どー -> どお, のー -> のお, ほー -> ほお, ぼー -> ぼお, ぽー -> ぽお, もー -> もお, ょー -> ょお,
   よー -> よお, ろー -> ろお, をー -> をお
   
   んー -> んん

.. _normalizer-nfkc100-unify-kana-hyphen:

``unify_kana_hyphen``
"""""""""""""""""""""

.. versionadded:: 13.0.1

This option enables to normalize "-" (U+002D HYPHEN-MINUS) to a vowel of a previous kana letter.

If a previous kana letter is "ん" , "-" is normalized to "ん",
And a previous kana letter is "ン" , "-" is normalized to "ン".

.. code-block::

   ァ- -> ァア, ア- -> アア, ヵ- -> ヵア, カ- -> カア, ガ- -> ガア, サ- -> サア, ザ- -> ザア, 
   タ- -> タア, ダ- -> ダア, ナ- -> ナア, ハ- -> ハア, バ- -> バア, パ- -> パア, マ- -> マア, 
   ャ- -> ャア, ヤ- -> ヤア, ラ- -> ラア, ヮ- -> ヮア, ワ- -> ワア, ヷ- -> ヷア,
   ィ- -> ィイ, イ- -> イイ, キ- -> キイ, ギ- -> ギイ, シ- -> シイ, ジ- -> ジイ, チ- -> チイ,
   ヂ- -> ヂイ, ニ- -> ニイ, ヒ- -> ヒイ, ビ- -> ビイ, ピ- -> ピイ, ミ- -> ミイ, リ- -> リイ,
   ヰ- -> ヰイ, ヸ- -> ヸイ, 
   
   ゥ- -> ゥウ, ウ- -> ウウ, ク- -> クウ, グ- -> グウ, ス- -> スウ, ズ- -> ズウ, ツ- -> ツウ,
   ヅ- -> ヅウ, ヌ- -> ヌウ, フ- -> フウ, ブ- -> ブウ, プ- -> プウ, ム- -> ムウ, ュ- -> ュウ,
   ユ- -> ユウ, ル- -> ルウ, ヱ- -> ヱウ, ヴ- -> ヴウ,
   
   ェ- -> ェエ, エ- -> エエ, ヶ- -> ヶエ, ケ- -> ケエ, ゲ- -> ゲエ, セ- -> セエ, ゼ- -> ゼエ,
   テ- -> テエ, デ- -> デエ, ネ- -> ネエ, ヘ- -> ヘエ, ベ- -> ベエ, ペ- -> ペエ, メ- -> メエ,
   レ- -> レエ, ヹ- -> ヹエ,
   
   ォ- -> ォオ, オ- -> オオ, コ- -> コオ, ゴ- -> ゴオ, ソ- -> ソオ, ゾ- -> ゾオ, ト- -> トオ,
   ド- -> ドオ, ノ- -> ノオ, ホ- -> ホオ, ボ- -> ボオ, ポ- -> ポオ, モ- -> モオ, ョ- -> ョオ,
   ヨ- -> ヨオ, ロ- -> ロオ, ヲ- -> ヲオ, ヺ- -> ヺオ, 
   
   ン- -> ンン
   
   ぁ- -> ぁあ, あ- -> ああ, ゕ- -> ゕあ, か- -> かあ, が- -> があ, さ- -> さあ, ざ- -> ざあ, 
   た- -> たあ, だ- -> だあ, な- -> なあ, は- -> はあ, ば- -> ばあ, ぱ- -> ぱあ, ま- -> まあ, 
   ゃ- -> ゃあ, や- -> やあ, ら- -> らあ, ゎ- -> ゎあ, わ- -> わあ 
   
   ぃ- -> ぃい, い- -> いい, き- -> きい, ぎ- -> ぎい, し- -> しい, じ- -> じい, ち- -> ちい,
   ぢ- -> ぢい, に- -> にい, ひ- -> ひい, び- -> びい, ぴ- -> ぴい, み- -> みい, り- -> りい,
   ゐ- -> ゐい
   
   ぅ- -> ぅう, う- -> うう, く- -> くう, ぐ- -> ぐう, す- -> すう, ず- -> ずう, つ- -> つう,
   づ- -> づう, ぬ- -> ぬう, ふ- -> ふう, ぶ- -> ぶう, ぷ- -> ぷう, む- -> むう, ゅ- -> ゅう,
   ゆ- -> ゆう, る- -> るう, ゑ- -> ゑう, ゔ- -> ゔう
   
   ぇ- -> ぇえ, え- -> ええ, ゖ- -> ゖえ, け- -> けえ, げ- -> げえ, せ- -> せえ, ぜ- -> ぜえ,
   て- -> てえ, で- -> でえ, ね- -> ねえ, へ- -> へえ, べ- -> べえ, ぺ- -> ぺえ, め- -> めえ,
   れ- -> れえ
   
   ぉ- -> ぉお, お- -> おお, こ- -> こお, ご- -> ごお, そ- -> そお, ぞ- -> ぞお, と- -> とお,
   ど- -> どお, の- -> のお, ほ- -> ほお, ぼ- -> ぼお, ぽ- -> ぽお, も- -> もお, ょ- -> ょお,
   よ- -> よお, ろ- -> ろお, を- -> をお
   
   ん- -> んん

See also
----------

* :doc:`../commands/normalize`
