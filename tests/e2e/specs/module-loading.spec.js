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
});