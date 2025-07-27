#!/usr/bin/env node

/**
 * Node.js Example for jwwlib-wasm
 *
 * This example demonstrates how to use jwwlib-wasm in a Node.js environment
 * to read and process JWW files.
 */

import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import jwwlib from "jwwlib-wasm";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

async function main() {
	try {
		console.log("Initializing jwwlib-wasm...");

		// Initialize the WASM module
		const Module = await jwwlib();
		console.log("✓ Module initialized");

		// Debug: Check what's available in Module
		console.log("Module type:", typeof Module);
		console.log(
			"Available methods:",
			Object.keys(Module)
				.filter((key) => typeof Module[key] === "function")
				.slice(0, 10),
		);

		// Create a document instance
		if (!Module.JWWDocumentWASM) {
			throw new Error("JWWDocumentWASM not found in module");
		}
		const doc = new Module.JWWDocumentWASM();
		console.log("✓ Document created");

		// Read a JWW file
		const jwwPath = join(__dirname, "../tests/fixtures/test-entities.jww");
		console.log(`\nReading JWW file: ${jwwPath}`);

		const buffer = readFileSync(jwwPath);
		const uint8Array = new Uint8Array(buffer);

		// Load the file
		const success = doc.readFile(uint8Array);

		if (success) {
			console.log("✓ File loaded successfully\n");

			// Display file information
			console.log("=== File Statistics ===");
			console.log(`Lines: ${doc.getLineCount()}`);
			console.log(`Circles: ${doc.getCircleCount()}`);
			console.log(`Arcs: ${doc.getArcCount()}`);
			console.log(`Points: ${doc.getPointCount()}`);
			console.log(`Texts: ${doc.getTextCount()}`);

			// Get some entity details
			console.log("\n=== First 5 Lines ===");
			const lineCount = Math.min(5, doc.getLineCount());
			for (let i = 0; i < lineCount; i++) {
				const line = doc.getLine(i);
				console.log(`Line ${i + 1}:`);
				console.log(`  From: (${line.x1.toFixed(2)}, ${line.y1.toFixed(2)})`);
				console.log(`  To: (${line.x2.toFixed(2)}, ${line.y2.toFixed(2)})`);
				console.log(`  Layer: ${line.layer || "default"}`);
			}

			// Get circle details
			console.log("\n=== First 3 Circles ===");
			const circleCount = Math.min(3, doc.getCircleCount());
			for (let i = 0; i < circleCount; i++) {
				const circle = doc.getCircle(i);
				console.log(`Circle ${i + 1}:`);
				console.log(
					`  Center: (${circle.cx.toFixed(2)}, ${circle.cy.toFixed(2)})`,
				);
				console.log(`  Radius: ${circle.radius.toFixed(2)}`);
				console.log(`  Layer: ${circle.layer || "default"}`);
			}

			// Get text entities
			console.log("\n=== Text Entities ===");
			const textCount = Math.min(5, doc.getTextCount());
			for (let i = 0; i < textCount; i++) {
				const text = doc.getText(i);
				console.log(`Text ${i + 1}: "${text.text}"`);
				console.log(`  Position: (${text.x.toFixed(2)}, ${text.y.toFixed(2)})`);
				console.log(`  Height: ${text.height.toFixed(2)}`);
			}

			// Memory usage
			if (doc.getMemoryUsage) {
				const memUsage = doc.getMemoryUsage();
				console.log(`\n=== Memory Usage ===`);
				console.log(`${(memUsage / 1024 / 1024).toFixed(2)} MB`);
			}
		} else {
			console.error("✗ Failed to load JWW file");

			// Check for parsing errors
			if (doc.getParsingErrors) {
				const errors = doc.getParsingErrors();
				const errorCount = errors.size();

				if (errorCount > 0) {
					console.error("\n=== Parsing Errors ===");
					for (let i = 0; i < errorCount; i++) {
						const error = errors.get(i);
						console.error(`Error ${i + 1}: ${error.message}`);
					}
				}
			}
		}

		// Clean up
		doc.delete();
		console.log("\n✓ Document cleaned up");
	} catch (error) {
		console.error("Error:", error.message);
		process.exit(1);
	}
}

// Run the example
main().catch(console.error);
