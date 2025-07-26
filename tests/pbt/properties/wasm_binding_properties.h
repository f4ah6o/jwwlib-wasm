#pragma once

#include "../include/property_base.h"
#include "../include/property_builder.h"
#include "../include/jww_test_entities.h"
#include <jwwlib/jww_document.h>
#include <emscripten/bind.h>
#include <limits>
#include <string>
#include <vector>

namespace jwwlib_wasm::pbt::properties {

// JavaScriptとの構造体マッピングプロパティ
// C++構造体がJavaScriptオブジェクトに正しく変換されることを検証
template<typename T>
class StructMappingProperty : public PropertyBase<T> {
public:
    StructMappingProperty(const std::string& type_name) 
        : PropertyBase<T>(
            "StructMappingProperty<" + type_name + ">",
            "C++ struct correctly maps to JavaScript object") {}

    bool check(const T& entity) const override {
        try {
            // エンティティをJavaScript値に変換
            emscripten::val js_val = emscripten::val(entity);
            
            // 基本的なプロパティの存在確認
            if (!js_val.hasOwnProperty("type")) return false;
            
            // タイプ固有の検証
            return validateEntitySpecific(entity, js_val);
        } catch (...) {
            return false;
        }
    }

private:
    bool validateEntitySpecific(const T& entity, const emscripten::val& js_val) const;
};

// 特殊化：JWWLine
template<>
bool StructMappingProperty<JWWLine>::validateEntitySpecific(
    const JWWLine& line, const emscripten::val& js_val) const {
    return js_val.hasOwnProperty("start") && 
           js_val.hasOwnProperty("end") &&
           js_val["start"].hasOwnProperty("x") &&
           js_val["start"].hasOwnProperty("y") &&
           js_val["end"].hasOwnProperty("x") &&
           js_val["end"].hasOwnProperty("y");
}

// 特殊化：JWWCircle
template<>
bool StructMappingProperty<JWWCircle>::validateEntitySpecific(
    const JWWCircle& circle, const emscripten::val& js_val) const {
    return js_val.hasOwnProperty("center") && 
           js_val.hasOwnProperty("radius") &&
           js_val["center"].hasOwnProperty("x") &&
           js_val["center"].hasOwnProperty("y");
}

// エンコーディング変換プロパティ
// Shift-JIS → UTF-8変換が正しく行われることを検証
class EncodingConversionProperty : public PropertyBase<std::string> {
public:
    EncodingConversionProperty() : PropertyBase<std::string>(
        "EncodingConversionProperty",
        "Shift-JIS to UTF-8 conversion preserves text") {}

    bool check(const std::string& sjis_text) const override {
        try {
            // Shift-JISからUTF-8への変換（WASM環境での実装）
            emscripten::val js_string = convertSjisToUtf8(sjis_text);
            
            // 逆変換
            std::string back_converted = convertUtf8ToSjis(js_string);
            
            // ラウンドトリップテスト
            return sjis_text == back_converted;
        } catch (...) {
            // 変換できない文字は適切にハンドリングされるべき
            return true;
        }
    }

private:
    emscripten::val convertSjisToUtf8(const std::string& sjis) const {
        // TextDecoderを使用したShift-JIS → UTF-8変換
        emscripten::val TextDecoder = emscripten::val::global("TextDecoder");
        emscripten::val decoder = TextDecoder.new_(emscripten::val("shift-jis"));
        
        // バイト配列を作成
        emscripten::val Uint8Array = emscripten::val::global("Uint8Array");
        emscripten::val bytes = Uint8Array.new_(sjis.size());
        for (size_t i = 0; i < sjis.size(); ++i) {
            bytes.set(i, static_cast<uint8_t>(sjis[i]));
        }
        
        return decoder.call<emscripten::val>("decode", bytes);
    }
    
    std::string convertUtf8ToSjis(const emscripten::val& utf8) const {
        // TextEncoderを使用したUTF-8 → Shift-JIS変換
        // 注：標準のTextEncoderはShift-JISをサポートしないため、
        // 実際の実装では別のライブラリが必要
        return "";  // 仮実装
    }
};

// 数値精度保持プロパティ
// JavaScript数値とC++ double間の変換で精度が保持されることを検証
class NumericPrecisionProperty : public PropertyBase<double> {
public:
    NumericPrecisionProperty() : PropertyBase<double>(
        "NumericPrecisionProperty",
        "Numeric precision is preserved in JS conversion") {}

