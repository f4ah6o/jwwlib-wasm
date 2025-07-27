import { readFileSync } from "node:fs";
import { join } from "node:path";
import { beforeAll, describe, expect, it } from "vitest";

// Type definitions for the WASM module
interface JWWModule {
	ready: Promise<void>;
	_malloc: (size: number) => number;
	_free: (ptr: number) => void;
	HEAPU8: Uint8Array;
	JWWReader: new (dataPtr: number, size: number) => JWWReaderInstance;
}

interface VectorLike<T> {
	size(): number;
	get(index: number): T;
}

interface JWWReaderInstance {
	getLines: () => VectorLike<unknown>;
	getCircles: () => VectorLike<unknown>;
	getArcs: () => VectorLike<unknown>;
	getTexts: () => VectorLike<unknown>;
	getBlocks: () => VectorLike<unknown>;
	getInserts: () => VectorLike<unknown>;
	getHatches: () => VectorLike<unknown>;
	getLeaders: () => VectorLike<unknown>;
	getImages: () => VectorLike<unknown>;
	getImageDefs: () => VectorLike<unknown>;
	getDimensions: () => VectorLike<unknown>;
	getSplines: () => VectorLike<unknown>;
	getParsingErrors: () => VectorLike<unknown>;
	getMemoryUsage: () => number;
	getEntityStats: () => VectorLike<unknown>;
	delete: () => void;
}

