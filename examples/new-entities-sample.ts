/**
 * Sample code for using new entity types in jwwlib-wasm
 * This file demonstrates TypeScript usage with the new entity types
 */

import type {
	BlockData,
	DimensionData,
	HatchData,
	ImageData,
	InsertData,
	JWWModule,
	JWWReader,
	LeaderData,
	ParseError,
} from "../src/types/jww";

// Initialize the module
async function initializeJWWModule(): Promise<JWWModule> {
	// @ts-ignore - createJWWModule is defined by Emscripten
	const Module = await createJWWModule();
	return Module;
}

// Example 1: Basic file loading with new entities
async function loadJWWFile(file: File): Promise<JWWReader> {
	const Module = await initializeJWWModule();

	// Read file as ArrayBuffer
	const arrayBuffer = await file.arrayBuffer();
	const fileData = new Uint8Array(arrayBuffer);

	// Allocate WASM memory
	const dataPtr = Module._malloc(fileData.length);
	if (!dataPtr) {
		throw new Error("Failed to allocate memory");
	}

	try {
		// Copy data to WASM memory
		Module.HEAPU8.set(fileData, dataPtr);

		// Create reader with file size hint for optimization
		const reader = new Module.JWWReader(dataPtr, fileData.length);

		return reader;
	} finally {
		// Always free allocated memory
		Module._free(dataPtr);
	}
}

// Example 2: Working with blocks and inserts
async function processBlocksAndInserts(reader: JWWReader): Promise<void> {
	// Get all blocks
	const blocks = reader.getBlocks();
	console.log(`Found ${blocks.size()} block definitions`);

	// Build block index for quick lookup
	const blockIndex = new Map<string, BlockData>();
	for (let i = 0; i < blocks.size(); i++) {
		const block = blocks.get(i);
		blockIndex.set(block.name, block);

		console.log(`Block "${block.name}":`);
		console.log(
			`  Base point: (${block.baseX}, ${block.baseY}, ${block.baseZ})`,
		);
		if (block.description) {
			console.log(`  Description: ${block.description}`);
		}
	}

	// Process inserts (block references)
	const inserts = reader.getInserts();
	console.log(`\nFound ${inserts.size()} block inserts`);

	for (let i = 0; i < inserts.size(); i++) {
		const insert = inserts.get(i);

		// Check if block exists
		const blockDef = blockIndex.get(insert.blockName);
		if (!blockDef) {
			console.warn(`Insert references undefined block: ${insert.blockName}`);
			continue;
		}

		console.log(`Insert of "${insert.blockName}":`);
		console.log(`  Position: (${insert.ipx}, ${insert.ipy}, ${insert.ipz})`);
		console.log(`  Scale: (${insert.sx}, ${insert.sy}, ${insert.sz})`);
		console.log(`  Rotation: ${(insert.angle * 180) / Math.PI}°`);

		// Handle array inserts
		if (insert.cols > 1 || insert.rows > 1) {
			console.log(`  Array: ${insert.cols} x ${insert.rows}`);
			console.log(`  Spacing: ${insert.colSpacing} x ${insert.rowSpacing}`);
		}
	}
}

