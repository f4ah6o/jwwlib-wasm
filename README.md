# jwwlib-wasm

A WebAssembly library for reading JWW (JW_CAD) files in the browser.

## Overview

jwwlib-wasm is a WebAssembly port of the JWW file reading functionality from LibreCAD's jwwlib. It allows you to parse and extract geometric data from JWW files directly in web browsers.

## Features

- Read JWW (JW_CAD) format files
- Extract geometric entities:
  - Lines
  - Circles
  - Arcs
  - (More entity types coming soon)
- Pure client-side processing
- No server dependencies
- Easy JavaScript API

## Installation

### Using npm

```bash
npm install jwwlib-wasm
```

### Using CDN

```html
<script src="https://unpkg.com/jwwlib-wasm/dist/jwwlib.js"></script>
```

## Quick Start

```javascript
import { JWWReader } from 'jwwlib-wasm';

// Initialize the WASM module
await JWWReader.init();

// Read a JWW file
const fileBuffer = await fetch('example.jww').then(r => r.arrayBuffer());
const reader = new JWWReader(fileBuffer);

// Get all entities
const entities = reader.getEntities();

// Process entities
entities.forEach(entity => {
  switch(entity.type) {
    case 'LINE':
      console.log(`Line from (${entity.x1}, ${entity.y1}) to (${entity.x2}, ${entity.y2})`);
      break;
    case 'CIRCLE':
      console.log(`Circle at (${entity.cx}, ${entity.cy}) with radius ${entity.radius}`);
      break;
    case 'ARC':
      console.log(`Arc at (${entity.cx}, ${entity.cy})`);
      break;
  }
});
```

## Building from Source

### Prerequisites

- CMake 3.10+
- Emscripten SDK
- Node.js 14+

### Build Steps

```bash
# Clone the repository
git clone https://github.com/yourusername/jwwlib-wasm.git
cd jwwlib-wasm

# Install dependencies
npm install

# Build WASM module
npm run build:wasm

# Build JavaScript wrapper
npm run build

# Run tests
npm test
```

## Examples

See the [examples](examples/) directory for:
- Basic usage example
- Gallery of JWW file visualizations
- Integration with canvas rendering

## API Documentation

### `JWWReader.init()`
Initialize the WebAssembly module. Must be called before creating any reader instances.

### `new JWWReader(buffer)`
Create a new reader instance with the JWW file buffer.

### `reader.getEntities()`
Get all geometric entities from the JWW file.

### `reader.getHeader()`
Get file header information.

## License

This project is licensed under the GNU General Public License v2.0 - see the [LICENSE](LICENSE) file for details.

This software contains code derived from:
- LibreCAD (https://github.com/LibreCAD/LibreCAD)
- Original jwwlib by Rallaz

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Roadmap

- [ ] Support for text entities
- [ ] Support for dimensions
- [ ] Support for blocks/groups
- [ ] DXF export functionality
- [ ] Improved memory efficiency
- [ ] Complete character encoding support

## Acknowledgments

- LibreCAD team for the original jwwlib implementation
- RibbonSoft for the original dxflib project
- JW_CAD community for the file format documentation