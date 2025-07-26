import { test, expect } from '@playwright/test';

test.describe('WASM Module Loading', () => {
  test('should load the WASM module successfully', async ({ page }) => {
    await page.goto('/examples/test.html');
    
    // Wait for module to load
    await page.waitForSelector('#status:has-text("Module loaded successfully!")', {
      timeout: 10000
    });
    
    // Check that module info is displayed
    const output = await page.locator('#output').textContent();
    expect(output).toContain('"ready": true');
    expect(output).toContain('"hasJWWReader": true');
  });

  test('should create JWWReader instance', async ({ page }) => {
    await page.goto('/examples/test.html');
    
    // Wait for module to load
    await page.waitForSelector('#status:has-text("Module loaded successfully!")');
    
    // Test creating a reader instance
    const result = await page.evaluate(async () => {
      if (typeof createJWWModule === 'undefined') {
        return { error: 'createJWWModule not found' };
      }
      
      const JWWModule = await createJWWModule();
      const reader = new JWWModule.JWWReader();
      const hasReader = reader !== null && reader !== undefined;
      
      // Clean up
      if (reader && reader.delete) {
        reader.delete();
      }
      
      return { hasReader };
    });
    
    expect(result.hasReader).toBe(true);
  });

  test('should create JWWDocumentWASM instance', async ({ page }) => {
    await page.goto('/examples/test.html');
    
    // Wait for module to load
    await page.waitForSelector('#status:has-text("Module loaded successfully!")');
    
    // Test creating a document instance with new interface
    const result = await page.evaluate(async () => {
      if (typeof createJWWModule === 'undefined') {
        return { error: 'createJWWModule not found' };
      }
      
      const JWWModule = await createJWWModule();
      
      // Check if JWWDocumentWASM exists
      if (!JWWModule.JWWDocumentWASM) {
        return { error: 'JWWDocumentWASM not found in module' };
      }
      
      const document = new JWWModule.JWWDocumentWASM();
      const hasDocument = document !== null && document !== undefined;
      
      // Test methods exist
      const methods = {
        hasLoadFromMemory: typeof document.loadFromMemory === 'function',
        hasGetEntities: typeof document.getEntities === 'function',
        hasGetLayers: typeof document.getLayers === 'function',
        hasGetEntityCount: typeof document.getEntityCount === 'function',
        hasGetLayerCount: typeof document.getLayerCount === 'function',
        hasHasError: typeof document.hasError === 'function',
        hasGetLastError: typeof document.getLastError === 'function',
        hasDispose: typeof document.dispose === 'function',
        hasGetMemoryUsage: typeof document.getMemoryUsage === 'function'
      };
      
      // Clean up
      if (document && document.dispose) {
        document.dispose();
      }
      
      return { hasDocument, methods };
    });
    
    expect(result.hasDocument).toBe(true);
    expect(result.methods.hasLoadFromMemory).toBe(true);
    expect(result.methods.hasGetEntities).toBe(true);
    expect(result.methods.hasGetLayers).toBe(true);
    expect(result.methods.hasGetEntityCount).toBe(true);
    expect(result.methods.hasGetLayerCount).toBe(true);
    expect(result.methods.hasHasError).toBe(true);
    expect(result.methods.hasGetLastError).toBe(true);
    expect(result.methods.hasDispose).toBe(true);
    expect(result.methods.hasGetMemoryUsage).toBe(true);
  });
});