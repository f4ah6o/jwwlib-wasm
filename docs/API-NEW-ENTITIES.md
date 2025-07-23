# New Entity Types API Documentation

This document describes the new entity types added to jwwlib-wasm for comprehensive JWW file support.

## Table of Contents
- [Block and Insert Entities](#block-and-insert-entities)
- [Hatch Entities](#hatch-entities)
- [Extended Dimension Types](#extended-dimension-types)
- [Leader Entities](#leader-entities)
- [Image Entities](#image-entities)
- [Memory Management](#memory-management)
- [Error Handling](#error-handling)

## Block and Insert Entities

### Block Definition (CDataList)

Blocks are reusable collections of entities that can be inserted multiple times in a drawing.

```javascript
const blocks = reader.getBlocks();
// Returns array of JSBlockData objects

// JSBlockData structure:
{
  name: string,        // Block name
  baseX: number,       // Base point X coordinate
  baseY: number,       // Base point Y coordinate
  baseZ: number,       // Base point Z coordinate
  description: string  // Block description (may be empty)
}
```

### Block Reference (CDataBlock / Insert)

Inserts are references to block definitions placed in the drawing.

```javascript
const inserts = reader.getInserts();
// Returns array of JSInsertData objects

// JSInsertData structure:
{
  blockName: string,    // Name of referenced block
  ipx: number,          // Insertion point X
  ipy: number,          // Insertion point Y
  ipz: number,          // Insertion point Z
  sx: number,           // Scale factor X
  sy: number,           // Scale factor Y
  sz: number,           // Scale factor Z
  angle: number,        // Rotation angle (radians)
  cols: number,         // Number of columns (for arrays)
  rows: number,         // Number of rows (for arrays)
  colSpacing: number,   // Column spacing
  rowSpacing: number,   // Row spacing
  color: number         // Entity color
}
```

## Hatch Entities

Hatches are filled areas defined by boundaries.

```javascript
const hatches = reader.getHatches();
// Returns array of JSHatchData objects

// JSHatchData structure:
{
  patternType: number,   // 0=user-defined, 1=predefined, 2=custom
  patternName: string,   // Pattern name (e.g., "SOLID", "LINE")
  solid: boolean,        // True for solid fill
  angle: number,         // Pattern angle (radians)
  scale: number,         // Pattern scale
  loops: Array<{         // Boundary loops
    type: number,        // Loop type
    edges: Array<{       // Loop edges
      type: number,      // 1=line, 2=arc, 3=ellipse, 4=spline
      // Line edge:
      x1?: number,
      y1?: number,
      x2?: number,
      y2?: number,
      // Arc edge:
      cx?: number,
      cy?: number,
      radius?: number,
      angle1?: number,
      angle2?: number,
      // Polyline edge:
      vertices?: Array<{x, y, z, bulge}>
    }>,
    isCCW: boolean       // Counter-clockwise flag
  }>,
  color: number          // Entity color
}
```

## Extended Dimension Types

### Dimension Types

```javascript
const dimensions = reader.getDimensions();

// Dimension types:
// 0 = LINEAR
// 1 = ALIGNED
// 2 = RADIAL
// 3 = DIAMETRIC
// 4 = ANGULAR
// 5 = ANGULAR3P
// 6 = ORDINATE

// JSDimensionData structure:
{
  type: number,          // Dimension type (see above)
  dpx: number,           // Definition point X
  dpy: number,           // Definition point Y
  dpz: number,           // Definition point Z
  mpx: number,           // Text middle point X
  mpy: number,           // Text middle point Y
  mpz: number,           // Text middle point Z
  attachmentPoint: number, // Text attachment point
  text: string,          // Dimension text
  angle: number,         // Text angle
  
  // Additional fields based on type:
  dpx1?: number,         // First definition point X
  dpy1?: number,         // First definition point Y
  dpz1?: number,         // First definition point Z
  dpx2?: number,         // Second definition point X
  dpy2?: number,         // Second definition point Y
  dpz2?: number,         // Second definition point Z
  dimLineAngle?: number, // Dimension line angle
  
  color: number          // Entity color
}
```

### Radial and Diametric Dimensions

Radial dimensions measure the radius of circles and arcs.
Diametric dimensions measure the diameter of circles.

### Angular Dimensions

Angular dimensions measure angles between lines or points.
- Angular: Measures angle between two lines
- Angular3P: Measures angle defined by three points

### Ordinate Dimensions

Ordinate dimensions show the X or Y coordinate of a feature.

## Leader Entities

Leaders are lines with arrows pointing to features, often with associated text.

```javascript
const leaders = reader.getLeaders();
// Returns array of JSLeaderData objects

// JSLeaderData structure:
{
  arrowHeadFlag: number,       // Arrow head style
  pathType: number,            // 0=straight, 1=spline
  annotationType: number,      // Type of annotation
  dimScaleOverall: number,     // Overall scale
  arrowHeadSize: number,       // Arrow head size
  vertices: Array<{            // Leader vertices
    x: number,
    y: number,
    z: number,
    bulge: number              // For curved segments
  }>,
  annotationReference: string, // Associated annotation
  color: number                // Entity color
}
```

## Image Entities

### Image Definitions

```javascript
const imageDefs = reader.getImageDefs();
// Returns array of JSImageDefData objects

// JSImageDefData structure:
{
  fileName: string,      // Image file name/path
  sizeX: number,         // Image width in pixels
  sizeY: number,         // Image height in pixels
  imageData: string      // Base64 encoded image data (if embedded)
}
```

### Image References

```javascript
const images = reader.getImages();
// Returns array of JSImageData objects

// JSImageData structure:
{
  imageDefHandle: string,      // Reference to image definition
  ipx: number,                 // Insertion point X
  ipy: number,                 // Insertion point Y
  ipz: number,                 // Insertion point Z
  ux: number,                  // U-vector X (width direction)
  uy: number,                  // U-vector Y
  uz: number,                  // U-vector Z
  vx: number,                  // V-vector X (height direction)
  vy: number,                  // V-vector Y
  vz: number,                  // V-vector Z
  width: number,               // Display width
  height: number,              // Display height
  displayProperties: number,   // Display properties flags
  brightness: number,          // Brightness (0-100)
  contrast: number,            // Contrast (0-100)
  fade: number,                // Fade (0-100)
  clippingState: boolean,      // Clipping enabled
  clippingVertices: Array<{    // Clipping boundary
    x: number,
    y: number,
    z: number,
    bulge: number
  }>
}
```

## Memory Management

### Memory Usage Information

```javascript
// Get estimated memory usage
const memoryUsage = reader.getMemoryUsage();
console.log(`Memory usage: ${memoryUsage} bytes`);

// Get entity statistics
const stats = reader.getEntityStats();
// Returns Map<string, number> with entity counts by type
stats.forEach((count, type) => {
  console.log(`${type}: ${count} entities`);
});
```

### Memory Optimization Features

1. **Pre-allocated Capacity**: Common entity types have pre-allocated vector capacity
2. **Block Deduplication**: Duplicate block definitions are detected and prevented
3. **Efficient String Storage**: String data is stored efficiently
4. **Direct Memory Access**: File data is processed directly without intermediate copies

## Error Handling

### Parsing Errors

```javascript
const errors = reader.getParsingErrors();
// Returns array of JSParseError objects

// JSParseError structure:
{
  type: ParseErrorType,  // Error type enum
  message: string,       // Error description
  entityType: string,    // Entity type that caused error
  lineNumber: number     // Line number (if applicable)
}

// ParseErrorType enum:
// NONE = 0
// INVALID_BLOCK_REFERENCE = 1
// INVALID_IMAGE_REFERENCE = 2
// INVALID_HATCH_BOUNDARY = 3
// INVALID_LEADER_PATH = 4
// INVALID_DIMENSION_DATA = 5
// MEMORY_ALLOCATION_FAILED = 6
// UNKNOWN_ENTITY_TYPE = 7
```

### Error Recovery

The parser continues processing even when errors are encountered:
- Invalid block references are logged but don't stop parsing
- Malformed entities are skipped
- Memory allocation failures are reported

## Usage Example

```javascript
// Load WASM module
const Module = await createJwwModule();

// Read JWW file
const fileData = new Uint8Array(fileBuffer);
const dataPtr = Module._malloc(fileData.length);
Module.HEAPU8.set(fileData, dataPtr);

// Create reader
const reader = new Module.JWWReader(dataPtr, fileData.length);

// Process new entity types
const blocks = reader.getBlocks();
const inserts = reader.getInserts();
const hatches = reader.getHatches();
const leaders = reader.getLeaders();
const images = reader.getImages();

// Check for errors
const errors = reader.getParsingErrors();
if (errors.size() > 0) {
  console.warn('Parsing errors:', errors);
}

// Get memory statistics
const memUsage = reader.getMemoryUsage();
const stats = reader.getEntityStats();

// Cleanup
reader.delete();
Module._free(dataPtr);
```

## Performance Considerations

1. **Large Files**: For files > 1MB, memory is pre-allocated based on file size
2. **Block Libraries**: Files with many blocks benefit from deduplication
3. **Complex Hatches**: Hatches with many loops may require more processing time
4. **Image Data**: Embedded images increase memory usage significantly

## Limitations

1. **Image Embedding**: Base64 encoding of image data is not yet implemented
2. **Hatch Patterns**: Custom hatch patterns may not be fully supported
3. **Leader Annotations**: Complex leader annotations may be simplified
4. **Memory Limits**: WASM heap size limits apply (can be configured)