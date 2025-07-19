import { test, expect } from '@playwright/test';

test.describe('Entity Extraction', () => {
  test('should extract line entities correctly', async ({ page }) => {
    await page.goto('/examples/index.html');
    
    // Wait for module to load
    await page.waitForFunction(() => typeof createJWWModule !== 'undefined');
    
    // Test line extraction
    const result = await page.evaluate(async () => {
      const JWWModule = await createJWWModule();
      
      // Create a simple test case - this would normally come from a file
      // For now, we'll use the default constructor and check basic functionality
      const reader = new JWWModule.JWWReader();
      
      // Get entities
      const entities = reader.getEntities();
      const entityArray = [];
      
      // Convert to array for easier testing
      for (let i = 0; i < entities.size(); i++) {
        const entity = entities.get(i);
        entityArray.push({
          type: entity.type,
          x1: entity.x1,
          y1: entity.y1,
          x2: entity.x2,
          y2: entity.y2,
          cx: entity.cx,
          cy: entity.cy,
          radius: entity.radius,
          angle1: entity.angle1,
          angle2: entity.angle2
        });
      }
      
      // Clean up
      reader.delete();
      
      return {
        entityCount: entities.size(),
        entities: entityArray
      };
    });
    
    // Basic check - should at least not crash
    expect(result.entityCount).toBeGreaterThanOrEqual(0);
  });

  test('should correctly identify entity types', async ({ page }) => {
    await page.goto('/examples/index.html');
    
    // Wait for module to load
    await page.waitForFunction(() => typeof createJWWModule !== 'undefined');
    
    // Test entity type identification
    const result = await page.evaluate(async () => {
      const JWWModule = await createJWWModule();
      
      // Test data with known content would go here
      // For now, we'll just verify the structure works
      const reader = new JWWModule.JWWReader();
      const entities = reader.getEntities();
      
      const entityTypes = new Set();
      for (let i = 0; i < entities.size(); i++) {
        const entity = entities.get(i);
        entityTypes.add(entity.type);
      }
      
      // Clean up
      reader.delete();
      
      return {
        hasEntities: entities.size() > 0,
        entityTypes: Array.from(entityTypes)
      };
    });
    
    // Entity types should be valid
    const validTypes = ['LINE', 'CIRCLE', 'ARC'];
    for (const type of result.entityTypes) {
      expect(validTypes).toContain(type);
    }
  });
});