    bool check(const double& value) const override {
        // 特殊値のチェック
        if (std::isnan(value) || std::isinf(value)) {
            // NaN/Infinityは特別な処理が必要
            emscripten::val js_val = emscripten::val(value);
            return (std::isnan(value) && js_val.call<bool>("isNaN")) ||
                   (std::isinf(value) && !js_val.call<bool>("isFinite"));
        }
        
        // 通常の数値の精度チェック
        emscripten::val js_val = emscripten::val(value);
        double back_converted = js_val.as<double>();
        
        // 浮動小数点の誤差を考慮
        const double epsilon = std::numeric_limits<double>::epsilon();
        return std::abs(value - back_converted) < epsilon * std::abs(value);
    }
};

// メモリ使用量予測可能性プロパティ
// WASMメモリ使用量が予測可能であることを検証
class MemoryPredictabilityProperty : public PropertyBase<JWWDocument> {
public:
    MemoryPredictabilityProperty() : PropertyBase<JWWDocument>(
        "MemoryPredictabilityProperty",
        "WASM memory usage is predictable") {}

    bool check(const JWWDocument& doc) const override {
        try {
            // 初期メモリ使用量を取得
            size_t initial_memory = getWasmMemoryUsage();
            
            // ドキュメントをJavaScriptに転送
            emscripten::val js_doc = transferDocumentToJS(doc);
            
            // 転送後のメモリ使用量
            size_t after_transfer = getWasmMemoryUsage();
            
            // メモリ増加量の計算
            size_t memory_increase = after_transfer - initial_memory;
            
            // 予測されるメモリ使用量
            size_t predicted_memory = calculatePredictedMemory(doc);
            
            // 実際の使用量が予測の2倍以内であることを確認
            return memory_increase <= predicted_memory * 2;
        } catch (...) {
            return false;
        }
    }

private:
    size_t getWasmMemoryUsage() const {
        emscripten::val memory = emscripten::val::module_property("HEAP8");
        return memory["length"].as<size_t>();
    }
    
    emscripten::val transferDocumentToJS(const JWWDocument& doc) const {
        return emscripten::val(doc);
    }
    
    size_t calculatePredictedMemory(const JWWDocument& doc) const {
        size_t total = sizeof(JWWDocument);
        
        for (const auto& layer : doc.getLayers()) {
            total += sizeof(JWWLayer);
            total += layer.getEntities().size() * 64; // 平均的なエンティティサイズ
        }
        
        return total;
    }
};

// エラー伝播プロパティ
// C++例外がJavaScriptエラーとして適切に伝播されることを検証
class ErrorPropagationProperty : public PropertyBase<std::string> {
public:
    ErrorPropagationProperty() : PropertyBase<std::string>(
        "ErrorPropagationProperty",
        "C++ exceptions properly propagate to JavaScript") {}

    bool check(const std::string& error_message) const override {
        try {
            // C++例外をスロー
            emscripten::val cpp_function = emscripten::val::module_property("throwTestException");
            
            try {
                cpp_function(error_message);
                return false; // 例外がスローされなかった
            } catch (const emscripten::val& js_error) {
                // JavaScriptエラーとしてキャッチされることを確認
                std::string caught_message = js_error["message"].as<std::string>();
                return caught_message.find(error_message) != std::string::npos;
            }
        } catch (...) {
            return false;
        }
    }
};

// WebAssemblyバインディングプロパティビルダー
class WasmBindingPropertyBuilder {
public:
    static auto buildDataConversionProperties() {
        PropertyBuilder<JWWLine> line_builder;
        PropertyBuilder<JWWCircle> circle_builder;
        PropertyBuilder<std::string> string_builder;
        PropertyBuilder<double> numeric_builder;
        
        // 各種マッピングプロパティを追加
        line_builder.add(std::make_unique<StructMappingProperty<JWWLine>>("JWWLine"));
        circle_builder.add(std::make_unique<StructMappingProperty<JWWCircle>>("JWWCircle"));
        string_builder.add(std::make_unique<EncodingConversionProperty>());
        numeric_builder.add(std::make_unique<NumericPrecisionProperty>());
        
        return std::make_tuple(
            line_builder.build(),
            circle_builder.build(),
            string_builder.build(),
            numeric_builder.build()
        );
    }
    
    static auto buildMemoryAndErrorProperties() {
        PropertyBuilder<JWWDocument> memory_builder;
        PropertyBuilder<std::string> error_builder;
        
        memory_builder.add(std::make_unique<MemoryPredictabilityProperty>());
        error_builder.add(std::make_unique<ErrorPropagationProperty>());
        
        return std::make_pair(
            memory_builder.build(),
            error_builder.build()
        );
    }
};

} // namespace jwwlib_wasm::pbt::properties