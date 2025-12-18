import { exec } from "child_process";
import { promisify } from "util";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const execAsync = promisify(exec);

export function runCpp(args = []) {
  // Build command - always use ../../indices/ from web/backend
  const command = `./main ../../indices/ ${args.map(arg => `"${arg.replace(/"/g, '\\"')}"`).join(' ')}`;
  
  console.log(`🚀 Running: ${command}`);
  
  return execAsync(command, {
    cwd: __dirname,  // Run from web/backend
    timeout: 30000,
    maxBuffer: 10 * 1024 * 1024  // 10MB buffer
  })
  .then(result => {
    console.log(`✅ Output received (${result.stdout.length} chars)`);
    return result.stdout;
  })
  .catch(error => {
    console.error(`❌ Error:`, error);
    throw new Error(`C++ failed: ${error.stderr || error.message}`);
  });
}
