import { test as base } from '@playwright/test';
import fs from 'fs';
import os from 'os';
import path from 'path';
import childProcess from 'child_process';
import getPort from 'get-port';

const EXECUTABLE_PATH = path.join(
    __dirname,
    '../../WebView2WTL.Sample/x64/Debug/WebView2.exe',
);

export const test = base.extend({
  browser: async ({ playwright }, use, testInfo) => {

    const cdpPort = await getPort({ port: Array.from({ length: 101 }, (_, i) => i + 9222) });
    // print the port number to the console
    console.log(`Using port: ${cdpPort}`);

   
    // Make sure that the executable exists and is executable
    fs.accessSync(EXECUTABLE_PATH, fs.constants.X_OK);
    const userDataDir = path.join(
        fs.realpathSync.native(os.tmpdir()),
        `playwright-webview2-tests/user-data-dir-${testInfo.workerIndex}`,
    );
    const webView2Process = childProcess.spawn(EXECUTABLE_PATH, [
     '--version=134.0.3124.72',
     '--channel=fixed',
     '--root=C:\\Users\\gillesg\\Downloads\\Microsoft.WebView2.FixedVersionRuntime.134.0.3124.72.x64',
     '--test',
     `--port=${cdpPort}`
    ], {
      shell: true,      
    });
    await new Promise<void>(resolve => webView2Process.stdout.on('data', data => {
       if (data.toString().includes('WebView2 initialized'))
         resolve();
     }));

    //const browser = await playwright.chromium.connectOverCDP(`http://127.0.0.1:${cdpPort}`);
    let browser;
    const maxRetries = 10;
    for (let attempt = 1; attempt <= maxRetries; attempt++) {
      try {
        browser = await playwright.chromium.connectOverCDP(`http://127.0.0.1:${cdpPort}`);
        console.log(`Connected to WebView2 on attempt ${attempt}`);
        break; // Exit the loop if the connection is successful
      } catch (error) {
        console.error(`Attempt ${attempt} to connect to WebView2 failed: ${error.message}`);
        if (attempt === maxRetries) {
          throw new Error('Failed to connect to WebView2 after multiple attempts');
        }
        await new Promise(resolve => setTimeout(resolve, 2000)); // Wait 1 second before retrying
      }
    }
    
    await use(browser);
    await browser.close();
    childProcess.execSync(`taskkill /pid ${webView2Process.pid} /T /F`);
   // fs.rmdirSync(userDataDir, { recursive: true });
  },
  context: async ({ browser }, use) => {
    const context = browser.contexts()[0];
    await use(context);
  },
  page: async ({ context }, use) => {
    const page = context.pages()[0];
    await use(page);
  },
});

export { expect } from '@playwright/test';