// Example 3: Processing hatches with complex boundaries
async function processHatches(reader: JWWReader): Promise<void> {
	const hatches = reader.getHatches();
	console.log(`Found ${hatches.size()} hatch entities`);

	for (let i = 0; i < hatches.size(); i++) {
		const hatch = hatches.get(i);

		console.log(`\nHatch ${i}:`);
		console.log(`  Pattern: ${hatch.patternName}`);
		console.log(`  Type: ${hatch.solid ? "Solid fill" : "Pattern fill"}`);

		if (!hatch.solid) {
			console.log(`  Angle: ${(hatch.angle * 180) / Math.PI}°`);
			console.log(`  Scale: ${hatch.scale}`);
		}

		// Process boundary loops
		console.log(`  Boundary loops: ${hatch.loops.size()}`);

		for (let j = 0; j < hatch.loops.size(); j++) {
			const loop = hatch.loops.get(j);
			console.log(`    Loop ${j}: ${loop.edges.size()} edges`);

			// Analyze edge types
			const edgeTypes = { line: 0, arc: 0, ellipse: 0, spline: 0 };

			for (let k = 0; k < loop.edges.size(); k++) {
				const edge = loop.edges.get(k);
				switch (edge.type) {
					case 1:
						edgeTypes.line++;
						break;
					case 2:
						edgeTypes.arc++;
						break;
					case 3:
						edgeTypes.ellipse++;
						break;
					case 4:
						edgeTypes.spline++;
						break;
				}
			}

			console.log(`      Edge types:`, edgeTypes);
			console.log(`      Direction: ${loop.isCCW ? "CCW" : "CW"}`);
		}
	}
}

// Example 4: Working with extended dimension types
async function processDimensions(reader: JWWReader): Promise<void> {
	const dimensions = reader.getDimensions();
	const dimTypeNames = [
		"LINEAR",
		"ALIGNED",
		"RADIAL",
		"DIAMETRIC",
		"ANGULAR",
		"ANGULAR3P",
		"ORDINATE",
	];

	// Count dimensions by type
	const typeCounts = new Map<string, number>();

	for (let i = 0; i < dimensions.size(); i++) {
		const dim = dimensions.get(i);
		const typeName = dimTypeNames[dim.type] || "UNKNOWN";

		typeCounts.set(typeName, (typeCounts.get(typeName) || 0) + 1);

		// Process specific dimension types
		switch (dim.type) {
			case 2: // RADIAL
				console.log(`Radial dimension: ${dim.text}`);
				console.log(`  Center: (${dim.dpx}, ${dim.dpy})`);
				console.log(`  Text at: (${dim.mpx}, ${dim.mpy})`);
				break;

			case 3: // DIAMETRIC
				console.log(`Diametric dimension: ${dim.text}`);
				console.log(`  Center: (${dim.dpx}, ${dim.dpy})`);
				break;

			case 4: // ANGULAR
				console.log(`Angular dimension: ${dim.text}`);
				console.log(
					`  Line 1: (${dim.dpx1}, ${dim.dpy1}) to (${dim.dpx2}, ${dim.dpy2})`,
				);
				// Additional points would be in separate fields
				break;

			case 5: // ANGULAR3P
				console.log(`3-point angular dimension: ${dim.text}`);
				console.log(
					`  Points: (${dim.dpx1}, ${dim.dpy1}), (${dim.dpx2}, ${dim.dpy2})`,
				);
				break;

			case 6: // ORDINATE
				console.log(`Ordinate dimension: ${dim.text}`);
				console.log(`  Feature: (${dim.dpx1}, ${dim.dpy1})`);
				console.log(`  Leader end: (${dim.dpx2}, ${dim.dpy2})`);
				break;
		}
	}

	console.log("\nDimension summary:");
	typeCounts.forEach((count, type) => {
		console.log(`  ${type}: ${count}`);
	});
}

// Example 5: Processing leaders with annotations
async function processLeaders(reader: JWWReader): Promise<void> {
	const leaders = reader.getLeaders();
	console.log(`Found ${leaders.size()} leader entities`);

	for (let i = 0; i < leaders.size(); i++) {
		const leader = leaders.get(i);

		console.log(`\nLeader ${i}:`);
		console.log(`  Arrow head style: ${leader.arrowHeadFlag}`);
		console.log(
			`  Path type: ${leader.pathType === 0 ? "Straight" : "Spline"}`,
		);
		console.log(`  Arrow size: ${leader.arrowHeadSize}`);

		// Process vertices
		const vertices = leader.vertices;
		console.log(`  Path points: ${vertices.size()}`);

		if (vertices.size() > 0) {
			// First vertex is typically the arrow point
			const first = vertices.get(0);
			console.log(`    Start: (${first.x}, ${first.y})`);

			// Last vertex connects to annotation
			const last = vertices.get(vertices.size() - 1);
			console.log(`    End: (${last.x}, ${last.y})`);

			// Calculate total length
			let totalLength = 0;
			for (let j = 1; j < vertices.size(); j++) {
				const v1 = vertices.get(j - 1);
				const v2 = vertices.get(j);
				const dx = v2.x - v1.x;
				const dy = v2.y - v1.y;
				totalLength += Math.sqrt(dx * dx + dy * dy);
			}
			console.log(`    Total length: ${totalLength.toFixed(2)}`);
		}

		if (leader.annotationReference) {
			console.log(`  Annotation ref: ${leader.annotationReference}`);
		}
	}
}

