// ES6 module wrapper for jwwlib-wasm
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

let moduleInstance = null;

async function init() {
	if (moduleInstance) {
		return moduleInstance;
	}

	// Node.js environment - use require with CommonJS wrapper
	if (typeof process !== 'undefined' && process.versions && process.versions.node) {
		const __filename = fileURLToPath(import.meta.url);
		const __dirname = dirname(__filename);
		const require = createRequire(import.meta.url);
		const createJWWModule = require(join(__dirname, '../../wasm/jwwlib.cjs'));
		
		// Read WASM file for Node.js
		const fs = await import('node:fs');
		const wasmPath = join(__dirname, '../../wasm/jwwlib.wasm');
		const wasmBinary = fs.readFileSync(wasmPath);
		
		moduleInstance = await createJWWModule({
			wasmBinary: wasmBinary
		});
		return moduleInstance;
	}
	
	// Browser/bundler environment - use dynamic import
	try {
		const module = await import("../../wasm/jwwlib.js");
		const createJWWModule = module.default || module.createJWWModule || module;
		if (typeof createJWWModule === 'function') {
			moduleInstance = await createJWWModule();
			return moduleInstance;
		}
	} catch (e) {
		throw new Error(`Failed to load WASM module: ${e.message}`);
	}
}

// Default export that initializes and returns the module
export default init;

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
