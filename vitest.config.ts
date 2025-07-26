import { defineConfig } from "vitest/config";

export default defineConfig({
	test: {
		globals: true,
		environment: "node",
		setupFiles: ["./tests/setup.ts"],
		testTimeout: 30000,
		include: ["tests/**/*.test.ts"],
		coverage: {
			reporter: ["text", "json", "html"],
			exclude: [
				"node_modules/",
				"tests/",
				"examples/",
				"dist/",
				"build/",
				"build-release/",
				"scripts/",
			],
		},
	},
});
