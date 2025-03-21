import { test, expect } from './webView2Test';

test('login test', async ({ page }, testInfo) => {
   const url = testInfo.project.metadata.url || 'https://foo.com';
   const username = testInfo.project.metadata.username || 'bad@foo.com';
   const password = testInfo.project.metadata.password || ''; 

   await page.goto(url);

   const inputFieldUsername = await page.waitForSelector('input[name="Username"]', { timeout: 5000 });
   if (!inputFieldUsername) {
      throw new Error('Input field not found');
   }   
   // Validate that the input field is enabled and visible
   expect(await inputFieldUsername.isEnabled()).toBe(true);
   expect(await inputFieldUsername.isVisible()).toBe(true);
   
   const inputFieldPassword = await page.waitForSelector('input[name="Password"]', { timeout: 5000 });
   if (!inputFieldPassword) {
      throw new Error('Input field not found');
   }   
   // Validate that the input field is enabled and visible
   expect(await inputFieldPassword.isEnabled()).toBe(true);
   expect(await inputFieldPassword.isVisible()).toBe(true);
   
   // Fill in the username and password
   await page.fill('input[name="Username"]', username);
   await page.fill('input[name="Password"]', password);

    // Wait for the "Login" button to be visible
    const loginButton = await page.waitForSelector('span.rz-button-text', { timeout: 5000 });
    if (!loginButton) {
      throw new Error('Login button not found');
    }
      // Validate that the "Login" button is visible
    expect(await loginButton.isVisible()).toBe(true);
    // Click the "Login" button
    await loginButton.click();

      // Verify the presence of <h1> element with text "Counter"
      const counterHeader = await page.waitForSelector('h1', { timeout: 5000 });
      if (!counterHeader) {
         throw new Error('Counter header not found');
      }
      const headerText = await counterHeader.textContent();
      expect(headerText).toBe('Counter');
      // throw an error if the header text is not found
      if (!headerText) {
         throw new Error('Header text not found');
      }
});