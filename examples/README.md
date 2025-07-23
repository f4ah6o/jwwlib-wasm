# jwwlib-wasm Examples

This directory contains example applications demonstrating how to use jwwlib-wasm.

## Examples

### 1. Basic Example (index.html)
A simple example showing how to:
- Load and initialize the WASM module
- Read JWW files
- Extract entity information
- Display entities on a canvas

### 2. Gallery Example (gallery.html)
A more advanced example featuring:
- Multiple file handling
- Thumbnail generation
- Grid layout display
- Entity statistics

### 3. New Entity Types Example (new-entities-example.html)
Comprehensive example demonstrating new entity support:
- Block definitions and inserts (references)
- Hatch patterns with complex boundaries
- Extended dimension types (radial, angular, ordinate)
- Leader entities with annotations
- Image entities with transformations
- Memory management and performance monitoring
- Error handling and recovery

### 4. TypeScript Sample (new-entities-sample.ts)
TypeScript code samples showing:
- Type-safe entity processing
- Batch processing for large files
- Progress callbacks for UI updates
- Error handling patterns
- Data export functionality

## Running the Examples

1. Build the WASM module first:
```bash
npm run build:wasm
```

2. Start a local web server:
```bash
npm run dev
```

3. Open the examples in your browser:
- Basic example: http://localhost:3000/examples/index.html
- Gallery: http://localhost:3000/examples/gallery.html
- New entities: http://localhost:3000/examples/new-entities-example.html

## Note

These examples require a web server to run due to CORS restrictions with WASM modules. They won't work if opened directly as files in the browser.