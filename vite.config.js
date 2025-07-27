import { defineConfig } from "vite";

export default defineConfig({
	root: ".",
	build: {
		lib: {
			entry: "src/js/jwwlib.js",
			name: "JWWLibWASM",
			fileName: "jwwlib",
			formats: ["es", "cjs"],
		},
		outDir: "dist",
		emptyOutDir: false,
		rollupOptions: {
			external: ["fs", "path", "url", "node:fs", "node:path", "node:url", "node:module"],
			output: {
				exports: "named",
				assetFileNames: (assetInfo) => {
					if (assetInfo.name === "jwwlib.wasm") {
						return "jwwlib.wasm";
					}
					return assetInfo.name;
				},
			},
		},
	},
	server: {
		port: 3000,
		headers: {
			"Cross-Origin-Embedder-Policy": "require-corp",
			"Cross-Origin-Opener-Policy": "same-origin",
		},
		fs: {
			allow: [".."],
		},
	},
	optimizeDeps: {
		exclude: ["jwwlib"],
	},
	assetsInclude: ["**/*.wasm"],
});
