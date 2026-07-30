from inkcards import pinyin


def test_numbered_to_diacritics_basic():
    assert pinyin.numbered_to_diacritics("ni3 hao3") == "nǐ hǎo"
    assert pinyin.numbered_to_diacritics("wo3 men5") == "wǒ men"
    assert pinyin.numbered_to_diacritics("zhong1 wen2") == "zhōng wén"


def test_tone_placement_rules():
    # 'a' and 'e' always take the mark.
    assert pinyin.numbered_to_diacritics("hao3") == "hǎo"
    assert pinyin.numbered_to_diacritics("xie4") == "xiè"
    # 'ou' marks the o.
    assert pinyin.numbered_to_diacritics("dou1") == "dōu"
    # otherwise the last vowel.
    assert pinyin.numbered_to_diacritics("gui4") == "guì"
    assert pinyin.numbered_to_diacritics("shui3") == "shuǐ"


def test_v_becomes_u_umlaut():
    assert pinyin.numbered_to_diacritics("nv3") == "nǚ"
    assert pinyin.numbered_to_diacritics("lv4") == "lǜ"


def test_neutral_tone_and_passthrough():
    # Tone 5 (neutral) drops the digit without adding a mark.
    assert pinyin.numbered_to_diacritics("de5") == "de"
    # Already-diacritic text is unchanged.
    assert pinyin.numbered_to_diacritics("nǐ hǎo") == "nǐ hǎo"
    # Plain text with no tone digits is unchanged.
    assert pinyin.numbered_to_diacritics("hello") == "hello"


def test_is_numbered():
    assert pinyin.is_numbered("ni3")
    assert not pinyin.is_numbered("nǐ")
    assert not pinyin.is_numbered("hello")


def test_normalise_trims():
    assert pinyin.normalise("  ni3 hao3  ") == "nǐ hǎo"
