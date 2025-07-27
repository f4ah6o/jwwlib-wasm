#!/usr/bin/env node
/**
 * Simple test to debug jwwlib-wasm module loading
 */

import { createRequire } from "node:module";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Test 1: Check WASM module directly
console.log("=== Test 1: Direct require ===");
try {
	const require = createRequire(import.meta.url);
	const wasmModule = require(join(__dirname, "../wasm/jwwlib.js"));
	console.log("WASM module type:", typeof wasmModule);
	console.log("WASM module keys:", Object.keys(wasmModule));
	console.log("WASM module.default:", typeof wasmModule.default);
	console.log("WASM module itself callable?", typeof wasmModule === "function");

	// Check if it's the module itself
	if (typeof wasmModule === "function") {
		console.log("\nTrying to call wasmModule directly...");
		const instance = await wasmModule();
		console.log("Instance type:", typeof instance);
		console.log(
			"Instance keys (first 10):",
			Object.keys(instance).slice(0, 10),
		);
		console.log("Has JWWDocumentWASM?", "JWWDocumentWASM" in instance);
	}
} catch (e) {
	console.error("Error in Test 1:", e.message);
}

// Test 2: Check src/js/jwwlib.js module
console.log("\n=== Test 2: Import wrapper module ===");
try {
	const jwwlib = await import("../src/js/jwwlib.js");
	console.log("jwwlib module keys:", Object.keys(jwwlib));
	console.log("jwwlib.default type:", typeof jwwlib.default);

	if (typeof jwwlib.default === "function") {
		console.log("\nTrying to call jwwlib.default()...");
		const Module = await jwwlib.default();
		console.log("Module type:", typeof Module);
		if (Module) {
			console.log("Module keys (first 10):", Object.keys(Module).slice(0, 10));
			console.log("Has JWWDocumentWASM?", "JWWDocumentWASM" in Module);
		}
	}
} catch (e) {
	console.error("Error in Test 2:", e.message);
	console.error("Stack:", e.stack);
}

// Test 3: Check package.json exports
console.log("\n=== Test 3: Package exports ===");
try {
	const jwwlib = await import("jwwlib-wasm");
	console.log("Package import keys:", Object.keys(jwwlib));
	console.log("Package default type:", typeof jwwlib.default);
} catch (e) {
	console.error("Error in Test 3:", e.message);
}
