# Security Policy

## Supported Versions

We release patches for security vulnerabilities. Which versions are eligible for receiving such patches depends on the CVSS v3.0 Rating:

| CVSS v3.0 | Supported Versions                        |
| --------- | ----------------------------------------- |
| 9.0-10.0  | Releases within the previous three months |
| 4.0-8.9   | Most recent release                       |

## Reporting a Vulnerability

Please report (suspected) security vulnerabilities to **security@example.com**. You will receive a response from us within 48 hours. If the issue is confirmed, we will release a patch as soon as possible depending on complexity but historically within a few days.

### What to Include in Your Report

Please include the following details in your security report:

1. **Description of the vulnerability**
   - Type of issue (e.g., buffer overflow, SQL injection, cross-site scripting, etc.)
   - Full paths of source file(s) related to the manifestation of the issue
   - The location of the affected source code (tag/branch/commit or direct URL)

2. **Steps to reproduce**
   - Step-by-step instructions to reproduce the issue
   - Proof-of-concept or exploit code (if possible)
   - Screenshots or videos if applicable

3. **Impact assessment**
   - Who can exploit this vulnerability, and what do they gain?
   - What kind of data could be exposed?
   - Can this lead to privilege escalation?

4. **Possible fixes**
   - If you have a suggestion for how to fix the issue, please include it

## Preferred Languages

We prefer all communications to be in English or Japanese.

## Disclosure Policy

- We will confirm the receipt of your vulnerability report within 48 hours
- We will confirm the issue and determine its impact within 7 days
- We will release a fix as soon as possible, typically within 30 days
- We will publicly disclose the vulnerability after the patch has been released

## Security Update Process

1. Security patches will be released as new versions
2. Security advisories will be published on GitHub
3. Direct notification will be sent to users who have registered for security updates

## Comments on this Policy

If you have suggestions on how this process could be improved, please submit a pull request.

## Security Best Practices for Users

### When Using jwwlib-wasm

1. **Always validate input files**
   - Only load JWW files from trusted sources
   - Implement file size limits
   - Validate file format before processing

2. **Memory management**
   - Dispose of reader instances when done
   - Monitor memory usage in production

3. **Content Security Policy**
   - When using in web applications, implement appropriate CSP headers
   - Restrict WebAssembly execution to trusted origins

4. **Keep dependencies updated**
   - Regularly update to the latest version
   - Monitor security advisories

### Example: Safe File Loading

```javascript
// Always validate file size
const MAX_FILE_SIZE = 50 * 1024 * 1024; // 50MB

async function loadJWWFile(file) {
  // Check file size
  if (file.size > MAX_FILE_SIZE) {
    throw new Error('File too large');
  }
  
  // Validate file extension
  if (!file.name.endsWith('.jww')) {
    throw new Error('Invalid file type');
  }
  
  try {
    const buffer = await file.arrayBuffer();
    const reader = new JWWReader(buffer);
    
    // Always dispose when done
    try {
      return reader.getEntities();
    } finally {
      reader.dispose();
    }
  } catch (error) {
    console.error('Failed to load JWW file:', error);
    throw error;
  }
}
```

## Security Features

jwwlib-wasm includes several security features:

1. **Memory safety**: Built with modern C++ and Emscripten safety features
2. **Input validation**: Validates JWW file format before processing
3. **Sandboxed execution**: WebAssembly provides a secure sandbox
4. **No file system access**: Operates only on provided buffers
5. **No network access**: Pure computational library