// Example 6: Processing images
async function processImages(reader: JWWReader): Promise<void> {
	const images = reader.getImages();
	const imageDefs = reader.getImageDefs();

	console.log(
		`Found ${images.size()} images with ${imageDefs.size()} definitions`,
	);

	// Build image definition index
	const imageDefIndex = new Map<string, any>();
	for (let i = 0; i < imageDefs.size(); i++) {
		const def = imageDefs.get(i);
		imageDefIndex.set(def.fileName, def);

		console.log(`\nImage definition "${def.fileName}":`);
		console.log(`  Size: ${def.sizeX} x ${def.sizeY} pixels`);
		if (def.imageData) {
			console.log(`  Embedded data: ${def.imageData.length} bytes (base64)`);
		}
	}

	// Process image instances
	for (let i = 0; i < images.size(); i++) {
		const image = images.get(i);

		console.log(`\nImage ${i}:`);
		console.log(`  Definition: ${image.imageDefHandle}`);
		console.log(`  Position: (${image.ipx}, ${image.ipy}, ${image.ipz})`);

		// Calculate display size from vectors
		const uLength = Math.sqrt(
			image.ux * image.ux + image.uy * image.uy + image.uz * image.uz,
		);
		const vLength = Math.sqrt(
			image.vx * image.vx + image.vy * image.vy + image.vz * image.vz,
		);
		console.log(
			`  Display size: ${uLength.toFixed(2)} x ${vLength.toFixed(2)}`,
		);

		// Image adjustments
		console.log(`  Brightness: ${image.brightness}`);
		console.log(`  Contrast: ${image.contrast}`);
		console.log(`  Fade: ${image.fade}`);

		// Clipping boundary
		if (image.clippingState && image.clippingVertices.size() > 0) {
			console.log(
				`  Clipping boundary: ${image.clippingVertices.size()} vertices`,
			);
		}
	}
}

// Example 7: Error handling and recovery
async function processWithErrorHandling(file: File): Promise<void> {
	let reader: JWWReader | null = null;

	try {
		reader = await loadJWWFile(file);

		// Check for parsing errors
		const errors = reader.getParsingErrors();
		if (errors.size() > 0) {
			console.warn(`File loaded with ${errors.size()} errors:`);

			const errorTypes = [
				"NONE",
				"INVALID_BLOCK_REFERENCE",
				"INVALID_IMAGE_REFERENCE",
				"INVALID_HATCH_BOUNDARY",
				"INVALID_LEADER_PATH",
				"INVALID_DIMENSION_DATA",
				"MEMORY_ALLOCATION_FAILED",
				"UNKNOWN_ENTITY_TYPE",
			];

			for (let i = 0; i < errors.size(); i++) {
				const error = errors.get(i);
				console.warn(`  ${errorTypes[error.type]}: ${error.message}`);
				if (error.entityType) {
					console.warn(`    Entity type: ${error.entityType}`);
				}
				if (error.lineNumber > 0) {
					console.warn(`    Line: ${error.lineNumber}`);
				}
			}
		}

		// Process entities even if there were errors
		await processBlocksAndInserts(reader);
		await processHatches(reader);
		await processDimensions(reader);
		await processLeaders(reader);
		await processImages(reader);

		// Show statistics
		const stats = reader.getEntityStats();
		console.log("\nEntity statistics:");
		for (const [type, count] of Object.entries(stats)) {
			if (count > 0) {
				console.log(`  ${type}: ${count}`);
			}
		}

		// Memory usage
		const memUsage = reader.getMemoryUsage();
		console.log(`\nMemory usage: ${(memUsage / 1024 / 1024).toFixed(2)} MB`);
	} catch (error) {
		console.error("Failed to process file:", error);
		throw error;
	} finally {
		// Always clean up
		if (reader) {
			reader.delete();
		}
	}
}

