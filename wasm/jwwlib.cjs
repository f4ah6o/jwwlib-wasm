// CommonJS wrapper for jwwlib.js
// This file ensures proper CommonJS module export

// Load the original jwwlib.js file
const fs = require('fs');
const path = require('path');
const vm = require('vm');

// Read the WASM module loader
const code = fs.readFileSync(path.join(__dirname, 'jwwlib.js'), 'utf8');

// Create a sandbox for execution
const sandbox = {
  module: { exports: {} },
  exports: {},
  require: require,
  __dirname: __dirname,
  __filename: __filename,
  console: console,
  process: process,
  global: global,
  Buffer: Buffer,
  setTimeout: setTimeout,
  setInterval: setInterval,
  clearTimeout: clearTimeout,
  clearInterval: clearInterval,
  setImmediate: setImmediate,
  clearImmediate: clearImmediate,
  URL: URL,
  XMLHttpRequest: typeof XMLHttpRequest !== 'undefined' ? XMLHttpRequest : undefined,
  fetch: typeof fetch !== 'undefined' ? fetch : undefined,
  self: typeof self !== 'undefined' ? self : undefined,
  window: typeof window !== 'undefined' ? window : undefined,
  document: typeof document !== 'undefined' ? document : undefined,
  location: typeof location !== 'undefined' ? location : undefined,
  WorkerGlobalScope: typeof WorkerGlobalScope !== 'undefined' ? WorkerGlobalScope : undefined
};

// Create context and run the script
vm.createContext(sandbox);
const script = new vm.Script(code);
script.runInContext(sandbox);

// Export the createJWWModule function
const createJWWModule = sandbox.module.exports;

// Wrap the function to provide WASM binary for Node.js
module.exports = function(moduleArg = {}) {
  // If no wasmBinary is provided and we're in Node.js, read it from disk
  if (!moduleArg.wasmBinary && typeof process !== 'undefined' && process.versions && process.versions.node) {
    const wasmPath = path.join(__dirname, 'jwwlib.wasm');
    moduleArg.wasmBinary = fs.readFileSync(wasmPath);
  }
  return createJWWModule(moduleArg);
};

module.exports.default = module.exports;