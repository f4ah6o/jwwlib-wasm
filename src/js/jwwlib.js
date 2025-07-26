// ES6 module wrapper for jwwlib-wasm

let moduleInstance = null;

export async function init() {
	if (moduleInstance) {
		return moduleInstance;
	}

	// Load the Emscripten module
	const module = await import("../../wasm/jwwlib.js");
	const createJWWModule = module.default || module;
	moduleInstance = await createJWWModule();

	return moduleInstance;
}

export class JWWReader {
	constructor(buffer) {
		if (!moduleInstance) {
			throw new Error("Module not initialized. Call init() first.");
		}

		// Allocate memory and copy buffer
		const dataPtr = moduleInstance._malloc(buffer.byteLength);
		moduleInstance.HEAPU8.set(new Uint8Array(buffer), dataPtr);

		// Create reader instance
		this.reader = new moduleInstance.JWWReader(dataPtr, buffer.byteLength);
		this.dataPtr = dataPtr;
	}

	getEntities() {
		return this.reader.getEntities();
	}

	getLines() {
		return this.reader.getLines();
	}

	getCircles() {
		return this.reader.getCircles();
	}

	getArcs() {
		return this.reader.getArcs();
	}

	getTexts() {
		return this.reader.getTexts();
	}

	getEllipses() {
		return this.reader.getEllipses();
	}

	getHeader() {
		return this.reader.getHeader();
	}

	dispose() {
		if (this.reader) {
			this.reader.delete();
			moduleInstance._free(this.dataPtr);
			this.reader = null;
			this.dataPtr = null;
		}
	}
}

// Re-export the module creation function for compatibility
// Re-export handled differently due to CommonJS/ESM compatibility
