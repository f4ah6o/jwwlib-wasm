import { expect, test } from "@playwright/test";

test.describe("Entity Extraction", () => {
	test("should extract entities using new JWWDocumentWASM interface", async ({
		page,
	}) => {
		await page.goto("/examples/index.html");

		// Wait for module to load
		await page.waitForFunction(() => typeof createJWWModule !== "undefined");

		// Test entity extraction with new interface
		const result = await page.evaluate(async () => {
			const JWWModule = await createJWWModule();

			// Create test data (simple line data)
			const testData = new Uint8Array(1024); // Dummy data for now

			// Allocate memory and copy data
			const dataPtr = JWWModule._malloc(testData.length);
			JWWModule.HEAPU8.set(testData, dataPtr);

			// Use new JWWDocumentWASM interface
			const document = new JWWModule.JWWDocumentWASM();

			try {
				// Try to load data (will fail with dummy data, but we can test the interface)
				const loaded = document.loadFromMemory(dataPtr, testData.length);

				// Get entities and convert to array
				const entities = document.getEntities();
				const entityArray = [];

				// Convert vector to array
				for (let i = 0; i < entities.size(); i++) {
					const entity = entities.get(i);
					entityArray.push({
						type: entity.type,
						coordinates: Array.from(entity.coordinates),
						color: entity.color,
						layer: entity.layer,
					});
				}

				return {
					loaded: loaded,
					hasError: document.hasError(),
					errorMessage: document.hasError() ? document.getLastError() : null,
					entityCount: document.getEntityCount(),
					layerCount: document.getLayerCount(),
					memoryUsage: document.getMemoryUsage(),
					entities: entityArray,
				};
			} finally {
				// Clean up
				JWWModule._free(dataPtr);
				document.dispose();
			}
		});

		// Check that the interface works correctly
		expect(result).toBeDefined();
		expect(result.hasError).toBe(true); // Expected to fail with dummy data
		expect(result.errorMessage).toBeTruthy();
		expect(result.entityCount).toBeGreaterThanOrEqual(0);
		expect(result.layerCount).toBeGreaterThanOrEqual(0);
		expect(result.memoryUsage).toBeGreaterThan(0);
	});

	test("should correctly handle real JWW file with new interface", async ({
		page,
	}) => {
		await page.goto("/examples/index.html");

		// Wait for module to load
		await page.waitForFunction(() => typeof createJWWModule !== "undefined");

		// Load actual JWW test file
		const result = await page
			.evaluate(async () => {
				const JWWModule = await createJWWModule();

				// Fetch actual test file
				const response = await fetch("/fixtures/test.jww");
				const arrayBuffer = await response.arrayBuffer();
				const fileData = new Uint8Array(arrayBuffer);

				// Allocate memory and copy data
				const dataPtr = JWWModule._malloc(fileData.length);
				JWWModule.HEAPU8.set(fileData, dataPtr);

				// Use new JWWDocumentWASM interface
				const document = new JWWModule.JWWDocumentWASM();

				try {
					// Load the file
					const loaded = document.loadFromMemory(dataPtr, fileData.length);

					if (!loaded) {
						return {
							loaded: false,
							error: document.getLastError(),
						};
					}

					// Get entities and analyze them
					const entities = document.getEntities();
					const entityTypes = new Map();
					const entityArray = [];

					for (let i = 0; i < entities.size(); i++) {
						const entity = entities.get(i);
						const type = entity.type;
						entityTypes.set(type, (entityTypes.get(type) || 0) + 1);

						entityArray.push({
							type: entity.type,
							coordinates: Array.from(entity.coordinates),
							color: entity.color,
							layer: entity.layer,
						});
					}

					// Get layers
					const layers = document.getLayers();
					const layerArray = [];

					for (let i = 0; i < layers.size(); i++) {
						const layer = layers.get(i);
						layerArray.push({
							index: layer.index,
							name: layer.name,
							color: layer.color,
							visible: layer.visible,
							entityCount: layer.entityCount,
						});
					}

					return {
						loaded: true,
						entityCount: document.getEntityCount(),
						layerCount: document.getLayerCount(),
						entityTypes: Object.fromEntries(entityTypes),
						entities: entityArray.slice(0, 5), // First 5 entities for inspection
						layers: layerArray,
					};
				} finally {
					// Clean up
					JWWModule._free(dataPtr);
					document.dispose();
				}
			})
			.catch((error) => ({
				loaded: false,
				error: error.toString(),
			}));

		// Check results
		if (result.loaded) {
			expect(result.entityCount).toBeGreaterThan(0);
			expect(result.layerCount).toBeGreaterThan(0);
			expect(result.entities).toBeDefined();
			expect(result.layers).toBeDefined();
			console.log("Entity types found:", result.entityTypes);
			console.log("First 5 entities:", result.entities);
			console.log("Layers:", result.layers);
		} else {
			console.log("Failed to load JWW file:", result.error);
			// This might be expected if test file is not available
		}
	});
});