// Example 8: Batch processing with progress
async function processLargeFile(file: File): Promise<void> {
	const Module = await initializeJWWModule();
	const arrayBuffer = await file.arrayBuffer();
	const fileData = new Uint8Array(arrayBuffer);

	console.log(
		`Processing ${file.name} (${(file.size / 1024 / 1024).toFixed(2)} MB)`,
	);

	// Set up progress callback
	const progressCallback = Module.addFunction(
		(current: number, total: number) => {
			const percentage = ((current / total) * 100).toFixed(1);
			console.log(`Progress: ${percentage}%`);

			// Update UI here
			// updateProgressBar(percentage);
		},
		"vii",
	);

	const dataPtr = Module._malloc(fileData.length);
	let reader: JWWReader | null = null;

	try {
		Module.HEAPU8.set(fileData, dataPtr);

		// Create reader with progress callback
		reader = new Module.JWWReader(dataPtr, fileData.length, progressCallback);

		// Process in batches for better performance
		const batchSize = 1000;
		const entities = reader.getLines();
		const totalEntities = entities.size();

		for (let i = 0; i < totalEntities; i += batchSize) {
			const end = Math.min(i + batchSize, totalEntities);

			// Process batch
			for (let j = i; j < end; j++) {
				const entity = entities.get(j);
				// Process entity...
			}

			// Yield to browser
			await new Promise((resolve) => setTimeout(resolve, 0));
		}
	} finally {
		Module._free(dataPtr);
		Module.removeFunction(progressCallback);
		if (reader) reader.delete();
	}
}

// Example 9: Export processed data
interface ProcessedData {
	blocks: Array<{ name: string; x: number; y: number }>;
	inserts: Array<{
		blockName: string;
		x: number;
		y: number;
		scale: number;
		angle: number;
	}>;
	hatches: Array<{ pattern: string; solid: boolean; loops: number }>;
	dimensions: Array<{ type: string; text: string; x: number; y: number }>;
	leaders: Array<{ vertices: number; length: number }>;
	images: Array<{
		file: string;
		x: number;
		y: number;
		width: number;
		height: number;
	}>;
}

async function exportProcessedData(reader: JWWReader): Promise<ProcessedData> {
	const data: ProcessedData = {
		blocks: [],
		inserts: [],
		hatches: [],
		dimensions: [],
		leaders: [],
		images: [],
	};

	// Export blocks
	const blocks = reader.getBlocks();
	for (let i = 0; i < blocks.size(); i++) {
		const block = blocks.get(i);
		data.blocks.push({
			name: block.name,
			x: block.baseX,
			y: block.baseY,
		});
	}

	// Export inserts
	const inserts = reader.getInserts();
	for (let i = 0; i < inserts.size(); i++) {
		const insert = inserts.get(i);
		data.inserts.push({
			blockName: insert.blockName,
			x: insert.ipx,
			y: insert.ipy,
			scale: (insert.sx + insert.sy) / 2,
			angle: (insert.angle * 180) / Math.PI,
		});
	}

	// Export other entities...
	// (Similar pattern for hatches, dimensions, leaders, images)

	return data;
}

// Export functions for use in other modules
export {
	initializeJWWModule,
	loadJWWFile,
	processBlocksAndInserts,
	processHatches,
	processDimensions,
	processLeaders,
	processImages,
	processWithErrorHandling,
	processLargeFile,
	exportProcessedData,
};
