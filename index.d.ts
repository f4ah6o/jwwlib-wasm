declare module 'jwwlib-wasm' {
  export interface JWWEntity {
    type: 'LINE' | 'CIRCLE' | 'ARC' | 'TEXT' | 'POLYLINE' | 'SPLINE' | 'ELLIPSE' | 'DIMENSION' | 'HATCH' | 'IMAGE' | 'BLOCK' | 'SOLID';
    layer?: number;
    color?: number;
    lineType?: number;
    lineWidth?: number;
  }

  export interface JWWLine extends JWWEntity {
    type: 'LINE';
    x1: number;
    y1: number;
    x2: number;
    y2: number;
  }

  export interface JWWCircle extends JWWEntity {
    type: 'CIRCLE';
    cx: number;
    cy: number;
    radius: number;
  }

  export interface JWWArc extends JWWEntity {
    type: 'ARC';
    cx: number;
    cy: number;
    radius: number;
    startAngle: number;
    endAngle: number;
  }

  export interface JWWText extends JWWEntity {
    type: 'TEXT';
    x: number;
    y: number;
    text: string;
    height?: number;
    angle?: number;
    style?: string;
  }

  export interface JWWPolyline extends JWWEntity {
    type: 'POLYLINE';
    points: Array<{ x: number; y: number }>;
    closed?: boolean;
  }

  export interface JWWDimension extends JWWEntity {
    type: 'DIMENSION';
    dimensionType: 'LINEAR' | 'ALIGNED' | 'ANGULAR' | 'RADIAL' | 'DIAMETRIC' | 'ORDINATE';
    points: Array<{ x: number; y: number }>;
    text?: string;
  }

  export interface JWWHeader {
    version?: string;
    author?: string;
    created?: Date;
    modified?: Date;
    scale?: number;
    unit?: string;
  }

  export interface JWWModule {
    initialize(): Promise<void>;
    loadFile(buffer: ArrayBuffer): JWWDocument;
    getVersion(): string;
    dispose(): void;
  }

  export interface JWWDocument {
    getHeader(): JWWHeader;
    getEntities(): JWWEntity[];
    getEntityCount(): number;
    getLayerCount(): number;
    getLayerName(index: number): string;
    dispose(): void;
  }

  export class JWWReader {
    static init(): Promise<void>;
    static isInitialized(): boolean;
    
    constructor(buffer: ArrayBuffer);
    
    getEntities(): JWWEntity[];
    getHeader(): JWWHeader;
    getLayerCount(): number;
    getLayerName(index: number): string;
    dispose(): void;
  }

  export default JWWReader;
}