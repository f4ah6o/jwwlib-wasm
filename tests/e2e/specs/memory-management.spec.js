import { test, expect } from '@playwright/test';

test.describe('Memory Management', () => {
  test('should not leak memory when creating and deleting readers', async ({ page }) => {
    await page.goto('/examples/test.html');
    
    // Wait for module to load
    await page.waitForSelector('#status:has-text("Module loaded successfully!")');
    
    // Test memory management
    const result = await page.evaluate(async () => {
      const JWWModule = await createJWWModule();
      const iterations = 100;
      
      // Create a small test buffer
      const testData = new Uint8Array(1024);
      
      // Get initial memory usage
      const initialMemory = JWWModule.HEAP8.length;
      
      // Create and delete many readers
      for (let i = 0; i < iterations; i++) {
        const dataPtr = JWWModule._malloc(testData.length);
        JWWModule.HEAPU8.set(testData, dataPtr);
        
        const reader = new JWWModule.JWWReader(dataPtr, testData.length);
        
        // Use the reader
        const entities = reader.getEntities();
        const header = reader.getHeader();
        
        // Clean up
        reader.delete();
        JWWModule._free(dataPtr);
      }
      
      // Get final memory usage
      const finalMemory = JWWModule.HEAP8.length;
      
      // Memory growth should be minimal
      const memoryGrowth = finalMemory - initialMemory;
      const growthPercentage = (memoryGrowth / initialMemory) * 100;
      
      return {
        initialMemory,
        finalMemory,
        memoryGrowth,
        growthPercentage,
        iterations
      };
    });
    
    // Memory should not grow significantly
    expect(result.growthPercentage).toBeLessThan(10); // Less than 10% growth
  });

  test('should handle large files without crashing', async ({ page }) => {
    await page.goto('/examples/test.html');
    
    // Wait for module to load
    await page.waitForSelector('#status:has-text("Module loaded successfully!")');
    
    // Test with a large buffer
    const result = await page.evaluate(async () => {
      const JWWModule = await createJWWModule();
      
      // Create a 1MB buffer
      const largeData = new Uint8Array(1024 * 1024);
      
      try {
        const dataPtr = JWWModule._malloc(largeData.length);
        JWWModule.HEAPU8.set(largeData, dataPtr);
        
        const reader = new JWWModule.JWWReader(dataPtr, largeData.length);
        
        // Try to get entities
        const entities = reader.getEntities();
        const entityCount = entities.size();
        
        // Clean up
        reader.delete();
        JWWModule._free(dataPtr);
        
        return {
          success: true,
          bufferSize: largeData.length,
          entityCount
        };
      } catch (error) {
        return {
          success: false,
          error: error.message,
          bufferSize: largeData.length
        };
      }
    });
    
    // Should handle large files without crashing
    expect(result.success).toBe(true);
    expect(result.bufferSize).toBe(1024 * 1024);
  });
});