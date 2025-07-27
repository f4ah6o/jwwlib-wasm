#!/usr/bin/env node

import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import semver from "semver";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const packageJson = JSON.parse(
	readFileSync(join(__dirname, "..", "package.json"), "utf8"),
);

const requiredVersion = packageJson.engines.node;
const currentVersion = process.version;

if (!semver.satisfies(currentVersion, requiredVersion)) {
	console.error(
		`\x1b[31mError: jwwlib-wasm requires Node.js version ${requiredVersion}\x1b[0m`,
	);
	console.error(`\x1b[31mCurrent version: ${currentVersion}\x1b[0m`);
	console.error("\x1b[33mPlease upgrade your Node.js version.\x1b[0m");
	process.exit(1);
}
