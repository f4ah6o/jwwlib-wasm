import fs from "node:fs";
import path from "node:path";
import { expect, test } from "@playwright/test";

test.describe("JWW File Reading", () => {
	test("should read a JWW file and get entities", async ({ page }) => {
		await page.goto("/examples/index.html");

		// Wait for module to load
		await page.waitForFunction(() => typeof createJWWModule !== "undefined", {
			timeout: 10000,
		});

		// Read test file
		const testFilePath = path.join(process.cwd(), "fixtures", "test.jww");
		const fileContent = fs.readFileSync(testFilePath);

		// Test file reading
		const result = await page.evaluate(async (fileBuffer) => {
			const JWWModule = await createJWWModule();

			// Convert to Uint8Array
			const uint8Array = new Uint8Array(fileBuffer);

			// Allocate memory and copy data
			const dataPtr = JWWModule._malloc(uint8Array.length);
			JWWModule.HEAPU8.set(uint8Array, dataPtr);

			// Create reader
			const reader = new JWWModule.JWWReader(dataPtr, uint8Array.length);

			// Get entities
			const entities = reader.getEntities();
			const entityCount = entities.size();

			// Get first entity if exists
			let firstEntity = null;
			if (entityCount > 0) {
				firstEntity = entities.get(0);
			}

			// Get header
			const header = reader.getHeader();

			// Clean up
			reader.delete();
			JWWModule._free(dataPtr);

			return {
				entityCount,
				firstEntity,
				header,
			};
		}, Array.from(fileContent));

		// Verify results
		expect(result.entityCount).toBeGreaterThan(0);
		expect(result.header).toBeTruthy();
		expect(result.header.version).toBe("JWW");
		expect(result.header.entityCount).toBe(result.entityCount);
	});

	test("should handle empty file gracefully", async ({ page }) => {
		await page.goto("/examples/index.html");

		// Wait for module to load
		await page.waitForFunction(() => typeof createJWWModule !== "undefined");

		// Test with empty buffer
		const result = await page.evaluate(async () => {
			const JWWModule = await createJWWModule();

			// Empty buffer
			const _uint8Array = new Uint8Array(0);

			// Allocate memory
			const dataPtr = JWWModule._malloc(1); // Allocate at least 1 byte

			try {
				// Create reader
				const reader = new JWWModule.JWWReader(dataPtr, 0);

				// Get entities
				const entities = reader.getEntities();
				const entityCount = entities.size();

				// Clean up
				reader.delete();
				JWWModule._free(dataPtr);

				return {
					success: true,
					entityCount,
				};
			} catch (error) {
				JWWModule._free(dataPtr);
				return {
					success: false,
					error: error.message,
				};
			}
		});

		// Should handle empty file without crashing
		expect(result.success).toBe(true);
		expect(result.entityCount).toBe(0);
	});
});
