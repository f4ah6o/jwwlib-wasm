// Text encoding utilities for JWW files

/**
 * Convert Shift-JIS bytes to UTF-8 string
 * @param {Array|Uint8Array} bytes - Shift-JIS encoded bytes
 * @returns {string} UTF-8 decoded string
 */
export function decodeShiftJIS(bytes) {
	try {
		// Convert to Uint8Array if needed
		const uint8Array =
			bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);

		// Use TextDecoder with shift-jis encoding
		const decoder = new TextDecoder("shift-jis");
		return decoder.decode(uint8Array);
	} catch (error) {
		console.warn("Failed to decode Shift-JIS text:", error);
		// Fallback: try to decode as UTF-8
		try {
			const decoder = new TextDecoder("utf-8");
			return decoder.decode(new Uint8Array(bytes));
		} catch (_utf8Error) {
			// Final fallback: return placeholder
			return "(encoding error)";
		}
	}
}

/**
 * Get decoded text from entity, with fallback
 * @param {Object} entity - Entity object with text and textBytes
 * @returns {string} Decoded text
 */
export function getEntityText(entity) {
	// If textBytes exists, try to decode it
	if (entity.textBytes) {
		// Check if it's a WASM vector (has size() method)
		if (entity.textBytes.size && typeof entity.textBytes.size === "function") {
			const bytes = [];
			for (let i = 0; i < entity.textBytes.size(); i++) {
				bytes.push(entity.textBytes.get(i));
			}
			if (bytes.length > 0) {
				return decodeShiftJIS(bytes);
			}
		}
		// Check if it's already an array
		else if (
			(Array.isArray(entity.textBytes) ||
				entity.textBytes instanceof Uint8Array) &&
			entity.textBytes.length > 0
		) {
			return decodeShiftJIS(entity.textBytes);
		}
	}

	// Otherwise use the text field (may have encoding issues)
	return entity.text || "";
}

/**
 * Process text data from JWW reader
 * @param {Object} textData - Text data from JWW reader
 * @returns {Object} Text data with decoded text
 */
export function processTextData(textData) {
	const processed = { ...textData };

	// If textBytes exists, decode it
	if (textData.textBytes?.size && textData.textBytes.size() > 0) {
		// Convert WASM vector to JavaScript array
		const bytes = [];
		for (let i = 0; i < textData.textBytes.size(); i++) {
			bytes.push(textData.textBytes.get(i));
		}
		processed.decodedText = decodeShiftJIS(bytes);
	} else {
		processed.decodedText = textData.text || "";
	}

	return processed;
}
