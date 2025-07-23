# Troubleshooting Guide for jwwlib-wasm New Entity Types

This guide helps resolve common issues when working with the new entity types (blocks, inserts, hatches, dimensions, leaders, and images) in jwwlib-wasm.

## Table of Contents
- [Common Issues](#common-issues)
- [Entity-Specific Problems](#entity-specific-problems)
- [Memory and Performance Issues](#memory-and-performance-issues)
- [Build and Compilation Issues](#build-and-compilation-issues)
- [Runtime Errors](#runtime-errors)
- [Debugging Tips](#debugging-tips)

## Common Issues

### 1. Module Loading Fails

**Problem**: `createJWWModule is not defined` or module fails to initialize

**Solutions**:
```javascript
// Ensure the script is loaded before calling createJWWModule
<script src="path/to/jwwlib.js"></script>
<script>
  // Wait for the script to load
  window.addEventListener('load', async () => {
    try {
      const Module = await createJWWModule();
      // Module is ready to use
    } catch (error) {
      console.error('Failed to load module:', error);
    }
  });
</script>
```

### 2. File Reading Errors

**Problem**: JWW file fails to load or parse

**Solutions**:
```javascript
// Check file size and allocate memory properly
const fileData = new Uint8Array(arrayBuffer);
if (fileData.length === 0) {
  throw new Error('Empty file');
}

// Ensure proper memory allocation
const dataPtr = Module._malloc(fileData.length);
if (!dataPtr) {
  throw new Error('Failed to allocate memory');
}

try {
  Module.HEAPU8.set(fileData, dataPtr);
  const reader = new Module.JWWReader(dataPtr, fileData.length);
  // Use reader...
} finally {
  // Always free memory
  Module._free(dataPtr);
}
```

### 3. Character Encoding Issues

**Problem**: Japanese text appears garbled or incorrect

**Solutions**:
```javascript
// Ensure proper UTF-8 handling
const text = reader.getTexts();
for (let i = 0; i < text.size(); i++) {
  const textEntity = text.get(i);
  // Text is already converted to UTF-8
  console.log(textEntity.text);
  
  // If you need the original bytes
  const bytes = textEntity.textBytes;
  // Process Shift-JIS bytes if needed
}
```

## Entity-Specific Problems

### Blocks and Inserts

**Problem**: Block references not found

**Symptom**: `INVALID_BLOCK_REFERENCE` errors

**Solutions**:
```javascript
// Check parsing errors
const errors = reader.getParsingErrors();
for (let i = 0; i < errors.size(); i++) {
  const error = errors.get(i);
  if (error.type === 1) { // INVALID_BLOCK_REFERENCE
    console.warn(`Missing block: ${error.message}`);
  }
}

// Verify block exists before using insert
const blocks = reader.getBlocks();
const blockNames = new Set();
for (let i = 0; i < blocks.size(); i++) {
  blockNames.add(blocks.get(i).name);
}

const inserts = reader.getInserts();
for (let i = 0; i < inserts.size(); i++) {
  const insert = inserts.get(i);
  if (!blockNames.has(insert.blockName)) {
    console.warn(`Insert references undefined block: ${insert.blockName}`);
  }
}
```

### Hatches

**Problem**: Hatch boundaries not displaying correctly

**Solutions**:
```javascript
// Check hatch loop integrity
const hatches = reader.getHatches();
for (let i = 0; i < hatches.size(); i++) {
  const hatch = hatches.get(i);
  
  if (hatch.loops.size() === 0) {
    console.warn(`Hatch ${i} has no boundary loops`);
    continue;
  }
  
  for (let j = 0; j < hatch.loops.size(); j++) {
    const loop = hatch.loops.get(j);
    if (loop.edges.size() === 0) {
      console.warn(`Hatch ${i}, loop ${j} has no edges`);
    }
    
    // Verify edge connectivity
    let lastPoint = null;
    for (let k = 0; k < loop.edges.size(); k++) {
      const edge = loop.edges.get(k);
      // Check edge continuity...
    }
  }
}
```

### Dimensions

**Problem**: Dimension text or geometry incorrect

**Solutions**:
```javascript
// Handle different dimension types properly
const dimensions = reader.getDimensions();
for (let i = 0; i < dimensions.size(); i++) {
  const dim = dimensions.get(i);
  
  switch (dim.type) {
    case 2: // RADIAL
    case 3: // DIAMETRIC
      // These use single definition point
      console.log(`Center: (${dim.dpx}, ${dim.dpy})`);
      break;
      
    case 4: // ANGULAR
    case 5: // ANGULAR3P
      // These use multiple points
      console.log(`Point 1: (${dim.dpx1}, ${dim.dpy1})`);
      console.log(`Point 2: (${dim.dpx2}, ${dim.dpy2})`);
      break;
      
    case 6: // ORDINATE
      // Feature location and leader endpoint
      console.log(`Feature: (${dim.dpx1}, ${dim.dpy1})`);
      console.log(`Leader end: (${dim.dpx2}, ${dim.dpy2})`);
      break;
  }
}
```

### Leaders

**Problem**: Leader path not rendering correctly

**Solutions**:
```javascript
// Ensure leader has vertices
const leaders = reader.getLeaders();
for (let i = 0; i < leaders.size(); i++) {
  const leader = leaders.get(i);
  
  if (leader.vertices.size() < 2) {
    console.warn(`Leader ${i} has insufficient vertices`);
    continue;
  }
  
  // Check for path type
  if (leader.pathType === 1) { // Spline
    // Requires special handling for smooth curves
    console.log('Spline leader - use interpolation');
  }
}
```

### Images

**Problem**: Images not displaying or wrong size

**Solutions**:
```javascript
// Verify image definitions exist
const images = reader.getImages();
const imageDefs = reader.getImageDefs();

// Build image definition map
const imageDefMap = new Map();
for (let i = 0; i < imageDefs.size(); i++) {
  const def = imageDefs.get(i);
  imageDefMap.set(def.fileName, def);
}

// Check image references
for (let i = 0; i < images.size(); i++) {
  const image = images.get(i);
  
  // Calculate actual display size from vectors
  const uLength = Math.sqrt(image.ux * image.ux + image.uy * image.uy);
  const vLength = Math.sqrt(image.vx * image.vx + image.vy * image.vy);
  
  console.log(`Image ${i}:`);
  console.log(`  Display size: ${uLength} x ${vLength}`);
  console.log(`  Stored size: ${image.width} x ${image.height}`);
}
```

## Memory and Performance Issues

### Large File Handling

**Problem**: Out of memory errors with large JWW files

**Solutions**:
```javascript
// Use file size hint for better memory allocation
const fileSize = fileData.length;
const reader = new Module.JWWReader(dataPtr, fileSize);

// Monitor memory usage
const memUsage = reader.getMemoryUsage();
console.log(`Memory used: ${(memUsage / 1024 / 1024).toFixed(2)} MB`);

// For very large files, process in batches
if (fileSize > 50 * 1024 * 1024) { // > 50MB
  console.warn('Large file detected, processing may be slow');
}
```

### Progress Monitoring

**Problem**: UI freezes during file loading

**Solutions**:
```javascript
// Use progress callback
const progressCallback = Module.addFunction((current, total) => {
  const percentage = (current / total * 100).toFixed(1);
  console.log(`Loading: ${percentage}%`);
  
  // Update UI (throttle updates)
  if (current % 1000 === 0 || current === total) {
    updateProgressBar(percentage);
  }
}, 'vii');

const reader = new Module.JWWReader(dataPtr, fileSize, progressCallback);
Module.removeFunction(progressCallback);
```

### Memory Leaks

**Problem**: Memory usage grows with repeated file loads

**Solutions**:
```javascript
// Always clean up readers
let currentReader = null;

function loadNewFile(fileData) {
  // Clean up previous reader
  if (currentReader) {
    currentReader.delete();
    currentReader = null;
  }
  
  // Create new reader
  const dataPtr = Module._malloc(fileData.length);
  try {
    Module.HEAPU8.set(fileData, dataPtr);
    currentReader = new Module.JWWReader(dataPtr, fileData.length);
  } finally {
    Module._free(dataPtr);
  }
}

// Clean up on page unload
window.addEventListener('beforeunload', () => {
  if (currentReader) {
    currentReader.delete();
  }
});
```

## Build and Compilation Issues

### WASM Build Fails

**Problem**: Compilation errors when building

**Solutions**:
```bash
# Ensure Emscripten is properly installed
emcc --version

# Clean build directory
rm -rf build build-release
mkdir build && cd build

# Configure with proper flags
emcmake cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build with verbose output
emmake make VERBOSE=1
```

### Missing Dependencies

**Problem**: Header files not found

**Solutions**:
```bash
# Check include paths in CMakeLists.txt
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm
)

# Verify dl_jww.h exists
ls -la include/dl_jww.h
```

## Runtime Errors

### Null Pointer Exceptions

**Problem**: Accessing deleted or null objects

**Solutions**:
```javascript
// Check object validity
if (!reader || reader.isDeleted()) {
  throw new Error('Reader object is invalid');
}

// Safe entity access
const blocks = reader.getBlocks();
if (blocks && blocks.size() > 0) {
  // Process blocks
}
```

### Vector Out of Bounds

**Problem**: Accessing vector elements beyond size

**Solutions**:
```javascript
// Always check size before accessing
const lines = reader.getLines();
const count = lines.size();

for (let i = 0; i < count; i++) {
  const line = lines.get(i);
  // Process line
}

// Never do this:
// const line = lines.get(lines.size()); // Out of bounds!
```

## Debugging Tips

### 1. Enable Debug Build

```bash
# Build with debug symbols
cd build
emcmake cmake -DCMAKE_BUILD_TYPE=Debug ..
emmake make
```

### 2. Use Browser Developer Tools

```javascript
// Add breakpoints in JavaScript
debugger;

// Log detailed information
console.group('Entity Details');
console.log('Blocks:', reader.getBlocks().size());
console.log('Memory:', reader.getMemoryUsage());
console.groupEnd();
```

### 3. Check WASM Module State

```javascript
// Verify module is loaded
console.log('Module ready:', Module.ready);
console.log('Runtime initialized:', Module.runtimeInitialized);

// Check available functions
console.log('JWWReader available:', typeof Module.JWWReader);
```

### 4. Monitor Performance

```javascript
// Time operations
console.time('File parsing');
const reader = new Module.JWWReader(dataPtr, fileSize);
console.timeEnd('File parsing');

// Profile memory
if (performance.memory) {
  console.log('JS Heap:', performance.memory.usedJSHeapSize / 1024 / 1024, 'MB');
}
```

### 5. Error Recovery

```javascript
// Wrap operations in try-catch
try {
  const reader = new Module.JWWReader(dataPtr, fileSize);
  // Process file...
} catch (error) {
  console.error('Processing failed:', error);
  
  // Check for specific error types
  if (error.message.includes('memory')) {
    console.error('Memory allocation failed - file may be too large');
  } else if (error.message.includes('parse')) {
    console.error('File parsing failed - file may be corrupted');
  }
  
  // Clean up resources
  if (dataPtr) Module._free(dataPtr);
}
```

## Getting Help

If you encounter issues not covered in this guide:

1. Check the [GitHub Issues](https://github.com/your-repo/jwwlib-wasm/issues)
2. Review the [API documentation](docs/API-NEW-ENTITIES.md)
3. Look at the [example code](examples/new-entities-example.html)
4. Enable debug logging and share the output when reporting issues

When reporting issues, please include:
- jwwlib-wasm version
- Browser and version
- Sample JWW file (if possible)
- Error messages and stack traces
- Minimal code to reproduce the issue