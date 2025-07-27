// Type definitions for jwwlib-wasm
// New unified interface for JWW document processing

export interface JSEntityData {
	type: string; // Entity type (LINE, CIRCLE, ARC, TEXT, etc.)
	coordinates: number[]; // Entity coordinates [x1,y1,x2,y2] for LINE, [cx,cy,radius] for CIRCLE, etc.
	properties: Map<string, number>; // Additional properties (angles, heights, etc.)
	color: number;
	layer: number;
}

export interface JSLayerData {
	index: number; // Layer index
	name: string; // Layer name
	color: number; // Layer color
	visible: boolean; // Layer visibility
	entityCount: number; // Number of entities in this layer
}

export interface JWWDocumentWASM {
	loadFromMemory(dataPtr: number, size: number): boolean;
	getEntities(): JSEntityData[];
	getLayers(): JSLayerData[];
	getEntityCount(): number;
	getLayerCount(): number;
	hasError(): boolean;
	getLastError(): string;
	dispose(): void;
	getMemoryUsage(): number;
}

export interface WASMModule {
	// Memory management
	_malloc(size: number): number;
	_free(ptr: number): void;

	// Heap access
	HEAPU8: Uint8Array;
	HEAP8: Int8Array;

	// String utilities
	UTF8ToString(ptr: number): string;
	stringToUTF8(str: string, outPtr: number, maxBytesToWrite: number): void;

	// File system
	FS: {
		readFile(path: string): Uint8Array;
		writeFile(path: string, data: Uint8Array): void;
		unlink(path: string): void;
	};

	// Classes
	JWWDocumentWASM: new () => JWWDocumentWASM;

	// Legacy classes (still available)
	JWWReader: any;
}

// Error handling
export class WASMError extends Error {
	constructor(message: string, code?: string);
	readonly code?: string;
}

// Module factory function
declare function createJWWModule(
	moduleOverrides?: Partial<WASMModule>,
): Promise<WASMModule>;

export default createJWWModule;
