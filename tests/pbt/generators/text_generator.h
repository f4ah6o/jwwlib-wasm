#ifndef PBT_TEXT_GENERATOR_H
#define PBT_TEXT_GENERATOR_H

#include <rapidcheck.h>
#include <string>
#include <vector>
#include <iconv.h>
#include <memory>

namespace jwwlib_pbt {
namespace generators {

/**
 * @brief Text generation utilities for JWW
 */
class TextGenerator {
private:
    /**
     * @brief Convert UTF-8 string to Shift-JIS
     */
    static std::string toShiftJIS(const std::string& utf8str) {
        if (utf8str.empty()) return "";
        
        iconv_t cd = iconv_open("SHIFT-JIS", "UTF-8");
        if (cd == (iconv_t)-1) {
            throw std::runtime_error("Failed to create iconv converter");
        }
        
        size_t in_size = utf8str.size();
        size_t out_size = in_size * 2; // Shift-JIS can be up to 2 bytes per character
        
        std::vector<char> out_buf(out_size);
        char* in_ptr = const_cast<char*>(utf8str.c_str());
        char* out_ptr = out_buf.data();
        
        size_t in_left = in_size;
        size_t out_left = out_size;
        
        size_t result = iconv(cd, &in_ptr, &in_left, &out_ptr, &out_left);
        iconv_close(cd);
        
        if (result == (size_t)-1) {
            throw std::runtime_error("Failed to convert to Shift-JIS");
        }
        
        return std::string(out_buf.data(), out_size - out_left);
    }
    
public:
    /**
     * @brief Generate Shift-JIS compatible Japanese text
     */
    static rc::Gen<std::string> genShiftJISText() {
        // Common Japanese characters for CAD drawings (all are Shift-JIS compatible)
        static const std::vector<std::string> commonChars = {
            // Hiragana
            "あ", "い", "う", "え", "お", "か", "き", "く", "け", "こ",
            "さ", "し", "す", "せ", "そ", "た", "ち", "つ", "て", "と",
            "な", "に", "ぬ", "ね", "の", "は", "ひ", "ふ", "へ", "ほ",
            "ま", "み", "む", "め", "も", "や", "ゆ", "よ",
            "ら", "り", "る", "れ", "ろ", "わ", "を", "ん",
            
            // Katakana
            "ア", "イ", "ウ", "エ", "オ", "カ", "キ", "ク", "ケ", "コ",
            "サ", "シ", "ス", "セ", "ソ", "タ", "チ", "ツ", "テ", "ト",
            "ナ", "ニ", "ヌ", "ネ", "ノ", "ハ", "ヒ", "フ", "ヘ", "ホ",
            "マ", "ミ", "ム", "メ", "モ", "ヤ", "ユ", "ヨ",
            "ラ", "リ", "ル", "レ", "ロ", "ワ", "ヲ", "ン",
            
            // Common Kanji for architectural drawings
            "建", "築", "図", "面", "寸", "法", "平", "立", "断",
            "基", "礎", "壁", "柱", "梁", "屋", "根", "窓", "扉",
            "階", "地", "下", "上", "北", "南", "東", "西", "中", "央",
            "内", "外", "左", "右", "前", "後", "高", "低", "大", "小",
            
            // Numbers and units
            "一", "二", "三", "四", "五", "六", "七", "八", "九", "十",
            "百", "千", "万", "円", "年", "月", "日", "時", "分", "秒",
            
            // Full-width alphanumeric
            "０", "１", "２", "３", "４", "５", "６", "７", "８", "９",
            "Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ", "Ｈ", "Ｉ", "Ｊ",
            "Ｋ", "Ｌ", "Ｍ", "Ｎ", "Ｏ", "Ｐ", "Ｑ", "Ｒ", "Ｓ", "Ｔ",
            "Ｕ", "Ｖ", "Ｗ", "Ｘ", "Ｙ", "Ｚ"
        };
        
        return rc::gen::apply(
            [](const std::vector<std::string>& chars) {
                std::string result;
                for (const auto& ch : chars) {
                    result += ch;
                }
                try {
                    return toShiftJIS(result);
                } catch (...) {
                    // Fallback to empty string if conversion fails
                    return std::string("");
                }
            },
            rc::gen::container<std::vector<std::string>>(
                rc::gen::element(commonChars),
                rc::gen::inRange(size_t(1), size_t(20))
            )
        );
    }
    
    /**
     * @brief Generate architectural term in Shift-JIS
     */
    static rc::Gen<std::string> genArchitecturalTerm() {
        static const std::vector<std::string> terms = {
            "建築図面", "平面図", "立面図", "断面図", "詳細図",
            "基礎伏図", "床伏図", "天井伏図", "屋根伏図", "構造図",
            "配筋図", "設備図", "電気設備", "給排水設備", "空調設備",
            "外構図", "仕上表", "建具表", "展開図", "矩計図",
            "１階平面図", "２階平面図", "３階平面図", "地下１階", "屋上階",
            "ＧＬ＋０", "ＦＬ＋０", "天井高２４００", "階高３０００",
            "鉄筋コンクリート造", "鉄骨造", "木造", "混構造",
            "耐火建築物", "準耐火建築物", "防火地域", "準防火地域"
        };
        
        return rc::gen::apply(
            [](const std::string& term) {
                try {
                    return toShiftJIS(term);
                } catch (...) {
                    return std::string("TEXT");
                }
            },
            rc::gen::element(terms)
        );
    }
    
    /**
     * @brief Generate dimension text in Shift-JIS
     */
    static rc::Gen<std::string> genDimensionText() {
        return rc::gen::apply(
            [](int value, const std::string& unit) {
                std::string text = std::to_string(value) + unit;
                try {
                    return toShiftJIS(text);
                } catch (...) {
                    return std::to_string(value);
                }
            },
            rc::gen::inRange(1, 99999),
            rc::gen::element<std::string>({"mm", "m", "㎜", "ｍ"})
        );
    }
    
    /**
     * @brief Generate mixed ASCII and Japanese text
     */
    static rc::Gen<std::string> genMixedShiftJISText() {
        return rc::gen::oneOf(
            // Pure ASCII
            rc::gen::container<std::string>(
                rc::gen::inRange('A', 'Z'),
                rc::gen::inRange(size_t(1), size_t(32))
            ),
            // Pure Japanese
            genShiftJISText(),
            // Architectural terms
            genArchitecturalTerm(),
            // Dimensions
            genDimensionText(),
            // Mixed
            rc::gen::apply(
                [](const std::string& ascii, const std::string& japanese) {
                    return ascii + "_" + japanese;
                },
                rc::gen::container<std::string>(
                    rc::gen::inRange('A', 'Z'),
                    rc::gen::inRange(size_t(1), size_t(10))
                ),
                genArchitecturalTerm()
            )
        );
    }
};

} // namespace generators
} // namespace jwwlib_pbt

#endif // PBT_TEXT_GENERATOR_H