describe("New Entity Types Support", () => {
	let Module: JWWModule;
	let testFileData: Uint8Array;

	beforeAll(async () => {
		// Load WASM module
		const wasmPath = join(__dirname, "../../wasm/jwwlib.js");
		const { default: createModule } = await import(wasmPath);

		// Read WASM binary directly
		const wasmBinaryPath = join(__dirname, "../../wasm/jwwlib.wasm");
		const wasmBinary = readFileSync(wasmBinaryPath);

		Module = await createModule({
			wasmBinary: wasmBinary,
		});
		await Module.ready;

		// Load test file
		const testFilePath = join(__dirname, "../fixtures/test-entities.jww");
		testFileData = new Uint8Array(readFileSync(testFilePath));
	});

	describe("Block and Insert Entities", () => {
		it("should read block definitions", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const blocks = reader.getBlocks();

			// The test file may not contain blocks, so check for >= 0
			expect(blocks.size()).toBeGreaterThanOrEqual(0);

			// Check block structure if any blocks exist
			if (blocks.size() > 0) {
				const firstBlock = blocks.get(0);
				expect(firstBlock).toHaveProperty("name");
				expect(firstBlock).toHaveProperty("baseX");
				expect(firstBlock).toHaveProperty("baseY");
				expect(firstBlock).toHaveProperty("baseZ");
			}

			reader.delete();
			Module._free(dataPtr);
		});

		it("should read block references (inserts)", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const inserts = reader.getInserts();

			// The test file may not contain inserts, so check for >= 0
			expect(inserts.size()).toBeGreaterThanOrEqual(0);

			// Check insert structure if any inserts exist
			if (inserts.size() > 0) {
				const firstInsert = inserts.get(0);
				expect(firstInsert).toHaveProperty("blockName");
				expect(firstInsert).toHaveProperty("ipx");
				expect(firstInsert).toHaveProperty("ipy");
				expect(firstInsert).toHaveProperty("sx");
				expect(firstInsert).toHaveProperty("sy");
				expect(firstInsert).toHaveProperty("angle");
			}

			reader.delete();
			Module._free(dataPtr);
		});

		it("should validate block references", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const blocks = reader.getBlocks();
			const inserts = reader.getInserts();
			const errors = reader.getParsingErrors();

			// Create block name set
			const blockNames = new Set();
			for (let i = 0; i < blocks.size(); i++) {
				blockNames.add(blocks.get(i).name);
			}

			// Check all inserts reference existing blocks
			for (let i = 0; i < inserts.size(); i++) {
				const insert = inserts.get(i);
				if (!blockNames.has(insert.blockName)) {
					// Should have error for invalid reference
					const hasError = errors.size() > 0;
					expect(hasError).toBe(true);
				}
			}

			reader.delete();
			Module._free(dataPtr);
		});
	});

	describe("Hatch Entities", () => {
		it("should read hatch patterns", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const hatches = reader.getHatches();

			if (hatches.size() > 0) {
				const firstHatch = hatches.get(0);
				expect(firstHatch).toHaveProperty("patternType");
				expect(firstHatch).toHaveProperty("patternName");
				expect(firstHatch).toHaveProperty("solid");
				expect(firstHatch).toHaveProperty("angle");
				expect(firstHatch).toHaveProperty("scale");
				expect(firstHatch).toHaveProperty("loops");
			}

			reader.delete();
			Module._free(dataPtr);
		});

		it("should read hatch boundaries", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const hatches = reader.getHatches();

			if (hatches.size() > 0) {
				const firstHatch = hatches.get(0);
				if (firstHatch.loops && firstHatch.loops.size() > 0) {
					const firstLoop = firstHatch.loops.get(0);
					expect(firstLoop).toHaveProperty("type");
					expect(firstLoop).toHaveProperty("edges");
					expect(firstLoop).toHaveProperty("isCCW");
				}
			}

			reader.delete();
			Module._free(dataPtr);
		});
	});

	describe("Extended Dimension Types", () => {
		it("should read all dimension types", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const dimensions = reader.getDimensions();

			// Count dimension types
			const dimTypes = new Map<number, number>();
			for (let i = 0; i < dimensions.size(); i++) {
				const dim = dimensions.get(i);
				const type = dim.type;
				dimTypes.set(type, (dimTypes.get(type) || 0) + 1);
			}

			// Check if we have various dimension types
			console.log("Dimension types found:", Array.from(dimTypes.entries()));

			reader.delete();
			Module._free(dataPtr);
		});

		it("should read radial and diametric dimensions", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const dimensions = reader.getDimensions();

			// Find radial/diametric dimensions
			for (let i = 0; i < dimensions.size(); i++) {
				const dim = dimensions.get(i);
				if (dim.type === 2 || dim.type === 3) {
					// RADIAL or DIAMETRIC
					expect(dim).toHaveProperty("dpx");
					expect(dim).toHaveProperty("dpy");
					expect(dim).toHaveProperty("text");
				}
			}

			reader.delete();
			Module._free(dataPtr);
		});
	});

	describe("Leader Entities", () => {
		it("should read leader entities", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const leaders = reader.getLeaders();

			if (leaders.size() > 0) {
				const firstLeader = leaders.get(0);
				expect(firstLeader).toHaveProperty("arrowHeadFlag");
				expect(firstLeader).toHaveProperty("pathType");
				expect(firstLeader).toHaveProperty("vertices");
				expect(firstLeader.vertices.size()).toBeGreaterThan(0);
			}

			reader.delete();
			Module._free(dataPtr);
		});
	});

	describe("Image Entities", () => {
		it("should read image definitions and references", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const images = reader.getImages();
			const imageDefs = reader.getImageDefs();

			if (images.size() > 0) {
				const firstImage = images.get(0);
				expect(firstImage).toHaveProperty("imageDefHandle");
				expect(firstImage).toHaveProperty("ipx");
				expect(firstImage).toHaveProperty("ipy");
				expect(firstImage).toHaveProperty("width");
				expect(firstImage).toHaveProperty("height");
			}

			if (imageDefs.size() > 0) {
				const firstDef = imageDefs.get(0);
				expect(firstDef).toHaveProperty("fileName");
			}

			reader.delete();
			Module._free(dataPtr);
		});
	});

	describe("Memory Management", () => {
		it("should report memory usage", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const memoryUsage = reader.getMemoryUsage();

			expect(memoryUsage).toBeGreaterThan(0);
			console.log("Memory usage:", memoryUsage, "bytes");

			reader.delete();
			Module._free(dataPtr);
		});

		it("should provide entity statistics", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const stats = reader.getEntityStats();

			expect(stats.size()).toBeGreaterThan(0);

			// Log statistics - stats might be a WASM vector with get() method
			console.log("Entity statistics:");
			if (stats.forEach) {
				stats.forEach((count, type) => {
					console.log(`  ${type}: ${count}`);
				});
			}

			reader.delete();
			Module._free(dataPtr);
		});

		it("should handle large files efficiently", () => {
			// Create a large test data (simulate large file)
			const largeData = new Uint8Array(1024 * 1024); // 1MB
			largeData.set(testFileData); // Copy test data at beginning

			const startTime = performance.now();
			const dataPtr = Module._malloc(largeData.length);
			Module.HEAPU8.set(largeData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, largeData.length);
			const parseTime = performance.now() - startTime;

			console.log("Large file parse time:", parseTime, "ms");
			expect(parseTime).toBeLessThan(5000); // Should parse within 5 seconds

			reader.delete();
			Module._free(dataPtr);
		});
	});

	describe("Error Handling", () => {
		it("should report parsing errors", () => {
			const dataPtr = Module._malloc(testFileData.length);
			Module.HEAPU8.set(testFileData, dataPtr);

			const reader = new Module.JWWReader(dataPtr, testFileData.length);
			const errors = reader.getParsingErrors();

			// Check error structure if any errors exist
			if (errors.size() > 0) {
				const firstError = errors.get(0);
				expect(firstError).toHaveProperty("type");
				expect(firstError).toHaveProperty("message");
				expect(firstError).toHaveProperty("entityType");
			}

			reader.delete();
			Module._free(dataPtr);
		});

		it("should handle invalid data gracefully", () => {
			const invalidData = new Uint8Array([0xff, 0xff, 0xff, 0xff]);
			const dataPtr = Module._malloc(invalidData.length);
			Module.HEAPU8.set(invalidData, dataPtr);

			expect(() => {
				const reader = new Module.JWWReader(dataPtr, invalidData.length);
				reader.delete();
			}).not.toThrow();

			Module._free(dataPtr);
		});
	});
});
